#!/bin/sh
set -ex

python tests/shabby/slow_memcached_server.py &
pid=$!
echo "pid of slow memcached server: $pid"

