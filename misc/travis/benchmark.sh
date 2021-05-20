#!/bin/sh
set -ex

pip install -e .
python ./misc/runbench.py
