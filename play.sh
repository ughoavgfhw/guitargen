#!/bin/bash

make CFLAGS=-O3 LDFLAGS=-lm guitargen
./guitargen | aplay -f cd -t raw -c 1 2> /dev/null
# 48000 sample rate: -f dat
