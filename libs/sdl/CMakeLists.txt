add_library(sdl.hdll
    sdl.c
    gl.c
)

if ((NOT WIN32) AND DOWNLOAD_DEPENDENCIES)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.10/SDL3-3.4.10.tar.gz
        URL_HASH SHA256=12b34280415ec8418c864408b93d008a20a6530687ee613d60bfbd20411f2785
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
        if(NOT MINGW)
            target_include_directories(sdl.hdll
                PRIVATE
                ${INCLUDES_BASE_DIR}/gl
            )
        endif()
    endif()

    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
endif()

set_as_hdll(sdl)
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
    if(APPLE)
        # Only Apple's framework can serve the CGL context SDL creates.
        # find_package(OpenGL) picks a Homebrew Mesa libGL ahead of it when
        # CMAKE_FIND_FRAMEWORK is LAST; that links fine and then every gl*
        # call returns 0/NULL.
        find_library(HL_APPLE_OPENGL_FRAMEWORK OpenGL REQUIRED)
        set(HL_SDL_GL_LIBRARY ${HL_APPLE_OPENGL_FRAMEWORK})
    else()
        set(HL_SDL_GL_LIBRARY ${OPENGL_gl_LIBRARY})
    endif()
    target_link_libraries(sdl.hdll
        libhl
        ${HL_SDL_GL_LIBRARY}
    )
endif()


install(
    TARGETS sdl.hdll
    DESTINATION ${HDLL_DESTINATION}
)
