add_library(os.hdll SHARED
    os.cpp
)

set_as_hdll(os)

target_include_directories(os.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}/clip
)

target_link_libraries(os.hdll
    libhl
    clip
)

install(
    TARGETS
    os.hdll
    DESTINATION ${HDLL_DESTINATION}
)
