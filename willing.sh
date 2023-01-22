#!/bin/sh
# This file is a part of the LDM package
# It's an example script that can be called by ldm
# the script will be startet if an XDMCP message is received
# the script is called with the ip address of client
# if the script returns 0 LDM will accept the connection
# in any other case LDM will not responde

echo request from: $*

exit 0;
