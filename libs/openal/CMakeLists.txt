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
else()
    find_package(OpenAL REQUIRED)
endif()

add_library(openal.hdll SHARED openal.c)

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
