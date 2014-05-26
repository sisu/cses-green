#!/bin/bash

set -e

ln -s /cses_judge/input/source /tmp/source.cpp
g++ /tmp/source.cpp -o /cses_judge/output/binary -std=c++11 -O2 -Wall > /cses_judge/output/stdout 2> /cses_judge/output/stderr
exit
