#!/bin/bash
set -ex
VER="0.16.3"
URL="https://github.com/gabime/spdlog/archive/v${VER}.tar.gz"
echo "Downloading $URL"
curl $URL -L -o /tmp/spdlog.tar.gz
tar xvf /tmp/spdlog.tar.gz
rm -rf spdlog
cp -r spdlog-${VER}/include/spdlog  spdlog
rm -rf spdlog-${VER}
patch -p2 < spdlog.patch
echo "Use git status, add all files and commit changes."
