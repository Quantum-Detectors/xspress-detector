#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

export HDF5_PLUGIN_PATH=prefix/hdf5_1.10/h5plugin

prefix/bin/frameProcessor --ctrl=tcp://0.0.0.0:10014 --ready=tcp://127.0.0.1:10011 --release=tcp://127.0.0.1:10012 --json_file=$SCRIPT_DIR/fp2.json --logconfig $SCRIPT_DIR/log4cxx.xml
