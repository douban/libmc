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
rm spdlog/details/async_log_helper.h
rm spdlog/details/async_logger_impl.h
rm spdlog/details/file_helper.h
rm spdlog/details/mpmc_bounded_q.h
rm spdlog/sinks/android_sink.h
rm spdlog/sinks/ansicolor_sink.h
rm spdlog/sinks/dist_sink.h
rm spdlog/sinks/file_sinks.h
rm spdlog/sinks/msvc_sink.h
rm spdlog/sinks/null_sink.h
rm spdlog/sinks/ostream_sink.h
rm spdlog/sinks/syslog_sink.h
rm spdlog/sinks/wincolor_sink.h
rm spdlog/sinks/windebug_sink.h
patch -p2 < spdlog.patch
echo "Use git status, add all files and commit changes."
