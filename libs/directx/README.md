# DirectX 12 Agility SDK Setup

## DirectX 12 Agility SDK

To build and run the project with the DirectX 12 Agility SDK, ensure that the required Agility SDK package version matches the value of HL_DX12_AGILITY_VERSION.

These files must be placed in a D3D12 directory located next to the application executable:

```text
hl.exe
D3D12/
  +-- D3D12Core.dll
  +-- d3d12SDKLayers.dll
```

If the versions do not match, the DirectX 12 debug/SDK layers may fail to load or the application may not start correctly.

## Nsight  Aftermath (Enabled by Default with Agility SDK)

The default Agility SDK configuration enables **Nsight  Aftermath** for GPU crash diagnostics.

To build with this configuration, the Nsight  Aftermath SDK must be available in the following locations relative to the HashLink repository:

- Header files:
  ```
  <hashlink>/include/aftermath/
  ```
- Library files:
  ```
  <hashlink>/include/aftermath/x64
  <hashlink>/include/aftermath/x86
  ```

In addition, the required Nsight Aftermath DLLs must be placed next to the application executable so they can be loaded at runtime.

### Building Without Aftermath

When building the **HashLink** `ReleaseDX12Agility` configuration, Aftermath can be disabled by building the **dx12** project using the **Release** configuration instead of **ReleaseAftermath**.

This excludes Nsight Aftermath from the build and removes the dependency on the Aftermath SDK and runtime DLLs.