#!/bin/sh

VPYTHON=Python-2.7.2 

EXT_PYTHON=$XMIPP_HOME/external/python 
export LD_LIBRARY_PATH=$EXT_PYTHON/$VPYTHON:$LD_LIBRARY_PATH 
export PYTHONPATH=$XMIPP_HOME/lib:$XMIPP_HOME/protocols:$XMIPP_HOME/applications/tests/pythonlib:$XMIPP_HOME/lib/python2.7/site-packages:$PYTHONPATH 

xmipp_volume_align_prog "$@"
