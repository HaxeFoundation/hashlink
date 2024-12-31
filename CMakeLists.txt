cmake_minimum_required(VERSION 3.1)

set(HL_VERSION_MAJOR 1)
set(HL_VERSION_MINOR 10)
set(HL_VERSION_PATCH 0)
set(HL_VERSION ${HL_VERSION_MAJOR}.${HL_VERSION_MINOR}.${HL_VERSION_PATCH})

cmake_policy(SET CMP0042 NEW)

if(WIN32)
    project(hashlink C CXX) # C++ required for directx
else()
    project(hashlink C)
endif()

include(GNUInstallDirs)
include(FindPkgConfig)
include(CTest)

# force Unicode over Multi-byte
if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/other/cmake
)

if(CMAKE_VERSION VERSION_LESS "3.1")
    set(CMAKE_C_FLAGS "-std=c11 ${CMAKE_C_FLAGS}")
else()
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_C_STANDARD_REQUIRED ON)
endif()

# put output in "bin"

set(OUTPUT_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

include_directories(
    src
    include/pcre
)

file(GLOB pcre_srcs
    include/pcre/*.c
)

file(GLOB srcs
    src/code.c
    src/debugger.c
    src/gc.c
    src/jit.c
    src/module.c
    src/std/*.c
)

list(REMOVE_ITEM srcs "src/allocator.c")

add_library(libhl SHARED
    ${pcre_srcs}
    ${srcs}
)

set(public_headers
    src/hl.h
    src/hlc.h
)

set_target_properties(libhl
    PROPERTIES
    PUBLIC_HEADER "${public_headers}"
)

set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME libhl
)

set_target_properties(libhl
    PROPERTIES
    COMPILE_DEFINITIONS "_USRDLL;LIBHL_EXPORTS"
)

set_target_properties(libhl
    PROPERTIES
    VERSION ${HL_VERSION}
    SOVERSION ${HL_VERSION_MAJOR}
    COMPILE_DEFINITIONS "_USRDLL;LIBHL_EXPORTS"
)


if(WIN32)
    target_link_libraries(libhl ws2_32 user32)
else()
    target_link_libraries(libhl m dl pthread)
endif()


#add_subdirectory(libs)
