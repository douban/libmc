#!/bin/sh
set -e
python setup.py install
python ./misc/runbench.py
