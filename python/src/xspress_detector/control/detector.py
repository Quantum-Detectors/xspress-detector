import asyncio
import copy
import logging
import getpass
import inspect
import time
import math

from enum import Enum
from functools import partial
from datetime import datetime
from contextlib import suppress
from numbers import Number as number

from odin.adapters.adapter import ApiAdapterRequest, ApiAdapter
from odin_data.control.ipc_message import IpcMessage

from .client import AsyncClient
from .util import ListModeIPPortGen
from .debug import debug_method
from .parameter_tree import (
    WriteOnlyVirtualParameter,
    VirtualParameter,
    ReadOnlyVirtualParameter,
    TransparentVirtualParameter,
    ValueParameter,
    TransparentValueParameter,
    ListParameter,
    XspressParameterTree,
    bound_validator,
)


def raise_exception(ex_class, message):
    """
    Utility function for raising exceptions from lambdas
    """
    raise ex_class(message)


ENDPOINT_TEMPLATE = "tcp://{}:{}"
UPDATE_PARAMS_PERIOD_SECONDS = 0.5
QUEUE_SIZE_LIMIT = 10

XSPRESS_MODE_MCA = "mca"
XSPRESS_MODE_LIST = "list"
XSPRESS_EXPOSURE_LOWER_LIMIT = 1.0 / 1000000
XSPRESS_EXPOSURE_UPPER_LIMIT = 20.0

NUM_FR_MCA = 8
NUM_FR_LIST = 8

FR_INIT_TIME = {
    XSPRESS_MODE_MCA: 1,  # seconds. Arbitrary wait time.
    XSPRESS_MODE_LIST: 1,  # seconds. This is a macro in the FR List cpp code
}


def nearest_mult_of_5_up(x):
    return math.ceil(x / 5) * 5


def is_coroutine(object):
    """
    see: https://stackoverflow.com/questions/67020609/partial-of-a-class-coroutine-isnt-a-coroutine-why
    """
    while isinstance(object, partial):
        object = object.func
    return inspect.iscoroutinefunction(object)


class AsyncPeriodicJob:
    def __init__(self, callback, time_seconds, time_min=0.001):
        self.callback = callback
        self._time_min = time_min
        self.time = 1
        self.set_time(time_seconds)
        self._running = False
        self._task = None

        self._last_logging_time = 0
        self.logging_interval = (
            30  # testing increase the time necessary for the configuration
        )

    def start(self):
        if not self._running:
            # Start task to call func periodically:
            self._task = asyncio.ensure_future(self._run())
            self._running = True

    async def stop(self):
        if self._running:
            # Stop task and await it stopped:
            self._task.cancel()
            self._running = False
            with suppress(asyncio.CancelledError):
                await self._task

    async def _run(self):
        while True:
            try:
                await self.callback()
            except Exception as e:
                current = time.time()
                if current - self._last_logging_time > self.logging_interval:
                    logging.error(
                        f"{self.__class__}._run: raised and cought exception{e}"
                    )
                    self._last_logging_time = current
            await asyncio.sleep(self.time)

    def is_running(self):
        return self._running

    def set_time(self, time):
        if time < self._time_min:
            logging.warning(
                f"{self.__class__}.__init__: setting time to minimum allowed = {self._time_min}"
            )
            self.time = self._time_min
        else:
            self.time = time

    def get_time(self):
        return self.time


class MessageType(Enum):
    CMD = 1
    CONFIG = 2
    REQUEST = 3
    APP = 4
    DAQ = 5


def _build_message(message_type: MessageType, config: dict = None):
    if message_type == MessageType.REQUEST:
        msg = IpcMessage("cmd", XspressDetectorStr.CONFIG_REQUEST)
        return msg
    elif message_type == MessageType.APP:
        params_group = XspressDetectorStr.APP
    elif message_type == MessageType.CONFIG:
        params_group = XspressDetectorStr.CONFIG
    elif message_type == MessageType.CMD:
        params_group = XspressDetectorStr.CMD
    elif message_type == MessageType.DAQ:
        params_group = XspressDetectorStr.CONFIG_DAQ
    else:
        raise XspressDetectorException(
            f"xspress_odin._build_message: {message_type} is not type MessageType"
        )
    msg = IpcMessage("cmd", "configure")
    msg.set_param(params_group, config)
    return msg


class XspressTriggerMode:
    TM_SOFTWARE = 0
    TM_TTL_RISING_EDGE = 1
    TM_BURST = 2
    TM_TTL_VETO_ONLY = 3
    TM_SOFTWARE_START_STOP = 4
    TM_IDC = 5
    TM_TTL_BOTH = 6
    TM_LVDS_VETO_ONLY = 7
    TM_LVDS_BOTH = 8

    def str2int(mode):
        x = XspressTriggerMode
        map = dict(
            software=x.TM_SOFTWARE,
            ttl_rising_edge=x.TM_TTL_RISING_EDGE,
            burst=x.TM_BURST,
            ttl_veto_only=x.TM_TTL_VETO_ONLY,
            software_start_stop=x.TM_SOFTWARE_START_STOP,
            idc=x.TM_IDC,
            ttl_both=x.TM_TTL_BOTH,
            lvds_veto_only=x.TM_LVDS_VETO_ONLY,
            lvds_both=x.TM_LVDS_BOTH,
        )
        return map[mode]


class XspressDetectorStr:
    STATUS = "status"
    STATUS_ERROR = "error"
    STATUS_STATE = "state"
    STATUS_CONNECTED = "connected"
    STATUS_RECONNECT_REQUIRED = "reconnect_required"
    STATUS_SENSOR = "sensor"
    STATUS_SENSOR_HEIGHT = "height"
    STATUS_SENSOR_WIDTH = "width"
    STATUS_SENSOR_BYTES = "bytes"
    STATUS_MANUFACTURER = "manufacturer"
    STATUS_MODEL = "model"
    STATUS_ACQ_COMPLETE = "acquisition_complete"
    STATUS_FRAMES = "frames_acquired"
    STATUS_SCALAR_0 = "scalar_0"
    STATUS_SCALAR_1 = "scalar_1"
    STATUS_SCALAR_2 = "scalar_2"
    STATUS_SCALAR_3 = "scalar_3"
    STATUS_SCALAR_4 = "scalar_4"
    STATUS_SCALAR_5 = "scalar_5"
    STATUS_SCALAR_6 = "scalar_6"
    STATUS_SCALAR_7 = "scalar_7"
    STATUS_SCALAR_8 = "scalar_8"
    STATUS_DTC = "dtc"
    STATUS_INP_EST = "inp_est"
    STATUS_TEMP_0 = "temp_0"
    STATUS_TEMP_1 = "temp_1"
    STATUS_TEMP_2 = "temp_2"
    STATUS_TEMP_3 = "temp_3"
    STATUS_TEMP_4 = "temp_4"
    STATUS_TEMP_5 = "temp_5"
    STATUS_CH_FRAMES = "ch_frames_acquired"
    STATUS_FEM_DROPPED_FRAMES = "fem_dropped_frames"
    STATUS_CARDS_CONNECTED = "cards_connected"
    STATUS_NUM_CH_CONNECTED = "num_ch_connected"

    ADAPTER = "adapter"
    ADAPTER_RESET = "reset"
    ADAPTER_SCAN = "scan"
    ADAPTER_UPDATE = "update"
    ADAPTER_START_TIME = "start_time"
    ADAPTER_UP_TIME = "up_time"
    ADAPTER_CONNECTED = "connected"
    ADAPTER_USERNAME = "username"
    ADAPTER_CONFIG_RAW = "config_raw"
    ADAPTER_DEBUG_LEVEL = "debug_level"

    APP = "app"
    APP_SHUTDOWN = "shutdow"
    APP_DEBUG = "debug_level"
    APP_CTRL_ENDPOINT = "ctrl_endpoint"
    CONFIG_REQUEST = "request_configuration"

    CONFIG = "config"
    CONFIG_NUM_CARDS = "num_cards"
    CONFIG_NUM_TF = "num_tf"
    CONFIG_BASE_IP = "base_ip"
    CONFIG_MAX_CHANNELS = "max_channels"
    CONFIG_MCA_CHANNELS = "mca_channels"
    CONFIG_MAX_SPECTRA = "max_spectra"
    CONFIG_DEBUG = "debug"
    CONFIG_CONFIG_PATH = "config_path"
    CONFIG_CONFIG_SAVE_PATH = "config_save_path"
    CONFIG_USE_RESGRADES = "use_resgrades"
    CONFIG_RUN_FLAGS = "run_flags"
    CONFIG_DTC_ENERGY = "dtc_energy"
    CONFIG_TRIGGER_MODE = "trigger_mode"
    CONFIG_INVERT_F0 = "invert_f0"
    CONFIG_INVERT_VETO = "invert_veto"
    CONFIG_DEBOUNCE = "debounce"
    CONFIG_EXPOSURE_TIME = "exposure_time"
    CONFIG_NUM_IMAGES = "num_images"  # so only "num_images" is used

    CONFIG_MODE = "mode"
    CONFIG_MODE_CONTROL = "mode_control"
    CONFIG_SCA5_LOW = "sca5_low_lim"
    CONFIG_SCA5_HIGH = "sca5_high_lim"
    CONFIG_SCA6_LOW = "sca6_low_lim"
    CONFIG_SCA6_HIGH = "sca6_high_lim"
    CONFIG_SCA4_THRESH = "sca4_threshold"

    CONFIG_DTC_FLAGS = "dtc_flags"
    CONFIG_DTC_ALL_EVT_OFF = "dtc_all_evt_off"
    CONFIG_DTC_ALL_EVT_GRAD = "dtc_all_evt_grad"
    CONFIG_DTC_ALL_EVT_RATE_OFF = "dtc_all_evt_rate_off"
    CONFIG_DTC_ALL_EVT_RATE_GRAD = "dtc_all_evt_rate_grad"
    CONFIG_DTC_IN_WIN_OFF = "dtc_in_win_off"
    CONFIG_DTC_IN_WIN_GRAD = "dtc_in_win_grad"
    CONFIG_DTC_IN_WIN_RATE_OFF = "dtc_in_win_rate_off"
    CONFIG_DTC_IN_WIN_RATE_GRAD = "dtc_in_win_rate_grad"

    CONFIG_DAQ = "daq"
    CONFIG_DAQ_ENABLED = "enabled"
    CONFIG_DAQ_ZMQ_ENDPOINTS = "endpoints"

    CMD = "command"
    CMD_RECONFIGURE = "reconfigure"
    CMD_CONNECT = "connect"
    CMD_DISCONNECT = "disconnect"
    CMD_SAVE = "save"
    CMD_RESTORE = "restore"
    CMD_START = "start"
    CMD_STOP = "stop"
    CMD_TRIGGER = "trigger"
    CMD_START_ACQUISITION = "start_acquisition"
    CMD_STOP_ACQUISITION = "stop_acquisition"

    VERSION = "version"
    VERSION_XSPRESS_DETECTOR = "xspress-detector"
    VERSION_XSPRESS_DETECTOR_MAJOR = "major"
    VERSION_XSPRESS_DETECTOR_MINOR = "minor"
    VERSION_XSPRESS_DETECTOR_PATCH = "patch"
    VERSION_XSPRESS_DETECTOR_SHORT = "short"
    VERSION_XSPRESS_DETECTOR_FULL = "full"

    PROCESS = "process"
    PROCESS_NUM_LIST = "num_list"
    PROCESS_NUM_MCA = "num_mca"
    PROCESS_NUM_CHANS_MCA = "num_chan_mca"
    PROCESS_NUM_CHANS_LIST = "num_chan_list"

    API = "api"


class XspressDetectorException(Exception):
    pass


class NotAcknowledgeException(Exception):
    pass


class XspressDetector(object):
    def __init__(
        self,
        ip: str,
        port: int,
        debug_level=logging.INFO,
        num_process_mca=NUM_FR_MCA,
        num_process_list=NUM_FR_LIST,
    ):
        self.logger = logging.getLogger()
        self.logger.setLevel(debug_level)

        self.endpoint = ENDPOINT_TEMPLATE.format("0.0.0.0", 12000)
        self.start_time = datetime.now()
        self.username = getpass.getuser()
        self.config_raw: dict = {}

        self._async_client = AsyncClient(ip, port)
        self.timeout = 1
        self.sched = AsyncPeriodicJob(self.read_config, UPDATE_PARAMS_PERIOD_SECONDS)

        # process params
        self.num_process_list = num_process_list
        self.num_process_mca = num_process_mca

        # root level parameter tree members
        self.ctr_endpoint = ENDPOINT_TEMPLATE.format(ip, port)

        self.max_channels: int = 0
        self.mca_channels: int = 0
        self.max_spectra: int = 0
        self.use_resgrades: bool = False
        self.run_flags: int = 0

        self.mode: str = ""  # 'mca' or 'list' for readback
        self.acquisition_complete: bool = False

        tree = {
            XspressDetectorStr.API: TransparentValueParameter(str, ""),
            XspressDetectorStr.APP: {
                XspressDetectorStr.APP_DEBUG: ValueParameter(
                    int,
                    0,
                    partial(self._put, MessageType.APP, XspressDetectorStr.APP_DEBUG),
                ),
                XspressDetectorStr.APP_CTRL_ENDPOINT: VirtualParameter(
                    str,
                    lambda: self.ctr_endpoint,
                    partial(self._set, "ctr_endpoint"),
                    partial(self._put, MessageType.APP, XspressDetectorStr.APP_DEBUG),
                ),
                XspressDetectorStr.APP_SHUTDOWN: WriteOnlyVirtualParameter(
                    int,
                    partial(
                        self._put, MessageType.APP, XspressDetectorStr.APP_SHUTDOWN
                    ),
                ),
            },
            XspressDetectorStr.CONFIG_DAQ: {
                XspressDetectorStr.CONFIG_DAQ_ENABLED: ValueParameter(
                    bool,
                    False,
                    partial(
                        self._put,
                        MessageType.DAQ,
                        XspressDetectorStr.CONFIG_DAQ_ENABLED,
                    ),
                ),
                XspressDetectorStr.CONFIG_DAQ_ZMQ_ENDPOINTS: ListParameter(),
            },
            XspressDetectorStr.CONFIG_REQUEST: WriteOnlyVirtualParameter(
                int, self.read_config
            ),
            XspressDetectorStr.ADAPTER: {
                XspressDetectorStr.ADAPTER_START_TIME: ReadOnlyVirtualParameter(
                    str, lambda: self.start_time.strftime("%B %d, %Y %H:%M:%S")
                ),
                XspressDetectorStr.ADAPTER_UP_TIME: ReadOnlyVirtualParameter(
                    str, lambda: str(datetime.now() - self.start_time)
                ),
                XspressDetectorStr.ADAPTER_CONNECTED: ReadOnlyVirtualParameter(
                    bool, self._async_client.is_connected
                ),
                XspressDetectorStr.ADAPTER_USERNAME: ReadOnlyVirtualParameter(
                    str, lambda: self.username
                ),
                XspressDetectorStr.ADAPTER_SCAN: VirtualParameter(
                    number, self.sched.get_time, put_cb=self.sched.set_time
                ),
                XspressDetectorStr.ADAPTER_CONFIG_RAW: ReadOnlyVirtualParameter(
                    str, lambda: self.config_raw
                ),
                XspressDetectorStr.ADAPTER_DEBUG_LEVEL: VirtualParameter(
                    int, lambda: self.logger.level, put_cb=self.logger.setLevel
                ),
                XspressDetectorStr.ADAPTER_UPDATE: WriteOnlyVirtualParameter(
                    int, self.do_updates
                ),
                XspressDetectorStr.ADAPTER_RESET: WriteOnlyVirtualParameter(
                    int, self.reset
                ),
            },
            XspressDetectorStr.STATUS: {
                XspressDetectorStr.STATUS_SENSOR: {
                    XspressDetectorStr.STATUS_SENSOR_HEIGHT: ReadOnlyVirtualParameter(
                        int, lambda: self.mca_channels
                    ),
                    XspressDetectorStr.STATUS_SENSOR_WIDTH: ReadOnlyVirtualParameter(
                        int, lambda: self.max_spectra
                    ),
                    XspressDetectorStr.STATUS_SENSOR_BYTES: ReadOnlyVirtualParameter(
                        int, lambda: self.max_spectra * self.mca_channels * 4
                    ),  # 4 bytes per int32 point
                },
                XspressDetectorStr.STATUS_MANUFACTURER: TransparentValueParameter(
                    str, "Quantum Detectors"
                ),
                XspressDetectorStr.STATUS_MODEL: TransparentValueParameter(
                    str, "Xspress 3"
                ),
                XspressDetectorStr.STATUS_ACQ_COMPLETE: TransparentVirtualParameter(
                    bool,
                    lambda: self.acquisition_complete,
                    partial(self._set, "acquisition_complete"),
                ),
                XspressDetectorStr.STATUS_FRAMES: TransparentValueParameter(int, 0),
                XspressDetectorStr.STATUS_SCALAR_0: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_1: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_2: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_3: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_4: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_5: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_6: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_7: ListParameter(),
                XspressDetectorStr.STATUS_SCALAR_8: ListParameter(),
                XspressDetectorStr.STATUS_DTC: ListParameter(),
                XspressDetectorStr.STATUS_INP_EST: ListParameter(),
                XspressDetectorStr.STATUS_ERROR: TransparentValueParameter(str, ""),
                XspressDetectorStr.STATUS_STATE: TransparentValueParameter(str, ""),
                XspressDetectorStr.STATUS_CONNECTED: TransparentValueParameter(
                    bool, False
                ),
                XspressDetectorStr.STATUS_RECONNECT_REQUIRED: TransparentValueParameter(
                    bool, False
                ),
                XspressDetectorStr.STATUS_TEMP_0: ListParameter(),
                XspressDetectorStr.STATUS_TEMP_1: ListParameter(),
                XspressDetectorStr.STATUS_TEMP_2: ListParameter(),
                XspressDetectorStr.STATUS_TEMP_3: ListParameter(),
                XspressDetectorStr.STATUS_TEMP_4: ListParameter(),
                XspressDetectorStr.STATUS_TEMP_5: ListParameter(),
                XspressDetectorStr.STATUS_CH_FRAMES: ListParameter(),
                XspressDetectorStr.STATUS_FEM_DROPPED_FRAMES: ListParameter(),
                XspressDetectorStr.STATUS_CARDS_CONNECTED: ListParameter(),
                XspressDetectorStr.STATUS_NUM_CH_CONNECTED: ListParameter(),
            },
            XspressDetectorStr.CONFIG: {
                XspressDetectorStr.CONFIG_MODE_CONTROL: WriteOnlyVirtualParameter(
                    int, self.set_mode
                ),
                XspressDetectorStr.CONFIG_MODE: TransparentVirtualParameter(
                    str, lambda: self.mode, partial(self._set, "mode")
                ),
                XspressDetectorStr.CONFIG_NUM_CARDS: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_NUM_CARDS,
                    ),
                ),
                XspressDetectorStr.CONFIG_NUM_TF: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put, MessageType.CONFIG, XspressDetectorStr.CONFIG_NUM_TF
                    ),
                ),
                XspressDetectorStr.CONFIG_BASE_IP: ValueParameter(
                    str,
                    "",
                    partial(
                        self._put, MessageType.CONFIG, XspressDetectorStr.CONFIG_BASE_IP
                    ),
                ),
                XspressDetectorStr.CONFIG_MAX_CHANNELS: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_MAX_CHANNELS,
                    ),
                ),
                XspressDetectorStr.CONFIG_MCA_CHANNELS: VirtualParameter(
                    int,
                    lambda: self.mca_channels,
                    partial(self._set, "mca_channels"),
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_MCA_CHANNELS,
                    ),
                ),
                XspressDetectorStr.CONFIG_MAX_SPECTRA: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_MAX_SPECTRA,
                    ),
                ),
                XspressDetectorStr.CONFIG_DEBUG: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put, MessageType.CONFIG, XspressDetectorStr.CONFIG_DEBUG
                    ),
                ),
                XspressDetectorStr.CONFIG_CONFIG_PATH: ValueParameter(
                    str,
                    "",
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_CONFIG_PATH,
                    ),
                ),
                XspressDetectorStr.CONFIG_CONFIG_SAVE_PATH: ValueParameter(
                    str,
                    "",
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_CONFIG_SAVE_PATH,
                    ),
                ),
                XspressDetectorStr.CONFIG_USE_RESGRADES: VirtualParameter(
                    bool,
                    lambda: self.use_resgrades,
                    partial(self._set, "use_resgrades"),
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_USE_RESGRADES,
                    ),
                ),
                XspressDetectorStr.CONFIG_RUN_FLAGS: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_RUN_FLAGS,
                    ),
                ),
                XspressDetectorStr.CONFIG_DTC_ENERGY: ValueParameter(
                    number,
                    0.0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_DTC_ENERGY,
                    ),
                ),
                XspressDetectorStr.CONFIG_TRIGGER_MODE: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_TRIGGER_MODE,
                    ),
                ),
                XspressDetectorStr.CONFIG_INVERT_F0: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_INVERT_F0,
                    ),
                ),
                XspressDetectorStr.CONFIG_INVERT_VETO: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_INVERT_VETO,
                    ),
                ),
                XspressDetectorStr.CONFIG_DEBOUNCE: ValueParameter(
                    int,
                    0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_DEBOUNCE,
                    ),
                ),
                XspressDetectorStr.CONFIG_EXPOSURE_TIME: ValueParameter(
                    number,
                    1.0,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_EXPOSURE_TIME,
                    ),
                    validators=[
                        bound_validator(
                            XSPRESS_EXPOSURE_LOWER_LIMIT, XSPRESS_EXPOSURE_UPPER_LIMIT
                        )
                    ],
                ),
                XspressDetectorStr.CONFIG_NUM_IMAGES: ValueParameter(
                    int,
                    1,
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_NUM_IMAGES,
                    ),
                ),
                XspressDetectorStr.CONFIG_SCA5_LOW: ListParameter(
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_SCA5_LOW,
                    )
                ),
                XspressDetectorStr.CONFIG_SCA5_HIGH: ListParameter(
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_SCA5_HIGH,
                    )
                ),
                XspressDetectorStr.CONFIG_SCA6_LOW: ListParameter(
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_SCA6_LOW,
                    )
                ),
                XspressDetectorStr.CONFIG_SCA6_HIGH: ListParameter(
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_SCA6_HIGH,
                    )
                ),
                XspressDetectorStr.CONFIG_SCA4_THRESH: ListParameter(
                    partial(
                        self._put,
                        MessageType.CONFIG,
                        XspressDetectorStr.CONFIG_SCA4_THRESH,
                    )
                ),
                XspressDetectorStr.CONFIG_DTC_FLAGS: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_ALL_EVT_OFF: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_ALL_EVT_GRAD: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_ALL_EVT_RATE_OFF: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_ALL_EVT_RATE_GRAD: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_IN_WIN_OFF: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_IN_WIN_GRAD: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_IN_WIN_RATE_OFF: ListParameter(),
                XspressDetectorStr.CONFIG_DTC_IN_WIN_RATE_GRAD: ListParameter(),
            },
            XspressDetectorStr.CMD: {
                XspressDetectorStr.CMD_CONNECT: WriteOnlyVirtualParameter(
                    int, self.connect
                ),
                XspressDetectorStr.CMD_DISCONNECT: WriteOnlyVirtualParameter(
                    int,
                    partial(
                        self._put, MessageType.CMD, XspressDetectorStr.CMD_DISCONNECT
                    ),
                ),
                XspressDetectorStr.CMD_SAVE: WriteOnlyVirtualParameter(
                    int,
                    partial(self._put, MessageType.CMD, XspressDetectorStr.CMD_SAVE),
                ),
                XspressDetectorStr.CMD_RESTORE: WriteOnlyVirtualParameter(
                    int,
                    partial(self._put, MessageType.CMD, XspressDetectorStr.CMD_RESTORE),
                ),
                XspressDetectorStr.CMD_START: WriteOnlyVirtualParameter(
                    int,
                    partial(self._put, MessageType.CMD, XspressDetectorStr.CMD_START),
                ),
                XspressDetectorStr.CMD_STOP: WriteOnlyVirtualParameter(
                    int,
                    partial(self._put, MessageType.CMD, XspressDetectorStr.CMD_STOP),
                ),
                XspressDetectorStr.CMD_TRIGGER: WriteOnlyVirtualParameter(
                    int,
                    partial(self._put, MessageType.CMD, XspressDetectorStr.CMD_TRIGGER),
                ),
                XspressDetectorStr.CMD_START_ACQUISITION: WriteOnlyVirtualParameter(
                    int, partial(self.acquire, 1), validate=False
                ),
                XspressDetectorStr.CMD_STOP_ACQUISITION: WriteOnlyVirtualParameter(
                    int, partial(self.acquire, 0), validate=False
                ),
                XspressDetectorStr.CMD_RECONFIGURE: WriteOnlyVirtualParameter(
                    int,
                    self.reconfigure,
                    validators=[
                        lambda x: None
                        if self.acquisition_complete
                        else raise_exception(
                            RuntimeError, "Cannot reconfigure while acquiring"
                        )
                    ],
                ),
            },
            XspressDetectorStr.VERSION: {
                XspressDetectorStr.VERSION_XSPRESS_DETECTOR: {
                    XspressDetectorStr.VERSION_XSPRESS_DETECTOR_FULL: TransparentValueParameter(
                        str, ""
                    ),
                    XspressDetectorStr.VERSION_XSPRESS_DETECTOR_MAJOR: TransparentValueParameter(
                        int, 0
                    ),
                    XspressDetectorStr.VERSION_XSPRESS_DETECTOR_MINOR: TransparentValueParameter(
                        int, 0
                    ),
                    XspressDetectorStr.VERSION_XSPRESS_DETECTOR_PATCH: TransparentValueParameter(
                        int, 0
                    ),
                    XspressDetectorStr.VERSION_XSPRESS_DETECTOR_SHORT: TransparentValueParameter(
                        str, ""
                    ),
                }
            },
            XspressDetectorStr.PROCESS: {
                XspressDetectorStr.PROCESS_NUM_MCA: ReadOnlyVirtualParameter(
                    int, lambda: self.num_process_mca
                ),
                XspressDetectorStr.PROCESS_NUM_LIST: ReadOnlyVirtualParameter(
                    int, lambda: self.num_process_list
                ),
                XspressDetectorStr.PROCESS_NUM_CHANS_MCA: ReadOnlyVirtualParameter(
                    int, lambda: self.num_chan_per_process_mca
                ),
                XspressDetectorStr.PROCESS_NUM_CHANS_LIST: ReadOnlyVirtualParameter(
                    int, lambda: self.num_chan_per_process_list
                ),
            },
        }
        self.parameter_tree = XspressParameterTree(tree)

    @property
    def num_chan_per_process_list(self):
        """
        max_channels: in list mode is mca_channels+1.
        For list mode the last process will sometimes have fewer 'active' channels, as each card has 10 chans (I believe).
        e.g. when mca_channels = 36 and list_channels = 37,
        then num_chan_per_process_list = 5 if num_process_list = 8,
        or num_chan_per_process_list = 40 if num_process_list = 1,
        """
        # TODO: work out why Xspress 4 needed a different number of processors
        # for list mode vs MCA mode
        # return nearest_mult_of_5_up(self.mca_channels) // self.num_process_list
        return self.num_chan_per_process_mca

    @property
    def num_chan_per_process_mca(self):
        """
        this is correct if num_process % mca_channels == 0
        """
        return self.mca_channels // self.num_process_mca

    def set_fr_handler(self, handler):
        self.fr_handler: ApiAdapter = handler
        self.logger.error(f"self.fr_handler set to {handler}")
        req = ApiAdapterRequest(None, accept="application/json")
        resp = self.fr_handler.get(path="config", request=req).data
        logging.error(f"recieved from fr adaptor: {resp}")

    def set_fp_handler(self, handler):
        self.fp_handler: ApiAdapter = handler
        self.logger.error(f"self.fp_handler set to {handler}")
        req = ApiAdapterRequest(None, accept="application/json")
        resp = self.fr_handler.get(path="config", request=req).data
        logging.error(f"recieved from fr adaptor: {resp}")

    async def configure_fps(self, mode: str):
        list_command = {"execute": {"index": "list"}}
        mca_command = {"execute": {"index": "mca"}, "hdf": {"dataset": {}}}
        command = mca_command if mode == XSPRESS_MODE_MCA else list_command
        # TODO: check why Xspress 4 has a different number of FPs and FRs
        num_process = self.num_process_mca
        logging.warning(f"Configuring {num_process} frame processors for mode {mode}")
        # must copy otherwise we'll modify the same dict later
        configs = [copy.deepcopy(command) for _ in range(num_process)]

        if mode == XSPRESS_MODE_MCA:
            dataset_values = {"dims": [1, 4096], "chunks": [1, 1, 4096]}
            for i in range(self.mca_channels):
                fp_index = i // self.num_chan_per_process_mca
                configs[fp_index]["hdf"]["dataset"][f"mca_{i}"] = dataset_values

        tasks = ()
        for client, config in zip(self.fp_clients, configs):
            tasks += (asyncio.create_task(self.async_send_task(client, config)),)
        result = await asyncio.gather(*tasks)

    async def configure_frs(self, mode: str):
        """Configure the frame receivers for the acquisition mode selected

        MCA mode:

        - the control server uses the Xspress library API to read the data
          using multiple worker threads
        - Each worker copies data using the Xspress library from the shared
          memory into Odin buffers - both MCA data and scalars

        List mode:

        - for Xspress 4 the frame receivers connect to each of the UDP ports
          (1 per channel) to get the list mode data
        - for Xspress 3 Mini Mk 2 the frame receivers connect to each TCP port
          (1 per ADC consisting of 2 channels and 2 markers) to get list mode
          data

        Args:
            mode (str): Acquisition mode

        Raises:
            ValueError: raised if an invalid acquisition mode is selected
        """
        if mode == XSPRESS_MODE_LIST:
            # TODO: don't break compatibility with Xspress 4
            #  - IP addresses and ports
            #  - decoder plugin to use
            # One ADC card per process pair (one TCP connection each)
            ip_and_ports = [(f"192.168.0.{card_num+1}", [30125]) for card_num in range(1, self.num_cards+1)]
            self.logger.info(f"Connecting to list mode sockets {ip_and_ports}")
            configs = [
                {
                    "rx_ports": ",".join([str(p) for p in ports]),
                    "rx_type": "tcp",
                    "decoder_type": "X3X2ListMode",
                    "rx_address": ip,
                    "rx_recv_buffer_size": 30000000,
                }
                for ip, ports in ip_and_ports
            ]
        elif mode == XSPRESS_MODE_MCA:
            configs = [
                {
                    "rx_ports": "{},".format(index + 15150),
                    "rx_type": "zmq",
                    "decoder_type": "Xspress",
                    "rx_address": "127.0.0.1",
                }
                for index in range(self.num_process_mca)
            ]
        else:
            raise ValueError("invalid mode requested")

        tasks = ()
        for client, config in zip(self.fr_clients, configs):
            tasks += (asyncio.create_task(self.async_send_task(client, config)),)
        result = await asyncio.gather(*tasks)

    async def async_send_task(self, client, config):
        msg = IpcMessage("cmd", "configure")
        msg.set_params(config)
        await client.wait_till_connected()
        resp = await client.send_recv(msg)
        if resp.get_msg_type() == resp.NACK:
            raise NotAcknowledgeException()
        return resp

    async def reconfigure(self, *unused):
        mode = (
            self.mode
        )  # save local copy so the value doesn't change from under our feet

        # This seems to help when dealing with detectors that don't have 8 channels.
        # Apparently when the detector has a number of channels different from that defined in 'DEFAULT_MAX_CHANNELS',
        # at XspressDetector.h, we have some problems when trying to connect to it straight away using the reconfigure button.
        await self.reset()
        await asyncio.sleep(FR_INIT_TIME[mode])

        resp = await self._async_client.send_recv(self.configuration.get())
        # resp = await self._put(MessageType.CONFIG, XspressDetectorStr.CONFIG_CONFIG_PATH, self.settings_paths[mode])
        resp = await self._put(MessageType.CMD, XspressDetectorStr.CMD_DISCONNECT, 1)
        # TODO: don't break compatibility with Xspress 4
        chans = self.mca_channels #  if mode == XSPRESS_MODE_MCA else self.mca_channels + 1
        await self._put(
            MessageType.CONFIG, XspressDetectorStr.CONFIG_MAX_CHANNELS, chans
        )
        self.max_channels = chans
        await self.configure_frs(mode)
        await self.configure_fps(mode)
        await asyncio.sleep(FR_INIT_TIME[mode])
        resp = await self.connect()
        if mode == XSPRESS_MODE_MCA:
            resp = await self._async_client.send_recv(self.configuration.get_daq())
        return resp

    async def connect(self, *unused):
        msg = _build_message(MessageType.CMD, {XspressDetectorStr.CMD_CONNECT: 1})
        return await self._async_client.send_recv(msg, timeout=20)

    async def acquire(self, value, *unused):
        if value:
            reply = await self._put(MessageType.CMD, XspressDetectorStr.CMD_START, 1)
            self.acquisition_complete = False
            return reply
        else:
            return await self._put(MessageType.CMD, XspressDetectorStr.CMD_STOP, 1)

    async def set_mode(self, value):
        if value == 0:
            return await self._put(
                MessageType.CONFIG, XspressDetectorStr.CONFIG_MODE, XSPRESS_MODE_MCA
            )
        else:  # mode == "list"
            return await self._put(
                MessageType.CONFIG, XspressDetectorStr.CONFIG_MODE, XSPRESS_MODE_LIST
            )

    async def do_updates(self, value: int):
        if value:
            self.sched.start()
        else:
            await self.sched.stop()

    def _set(self, attr_name, value):
        setattr(self, attr_name, value)

    async def _put(self, message_type: MessageType, config_str: str, value: any):
        self.logger.debug(debug_method())
        if not self._async_client.is_connected():
            raise ConnectionError(
                "Control server is not connected! Check if it is running and tcp endpoint is configured correctly"
            )
        msg = _build_message(message_type, {config_str: value})
        resp = await self._async_client.send_recv(msg)
        return resp

    def get(self, path):
        return self.parameter_tree.get(path)

    def configure(
        self,
        num_cards,
        num_tf,
        base_ip,
        max_channels,
        max_spectra,
        settings_path,
        run_flags,
        debug,
        daq_endpoints,
    ):
        self.logger.critical(debug_method())
        self.max_channels = max_channels
        self.mca_channels = max_channels
        self.num_cards = num_cards
        self.max_spectra = max_spectra
        self.run_flags = run_flags
        self.fr_clients = [
            AsyncClient("127.0.0.1", 10000 + (10 * port_offset))
            for port_offset in range(self.num_process_mca)
        ]
        self.fp_clients = [
            AsyncClient("127.0.0.1", 10004 + (10 * port_offset))
            for port_offset in range(self.num_process_mca)
        ]

        x = XspressDetectorStr
        logging.info("MessageType.CONFIG = {}".format(MessageType.CONFIG))

        class Configuration:
            has_been_called_once = False

            def __init__(self):
                self.initial_config_msg = _build_message(
                    MessageType.CONFIG,
                    {
                        x.CONFIG_NUM_CARDS: num_cards,
                        x.CONFIG_NUM_TF: num_tf,
                        x.CONFIG_BASE_IP: base_ip,
                        x.CONFIG_MAX_CHANNELS: max_channels,
                        x.CONFIG_MCA_CHANNELS: max_channels,
                        x.CONFIG_MAX_SPECTRA: max_spectra,
                        x.CONFIG_CONFIG_PATH: settings_path,
                        x.CONFIG_DEBUG: debug,
                    },
                )
                self.re_config_message = _build_message(
                    MessageType.CONFIG,
                    {  # these are constants that the user is unlikely to change
                        x.CONFIG_NUM_CARDS: num_cards,
                        x.CONFIG_BASE_IP: base_ip,
                        x.CONFIG_MAX_SPECTRA: max_spectra,
                    },
                )
                self.initial_daq_msg = _build_message(
                    MessageType.DAQ,
                    {
                        x.CONFIG_DAQ_ZMQ_ENDPOINTS: daq_endpoints,
                        x.CONFIG_DAQ_ENABLED: True,
                    },
                )

            def get(self):
                if self.has_been_called_once:
                    return self.re_config_message
                else:
                    self.has_been_called_once = True
                    return self.initial_config_msg

            def get_daq(self):
                return self.initial_daq_msg

            def get_initial(self):
                return self.initial_config_msg

        self.configuration = Configuration()
        self.sched.start()

    async def reset(self, *unused):
        resp = await self._async_client.send_recv(self.configuration.get_initial())
        if self.mode == XSPRESS_MODE_MCA:
            resp = await self._async_client.send_recv(self.configuration.get_daq())
        return resp

    async def read_config(self, *unused):
        msg = _build_message(MessageType.REQUEST)
        ipc_msg = await self._async_client.send_recv(msg, loud=False)
        BASE_PATH = ""
        if not ipc_msg.is_valid():
            raise XspressDetectorException("IpcMessage recieved is not valid")
        self.config_raw = ipc_msg.get_params()
        self._param_tree_set_recursive(BASE_PATH, ipc_msg.get_params())
        return ipc_msg

    def _param_tree_set_recursive(self, path, params):
        if not isinstance(params, dict):
            try:
                path = path.strip("/")
                self.parameter_tree.set(path, params)
                # self.logger.debug(f"XspressDetector._param_tree_set_recursive: parameter path {path} was set to {params}")
            except KeyError as e:
                self.logger.error(e)
                self.logger.warning(
                    (
                        f"XspressDetector._param_tree_set_recursive: parameter path {path} is not in parameter tree\n"
                        f"params = {params}"
                    )
                )
        else:
            for param_name, params in params.items():
                self._param_tree_set_recursive(f"{path}/{param_name}", params)
