set(INCLUDES_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)

function(set_as_hdll target)
    set_target_properties(${target}.hdll
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${target}
        SUFFIX .hdll
    )
endfunction()

if(WIN32)
    option(WITH_DIRECTX "Build directx.hdll." ON)
    if(WITH_DIRECTX)
        add_subdirectory(directx)
    endif()
endif()

option(WITH_FMT "Build fmt.hdll." ON)
if(WITH_FMT)
    add_subdirectory(fmt)
endif()

# add_subdirectory(mesa)

if(WIN32)
    option(WITH_OPENAL "Build openal.hdll." ON)
    if(WITH_OPENAL)
        add_subdirectory(openal)
    endif()
endif()

option(WITH_SDL "Build sdl.hdll." ON)
if(WITH_SDL)
    add_subdirectory(sdl)
endif()

option(WITH_SQLITE "Build sqlite.hdll." ON)
if(WITH_SQLITE)
    add_subdirectory(sqlite)
endif()

if(WIN32)
    option(WITH_SSL "Build ssl.hdll." ON)
    if(WITH_SSL)
    add_subdirectory(ssl)
    endif()
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
