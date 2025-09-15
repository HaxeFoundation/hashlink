add_library(openal.hdll openal.c)

option(WITH_STATIC_OPENAL "Link openal.hdll to static openal-soft" OFF)

if (WITH_STATIC_OPENAL)
    target_compile_definitions(openal.hdll PRIVATE OPENAL_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

if(WIN32)
    if(MSVC)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(OPENAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win64)
        install(FILES ${INCLUDES_BASE_DIR}/openal/bin/Win64/soft_oal.dll TYPE BIN RENAME OpenAL32.dll)
    else()
        find_library(OPENAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win32)
        install(FILES ${INCLUDES_BASE_DIR}/openal/bin/Win32/soft_oal.dll TYPE BIN RENAME OpenAL32.dll)
    endif()

    set(OPENAL_INCLUDE_DIR ${INCLUDES_BASE_DIR}/openal/include)
    add_library(OpenAL::OpenAL SHARED IMPORTED)
    set_target_properties(OpenAL::OpenAL PROPERTIES
        IMPORTED_IMPLIB ${OPENAL_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${OPENAL_INCLUDE_DIR}
    )
elseif(DOWNLOAD_DEPENDENCIES)
	FetchContent_Declare(openal-soft
        URL https://github.com/kcat/openal-soft/archive/refs/tags/1.24.3.tar.gz
        URL_HASH SHA256=7e1fecdeb45e7f78722b776c5cf30bd33934b961d7fd2a11e0494e064cc631ce
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
    )

    set(ALSOFT_EXAMPLES OFF CACHE INTERNAL "")
    set(ALSOFT_UTILS OFF CACHE INTERNAL "")
    if (WITH_STATIC_OPENAL)
        set(LIBTYPE STATIC)
    endif()

    FetchContent_MakeAvailable(openal-soft)
    install(
    TARGETS
        OpenAL
    LIBRARY
    RUNTIME
)
else()
    find_package(OpenAL REQUIRED)
endif()

set_as_hdll(openal)

target_link_libraries(openal.hdll
    libhl
    OpenAL::OpenAL
)

install(
    TARGETS
        openal.hdll
    DESTINATION ${HDLL_DESTINATION}
)
