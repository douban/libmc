#!/bin/sh
set -x
set -e
python setup.py install
python ./misc/runbench.py
