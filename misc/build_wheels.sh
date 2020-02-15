#!/bin/sh
set -ex

# Compile wheels
for CPYVER in $DIST_VERSIONS; do
    "/opt/python/${CPYVER}/bin/pip" wheel /io/ -w wheelhouse/
done

# Bundle external shared libraries into the wheels
for whl in wheelhouse/*.whl; do
    auditwheel repair "$whl" --plat $PLAT -w /io/wheelhouse/
done

chmod -R 0777 /io/wheelhouse/
