if(WIN32)
    if(MSVC)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(OpenAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win64)
    else()
        find_library(OpenAL_LIBRARY OpenAL32 PATHS ${INCLUDES_BASE_DIR}/openal/libs/Win32)
    endif()

    add_library(openal.hdll SHARED openal.c)

    set_as_hdll(openal)

    target_include_directories(openal.hdll
        PRIVATE
        ${INCLUDES_BASE_DIR}/openal/include
    )

    target_link_libraries(openal.hdll
        libhl
        ${OpenAL_LIBRARY}
    )

    install(
        TARGETS
            openal.hdll
        DESTINATION ${HDLL_DESTINATION}
    )
endif()
