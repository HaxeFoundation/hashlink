add_library(vulkan.hdll
    vulkan.cpp
)

set_as_hdll(vulkan)

target_include_directories(vulkan.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}
)

target_link_libraries(vulkan.hdll
    libhl
    dxcompiler
)

install(
    TARGETS
        vulkan.hdll
    DESTINATION ${HDLL_DESTINATION}
)
