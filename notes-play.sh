#!/bin/bash

make guitargen && \
(sleep 0.25; echo -n 2A; sleep 0.2; echo -n B; sleep 0.5; echo -n 3C; \
 sleep 3; echo -n D; sleep 1; echo -n E; sleep 0.5; echo -n F; \
 sleep 0.25; echo -nG; sleep 6) | ./guitargen
