#!/bin/sh
set -x
set -e
cppcheck --enable=all --error-exitcode=1 \
    --suppressions-list=misc/.cppcheck-supp -Iinclude src tests
