#!/bin/sh
set -e

pip install --upgrade wheel twine
python setup.py sdist
twine upload \
    --non-interactive \
    --skip-existing \
    --verbose \
    --repository-url https://upload.pypi.org/legacy/ \
    dist/*
