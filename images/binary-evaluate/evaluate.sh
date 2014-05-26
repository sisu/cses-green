#!/bin/bash

set -e

/cses_judge/input/binary \
	/cses_judge/input/output \
	/cses_judge/input/correct \
	/cses_judge/input/input \
	> /cses_judge/output/result \
	2> /cses_judge/output/stderr
exit
