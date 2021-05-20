#!/bin/sh
set -ex

echo "CXX=${CXX}"
python misc/generate_hash_dataset.py tests/resources/keys.txt
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DWITH_TESTING=ON  ..
make -j8
valgrind --leak-check=full make test
cd ..
python setup.py test
pip install -e .
./tests/shabby/run_test.sh
