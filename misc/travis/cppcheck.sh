#!/bin/sh
set -ex

# NOTE:(everpcpc) ubuntu:bionic with cppcheck 1.82 introduce a bug:
# https://bugs.launchpad.net/ubuntu/+source/cppcheck/+bug/1763841
# https://github.com/danmar/cppcheck/pull/1026
cppcheck --enable=all --error-exitcode=1 \
    --suppressions-list=misc/.cppcheck-supp -Iinclude src tests
