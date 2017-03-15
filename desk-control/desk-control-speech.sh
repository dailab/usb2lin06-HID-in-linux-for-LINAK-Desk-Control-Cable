#!/bin/bash

/usr/bin/stdbuf -o0 /usr/bin/pocketsphinx_continuous \
	-jsgf /usr/share/desk-control/desk.jsgf \
	-dict /usr/share/desk-control/desk.dic \
	-inmic yes -kws_threshold 1e-03 \
	-adcdev /dev/audio1  -logfn /dev/null \
	| /usr/bin/stdbuf -o0 /bin/egrep -v "^READY|^Listening" \
	> /var/run/desk-control

