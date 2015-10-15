#!/bin/sh
set -x
set -e
echo $CXX
cppcheck --enable=all --std=c99 --error-exitcode=1 \
    --suppressions-list=misc/.cppcheck-supp -Iinclude src tests
python misc/generate_hash_dataset.py tests/resources/keys.txt
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DWITH_TESTING=ON  ..
make -j8
valgrind --leak-check=full make test
cd ..
python setup.py test
python setup.py install
./tests/shabby/run_test.sh
cd src
go test
