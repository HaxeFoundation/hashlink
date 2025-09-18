add_library(sdl.hdll
    sdl.c
    gl.c
)

if ((NOT WIN32) AND DOWNLOAD_DEPENDENCIES)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-3.2.22.tar.gz
        URL_HASH SHA256=f29d00cbcee273c0a54f3f32f86bf5c595e8823a96b1d92a145aac40571ebfcc
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
    )
    set(SDL_TEST OFF CACHE INTERNAL "")
    if (ANDROID)
        set(SDL_SHARED ON CACHE INTERNAL "")
        set(SDL_STATIC OFF CACHE INTERNAL "")
    else()
        set(SDL_STATIC ON CACHE INTERNAL "")
        set(SDL_SHARED OFF CACHE INTERNAL "")
        if (BUILD_SHARED_LIBS)
            set(SDL_STATIC_PIC ON CACHE INTERNAL "")
        endif()
    endif()
    FetchContent_MakeAvailable(SDL3)
else()
    if(WIN32)
        set(SDL3_DIR ${INCLUDES_BASE_DIR}/sdl/cmake)
    endif()

    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
endif()

set_as_hdll(sdl)
target_include_directories(sdl.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}/gl
)
target_link_libraries(sdl.hdll
    libhl
    SDL3::SDL3
)
if(APPLE)
    target_compile_definitions(sdl.hdll PRIVATE GL_SILENCE_DEPRECATION)
endif()

if(WIN32)
    target_link_libraries(sdl.hdll
        winmm
        opengl32
    )
endif()

if(ANDROID)
    target_link_libraries(sdl.hdll GLESv3)
endif()

if((APPLE OR UNIX) AND NOT ANDROID)
    find_package(OpenGL REQUIRED)
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
