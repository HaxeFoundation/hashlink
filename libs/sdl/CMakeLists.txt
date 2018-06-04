if(WIN32)
    set(SDL2_PATH ${INCLUDES_BASE_DIR}/sdl)
endif()
find_package(SDL2 REQUIRED)

add_library(sdl.hdll SHARED
    sdl.c
    gl.c
)
set_as_hdll(sdl)
target_include_directories(sdl.hdll
    PRIVATE
    ${SDL2_INCLUDE_DIR}
    ${INCLUDES_BASE_DIR}/gl
)
target_link_libraries(sdl.hdll
    libhl
    ${SDL2_LIBRARY}
)

if(WIN32)
    target_link_libraries(sdl.hdll
        winmm
        opengl32
    )
endif()

if(APPLE)
    find_package(OPENGL REQUIRED)
    target_include_directories(sdl.hdll
        PRIVATE
        ${OPENGL_INCLUDE_DIR}
    )
    target_link_libraries(sdl.hdll
        libhl
        ${OPENGL_gl_LIBRARY}
    )
endif()


install(
    TARGETS sdl.hdll
    DESTINATION ${HDLL_DESTINATION}
)
