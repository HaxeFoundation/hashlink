add_library(clipboard.hdll SHARED
    clipboard.cpp
)

set_as_hdll(clipboard)

target_include_directories(clipboard.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}/clip
)

target_link_libraries(clipboard.hdll
    libhl
    clip
)

install(
    TARGETS
    clipboard.hdll
    DESTINATION ${HDLL_DESTINATION}
)
