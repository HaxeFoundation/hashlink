set(INCLUDES_BASE_DIR ${CMAKE_SOURCE_DIR}/include)

function(set_as_hdll target)
    set_target_properties(${target}.hdll
        PROPERTIES
        PREFIX ""
        OUTPUT_NAME ${target}
        SUFFIX .hdll
    )
endfunction()

add_subdirectory(bullet)

if(WIN32)
	add_subdirectory(directx)
endif()

add_subdirectory(fmt)
# add_subdirectory(mesa)
add_subdirectory(openal)
add_subdirectory(sdl)
add_subdirectory(sqlite)
add_subdirectory(ssl)
add_subdirectory(ui)
add_subdirectory(uv)
add_subdirectory(video)
