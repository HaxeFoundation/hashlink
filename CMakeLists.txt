cmake_minimum_required(VERSION 3.13)

set(HL_VERSION_MAJOR 1)
set(HL_VERSION_MINOR 16)
set(HL_VERSION_PATCH 0)
set(HL_VERSION ${HL_VERSION_MAJOR}.${HL_VERSION_MINOR}.${HL_VERSION_PATCH})

cmake_policy(SET CMP0042 NEW)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.24)
	cmake_policy(VERSION 3.24)
endif()

if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
endif()
project(hashlink)

include(GNUInstallDirs)
include(FindPkgConfig)
include(CTest)

set(WITH_VM_DEFAULT ON)
# VM now supports x86, x86-64, and AArch64 architectures

option(WITH_VM "Whether to build the Hashlink virtual machine" ${WITH_VM_DEFAULT})
option(WITH_LLVM_AOT "Whether to build the hl2llvm AOT compiler" OFF)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
if(BUILD_SHARED_LIBS)
    # ensure third-party static libs are built with PIC
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()
if(ANDROID)
    set(_DOWNLOAD_DEPENDENCIES ON)
else()
    set(_DOWNLOAD_DEPENDENCIES OFF)
endif()
option(DOWNLOAD_DEPENDENCIES "Download & build third-party dependencies" ${_DOWNLOAD_DEPENDENCIES})

if(MSVC)
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set (CMAKE_INSTALL_PREFIX "C:/HaxeToolkit/hashlink" CACHE PATH "default install path" FORCE)
    endif()
endif()
option(FLAT_INSTALL_TREE "Flat install tree" ${MSVC})
if(FLAT_INSTALL_TREE)
    set(CMAKE_INSTALL_BINDIR .)
    set(CMAKE_INSTALL_LIBDIR .)
endif()
# force Unicode over Multi-byte
if(MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    add_compile_options(/permissive-)
endif()

if (MINGW)
    add_compile_options(-municode)
    add_link_options(-municode)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/other/cmake
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# put output in "bin"

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# avoid the extra "Debug", "Release" directories
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
endforeach()

file(GLOB pcre_srcs
    include/pcre/pcre2_auto_possess.c
    include/pcre/pcre2_chartables.c
    include/pcre/pcre2_compile.c
    include/pcre/pcre2_config.c
    include/pcre/pcre2_context.c
    include/pcre/pcre2_convert.c
    include/pcre/pcre2_dfa_match.c
    include/pcre/pcre2_error.c
    include/pcre/pcre2_extuni.c
    include/pcre/pcre2_find_bracket.c
    include/pcre/pcre2_jit_compile.c
    include/pcre/pcre2_maketables.c
    include/pcre/pcre2_match_data.c
    include/pcre/pcre2_match.c
    include/pcre/pcre2_newline.c
    include/pcre/pcre2_ord2utf.c
    include/pcre/pcre2_pattern_info.c
    include/pcre/pcre2_script_run.c
    include/pcre/pcre2_serialize.c
    include/pcre/pcre2_string_utils.c
    include/pcre/pcre2_study.c
    include/pcre/pcre2_substitute.c
    include/pcre/pcre2_substring.c
    include/pcre/pcre2_tables.c
    include/pcre/pcre2_ucd.c
    include/pcre/pcre2_valid_utf.c
    include/pcre/pcre2_xclass.c
)

file(GLOB std_srcs
    src/std/array.c
    src/std/buffer.c
    src/std/bytes.c
    src/std/cast.c
    src/std/date.c
    src/std/debug.c
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
    src/std/track.c
    src/std/types.c
    src/std/ucs2.c
    src/std/thread.c
    src/std/process.c
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    list(APPEND std_srcs
        include/mdbg/mdbg.c
        include/mdbg/mach_excServer.c
        include/mdbg/mach_excUser.c
    )
endif()

if(ANDROID)
    list(APPEND std_srcs
        src/std/sys_android.c
    )
endif()

if(IOS_PLATFORM)
    list(APPEND std_srcs
        src/std/sys_ios.m
    )
endif()

add_library(libhl
    ${pcre_srcs}
    src/gc.c
    ${std_srcs}
)

target_include_directories(libhl
    PUBLIC src
    PRIVATE include include/pcre
)

# C11 / C++11 are required for features such as unicode strings
target_compile_features(libhl PUBLIC cxx_std_11 c_std_11)

set(public_headers
    src/hl.h
    src/hlc.h
)

set_target_properties(libhl
    PROPERTIES
    PUBLIC_HEADER "${public_headers}"
)

if(MSVC)
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
    COMPILE_DEFINITIONS "_USRDLL;LIBHL_EXPORTS;HAVE_CONFIG_H;PCRE2_CODE_UNIT_WIDTH=16"
)

if (WITH_VM)
    # Select JIT backend based on architecture
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(JIT_SOURCES
            src/jit_aarch64.c
            src/jit_aarch64_emit.c
            src/jit_shared.c
        )
    else()
        set(JIT_SOURCES
            src/jit_x86.c
            src/jit_shared.c
        )
    endif()

    add_executable(hl
        src/code.c
        ${JIT_SOURCES}
        src/main.c
        src/module.c
        src/debugger.c
        src/profile.c
    )
    if(APPLE)
        set_target_properties(hl PROPERTIES
            INSTALL_RPATH "@executable_path;@executable_path/../${CMAKE_INSTALL_LIBDIR}"
        )
    elseif (UNIX)
        set_target_properties(hl PROPERTIES
            INSTALL_RPATH "$ORIGIN;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
        )
    endif()

    target_link_libraries(hl libhl)

    if (WIN32)
        target_link_libraries(hl user32)
    endif()
endif()

if(WIN32)
    target_link_libraries(libhl ws2_32 user32 winmm)
else()
    target_link_libraries(libhl m dl)

    if(ANDROID)
        target_link_libraries(libhl log)
    else()
        target_link_libraries(libhl pthread)
    endif()
endif()

#####################
# LLVM AOT Compiler (hl2llvm)
if(WITH_LLVM_AOT)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    # LLVM definitions and includes
    add_definitions(${LLVM_DEFINITIONS})

    # Source files for hl2llvm
    set(HL2LLVM_SOURCES
        src/llvm/hl2llvm_main.c
        src/llvm/llvm_codegen.c
        src/llvm/llvm_types.c
        src/llvm/llvm_runtime.c
        src/llvm/llvm_ops_constants.c
        src/llvm/llvm_ops_arith.c
        src/llvm/llvm_ops_control.c
        src/llvm/llvm_ops_memory.c
        src/llvm/llvm_ops_calls.c
        src/llvm/llvm_ops_closures.c
        src/llvm/llvm_ops_types.c
        src/llvm/llvm_ops_objects.c
        src/llvm/llvm_ops_enums.c
        src/llvm/llvm_ops_refs.c
        src/llvm/llvm_ops_exceptions.c
        src/llvm/llvm_ops_misc.c
        src/code.c
    )

    add_executable(hl2llvm ${HL2LLVM_SOURCES})

    # AOT runtime library (provides module loading for AOT binaries)
    add_library(aot_runtime STATIC
        src/llvm/aot_runtime.c
        src/module.c
        src/code.c
    )
    target_include_directories(aot_runtime PRIVATE src)
    target_link_libraries(aot_runtime libhl)

    # Make hl2llvm depend on aot_runtime so both are built together
    add_dependencies(hl2llvm aot_runtime)

    target_include_directories(hl2llvm
        PRIVATE
            src
            ${LLVM_INCLUDE_DIRS}
    )

    # Get LLVM libraries
    llvm_map_components_to_libnames(LLVM_LIBS
        core
        analysis
        bitwriter
        target
        ${LLVM_TARGETS_TO_BUILD}
    )

    target_link_libraries(hl2llvm
        libhl
        ${LLVM_LIBS}
    )

    if(APPLE)
        set_target_properties(hl2llvm PROPERTIES
            INSTALL_RPATH "@executable_path;@executable_path/../${CMAKE_INSTALL_LIBDIR}"
        )
    elseif(UNIX)
        set_target_properties(hl2llvm PROPERTIES
            INSTALL_RPATH "$ORIGIN;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
        )
    endif()

endif()

if(BUILD_TESTING)

    find_program(
        HAXE_COMPILER
        haxe
    )

    if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "Win32")
        SET(HAXE_FLAGS -D hl-legacy32)
    else()
        SET(HAXE_FLAGS )
    endif()

    if(MINGW)
        set(HLC_LIBRARIES dbghelp)
    else()
        set(HLC_LIBRARIES)
    endif()

    #####################
    # hello.hl

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main HelloWorld
    )
    add_custom_target(hello.hl ALL
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
    )

    #####################
    # threads.hl

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main Threads
    )
    add_custom_target(threads.hl ALL
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
    )

    #####################
    # uvsample.hl

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample.hl
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample.hl
            -cp ${CMAKE_SOURCE_DIR}/other/uvsample -main UVSample
    )
    add_custom_target(uvsample.hl ALL
        DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample.hl
    )

    #####################
    # hello.c

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main HelloWorld
    )
    add_executable(hello
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello/hello.c
    )
    set_target_properties(hello
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello
    )
    target_include_directories(hello
        PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello
    )
    target_link_libraries(hello
        libhl
        ${HLC_LIBRARIES}
    )

    #####################
    # threads.c

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
            -cp ${CMAKE_SOURCE_DIR}/other/tests -main Threads
    )
    add_executable(threads
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads/threads.c
    )
    set_target_properties(threads
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads
    )
    target_include_directories(threads
        PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads
    )
    target_link_libraries(threads
        libhl
        ${HLC_LIBRARIES}
    )

    #####################
    # uvsample.c

    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample/uvsample.c
        COMMAND ${HAXE_COMPILER}
            ${HAXE_FLAGS}
            -hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample/uvsample.c
            -cp ${CMAKE_SOURCE_DIR}/other/uvsample -main UVSample
    )
    add_executable(uvsample
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample/uvsample.c
    )
    set_target_properties(uvsample
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample
    )
    target_include_directories(uvsample
        PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample
    )
    target_link_libraries(uvsample
        libhl
        uv.hdll
        ${HLC_LIBRARIES}
    )

    #####################
    # header_macro_clashes.c

    add_executable(header_macro_clashes
        ${CMAKE_SOURCE_DIR}/other/tests/header_macro_clashes/header_macro_clashes.c
    )
    set_target_properties(header_macro_clashes
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/
    )
    target_link_libraries(header_macro_clashes
        libhl
        ${HLC_LIBRARIES}
    )

    #####################
    # Tests
    if (WITH_VM)
        add_test(NAME version
            COMMAND hl --version
        )
        set_tests_properties(version
            PROPERTIES
            PASS_REGULAR_EXPRESSION "${HL_VERSION}"
        )

        add_test(NAME hello.hl
            COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/hello.hl
        )
        add_test(NAME threads.hl
            COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/threads.hl
        )
        add_test(NAME uvsample.hl
            COMMAND hl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/uvsample.hl 6001
        )

        #####################
        # Minimal JIT Tests
        # These test individual opcodes without pulling in the Haxe stdlib

        # Common sources for all minimal JIT tests
        set(MINIMAL_JIT_SOURCES
            src/code.c
            ${JIT_SOURCES}
            src/module.c
            src/debugger.c
            src/profile.c
        )

        # Macro to add a minimal JIT test
        macro(add_minimal_jit_test name)
            add_executable(${name}
                ${CMAKE_SOURCE_DIR}/other/tests/minimal/${name}.c
                ${MINIMAL_JIT_SOURCES}
            )
            target_include_directories(${name}
                PRIVATE ${CMAKE_SOURCE_DIR}/other/tests/minimal
            )
            target_link_libraries(${name}
                libhl
            )
            set_target_properties(${name}
                PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/minimal
            )
            add_test(NAME ${name} COMMAND ${name})
        endmacro()

        # Add all minimal JIT tests
        add_minimal_jit_test(test_int_ops)
        add_minimal_jit_test(test_float_ops)
        add_minimal_jit_test(test_bool_ops)
        add_minimal_jit_test(test_control_flow)
        add_minimal_jit_test(test_i64_ops)
        add_minimal_jit_test(test_calls)
        add_minimal_jit_test(test_strings)
        add_minimal_jit_test(test_globals)
        add_minimal_jit_test(test_natives)
        add_minimal_jit_test(test_closures)
        add_minimal_jit_test(test_objects)
        add_minimal_jit_test(test_dynamic)
        add_minimal_jit_test(test_callbacks)
        add_minimal_jit_test(test_native_field)
        add_minimal_jit_test(test_binop_inplace)
        add_minimal_jit_test(test_enum)
        add_minimal_jit_test(test_instance_closure)
        add_minimal_jit_test(test_memory_ops)
        add_minimal_jit_test(test_array_ops)
        add_minimal_jit_test(test_ref_ops)
        add_minimal_jit_test(test_unsigned_ops)
        add_minimal_jit_test(test_switch)
        add_minimal_jit_test(test_jumps_unsigned)
        add_minimal_jit_test(test_type_ops)
        add_minimal_jit_test(test_exceptions)
        add_minimal_jit_test(test_methods)
        add_minimal_jit_test(test_virtual_fields)

        # Bytecode dump utility (needs code.c for hl_code_read)
        add_executable(hldump
            ${CMAKE_SOURCE_DIR}/other/tests/minimal/hldump.c
            ${CMAKE_SOURCE_DIR}/src/code.c
        )
        target_link_libraries(hldump libhl)
        set_target_properties(hldump
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test/minimal
        )

    endif()

    add_test(NAME hello
        COMMAND hello
    )
    add_test(NAME threads
        COMMAND threads
    )
    add_test(NAME uvsample
        COMMAND uvsample 6002
    )
endif()

#####################
# Packaging

set(CPACK_OUTPUT_FILE_PREFIX ${OUTPUT_DIR})

set(CPACK_PACKAGE_VENDOR "Haxe Foundation")
set(CPACK_PACKAGE_VERSION_MAJOR ${HL_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${HL_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${HL_VERSION_PATCH})

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(HDLL_DESTINATION
    ${CMAKE_INSTALL_LIBDIR}
)

set(INSTALL_TARGETS libhl)
if (WITH_VM)
    list(APPEND INSTALL_TARGETS hl)
endif()
if (WITH_LLVM_AOT)
    list(APPEND INSTALL_TARGETS hl2llvm)
endif()

install(
    TARGETS
        ${INSTALL_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(
    FILES src/hlc_main.c
    TYPE INCLUDE
)

# uninstall target
# https://gitlab.kitware.com/cmake/community/wikis/FAQ#can-i-do-make-uninstall-with-cmake
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/other/cmake/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


if(
    NOT DEFINED ENV{APPVEYOR_REPO_TAG_NAME}
    AND
    DEFINED ENV{APPVEYOR_REPO_COMMIT}
)
    string(SUBSTRING $ENV{APPVEYOR_REPO_COMMIT} 0 7 short_commit)
endif()

if(NOT DEFINED ENV{TRAVIS_TAG} AND DEFINED ENV{TRAVIS_COMMIT})
    string(SUBSTRING $ENV{TRAVIS_COMMIT} 0 7 short_commit)
endif()

if(DEFINED short_commit)
    set(CPACK_PACKAGE_VERSION "${HL_VERSION}+${short_commit}")
endif()


include(CPack)

add_subdirectory(libs)
