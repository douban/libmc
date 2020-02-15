#!/bin/sh
set -ex

docker run --rm \
    -v `pwd`:/io \
    -e DIST_VERSIONS="cp27-cp27mu cp35-cp35m cp36-cp36m cp37-cp37m cp38-cp38" \
    -e PLAT=$PLAT \
    quay.io/pypa/$PLAT \
    /io/misc/build-wheels.sh

mkdir dist
ls -lh wheelhouse/*${PLAT}.whl
cp wheelhouse/*${PLAT}.whl dist/
