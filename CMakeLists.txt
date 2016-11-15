cmake_minimum_required(VERSION 2.8.12)

include(GNUInstallDirs)
project(hashlink C)

if (CMAKE_VERSION VERSION_LESS "3.1")
    set (CMAKE_C_FLAGS "-std=c11 ${CMAKE_C_FLAGS}")
else ()
    set (CMAKE_C_STANDARD 11)
endif ()

# put output in "bin"

set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)
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
)

add_library(libhl SHARED
    ${pcre_srcs}
    src/alloc.c
    ${std_srcs}
)

if(WIN32)
    set_target_properties(libhl
        PROPERTIES
        OUTPUT_NAME std
        SUFFIX .hdll
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
	COMPILE_DEFINITIONS "_USRDLL;HLDLL_EXPORTS"
)

add_executable(hl
    src/callback.c
    src/code.c
    src/jit.c
    src/main.c
    src/module.c
)

target_link_libraries(hl libhl)

if(WIN32)
	target_link_libraries(libhl ws2_32 user32)
	target_link_libraries(hl user32)
endif()