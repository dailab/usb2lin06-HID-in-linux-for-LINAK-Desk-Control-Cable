#!/bin/bash

FIFO=/var/run/desk-control


if [ ! -e $FIFO ]; then
        echo "FIFO does not exit, creating it...";
        mkfifo $FIFO
elif [ ! -p $FIFO ]; then
        echo "FIFO is a regular file, creating it...";
        rm -f $FIFO
        mkfifo $FIFO
fi
 
/usr/bin/stdbuf -o0 /usr/bin/pocketsphinx_continuous \
        -jsgf /usr/share/desk-control/desk.jsgf \
        -dict /usr/share/desk-control/desk.dic \
        -inmic yes -kws_threshold 1e-03 \
        -adcdev /dev/audio1  -logfn /dev/null \
        | /usr/bin/stdbuf -o0 /bin/egrep -v "^READY|^Listening" \
        > $FIFO
