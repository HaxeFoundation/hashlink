option(WITH_DIRECTX "Build directx.hdll." ON)

if(WITH_DIRECTX)
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
endif()

option(WITH_DX12 "Build dx12.hdll." ON)

if(WITH_DX12)
    add_library(dx12.hdll SHARED
        dx12.cpp
    )

    set_as_hdll(dx12)

    target_link_libraries(dx12.hdll
        libhl
        d3d12
        dxgi
        dxcompiler
    )

    install(
        TARGETS
            dx12.hdll
        DESTINATION ${HDLL_DESTINATION}
    )
endif()
