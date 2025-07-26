add_library(openal.hdll openal.c)

if(WIN32)
    if(MSVC)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(OPENAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win64)
    else()
        find_library(OPENAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win32)
    endif()

    set(OPENAL_INCLUDE_DIR ${INCLUDES_BASE_DIR}/openal/include)
elseif(ANDROID)
	ExternalProject_Add(openal-soft
        URL https://github.com/kcat/openal-soft/archive/refs/tags/1.24.3.tar.gz
        URL_HASH SHA256=7e1fecdeb45e7f78722b776c5cf30bd33934b961d7fd2a11e0494e064cc631ce
        CMAKE_ARGS
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            -DANDROID_ABI=${CMAKE_ANDROID_ARCH_ABI}
        # INSTALL_BYPRODUCTS in CMake 3.26+
        BUILD_BYPRODUCTS <INSTALL_DIR>/${CMAKE_INSTALL_LIBDIR}/libopenal.so
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    ExternalProject_Get_Property(openal-soft INSTALL_DIR)

    add_library(openal SHARED IMPORTED)
    set_target_properties(openal PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/libopenal.so)

    set(OPENAL_INCLUDE_DIR ${INSTALL_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
    set(OPENAL_LIBRARY openal)

    add_dependencies(openal openal-soft)
else()
    find_package(OpenAL REQUIRED)
endif()

set_as_hdll(openal)

target_link_libraries(openal.hdll
    libhl
    ${OPENAL_LIBRARY}
)

target_include_directories(openal.hdll
    PRIVATE
    ${OPENAL_INCLUDE_DIR}
)

install(
    TARGETS
        openal.hdll
    DESTINATION ${HDLL_DESTINATION}
)
