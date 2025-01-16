#!/bin/bash

# device stable delay
# sleep 10 && sync

#--------------------------
# ODROID-C4 NLP Server enable
#--------------------------
/usr/bin/sync && /root/nlp_server.new/nlp_server.new > /dev/null 2>&1
