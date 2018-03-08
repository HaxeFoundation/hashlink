# Toolchain file for building a 32bit version on a 64bit host.
#
# Usage:
# cmake -DCMAKE_TOOLCHAIN_FILE=other/cmake/linux32.toolchain.cmake <sourcedir>

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32" CACHE STRING "c++ flags")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32" CACHE STRING "c flags")
