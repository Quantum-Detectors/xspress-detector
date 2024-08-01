#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

prefix/bin/frameReceiver --sharedbuf=odin_buf_2 -m 1048576000 --iothreads 1 --ctrl=tcp://0.0.0.0:10010 --ready=tcp://127.0.0.1:10011 --release=tcp://127.0.0.1:10012 --json_file=$SCRIPT_DIR/fr2.json --logconfig $SCRIPT_DIR/log4cxx.xml
