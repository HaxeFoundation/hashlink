set(INCLUDES_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if (BUILD_SHARED_LIBS)
    set(HDLL_SUFFIX .hdll)
else()
    set(HDLL_SUFFIX .hdll${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

function(set_as_hdll target)
    set_target_properties(${target}.hdll
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${target}
        SUFFIX ${HDLL_SUFFIX}
    )
endfunction()

if (DOWNLOAD_DEPENDENCIES)
    set(CMAKE_COMMON_DEP_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        $<$<BOOL:${ANDROID_PLATFORM}>:-DANDROID_PLATFORM=${ANDROID_PLATFORM}>
        $<$<BOOL:${ANDROID_ABI}>:-DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}>
        $<$<BOOL:${CMAKE_OSX_ARCHITECTURES}>:-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}>
        $<$<BOOL:${CMAKE_OSX_DEPLOYMENT_TARGET}>:-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}>
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON
        -DCMAKE_MESSAGE_LOG_LEVEL=ERROR
        -DCMAKE_INSTALL_MESSAGE=LAZY
    )
    include(ExternalProject)
    include(FetchContent)
endif()

if(WIN32)
    add_subdirectory(directx)
endif()

option(WITH_FMT "Build fmt.hdll." ON)
if(WITH_FMT)
    add_subdirectory(fmt)
endif()

# add_subdirectory(mesa)

option(WITH_OPENAL "Build openal.hdll." ON)
if(WITH_OPENAL)
    add_subdirectory(openal)
endif()

option(WITH_SDL "Build sdl.hdll." ON)
if(WITH_SDL)
    add_subdirectory(sdl)
endif()

option(WITH_SQLITE "Build sqlite.hdll." ON)
if(WITH_SQLITE)
    add_subdirectory(sqlite)
endif()

option(WITH_SSL "Build ssl.hdll." ON)
if(WITH_SSL)
    add_subdirectory(ssl)
endif()

option(WITH_UI "Build ui.hdll." ON)
if(WITH_UI)
    add_subdirectory(ui)
endif()

option(WITH_UV "Build uv.hdll." ON)
if(WITH_UV)
    add_subdirectory(uv)
endif()

option(WITH_VIDEO "Build video.hdll." ON)
if(WIN32)
    if(WITH_VIDEO)
        add_subdirectory(video)
    endif()
endif()

option(WITH_HEAPS "Build heaps.hdll." ON)
if(WITH_HEAPS)
    add_subdirectory(heaps)
endif()
