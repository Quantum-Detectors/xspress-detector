import os, time
import argparse
import json
import curses
import os
import logging
import zmq
import random
import tornado
import datetime


from enum import Enum
from functools import partial

from abc import ABC, abstractmethod
from odin_data.ipc_client import IpcClient
from odin_data.ipc_message import IpcMessage
from zmq.eventloop.zmqstream import ZMQStream
from zmq.utils.strtypes import unicode, cast_bytes
from odin_data.ipc_channel import _cast_str
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError


ENDPOINT_TEMPLATE = "tcp://{}:{}"

class MessageType(Enum):
    CMD = 1
    CONFIG = 2
    REQUEST = 3

class XspressTriggerMode:
    TM_SOFTWARE            = 0
    TM_TTL_RISING_EDGE     = 1
    TM_BURST               = 2
    TM_TTL_VETO_ONLY       = 3
    TM_SOFTWARE_START_STOP = 4
    TM_IDC                 = 5
    TM_TTL_BOTH            = 6
    TM_LVDS_VETO_ONLY      = 7
    TM_LVDS_BOTH           = 8

class XspressDetectorStr:
    CONFIG_SHUTDOWN              = "shutdown"
    CONFIG_DEBUG                 = "debug_level"
    CONFIG_CTRL_ENDPOINT         = "ctrl_endpoint"
    CONFIG_REQUEST               = "request_configuration"

    CONFIG_XSP                   = "xsp"
    CONFIG_XSP_NUM_CARDS         = "num_cards"
    CONFIG_XSP_NUM_TF            = "num_tf"
    CONFIG_XSP_BASE_IP           = "base_ip"
    CONFIG_XSP_MAX_CHANNELS      = "max_channels"
    CONFIG_XSP_MAX_SPECTRA       = "max_spectra"
    CONFIG_XSP_DEBUG             = "debug"
    CONFIG_XSP_CONFIG_PATH       = "config_path"
    CONFIG_XSP_CONFIG_SAVE_PATH  = "config_save_path"
    CONFIG_XSP_USE_RESGRADES     = "use_resgrades"
    CONFIG_XSP_RUN_FLAGS         = "run_flags"
    CONFIG_XSP_DTC_ENERGY        = "dtc_energy"
    CONFIG_XSP_TRIGGER_MODE      = "trigger_mode"
    CONFIG_XSP_INVERT_F0         = "invert_f0"
    CONFIG_XSP_INVERT_VETO       = "invert_veto"
    CONFIG_XSP_DEBOUNCE          = "debounce"
    CONFIG_XSP_EXPOSURE_TIME     = "exposure_time"
    CONFIG_XSP_FRAMES            = "frames"

    CONFIG_DAQ                   = "daq"
    CONFIG_DAQ_ENABLED           = "enabled"
    CONFIG_DAQ_ZMQ_ENDPOINTS     = "endpoints"

    CONFIG_CMD                   = "cmd"
    CONFIG_CMD_CONNECT           = "connect"
    CONFIG_CMD_SAVE              = "save"
    CONFIG_CMD_RESTORE           = "restore"
    CONFIG_CMD_START             = "start"
    CONFIG_CMD_STOP              = "stop"
    CONFIG_CMD_TRIGGER           = "trigger"


class XspressDetectorException(Exception):
    pass

class XspressClient:
    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip, port, callback=None):
        self.ip = ip
        self.port = port
        self.endpoint = ENDPOINT_TEMPLATE.format(ip, port)

        self.context = zmq.Context.instance()
        self.socket = self.context.socket(zmq.DEALER)
        self.socket.setsockopt(zmq.IDENTITY, self._generate_identity())  # pylint: disable=no-member
        self.socket.connect(self.endpoint)

        io_loop = tornado.ioloop.IOLoop.current()
        self.stream = ZMQStream(self.socket, io_loop=io_loop)
        self.callback = callback
        self.register_callback(self.callback)
        self.stream.on_send(self._on_send_callback)

        self.message_id = 0
        self.queue_size = 0


    def _generate_identity(self):
        identity = "{:04x}-{:04x}".format(
            random.randrange(0x10000), random.randrange(0x10000)
        )
        return cast_bytes(identity)

    def test_callback(self, msg):
        data = _cast_str(msg[0])
        logging.debug(f"data type = {type(data)}")
        logging.debug(f"data = {data}")
        ipc_msg = IpcMessage(from_str=data)
        logging.debug(f"queue size = {self.queue_size}")
        logging.debug(f"ZmqMessage: {msg}")
        logging.debug(f"IpcMessage __dict__: {ipc_msg.__dict__}")
        logging.debug(f"IpcMessage msg.params: {ipc_msg.get_params()}")

    def _on_send_callback(self, msg, status):
        self.queue_size += 1

    def register_callback(self, callback):
        def wrap(msg):
            self.queue_size -= 1
            callback(msg)
        self.stream.on_recv(wrap)

    def send_command(self, msg):
        pass

    def send_requst(self, msg: IpcMessage):
        self.stream.flush()
        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        logging.info("Sending message:\n%s", msg.encode())
        self.stream.send(cast_bytes(msg.encode()))
        logging.error(f"queue size = {self.queue_size}")

    def send_config(self, msg):
        pass

class XspressDetector(object):

    def __init__(self, ip: str, port: int, dubug=logging.ERROR):
        logging.getLogger().setLevel(logging.DEBUG)

        self._client = IpcClient(ip, port) # calls zmq_sock.connect() so might throw?
        self._async_client = XspressClient(ip, port, callback=self.on_recv_callback)
        self.timeout = 1
        self.ctrl_enpoint_connected: bool = False
        self.xsp_connected: bool = False

        # upper level parameter tree members
        self.ctr_endpoint = ENDPOINT_TEMPLATE.format(ip, port)
        self.ctr_debug = 0

        # xsp parameter tree members
        self.num_cards : int = 0
        self.num_tf : int = 0
        self.base_ip : str = None
        self.max_channels : str = 0
        self.max_spectra : int = 0
        self.debug : int = 0
        self.settings_path : str = None
        self.settings_save_path : str = None
        self.use_resgrade : bool = None
        self.run_flags : int = 0
        self.dtc_energy : int = 0
        self.trigger_mode : XspressTriggerMode = None
        self.num_frames : int = 0
        self.invert_f0 : bool = None
        self.invert_veto : bool = None
        self.debounce : bool = None # is this a bool I was guessing when writing this?
        self.exposure_time : double = 0.0
        self.frames : int = 0

        # daq parameter tree members
        self.daq_enabled : bool = 0
        self.daq_endpoints : list[str] = []

        # cmd parameter tree members. No state to store.
        # connect, save, restore, start, stop, trigger

        tree = \
        {
            XspressDetectorStr.CONFIG_DEBUG : (lambda: self.ctr_debug, partial(self._set, 'ctr_debug'), {}),
            XspressDetectorStr.CONFIG_CTRL_ENDPOINT: (lambda: self.ctr_endpoint, partial(self._set, 'ctr_endpoint'), {}),
            XspressDetectorStr.CONFIG_XSP :
            {
                XspressDetectorStr.CONFIG_XSP_BASE_IP : (lambda: self.base_ip, partial(self._set, 'base_ip'), {}),
                XspressDetectorStr.CONFIG_XSP_CONFIG_PATH : (lambda: self.settings_path, partial(self._set, "settings_path"), {}),
            }
        }
        self.parameter_tree = ParameterTree(tree)

    def _set(self, attr_name, value):
        setattr(self, attr_name, value)

    def get(self, path):
        try:
            return self.parameter_tree.get(path)
        except ParameterTreeError as e:
            log.error(f"parameter_tree error: {e}")
            raise XspressDetectorException(e)

    def on_recv_callback(self, msg):
        ADAPTER_BASE_PATH = ""
        data = _cast_str(msg[0])
        logging.debug(f"queue size = {self._async_client.queue_size}")
        logging.debug(f"message recieved = {data}")
        ipc_msg = IpcMessage(from_str=data)

        if not ipc_msg.is_valid():
            raise XspressDetectorException("IpcMessage recieved is not valid")
        if ipc_msg.get_msg_val() == XspressDetectorStr.CONFIG_REQUEST:
            if ipc_msg.get_msg_type() == IpcMessage.ACK and ipc_msg.get_params():
                self._param_tree_set_recursive(ADAPTER_BASE_PATH, ipc_msg.get_params())
            else:
                pass

    def _param_tree_set_recursive(self, path, params):
        if not isinstance(params, dict):
            try:
                # logging.debug(f"XspressDetector._param_tree_set_recursive: calling self.parameter_tree.set({path},{params})")
                path = path.strip("/")
                self.parameter_tree.set(path, params)
                logging.debug(f"XspressDetector._param_tree_set_recursive: parameter path {path} was set to {params}")
            except ParameterTreeError:
                # logging.warning(f"XspressDetector._param_tree_set_recursive: parameter path {path} is not in parameter tree")
                pass
        else:
            for param_name, params in params.items():
                self._param_tree_set_recursive(f"{path}/{param_name}", params)

    def configure(self, num_cards, num_tf, base_ip, max_channels, settings_path, debug):
        self.num_cards = num_cards
        self.num_tf = num_tf
        self.base_ip = base_ip
        self.max_channels = max_channels
        self.debug = debug
        self.settings_path = settings_path

        config = {
            "base_ip" : self.base_ip,
            "num_cards" : self.num_cards,
            "num_tf" : self.num_tf,
            "max_channels" : self.max_channels,
            "config_path" : self.settings_path,
            "debug" : self.debug,
            "run_flags": 2,
        }
        config_msg = self._build_config_message(MessageType.CONFIG, config)
        self._async_client.send_requst(config_msg)
        # success, _ = self._client._send_message(config_msg, timeout=self.timeout)

        # if not success:
        #     self.connected = False
            # raise XspressDetectorException("Failed to write configuration parameters to the control server")

        # self._connect()

        self.read_config()
        from zmq.eventloop.ioloop import IOLoop
        main_loop = IOLoop.current()
        logging.warning(f"main_loop type = {type(main_loop)}")
        sched = tornado.ioloop.PeriodicCallback(self.read_config, 2000)
        sched.start()

    def _connect(self):
        command = { "connect": None }
        command_message = self._build_config_message(MessageType.CMD, command)

        self._async_client.send_requst(command_message)
        # success, _ = self._client.send_configuration(command_message, target=XspressDetectorStr.CONFIG_CMD)
        # if not success:
        #     self.connected = False
        #     logging.error("could not connect to Xspress")
        #     raise XspressDetectorException("could not connect to the xspress unit")
        self.connected = True
        self._restore()


    def _restore(self):
        command = { "restore": None }
        command_message = self._build_config_message(MessageType.CMD, command)
        # success, reply =  self._client.send_configuration(command_message, target=XspressDetectorStr.CONFIG_CMD)
        self._async_client.send_requst(command_message)

    def _build_config_message(self, message_type: MessageType, config: dict = None):
        if message_type == MessageType.CONFIG:
            params_group = "xsp"
        elif message_type == MessageType.REQUEST:
            msg = IpcMessage("cmd", "request_configuration")
            return msg
        elif message_type == MessageType.CMD:
            params_group = "cmd"
        else:
            raise XspressDetectorException(f"XspressDetector._build_config_message: {message_type} is not type MessageType")
        params = {}
        for k, v in config.items():
            params[k] = v

        msg = IpcMessage("cmd", "configure")
        msg.set_param(params_group, params)
        return msg

    def read_config(self):
        self._async_client.send_requst(self._build_config_message(MessageType.REQUEST))


def main():
    params = {
        "base_ip": "192.168.0.1",
        "num_cards": 4,
        'max_channels': 36,
        "num_tf": 16384,
        "config_path": "/dls_sw/b18/epics/xspress4/xspress4.36chan_pb/settings",
        "run_flags": 2,
        "trigger_mode": XspressTriggerMode.TM_BURST,
        "frames": 121212121,
        "exposure_time": 999.20,
    }
    config_msg = XspressConfigMessage(params).get_message()
    xsp = XspressDetector("127.0.0.1", 12000)
    print(xsp.write_config(config_msg))
    print(xsp.read_config())
    print(xsp.send_command(XspressConfigMessage({"": None}).get_message()))
    # print(xsp.send_command(XspressCommandMessage({"stop": None}).get_message()))
    # print(xsp.send_command(XspressCommandMessage({"trigger": None}).get_message()))



if __name__ == '__main__':
    main()

