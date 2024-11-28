"""
Created on March 2022

:author: Alan Greer
"""
import json
import logging
import os
import asyncio
import time

from odin_data.control.odin_data_adapter import OdinDataAdapter
from odin_data.control.fp_compression_adapter import FPCompressionAdapter
from odin.adapters.adapter import (
    ApiAdapterResponse,
    request_types,
    response_types,
)
from tornado import escape
from tornado.escape import json_encode, json_decode


def bool_from_string(value):
    bool_value = False
    if value.lower() == 'true' or value.lower() == '1':
        bool_value = True
    return bool_value


class FPXspressAdapter(FPCompressionAdapter):
    """
    FPXspressAdapter class

    This class provides control of FP applications that are running as part of an
    Xspress detector

    The rank must be 0 for every FP and the number of processes set to 1.
    The number of frames and acquisition ID must also be sent to the process
    plugin of the FP.

    This class will override the setup_rank method to apply the corrections
    for an Xspress detector.
    """
    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        logging.debug("FPXspressAdapter init called")
        self._xsp_adapter = None
        self._xsp_adapter_name = "xspress"
        self._num_channels = 0
        self._num_process = 1
        self._mca_per_client = 0
        self._batch_size = 0
        super(FPXspressAdapter, self).__init__(**kwargs)

    def initialize(self, adapters):
        """Initialize the adapter after it has been loaded.
        Find and record the FR adapter for later error checks
        """
        logging.info("Intercepting the Xspress control adapter")
        if self._xsp_adapter_name in adapters:
            self._xsp_adapter = adapters[self._xsp_adapter_name]
            logging.info(
                "FP adapter initiated connection to Xspress adapter: {}".format(
                    self._xsp_adapter_name
                )
            )
            self._num_channels = self._xsp_adapter.detector.mca_channels
            self._num_process = self._xsp_adapter.detector.num_process_mca
            self._mca_per_client = self._num_channels // self._num_process
            self.data_datasets = ["mca_" + str(i) for i in range(self._num_channels)]
        else:
            logging.error("FP adapter could not connect to the Xspress adapter: {}".format(self._xsp_adapter_name))
        super(FPXspressAdapter, self).initialize(adapters)

    @request_types('application/json', 'application/vnd.odin-native')
    @response_types('application/json', default='application/json')
    def put(self, path, request):  # pylint: disable=W0613
        if path.startswith("config/hdf/dataset/data"):
            self.configure_data_datasets(path,request)
        if path.startswith("config/offset/offset_adjustment"):
            value = json_decode(request.body)
            self.configure_dataset_offset(value)
        if path == self._command:
            # Check the mode we are running in (mca or list)
            mode = self._xsp_adapter.detector.mode
            if mode == "list":
                num_clients = self._xsp_adapter.detector.num_process_list
                write = bool_from_string(str(escape.url_unescape(request.body)))
                if write:
                    config = {'hdf': {'write': write}}
                    # Before attempting to write files, make some simple error checks

                    # Check if we have a valid buffer status from the FR adapter
                    valid, reason = self.check_fr_status()
                    if not valid:
                        raise RuntimeError(reason)

                    # Check the file path is valid
                    if not os.path.isdir(str(self._param['config/hdf/file/path'])):
                        raise RuntimeError("Invalid path specified [{}]".format(str(self._param['config/hdf/file/path'])))
                    # Check the filename exists
                    if str(self._param['config/hdf/file/name']) == '':
                        raise RuntimeError("File name must not be empty")

                    # First setup the rank for the frameProcessor applications
                    self.setup_rank()
                    rank = 0
                    for client in self._clients[0:num_clients]:
                        # Send the configuration required to setup the acquisition
                        # The file path is the same for all clients
                        parameters = {
                            'hdf': {
                                'frames': self._param['config/hdf/frames']
                            }
                        }
                        # Send the number of frames first
                        client.send_configuration(parameters)
                        parameters = {
                            'hdf': {
                                'acquisition_id': self._param['config/hdf/acquisition_id'],
                                'file': {
                                    'path': str(self._param['config/hdf/file/path']),
                                    'name': str(self._param['config/hdf/file/name']),
                                    'extension': str(self._param['config/hdf/file/extension'])
                                }
                            }
                        }
                        client.send_configuration(parameters)
                        rank += 1
                    for client in self._clients[0:num_clients]:
                        # Send the configuration required to start the acquisition
                        client.send_configuration(config)
                else:
                    # Flush and close FPs in list mode
                    for client in self._clients[0:num_clients]:
                        try:
                            parameters = {
                                'xspress-list': {
                                    'flush': True
                                }
                            }
                            client.send_configuration(parameters)
                        except Exception as err:
                            logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                            logging.error("Error: %s", err)

                status_code = 200
                response = {}
                logging.info("Xspress FP adapter flushed in list mode")
                return ApiAdapterResponse(response, status_code=status_code)

        return super(FPXspressAdapter, self).put(path, request)


    def setup_rank(self):
        for client in self._clients:
            # First send a message to XspressProcessPlugin, to update how many MCA are going send per batch.
            try:
                parameters = {
                    "hdf": {
                        "process": {
                            "number": 1,
                            "rank": 0,
                        },
                    },
                    "xspress": {
                        "frames": self._param["config/hdf/frames"],
                        "acq_id": self._param["config/hdf/acquisition_id"],
                        "chunks": int(self._batch_size),
                    },
                    "xspress-list": {"reset": True},
                }
                logging.warning("Sending: {}".format(parameters))
                client.send_configuration(parameters)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)

    def configure_dataset_offset(self, offset):
        for client in self._clients:
            try:
                parameters = {
                    "xspress": {
                        "offset": int(offset),
                    },
                }
                logging.warning("Sending: {}".format(parameters))
                client.send_configuration(parameters)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)

    def configure_data_datasets(self, path, request):
        if "chunks" in str(escape.url_unescape(request.body)):
            value = json_decode(request.body)
            self._batch_size = value["chunks"][0]
        for client in range(self._num_process):
            for dataset in self.data_datasets[
                client * self._mca_per_client : (client + 1) * self._mca_per_client
            ]:
                dataset_path = path.replace(
                    "config/hdf/dataset/data",
                    "config/hdf/dataset/{}".format(dataset) + "/{}".format(client),
                )
                super(FPCompressionAdapter.__base__, self).put(dataset_path, request)
