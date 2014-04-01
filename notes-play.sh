#!/bin/bash

(sleep 0.25; echo -n A2; sleep 0.2; echo -n B2; sleep 0.5; echo -n C3; sleep 3; echo -n D3; sleep 1; echo -n E3; sleep 0.5; echo -n F3; sleep 0.25; echo -nG3; sleep 6) | ./play.sh
