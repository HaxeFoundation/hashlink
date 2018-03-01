if(WIN32)
    link_directories(${INCLUDES_BASE_DIR}/ffmpeg/lib)

    if(MSVC)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    add_library(video.hdll SHARED video.c)

    set_as_hdll(video)

    target_include_directories(video.hdll
        PRIVATE
        ${INCLUDES_BASE_DIR}/ffmpeg/include
    )

    target_link_libraries(video.hdll
        libhl
        avcodec
        avformat
        avutil
        swscale
    )

    install(
        TARGETS
            video.hdll
        DESTINATION .
    )
endif()
