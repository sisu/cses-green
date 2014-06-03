#!/bin/bash
# Usage: ./run_boxed.sh <time> <memory> <space-limit> <program> [program args]
# NOTE: This should be run as the user who runs the program.

t=$1
mem=$2
space=$3
prog=$4

ulimit -t $t
if [ $mem != 0 ]; then ulimit -u 10 -v $mem; else ulimit -u 1000; fi
ulimit -f $space
ulimit -s unlimited
timeout $1 ${@:4}
