#!/bin/bash
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd )"
LD_LIBRARY_PATH=prefix/control/libxspress-wrapper/support/ prefix/bin/xspressControl -j $SCRIPT_DIR/xspress.json