#!/bin/bash
echo -3 > status
echo runsafe: "$RUNSAFE"
#"$RUNSAFE" $IN/binary < $IN/input > stdout 2> stderr && echo 1 > status
$IN/binary $IN/output $IN/correct $IN/input > stdout 2> stderr && echo 1 > status
