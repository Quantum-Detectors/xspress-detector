"""Xspress ODIN server main functions.

This module implements the main entry point for the Xspress ODIN server. It handles parsing
configuration options, loading adapters and creating the appropriate HTTP server instances.

This is based on the standard ODIN server instance created by Tim Nicholls

Alan Greer, Diamond Light Source
"""
import sys
import logging
import signal
import threading

import tornado.ioloop

from odin.http.server import HttpServer
from odin.config.parser import ConfigParser, ConfigError
from odin_data.meta_writer.logconfig import setup_logging, add_graylog_handler


def shutdown_handler():  # pragma: no cover
    """Handle interrupt signals gracefully and shutdown IOLoop."""
    logging.info('Interrupt signal received, shutting down')
    tornado.ioloop.IOLoop.instance().stop()


def main(argv=None):
    """Run ODIN server.

    This function is the main entry point for the ODIN server. It parses configuration
    options from the command line and any files, resolves adapters and launches the main
    API server before entering the IO processing loop.

    :param argv: argument list to pass to parser if called programatically
    """
    config = ConfigParser()

    # Define configuration options and add to the configuration parser
    config.define('http_addr', default='0.0.0.0', option_help='Set HTTP server address')
    config.define('http_port', default=8888, option_help='Set HTTP server port')
    config.define('debug_mode', default=False, option_help='Enable tornado debug mode')
    config.define('access_logging', default=None, option_help="Set the tornado access log level",
                  metavar="debug|info|warning|error|none")
    config.define('static_path', default='./static', option_help='Set path for static file content')
    config.define('enable_cors', default=False, option_help='Enable cross-origin resource sharing')
    config.define('cors_origin', default='', option_help='Cross-origin resource sharing URL')

    # Xspress specific configuration options
    config.define('logserver', default=None, option_help="Graylog server address and :port")
    config.define('staticlogfields', default=None, option_help="Comma separated list of key=value fields to be attached to every log message")

    # Parse configuration options and any configuration file specified
    try:
        config.parse(argv)
    except ConfigError as e:
        logging.error('Failed to parse configuration: %s', e)
        return 2

    # Setup the logging configuration
    static_fields = None
    if config.staticlogfields is not None:
        static_fields = dict([f.split('=') for f in config.staticlogfields.replace(' ','').split(',')])

    if config.logserver is not None:
        logserver, logserverport = config.logserver.split(':')
        logserverport = int(logserverport)
        add_graylog_handler(logserver, logserverport, static_fields=static_fields)

    setup_logging()


    # Resolve the list of adapters specified
    try:
        adapters = config.resolve_adapters()
    except ConfigError as e:
        logging.warning('Failed to resolve API adapters: %s', e)
        logging.error("read file: %s", config.file_parsed)
        adapters = {}

    logging.info('Launching Xspress ODIN instance')

    # Launch the HTTP server
    http_server = HttpServer(config)
    http_server.listen(config.http_port, config.http_addr)

    logging.info('HTTP server listening on %s:%s', config.http_addr, config.http_port)

    # Register a SIGINT signal handler only if this is the main thread
    if isinstance(threading.current_thread(), threading._MainThread):  # pragma: no cover
        signal.signal(signal.SIGINT, lambda signum, frame: shutdown_handler())

    # Enter IO processing loop
    tornado.ioloop.IOLoop.instance().start()

    # At shutdown, clean up the state of the loaded adapters
    http_server.cleanup_adapters()

    logging.info('Xspress ODIN server shutdown')

    return 0


if __name__ == '__main__':  # pragma: no cover
    sys.exit(main())