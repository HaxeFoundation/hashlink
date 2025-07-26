add_library(sdl.hdll
    sdl.c
    gl.c
)

if (ANDROID)
	ExternalProject_Add(sdl2-project
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.8/SDL2-2.32.8.tar.gz
        URL_HASH SHA256=0ca83e9c9b31e18288c7ec811108e58bac1f1bb5ec6577ad386830eac51c787e
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            -DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}/libSDL2.so
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    ExternalProject_Get_Property(sdl2-project INSTALL_DIR)

    add_library(sdl2 SHARED IMPORTED)
    set_target_properties(sdl2 PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libSDL2.so)

    set(SDL2_INCLUDE_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/SDL2)
    set(SDL2_LIBRARY sdl2)

    add_dependencies(sdl2 sdl2-project)
else()
    if(WIN32)
        set(SDL2_PATH ${INCLUDES_BASE_DIR}/sdl)
    endif()

    find_package(SDL2 REQUIRED)
endif()

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

if(ANDROID)
    target_link_libraries(sdl.hdll m OpenSLES dl log android GLESv1_CM GLESv2 EGL GLESv3)
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
