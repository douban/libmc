cmake_minimum_required(VERSION 2.6)
project(gtest_builder C CXX)
include(ExternalProject)

ExternalProject_Add(googletest
    # SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk
    GIT_REPOSITORY https://github.com/svn2github/googletest.git
    CMAKE_ARGS -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG:PATH=DebugLibs
               -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE:PATH=ReleaseLibs
               -Dgtest_force_shared_crt=ON
    PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/include PARENT_SCOPE)

ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir} PARENT_SCOPE)
