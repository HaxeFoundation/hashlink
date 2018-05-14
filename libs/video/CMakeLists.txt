if(WIN32)
    if(MSVC)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    endif()

    find_library(FFMPEG_LIBRARY_avcodec avcodec PATHS ${INCLUDES_BASE_DIR}/ffmpeg/lib)
    find_library(FFMPEG_LIBRARY_avformat avformat PATHS ${INCLUDES_BASE_DIR}/ffmpeg/lib)
    find_library(FFMPEG_LIBRARY_avutil avutil PATHS ${INCLUDES_BASE_DIR}/ffmpeg/lib)
    find_library(FFMPEG_LIBRARY_swscale swscale PATHS ${INCLUDES_BASE_DIR}/ffmpeg/lib)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARY_avcodec} ${FFMPEG_LIBRARY_avformat} ${FFMPEG_LIBRARY_avutil} ${FFMPEG_LIBRARY_swscale})

    add_library(video.hdll SHARED video.c)

    set_as_hdll(video)

    target_include_directories(video.hdll
        PRIVATE
        ${INCLUDES_BASE_DIR}/ffmpeg/include
    )

    target_link_libraries(video.hdll
        libhl
        ${FFMPEG_LIBRARIES}
    )

    install(
        TARGETS
            video.hdll
        DESTINATION ${HDLL_DESTINATION}
    )
endif()
