#!/bin/sh
set -ex

python setup.py install
python ./misc/runbench.py
