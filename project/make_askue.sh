#!/bin/sh

export LIBRARY_PATH=/usr/lib/i386-linux-gnu:$LIBRARY_PATH

if [ ! -d "../bin" ]; then
    mkdir "../bin"
fi
make
