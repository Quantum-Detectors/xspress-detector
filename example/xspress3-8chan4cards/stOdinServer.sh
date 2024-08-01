#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"

prefix/bin/xspress_control --config=$SCRIPT_DIR/odin_server.cfg --logging=info --access_logging=ERROR --graylog_server graylog2.diamond.ac.uk:12210 --graylog_static_fields beamline=${BEAMLINE},application_name=xspress_control,detector=Xspress3
