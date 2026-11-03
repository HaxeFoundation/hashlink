add_library(vulkan.hdll
    vulkan.cpp
)

set_as_hdll(vulkan)

target_include_directories(vulkan.hdll
    PRIVATE
    ${INCLUDES_BASE_DIR}
)

if(WIN32)
    target_link_libraries(vulkan.hdll
        libhl
        dxcompiler
    )
else()
    find_path(DXC_INCLUDE_DIR dxc/dxcapi.h HINTS $ENV{VULKAN_SDK}/include)
    if(DXC_INCLUDE_DIR)
        target_include_directories(vulkan.hdll
            PRIVATE
            ${DXC_INCLUDE_DIR}
        )
        target_compile_definitions(vulkan.hdll PRIVATE HL_VULKAN_DXC)
    else()
        message(STATUS "dxc/dxcapi.h not found: vulkan.hdll will be built without a shader compiler")
    endif()
    target_link_libraries(vulkan.hdll
        libhl
        ${CMAKE_DL_LIBS}
    )
endif()

install(
    TARGETS
        vulkan.hdll
    DESTINATION ${HDLL_DESTINATION}
)
