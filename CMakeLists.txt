cmake_minimum_required(VERSION 2.6)
project(libmc)
set(CMAKE_MACOSX_RPATH 1)

set (MC_VERSION_MAJOR 0)
set (MC_VERSION_MINOR 5)
set (MC_VERSION_PATCH 5)

set (MC_VERSION ${MC_VERSION_MAJOR}.${MC_VERSION_MINOR})
set (MC_APIVERSION ${MC_VERSION}.${MC_VERSION_PATCH})

add_definitions(-DMC_USE_SMALL_VECTOR)

if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_COMMON "-Wall -fno-rtti -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -g2 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX DEBUG FLAGS" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG -O3 ${CMAKE_CXX_FLAGS_COMMON}" CACHE STRING "CXX RELEASE FLAGS" FORCE)
set(CMAKE_INSTALL_INCLUDE include CACHE PATH "Output directory for header files")
set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

include_directories(include)

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
)
add_library(mc STATIC ${mc_SOURCES})

install(
    FILES include/c_client.h
    DESTINATION ${CMAKE_INSTALL_INCLUDE}/${CMAKE_PROJECT}
)
install(
    TARGETS mc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    EXPORT mc
)

option(WITH_TESTING "Enable testing" OFF)
if (WITH_TESTING)
    set(EXT_PROJECTS_DIR ${CMAKE_SOURCE_DIR}/ext)
    add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
    enable_testing()
    add_subdirectory(tests)
endif (WITH_TESTING)
