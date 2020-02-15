#!/bin/bash
set -ex

echo "CXX=${CXX}"
./misc/memcached_server start &>/dev/null &
python misc/generate_hash_dataset.py tests/resources/keys.txt &>/dev/null &
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DWITH_TESTING=ON .. &>/dev/null
make -j8 &>/dev/null
wait
ARGS=-V make test
cd ..
./misc/memcached_server stop &>/dev/null &
