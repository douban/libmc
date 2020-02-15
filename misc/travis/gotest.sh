#!/bin/sh
set -ex

echo "CXX=${CXX}"
go version
cd golibmc
go test
