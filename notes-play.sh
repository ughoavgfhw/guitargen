#!/bin/bash

make guitargen && \
(sleep 0.25; printf '\x01\x00\xff'; sleep 0.2; printf '\x02\x00\xff'; \
 sleep 0.5; printf '\x01\x05\xff'; sleep 3; printf '\x01\x01\xff'; sleep 1; \
 printf '\x02\x01\xff'; sleep 0.5; printf '\x01\x06\xff'; sleep 0.25; \
 printf '\x01\x02\xff'; sleep 6) | ./guitargen
