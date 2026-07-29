# DirectX 12 Agility SDK Setup

## DirectX 12 Agility SDK

To build and run the project with the DirectX 12 Agility SDK, ensure that the required Agility SDK package version matches the value of `HL_DX12_AGILITY_VERSION`.

These files must be placed in a D3D12 directory located next to the application executable:

```text
hl.exe
D3D12/
  +-- D3D12Core.dll
  +-- d3d12SDKLayers.dll
```

If the versions do not match, the DirectX 12 debug/SDK layers may fail to load or the application may not start correctly.

## Nsight Aftermath (Enabled by Default with Agility SDK)

The default Agility SDK configuration enables **Nsight Aftermath** for GPU crash diagnostics (`HL_AFTERMATH`).

To build with this, the [Nsight Aftermath SDK](https://developer.nvidia.com/nsight-aftermath) (tested with 2026.3 - Version API 2.27) must be extracted to `<hashlink>/include/aftermath/`.

In addition, the required Nsight Aftermath DLLs must be placed next to the application executable so they can be loaded at runtime.

### Building Without Aftermath

When building the **HashLink** `ReleaseDX12Agility` configuration, Aftermath can be disabled by building the **dx12** project using the **Release** configuration instead of **ReleaseAll**.

This excludes Nsight Aftermath from the build and removes the dependency on the Aftermath SDK and runtime DLLs.

You can also manually remove the preprocessor (`HL_AFTERMATH`) from the **ReleaseAll** configuration if you want to use the others optional features.

## Streamline (Enabled by Default with Agility SDK)

The default Agility SDK configuration links against sl.interposer.lib to enable the DLSS implementation provided by hlheaps. (https://github.com/HeapsIO/hlheaps/).

The required Streamline DLLs must be placed next to the application executable so they can be loaded at runtime which you can find on the [Streamline GitHub](https://github.com/NVIDIA-RTX/Streamline) (tested with version API 2.12.0). 

### Building Without Streamline

When building the **HashLink** `ReleaseDX12Agility` configuration, Streamline can be disabled by building the **dx12** project using the **Release** configuration instead of **ReleaseAll**.

This excludes Streamline from the build and removes the dependency on the Streamline SDK and runtime DLLs.

You can also manually remove the preprocessor (`HL_STREAMLINE`) from the **ReleaseAll** configuration if you want to use the others optional features.