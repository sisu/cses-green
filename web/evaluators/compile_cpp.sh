#!/bin/bash

ln -s "$IN/source" s.cpp
echo compiling
echo $PWD
ls $PWD
g++ s.cpp -std=gnu++0x -Wall -O2 -o binary > stdout 2> stderr
echo compiling done
ls
