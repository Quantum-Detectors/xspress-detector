"""
adapter.py - EXCALIBUR API adapter for the ODIN server.

Tim Nicholls, STFC Application Engineering Group
"""

import traceback
import logging
import re
from tornado.escape import json_decode
from odin.adapters.adapter import request_types, response_types, ApiAdapterResponse
from odin.adapters.async_adapter import AsyncApiAdapter
from xspress_detector.control.detector import XspressDetector

from .debug import debug_method


class XspressAdapter(AsyncApiAdapter):
    """XspressAdapter class.

    This class provides the adapter interface between the ODIN server and the Xspress detector
    system, transforming the REST-like API HTTP verbs into the appropriate Xspress application
    control actions
    """

    def __init__(self, **kwargs):
        """Initialise the ExcaliburAdapter object.

        :param kwargs: keyword arguments passed to ApiAdapter as options.
        """
        # Initialise the ApiAdapter base class to store adapter options
        super(XspressAdapter, self).__init__(**kwargs)

        # Compile the regular expression used to resolve paths into actions and resources
        self.path_regexp = re.compile('(.*?)/(.*)')

        self.detector = None
        try:
            logging.info("[adapter class] self.options: {}".format(self.options))
            endpoint = self.options['endpoint']
            ip, port = endpoint.split(":")
            num_process = int(self.options["num_process"])
            # For X3X2 the number of list mode processes is the same as in MCA mode
            # TODO: handle with a config flag so we don't break compatibility with Xspress 4
            num_process_list = num_process
            self.detector = XspressDetector(ip, port, num_process_mca=num_process, num_process_list=num_process_list)
            logging.info(f"instatiated XspressDetector with ip = {ip} and port {port}\n num_process/list = {num_process}/{num_process_list}")

            num_cards = int(self.options['num_cards'])
            num_tf = int(self.options["num_tf"])
            base_ip = self.options['base_ip']
            max_channels = int(self.options['max_channels'])
            max_spectra = int(self.options["max_spectra"])
            settings_path = self.options['settings_path']
            run_flags = int(self.options['run_flags'])
            debug = int(self.options["debug"])
            # trigger_mode = XspressTriggerMode.str2int(self.options["trigger_mode"])
            daq_endpoints= self.options["daq_endpoints"].replace(" ", "").split(",")
            self.detector.configure(
                num_cards=num_cards,
                num_tf=num_tf,
                base_ip=base_ip,
                max_channels=max_channels,
                max_spectra=max_spectra,
                settings_path=settings_path,
                run_flags=run_flags,
                debug=debug,
                daq_endpoints=daq_endpoints,
            )
            logging.debug('done configuring detector')
        except Exception as e:
            logging.error('Unhandled Exception:\n %s', traceback.format_exc())
            exit(1) # as there's no way to recover from this
        logging.info('exiting XspressAdapter.__init__')

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    async def get(self, path, request):
        """Handle an HTTP GET request.

        This method is the implementation of the HTTP GET handler for ExcaliburAdapter.

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        logging.debug(f"XspressAdapter.get called with path: {path}")
        try:
            response = self.detector.get(path)
            if not isinstance(response, dict):
                response = {"value": response}

            respose = "{}".format(response)
            status_code = 200
        except LookupError as e:
            response = {'invalid path': str(e)}
            logging.error(f"XspressAdapter.get: Invalid path: {path}")
            status_code = 400

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    async def put(self, path, request):
        """Handle an HTTP PUT request.

        This method is the implementation of the HTTP PUT handler for ExcaliburAdapter/

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        logging.debug(debug_method())
        try:
            data = json_decode(request.body)
            response = await self.detector.parameter_tree.put(path, data)
            response = "{}".format(response)
            status_code = 200
        except ConnectionError as e:
            response = {'error': str(e)}
            status_code = 500
            logging.error(e)
        except (TypeError, ValueError) as e:
            response = {'error': 'Failed to decode PUT request body {}:\n {}'.format(data, e)}
            logging.error(traceback.format_exc())
            status_code = 400
        except Exception as e:
            logging.error(traceback.format_exc())
            response = {f'{type(e).__name__} was raised' : f'{e}'}
            status_code = 400


        return ApiAdapterResponse(response, status_code=status_code)

    # def initialize(self, adapters):
    #     '''
    #     Not used.
    #     At the moment the XspressDetector is managing the fp/fr processes by connecting to them directly
    #     in the future when the fp_xspress_adapter is extended to support switching modes we might use
    #     this method to get a handle to the fp/fr adapters.
    #     '''
    #     fr_adapter = adapters["fr"]
    #     fp_adapter = adapters["fp"]
    #     self.detector.set_fr_handler(fr_adapter)
    #     self.detector.set_fp_handler(fp_adapter)