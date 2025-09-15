add_library(uv.hdll
    uv.c
)

if(WIN32)
    target_sources(uv.hdll
        PRIVATE
            ${INCLUDES_BASE_DIR}/libuv/src/fs-poll.c
            ${INCLUDES_BASE_DIR}/libuv/src/inet.c
            ${INCLUDES_BASE_DIR}/libuv/src/threadpool.c
            ${INCLUDES_BASE_DIR}/libuv/src/uv-common.c
            ${INCLUDES_BASE_DIR}/libuv/src/version.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/async.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/core.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/dl.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/error.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/fs-event.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/fs.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/getaddrinfo.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/getnameinfo.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/handle.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/loop-watcher.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/pipe.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/poll.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/process-stdio.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/process.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/req.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/signal.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/snprintf.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/stream.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/tcp.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/thread.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/timer.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/tty.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/udp.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/util.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/winapi.c
            ${INCLUDES_BASE_DIR}/libuv/src/win/winsock.c
    )
elseif(DOWNLOAD_DEPENDENCIES)
	FetchContent_Declare(libuv-project
        URL https://github.com/libuv/libuv/archive/refs/tags/v1.51.0.tar.gz
        URL_HASH SHA256=27e55cf7083913bfb6826ca78cde9de7647cded648d35f24163f2d31bb9f51cd
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
    )
    set(_OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(BUILD_SHARED_LIBS OFF)
    set(LIBUV_BUILD_SHARED OFF CACHE INTERNAL "")
    set(LIBUV_BUILD_TESTS OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(libuv-project)
    set(BUILD_SHARED_LIBS ${_OLD_BUILD_SHARED_LIBS})
    add_library(LibUV::LibUV ALIAS uv_a)
else()
    find_package(LibUV)
endif()

set_as_hdll(uv)
target_link_libraries(uv.hdll
    libhl
)
if(WIN32)
    target_include_directories(uv.hdll
        PRIVATE
        ${INCLUDES_BASE_DIR}/libuv/include
    )
    target_link_libraries(uv.hdll
        psapi
        iphlpapi
        userenv
    )
    target_compile_definitions(uv.hdll
        PRIVATE
        UV_EXPORTS
        BUILDING_UV_SHARED
    )
else()
    target_link_libraries(uv.hdll
        LibUV::LibUV
    )
endif()

install(
    TARGETS
        uv.hdll
    DESTINATION ${HDLL_DESTINATION}
)
