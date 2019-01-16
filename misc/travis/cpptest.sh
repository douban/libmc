#!/bin/bash

set -xe

echo $CXX
./misc/memcached_server start
python misc/generate_hash_dataset.py tests/resources/keys.txt
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DWITH_TESTING=ON  ..
make -j8
make test
cd ..
./misc/memcached_server stop
