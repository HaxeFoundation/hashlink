find_package(DirectX)

add_library(directx.hdll SHARED
    directx.cpp
    gamecontroller.c
    window.c
)

set_as_hdll(directx)

target_link_libraries(directx.hdll
    libhl
    d3d11
    dxgi
    d3dcompiler
    xinput9_1_0
    dinput8
)

install(
    TARGETS
        directx.hdll
    DESTINATION ${HDLL_DESTINATION}
)
