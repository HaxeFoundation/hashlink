cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0042 NEW)

include(GNUInstallDirs)
include(FindPkgConfig)

if (WIN32)
    project(hashlink C CXX) # C++ required for directx
else()
    project(hashlink C)
endif()

# force Unicode over Multi-byte
if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/other/cmake ${CMAKE_MODULE_PATH})

if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_C_FLAGS "-std=c11 ${CMAKE_C_FLAGS}")
else ()
    set (CMAKE_C_STANDARD 11)
endif ()

# put output in "bin"

set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# avoid the extra "Debug", "Release" directories
# http://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set(HL_VERSION_MAJOR 0)
set(HL_VERSION_MINOR 1)
set(HL_VERSION_PATCH 0)
set(HL_VERSION ${HL_VERSION_MAJOR}.${HL_VERSION_MINOR}.${HL_VERSION_PATCH})

include_directories(
    src
    include/pcre
)

file(GLOB pcre_srcs
    include/pcre/*.c
)
file(GLOB std_srcs
    src/std/array.c
    src/std/buffer.c
    src/std/bytes.c
    src/std/cast.c
    src/std/date.c
    src/std/error.c
    src/std/file.c
    src/std/fun.c
    src/std/maps.c
    src/std/math.c
    src/std/obj.c
    src/std/random.c
    src/std/regexp.c
    src/std/socket.c
    src/std/string.c
    src/std/sys.c
    src/std/types.c
    src/std/ucs2.c
    src/std/thread.c
    src/std/process.c
)

if (ANDROID)
    list(APPEND std_srcs
        src/std/sys_android.c
    )
endif()

if (IOS_PLATFORM)
    list(APPEND std_srcs
        src/std/sys_ios.m
    )
endif()

add_library(libhl SHARED
    ${pcre_srcs}
    src/alloc.c
    ${std_srcs}
)

set_target_properties(libhl
    PROPERTIES
        PUBLIC_HEADER
            "src/hl.h;src/hlc.h;src/hlc_main.c"
)

if(WIN32)
    set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME libhl
    )
else()
    set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME hl
    )
endif()

set_target_properties(libhl
    PROPERTIES
    VERSION ${HL_VERSION}
    SOVERSION ${HL_VERSION_MAJOR}
    COMPILE_DEFINITIONS "_USRDLL;LIBHL_EXPORTS"
)

add_executable(hl
    src/code.c
    src/jit.c
    src/main.c
    src/module.c
    src/debugger.c
)

target_link_libraries(hl libhl)

if(WIN32)
    target_link_libraries(libhl ws2_32 user32)
    target_link_libraries(hl user32)
else()
    target_link_libraries(libhl m dl pthread)
endif()

#####################
# hello.hl

find_program(
    HAXE_COMPILER
    haxe
)

add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hello.hl
    COMMAND ${HAXE_COMPILER} -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hello.hl -cp ${CMAKE_SOURCE_DIR}/other/tests -main HelloWorld
)
add_custom_target(hello.hl ALL
    DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hello.hl
)

#####################
# Tests

include(CTest)
add_test(NAME hello.hl
    COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/hello.hl
)

#####################
# Packaging

set(CPACK_PACKAGE_VENDOR "Haxe Foundation")
set(CPACK_PACKAGE_VERSION_MAJOR ${HL_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HL_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HL_VERSION_PATCH})
set(CPACK_GENERATOR ZIP)

install(
    TARGETS
        hl
        libhl
    DESTINATION .
    PUBLIC_HEADER DESTINATION include
)

if (DEFINED ENV{APPVEYOR_REPO_COMMIT})
    set(CPACK_PACKAGE_VERSION $ENV{APPVEYOR_REPO_COMMIT})
endif()

include(CPack)

add_subdirectory(libs)
