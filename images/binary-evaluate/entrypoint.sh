#!/bin/bash

set -e

chown uolevi:uolevi /cses_judge/
chmod 700 /cses_judge/

sudo -u uolevi /imageinit/evaluate.sh
exit
