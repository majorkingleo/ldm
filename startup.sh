#!/bin/sh
# This file is a part of the LDM package
# It's an example script that can be called by ldm
# the script will be calles if a client send a MANAGE message 
# and the willing script has already accepted the connection
# the script is called as root you will have to switch to your
# autollgin user and call a startup script. In this example it's
# the startall.sh script which is also shipped with this package

su martin -c /home/martin/programs/ldm/startall.sh 
