cmake_minimum_required(VERSION 2.6)
project(mc)
set(CMAKE_MACOSX_RPATH 1)

add_definitions(-DMC_USE_SMALL_VECTOR)

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_COMMON "-Wall -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g2 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX DEBUG FLAGS" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX RELEASE FLAGS" FORCE)

FILE(
    GLOB mc_HEADERS
    include/*.h
    include/hashkit/*.h
    include/llvm/*.h
    include/rapidjson/*.h
)
FILE(
    GLOB mc_SOURCES
    src/*.cpp
    src/hashkit/*.cpp
)

include_directories(include)

add_library(mc STATIC ${mc_SOURCES})


set(EXT_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/ext)
add_subdirectory(${EXT_PROJECTS_DIR}/gtest)

enable_testing()
add_subdirectory(tests)
