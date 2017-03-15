#!/bin/bash

mkfifo -f /tmp/test-fifo

#stdbuf -o0 pocketsphinx_continuous -jsgf conf/desk.jsgf -dict conf/desk.dic -inmic yes -kws_threshold 1e-03 -logfn /dev/null | stdbuf -o0  egrep -v "^READY|^Listening" > /tmp/test-fifo

stdbuf -o0 pocketsphinx_continuous -jsgf conf/desk.jsgf -dict conf/desk.dic -inmic yes -kws_threshold 1e-03 -adcdev /dev/audio1  -logfn /dev/null | stdbuf -o0 egrep -v "^READY|^Listening" > /tmp/test-fifo
