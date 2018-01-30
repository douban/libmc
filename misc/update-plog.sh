#!/bin/bash
set -ex
VER="1.1.3"
URL="https://github.com/SergiusTheBest/plog/archive/${VER}.tar.gz"
echo "Downloading $URL"
curl $URL -L -o /tmp/plog.tar.gz

cd include
tar xvf /tmp/plog.tar.gz
rm -rf plog
cp -r plog-${VER}/include/plog plog
rm -rf plog-${VER}

rm plog/Converters/NativeEOLConverter.h
rm plog/Appenders/AndroidAppender.h
rm plog/Appenders/ColorConsoleAppender.h
rm plog/Appenders/DebugOutputAppender.h
rm plog/Appenders/EventLogAppender.h
rm plog/Formatters/FuncMessageFormatter.h
rm plog/Formatters/MessageOnlyFormatter.h
python ../misc/rm_plog_win32_code.py

echo "Use git status, add all files and commit changes."
