cmake_minimum_required(VERSION 2.8.2)
project(libmc)
set(CMAKE_MACOSX_RPATH 1)

set (MC_VERSION_MAJOR 1)
set (MC_VERSION_MINOR 4)
set (MC_VERSION_PATCH 1)

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
    # Download and unpack googletest at configure time
    configure_file(ext/gtest/CMakeLists.txt.in ext/gtest/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ext/gtest )
    if(result)
      message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
      RESULT_VARIABLE result
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/ext/gtest )
    if(result)
      message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Prevent overriding the parent project's compiler/linker
    # settings on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_BINARY_DIR}/googletest-src
                     ${CMAKE_BINARY_DIR}/googletest-build
                     EXCLUDE_FROM_ALL)

    # The gtest/gtest_main targets carry header search path
    # dependencies automatically when using CMake 2.8.11 or
    # later. Otherwise we have to add them here ourselves.
    if (CMAKE_VERSION VERSION_LESS 2.8.11)
      include_directories("${gtest_SOURCE_DIR}/include")
    endif()

    # Now simply link against gtest or gtest_main as needed. Eg
    enable_testing()
    add_subdirectory(tests)
endif (WITH_TESTING)
