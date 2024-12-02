#define HL_NAME(n) dx12_##n
#include <hl.h>

#ifdef HL_WIN_DESKTOP
#include <dxgi.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <dxcapi.h>
#endif

#ifdef HL_XBS
#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#include <dxcapi_xs.h>
#else
#include <d3d12_x.h>
#include <d3dx12_x.h>
#include <dxcapi_x.h>
#include <vector>
#endif

#define IID_PPV_ARGS_OLD(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)
#undef IID_PPV_ARGS
#define IID_PPV_ARGS IID_GRAPHICS_PPV_ARGS
#endif

#define DXERR(cmd)	{ HRESULT __ret = cmd; if( __ret == E_OUTOFMEMORY ) return NULL; if( __ret != S_OK ) ReportDxError(__ret,__LINE__); }
#define CHKERR(cmd) { HRESULT __ret = cmd; if( FAILED(__ret) ) ReportDxError(__ret,__LINE__); }

static int gs_constants[] = {
#ifdef _GAMING_XBOX_XBOXONE
	D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT,
#else
	D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,
#endif
	D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,
	D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
};

typedef struct {
	HWND wnd;
	ID3D12CommandQueue *commandQueue;
#ifndef HL_XBS
	IDXGIFactory4 *factory;
	IDXGIAdapter1 *adapter;
	IDXGISwapChain4 *swapchain;
	ID3D12Device *device;
	ID3D12Debug1 *debug;
	ID3D12DebugDevice *debugDevice;
	ID3D12InfoQueue *infoQueue;
#else
	ID3D12Device2 *device;
	ID3D12Debug *debug;
	ID3D12DebugDevice *debugDevice;
	// SwapChain
	UINT swapBufferCount;
	UINT backBufferIndex;
	ID3D12Resource **swapBuffers;
	D3D12XBOX_FRAME_PIPELINE_TOKEN pipelineToken;
	// Cached values
	IDXGIAdapter *adapter;
#endif
} dx_driver;

static dx_driver *static_driver = NULL;
static int CURRENT_NODEMASK = 0;

HL_PRIM void dx12_flush_messages();

static void ReportDxError( HRESULT err, int line ) {
	dx12_flush_messages();
	hl_error("DXERROR %X line %d",(DWORD)err,line);
}

static void OnDebugMessage(
D3D12_MESSAGE_CATEGORY Category,
D3D12_MESSAGE_SEVERITY Severity,
D3D12_MESSAGE_ID ID,
LPCSTR pDescription,
void *pContext ) {
	printf("%s\n", pDescription);
	fflush(stdout);
}

HL_PRIM varray *HL_NAME(list_devices)() {
#ifndef HL_XBS
	static int MAX_DEVICES = 64;
	int index = 0, write = 0;
	IDXGIAdapter1 *adapter = NULL;
	IDXGIFactory4 *factory = NULL;
	varray *arr = hl_alloc_array(&hlt_bytes, MAX_DEVICES);
	if( static_driver )
		factory = static_driver->factory;
	else {
		CHKERR(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
	}
	while( write < MAX_DEVICES && factory->EnumAdapters1(index++,&adapter) != DXGI_ERROR_NOT_FOUND ) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if( (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 )
			hl_aptr(arr,uchar*)[write++] = ustrdup(desc.Description);
		adapter->Release();
	}
	if( !static_driver )
		factory->Release();
	return arr;
#else
	varray *arr = hl_alloc_array(&hlt_bytes, 1);
	DXGI_ADAPTER_DESC desc;
	static_driver->adapter->GetDesc(&desc);
	hl_aptr(arr, uchar*)[0] = ustrdup(desc.Description);
	return arr;
#endif
}


HL_PRIM dx_driver *HL_NAME(create)( HWND window, int flags, uchar *dev_desc ) {
	UINT dxgiFlags = 0;
	dx_driver *drv = (dx_driver*)hl_gc_alloc_raw(sizeof(dx_driver));
	memset(drv, 0, sizeof(dx_driver));
	drv->wnd = window;

	if( flags & 1 ) {
		ID3D12Debug *debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();
#ifndef HL_XBS
		debugController->QueryInterface(&drv->debug);
		drv->debug->EnableDebugLayer();
		drv->debug->SetEnableGPUBasedValidation(true);
		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#else
		debugController->QueryInterface(IID_PPV_ARGS(&drv->debug));
#endif
		debugController->Release();
	}

#ifndef HL_XBS
	CHKERR(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&drv->factory)));

	UINT index = 0;
	IDXGIAdapter1 *adapter = NULL;
	while( drv->factory->EnumAdapters1(index++,&adapter) != DXGI_ERROR_NOT_FOUND ) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if( (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) || (dev_desc && !wcsstr(desc.Description,dev_desc)) ) {
			adapter->Release();
			continue;
		}
		if( SUCCEEDED(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&drv->device))) ) {
			drv->adapter = adapter;
			break;
		}
		adapter->Release();
	}
	if( !drv->device )
		return NULL;
	drv->device->SetName(L"HL_DX12");
	if( drv->debug ) {
		CHKERR(drv->device->QueryInterface(IID_PPV_ARGS(&drv->debugDevice)));
		CHKERR(drv->device->QueryInterface(IID_PPV_ARGS(&drv->infoQueue)));
		drv->infoQueue->ClearStoredMessages();
	}
#else
	D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
	params.Version = D3D12_SDK_VERSION;
#if defined(_DEBUG)
	params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#endif
	params.ProcessDebugFlags |= D3D12XBOX_PROCESS_DEBUG_FLAG_ENABLE_COMMON_STATE_PROMOTION;
	params.GraphicsCommandQueueRingSizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
	params.GraphicsScratchMemorySizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
	params.ComputeScratchMemorySizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
	params.DisableGeometryShaderAllocations = TRUE;
	params.DisableTessellationShaderAllocations = TRUE;
#ifdef _GAMING_XBOX_SCARLETT
	params.DisableDXR = TRUE;
	params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_NONE;
#endif

	CHKERR(D3D12XboxCreateDevice(nullptr,&params, IID_PPV_ARGS(&drv->device)))
	drv->device->SetName(L"HL_DX12_XBS");

	// Prepare for PresentX
	{
		IDXGIDevice1 *dxgiDevice;
		CHKERR(drv->device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
		IDXGIAdapter *dxgiAdapter;
		CHKERR(dxgiDevice->GetAdapter(&dxgiAdapter));
		DXGI_ADAPTER_DESC desc;
		dxgiAdapter->GetDesc(&desc);
		if (dev_desc) {
			wcsstr(desc.Description, dev_desc);
		}
		drv->adapter = dxgiAdapter;
	}
#endif

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = CURRENT_NODEMASK;
		CHKERR(drv->device->CreateCommandQueue(&desc, IID_PPV_ARGS(&drv->commandQueue)));
	}

	static_driver = drv;
	return drv;
}

#ifdef HL_XBS
void register_frame_events() {
	dx_driver *drv = static_driver;
	// Prepare PresentX
	IDXGIOutput *dxgiOutput;
	CHKERR(drv->adapter->EnumOutputs(0, &dxgiOutput));
	// May return S_OK, S_FALSE
	CHKERR(drv->device->SetFrameIntervalX(dxgiOutput, D3D12XBOX_FRAME_INTERVAL_60_HZ, drv->swapBufferCount - 1u /* Allow n-1 frames of latency */, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));
	// May return 0x10000000 on XBOXONE
	CHKERR(drv->device->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));

	// Prepare first pipeline token
	drv->pipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
	CHKERR(drv->device->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE, nullptr, D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE, &drv->pipelineToken));
}
#endif

HL_PRIM void HL_NAME(suspend)() {
#ifdef HL_XBS
	// Must be called from the render thread
	CHKERR(static_driver->commandQueue->SuspendX(0));
#endif
}

HL_PRIM void HL_NAME(resume)() {
#ifdef HL_XBS
	CHKERR(static_driver->commandQueue->ResumeX());
	register_frame_events();
#endif
}

HL_PRIM void HL_NAME(resize)( int width, int height, int buffer_count, DXGI_FORMAT format ) {
	dx_driver *drv = static_driver;
#ifndef HL_XBS
	if( drv->swapchain ) {
		CHKERR(drv->swapchain->ResizeBuffers(buffer_count, width, height, format, 0));
	} else {
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Format = format;
		desc.BufferCount = buffer_count;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc.Count = 1;

		IDXGISwapChain1 *swapchain = NULL;
		drv->factory->CreateSwapChainForHwnd(drv->commandQueue,drv->wnd,&desc,NULL,NULL,&swapchain);
		if( !swapchain ) CHKERR(E_INVALIDARG);
		swapchain->QueryInterface(IID_PPV_ARGS(&drv->swapchain));
		drv->factory->MakeWindowAssociation(drv->wnd, DXGI_MWA_NO_ALT_ENTER);
	}
#else
	if (drv->swapBuffers) {
		free(drv->swapBuffers);
	}

	// Create swap buffers
	CD3DX12_HEAP_PROPERTIES swapChainHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC swapChainBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height,
		1, // This resource has only one texture.
		1  // Use a single mipmap level.
	);
	swapChainBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
	swapChainOptimizedClearValue.Format = format;
	drv->swapBuffers = (ID3D12Resource**) malloc(sizeof(ID3D12Resource*) * buffer_count);
	if (drv->swapBuffers == nullptr)
		hl_error("malloc drv->swapBuffers failed");
	drv->swapBufferCount = buffer_count;
	drv->backBufferIndex = 0;
	for (int n = 0; n < buffer_count; n++)
	{
		drv->device->CreateCommittedResource(&swapChainHeapProperties, D3D12_HEAP_FLAG_ALLOW_DISPLAY, &swapChainBufferDesc, D3D12_RESOURCE_STATE_PRESENT, &swapChainOptimizedClearValue, IID_PPV_ARGS(&drv->swapBuffers[n]));
	}

	register_frame_events();
#endif
}

HL_PRIM void HL_NAME(present)( bool vsync ) {
	dx_driver *drv = static_driver;
#ifndef HL_XBS
	UINT syncInterval = vsync ? 1 : 0;
	UINT presentFlags = 0;
	CHKERR(drv->swapchain->Present(syncInterval, presentFlags));
#else
	D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
	planeParameters.Token = drv->pipelineToken;
	planeParameters.ResourceCount = 1;
	planeParameters.ppResources = &drv->swapBuffers[drv->backBufferIndex];
	CHKERR(drv->commandQueue->PresentX(1, &planeParameters, nullptr));
	drv->backBufferIndex = (drv->backBufferIndex + 1) % drv->swapBufferCount;
	// Prepare next pipeline token
	drv->pipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
	CHKERR(drv->device->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE, nullptr, D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE, &drv->pipelineToken));
#endif
}

HL_PRIM int HL_NAME(get_current_back_buffer_index)() {
#ifndef HL_XBS
	return static_driver->swapchain->GetCurrentBackBufferIndex();
#else
	return static_driver->backBufferIndex;
#endif
}

HL_PRIM void HL_NAME(signal)( ID3D12Fence *fence, int64 value ) {
	static_driver->commandQueue->Signal(fence,value);
}

HL_PRIM void HL_NAME(flush_messages)() {
#ifndef HL_XBS
	dx_driver *drv = static_driver;
	if( !drv->infoQueue ) return;
	int count = (int)drv->infoQueue->GetNumStoredMessages();
	if( !count ) return;
	int i;
	for(i=0;i<count;i++) {
		SIZE_T len = 0;
		drv->infoQueue->GetMessage(i, NULL, &len);
		D3D12_MESSAGE *msg = (D3D12_MESSAGE*)malloc(len);
		if( msg == NULL ) break;
		drv->infoQueue->GetMessage(i, msg, &len);
		printf("%s\n",msg->pDescription);
		free(msg);
		fflush(stdout);
	}
	drv->infoQueue->ClearStoredMessages();
#endif
}

HL_PRIM uchar *HL_NAME(get_device_name)() {
	DXGI_ADAPTER_DESC desc;
#ifndef HL_XBS
	IDXGIAdapter *adapter = NULL;
	if( !static_driver ) {
		IDXGIFactory4 *factory = NULL;
		CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
		if( factory ) factory->EnumAdapters(0,&adapter);
		if( !adapter )
			return USTR("Unknown");
	} else
		adapter = static_driver->adapter;
	adapter->GetDesc(&desc);
#else
	static_driver->adapter->GetDesc(&desc);
#endif
	return (uchar*)hl_copy_bytes((vbyte*)desc.Description, (int)(ustrlen((uchar*)desc.Description) + 1) * 2);
}

HL_PRIM int64 HL_NAME(get_timestamp_frequency)() {
	UINT64 f = 0;
	CHKERR(static_driver->commandQueue->GetTimestampFrequency(&f))
	return (int64)f;
}

#define _DRIVER _ABSTRACT(dx_driver)
#define _RES _ABSTRACT(dx_resource)

DEFINE_PRIM(_ARR, list_devices, _NO_ARG);
DEFINE_PRIM(_DRIVER, create, _ABSTRACT(dx_window) _I32 _BYTES);
DEFINE_PRIM(_VOID, resize, _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, present, _BOOL);
DEFINE_PRIM(_VOID, suspend, _NO_ARG);
DEFINE_PRIM(_VOID, resume, _NO_ARG);
DEFINE_PRIM(_I32, get_current_back_buffer_index, _NO_ARG);
DEFINE_PRIM(_VOID, signal, _RES _I64);
DEFINE_PRIM(_VOID, flush_messages, _NO_ARG);
DEFINE_PRIM(_BYTES, get_device_name, _NO_ARG);
DEFINE_PRIM(_I64, get_timestamp_frequency, _NO_ARG);

/// --- utilities (from d3dx12.h)

#ifndef HL_XBS
struct CD3DX12_TEXTURE_COPY_LOCATION : public D3D12_TEXTURE_COPY_LOCATION
{
	CD3DX12_TEXTURE_COPY_LOCATION() = default;
	explicit CD3DX12_TEXTURE_COPY_LOCATION(const D3D12_TEXTURE_COPY_LOCATION &o) noexcept :
		D3D12_TEXTURE_COPY_LOCATION(o)
	{}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource* pRes) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		PlacedFootprint = {};
	}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource* pRes, D3D12_PLACED_SUBRESOURCE_FOOTPRINT const& Footprint) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		PlacedFootprint = Footprint;
	}
	CD3DX12_TEXTURE_COPY_LOCATION(_In_ ID3D12Resource* pRes, UINT Sub) noexcept
	{
		pResource = pRes;
		Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		PlacedFootprint = {};
		SubresourceIndex = Sub;
	}
};

inline void MemcpySubresource(
	_In_ const D3D12_MEMCPY_DEST* pDest,
	_In_ const D3D12_SUBRESOURCE_DATA* pSrc,
	SIZE_T RowSizeInBytes,
	UINT NumRows,
	UINT NumSlices) noexcept
{
	for (UINT z = 0; z < NumSlices; ++z)
	{
		auto pDestSlice = static_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
		auto pSrcSlice = static_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(pDestSlice + pDest->RowPitch * y,
				   pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
				   RowSizeInBytes);
		}
	}
}

inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	_In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
	UINT64 RequiredSize,
	_In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	_In_reads_(NumSubresources) const UINT* pNumRows,
	_In_reads_(NumSubresources) const UINT64* pRowSizesInBytes,
	_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
{
	// Minor validation
	auto IntermediateDesc = pIntermediate->GetDesc();
	auto DestinationDesc = pDestinationResource->GetDesc();
	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
		IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset ||
		RequiredSize > SIZE_T(-1) ||
		(DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
			(FirstSubresource != 0 || NumSubresources != 1)))
	{
		return 0;
	}

	BYTE* pData;
	HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void**>(&pData));
	if (FAILED(hr))
	{
		return 0;
	}

	for (UINT i = 0; i < NumSubresources; ++i)
	{
		if (pRowSizesInBytes[i] > SIZE_T(-1)) return 0;
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i]) };
		MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i], pLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, nullptr);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		pCmdList->CopyBufferRegion(
			pDestinationResource, 0, pIntermediate, pLayouts[0].Offset, pLayouts[0].Footprint.Width);
	}
	else
	{
		for (UINT i = 0; i < NumSubresources; ++i)
		{
			CD3DX12_TEXTURE_COPY_LOCATION Dst(pDestinationResource, i + FirstSubresource);
			CD3DX12_TEXTURE_COPY_LOCATION Src(pIntermediate, pLayouts[i]);
			pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return RequiredSize;
}

inline UINT64 UpdateSubresources(
	_In_ ID3D12GraphicsCommandList* pCmdList,
	_In_ ID3D12Resource* pDestinationResource,
	_In_ ID3D12Resource* pIntermediate,
	UINT64 IntermediateOffset,
	_In_range_(0,D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
	_In_range_(0,D3D12_REQ_SUBRESOURCES-FirstSubresource) UINT NumSubresources,
	_In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA* pSrcData) noexcept
{
	UINT64 RequiredSize = 0;
	auto MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
	if (MemToAlloc > SIZE_MAX)
	{
	   return 0;
	}
	void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
	if (pMem == nullptr)
	{
	   return 0;
	}
	auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
	auto pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
	auto pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);

	auto Desc = pDestinationResource->GetDesc();
	static_driver->device->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);

	UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource, NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes, pSrcData);
	HeapFree(GetProcessHeap(), 0, pMem);
	return Result;
}

#endif

// ---- RESOURCES

HL_PRIM ID3D12Resource *HL_NAME(get_back_buffer)( int index ) {
	ID3D12Resource *buf = NULL;
#ifndef HL_XBS
	static_driver->swapchain->GetBuffer(index, IID_PPV_ARGS(&buf));
#else
	buf = static_driver->swapBuffers[index];
#endif
	return buf;
}

HL_PRIM ID3D12Resource *HL_NAME(create_committed_resource)( D3D12_HEAP_PROPERTIES *heapProperties, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_DESC *desc, D3D12_RESOURCE_STATES initialState, D3D12_CLEAR_VALUE *clearValue ) {
	ID3D12Resource *res = NULL;
#ifdef HL_XBS
	// In normal dx, INDIRECT_ARGUMENT is included in GENERIC_READ (but we never use it alone) , so we remove it to obtain GENERIC_READ in xbox
	initialState = (D3D12_RESOURCE_STATES)(initialState & ~D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
#endif
	DXERR(static_driver->device->CreateCommittedResource(heapProperties, heapFlags, desc, initialState, clearValue, IID_PPV_ARGS(&res)));
	return res;
}

HL_PRIM void HL_NAME(create_render_target_view)( ID3D12Resource *res, D3D12_RENDER_TARGET_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateRenderTargetView(res,desc,descriptor);
}

HL_PRIM void HL_NAME(create_depth_stencil_view)( ID3D12Resource *res, D3D12_DEPTH_STENCIL_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateDepthStencilView(res,desc,descriptor);
}

HL_PRIM void HL_NAME(create_constant_buffer_view)( D3D12_CONSTANT_BUFFER_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateConstantBufferView(desc,descriptor);
}

HL_PRIM void HL_NAME(create_unordered_access_view)( ID3D12Resource *res, ID3D12Resource *counter, D3D12_UNORDERED_ACCESS_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateUnorderedAccessView(res,counter,desc,descriptor);
}

HL_PRIM void HL_NAME(create_sampler)( D3D12_SAMPLER_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateSampler(desc,descriptor);
}

HL_PRIM void HL_NAME(create_shader_resource_view)( ID3D12Resource *res, D3D12_SHADER_RESOURCE_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateShaderResourceView(res,desc,descriptor);
}

HL_PRIM int64 HL_NAME(resource_get_gpu_virtual_address)( ID3D12Resource *res ) {
	return res->GetGPUVirtualAddress();
}

HL_PRIM void HL_NAME(resource_release)( IUnknown *res ) {
	res->Release();
}

HL_PRIM void HL_NAME(resource_set_name)( ID3D12Resource *res, vbyte *name ) {
	res->SetName((LPCWSTR)name);
}

HL_PRIM void *HL_NAME(resource_map)( ID3D12Resource *res, int subres, D3D12_RANGE *range ) {
	void *data = NULL;
	DXERR(res->Map(subres, range, &data));
	return data;
}

HL_PRIM void HL_NAME(resource_unmap)( ID3D12Resource *res, int subres, D3D12_RANGE *range ) {
	res->Unmap(subres, range);
}

HL_PRIM int64 HL_NAME(get_required_intermediate_size)( ID3D12Resource *res, int first, int count ) {
	auto desc = res->GetDesc();
	UINT64 size = 0;
	static_driver->device->GetCopyableFootprints(&desc, first, count, 0, NULL, NULL, NULL, &size);
	return size;
}

HL_PRIM bool HL_NAME(update_sub_resource)( ID3D12GraphicsCommandList *cmd, ID3D12Resource *res, ID3D12Resource *tmp, int64 tmpOffs, int first, int count, D3D12_SUBRESOURCE_DATA *data ) {
	return UpdateSubresources(cmd,res,tmp,(UINT64)tmpOffs,(UINT)first,(UINT)count,data) != 0;
}

HL_PRIM void HL_NAME(get_copyable_footprints)( D3D12_RESOURCE_DESC *desc, int first, int count, int64 offset, D3D12_PLACED_SUBRESOURCE_FOOTPRINT *layouts, int *numRows, int64 *rowSizes, int64 *totalBytes ) {
	static_driver->device->GetCopyableFootprints(desc, first, count, offset, layouts, (UINT*)numRows, (UINT64*)rowSizes, (UINT64*)totalBytes);
}

DEFINE_PRIM(_VOID, create_render_target_view, _RES _STRUCT _I64);
DEFINE_PRIM(_VOID, create_depth_stencil_view, _RES _STRUCT _I64);
DEFINE_PRIM(_VOID, create_shader_resource_view, _RES _STRUCT _I64);
DEFINE_PRIM(_VOID, create_constant_buffer_view, _STRUCT _I64);
DEFINE_PRIM(_VOID, create_unordered_access_view, _RES _RES _STRUCT _I64);
DEFINE_PRIM(_VOID, create_sampler, _STRUCT _I64);
DEFINE_PRIM(_RES, create_committed_resource, _STRUCT _I32 _STRUCT _I32 _STRUCT);
DEFINE_PRIM(_RES, get_back_buffer, _I32);
DEFINE_PRIM(_VOID, resource_release, _RES);
DEFINE_PRIM(_VOID, resource_set_name, _RES _BYTES);
DEFINE_PRIM(_I64, resource_get_gpu_virtual_address, _RES);
DEFINE_PRIM(_BYTES, resource_map, _RES _I32 _STRUCT);
DEFINE_PRIM(_VOID, resource_unmap, _RES _I32 _STRUCT);
DEFINE_PRIM(_I64, get_required_intermediate_size, _RES _I32 _I32);
DEFINE_PRIM(_BOOL, update_sub_resource, _RES _RES _RES _I64 _I32 _I32 _STRUCT);
DEFINE_PRIM(_VOID, get_copyable_footprints, _STRUCT _I32 _I32 _I64 _STRUCT _BYTES _BYTES _BYTES);

// ---- SHADERS

typedef struct {
#ifndef HL_XBS
	IDxcLibrary *library;
	IDxcCompiler *compiler;
#else
	IDxcUtils *utils;
	IDxcCompiler3 *compiler;
#endif
} dx_compiler;

HL_PRIM dx_compiler *HL_NAME(compiler_create)() {
	dx_compiler *comp = (dx_compiler*)hl_gc_alloc_raw(sizeof(dx_compiler));
	memset(comp,0,sizeof(dx_compiler));
#ifndef HL_XBS
	CHKERR(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&comp->library)));
	CHKERR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&comp->compiler)));
#else
	CHKERR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS_OLD(&comp->utils)));
	CHKERR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS_OLD(&comp->compiler)));
#endif
	return comp;
}

HL_PRIM vbyte *HL_NAME(compiler_compile)( dx_compiler *comp, uchar *source, uchar *profile, varray *args, int *dataLen ) {
	IDxcBlobEncoding *blob = NULL;
	IDxcOperationResult *result = NULL;
#ifndef HL_XBS
	comp->library->CreateBlobWithEncodingFromPinned(source,(int)ustrlen(source)*2,1200/*DXC_CP_UTF16*/,&blob);
#else
	comp->utils->CreateBlobFromPinned(source, (int)ustrlen(source) * 2, 1200/*DXC_CP_UTF16*/, &blob);
#endif
	if( blob == NULL )
		hl_error("Could not create blob");
#ifndef HL_XBS
	comp->compiler->Compile(blob,L"",L"main",profile,hl_aptr(args,LPCWSTR),args->size,NULL,0,NULL,&result);
#else
	BOOL knownEncoding = FALSE;
	UINT32 encoding = 0U;
	blob->GetEncoding(&knownEncoding, &encoding);
	DxcBuffer dxcBuffer = { blob->GetBufferPointer(), blob->GetBufferSize(), encoding };
	std::vector<LPCWSTR> arguments(hl_aptr(args, LPCWSTR), hl_aptr(args, LPCWSTR) + args->size);
	arguments.insert(arguments.end(), { L"-E", L"main", L"-T", profile, L"-D", L"__XBOX_DISABLE_PRECOMPILE" });
	CHKERR(comp->compiler->Compile(&dxcBuffer, arguments.data(), arguments.size(), NULL, IID_PPV_ARGS_OLD(&result)));
#endif
	HRESULT hr;
	result->GetStatus(&hr);
	if( !SUCCEEDED(hr) ) {
		IDxcBlobEncoding *error = NULL;
		result->GetErrorBuffer(&error);
		uchar *c = hl_to_utf16((char*)error->GetBufferPointer());
		blob->Release();
		result->Release();
		error->Release();
		hl_error("%s",c);
	}
	IDxcBlob *out = NULL;
	result->GetResult(&out);
	*dataLen = (int)out->GetBufferSize();
	vbyte *bytes = hl_copy_bytes((vbyte*)out->GetBufferPointer(), *dataLen);
	out->Release();
	blob->Release();
	result->Release();
	return bytes;
}

HL_PRIM vbyte *HL_NAME(serialize_root_signature)( D3D12_ROOT_SIGNATURE_DESC *signature, D3D_ROOT_SIGNATURE_VERSION version, int *dataLen ) {
	ID3DBlob *data = NULL;
	ID3DBlob *error = NULL;
	HRESULT r = D3D12SerializeRootSignature(signature,version, &data, &error);
	if( !SUCCEEDED(r) ) {
		uchar *c = error ? hl_to_utf16((char*)error->GetBufferPointer()) : USTR("Invalid argument");
		if( error ) error->Release();
		hl_error("%s",c);
	}
	*dataLen = (int)data->GetBufferSize();
	vbyte *bytes = hl_copy_bytes((vbyte*)data->GetBufferPointer(), *dataLen);
	data->Release();
	return bytes;
}

HL_PRIM ID3D12RootSignature *HL_NAME(rootsignature_create)( vbyte *bytes, int len ) {
	ID3D12RootSignature *sign = NULL;
	DXERR(static_driver->device->CreateRootSignature(CURRENT_NODEMASK, bytes, len, IID_PPV_ARGS(&sign)));
	return sign;
}

HL_PRIM ID3D12PipelineState *HL_NAME(create_graphics_pipeline_state)( D3D12_GRAPHICS_PIPELINE_STATE_DESC *desc ) {
	ID3D12PipelineState *state = NULL;
	// if shader is considered invalid, maybe you're missing dxil.dll
	DXERR(static_driver->device->CreateGraphicsPipelineState(desc,IID_PPV_ARGS(&state)));
	return state;
}

HL_PRIM ID3D12PipelineState *HL_NAME(create_compute_pipeline_state)( D3D12_COMPUTE_PIPELINE_STATE_DESC *desc ) {
	ID3D12PipelineState *state = NULL;
	// if shader is considered invalid, maybe you're missing dxil.dll
	DXERR(static_driver->device->CreateComputePipelineState(desc,IID_PPV_ARGS(&state)));
	return state;
}

HL_PRIM ID3D12CommandSignature *HL_NAME(create_command_signature)( D3D12_COMMAND_SIGNATURE_DESC *desc, ID3D12RootSignature *rootSign ) {
	ID3D12CommandSignature *sign = NULL;
	DXERR(static_driver->device->CreateCommandSignature(desc,rootSign,IID_PPV_ARGS(&sign)));
	return sign;
}

#define _COMPILER _ABSTRACT(dx_compiler)
DEFINE_PRIM(_COMPILER, compiler_create, _NO_ARG);
DEFINE_PRIM(_BYTES, compiler_compile, _COMPILER _BYTES _BYTES _ARR _REF(_I32));
DEFINE_PRIM(_BYTES, serialize_root_signature, _STRUCT _I32 _REF(_I32));
DEFINE_PRIM(_RES, rootsignature_create, _BYTES _I32);
DEFINE_PRIM(_RES, create_graphics_pipeline_state, _STRUCT);
DEFINE_PRIM(_RES, create_compute_pipeline_state, _STRUCT);
DEFINE_PRIM(_RES, create_command_signature, _STRUCT _RES);

// ---- HEAPS

HL_PRIM ID3D12DescriptorHeap *HL_NAME(descriptor_heap_create)( D3D12_DESCRIPTOR_HEAP_DESC *desc ) {
	ID3D12DescriptorHeap *heap = NULL;
	DXERR(static_driver->device->CreateDescriptorHeap(desc,IID_PPV_ARGS(&heap)));
	return heap;
}

HL_PRIM int HL_NAME(get_descriptor_handle_increment_size)( D3D12_DESCRIPTOR_HEAP_TYPE type ) {
	return static_driver->device->GetDescriptorHandleIncrementSize(type);
}

HL_PRIM int64 HL_NAME(descriptor_heap_get_handle)( ID3D12DescriptorHeap *heap, bool gpu ) {
	UINT64 handle = gpu ? heap->GetGPUDescriptorHandleForHeapStart().ptr : heap->GetCPUDescriptorHandleForHeapStart().ptr;
	return handle;
}

HL_PRIM ID3D12QueryHeap *HL_NAME(create_query_heap)( D3D12_QUERY_HEAP_DESC *desc ) {
	ID3D12QueryHeap *heap = NULL;
	DXERR(static_driver->device->CreateQueryHeap(desc,IID_PPV_ARGS(&heap)));
	return heap;
}

DEFINE_PRIM(_RES, descriptor_heap_create, _STRUCT);
DEFINE_PRIM(_I32, get_descriptor_handle_increment_size, _I32);
DEFINE_PRIM(_I64, descriptor_heap_get_handle, _RES _BOOL);
DEFINE_PRIM(_RES, create_query_heap, _STRUCT);

// ---- SYNCHRO

HL_PRIM ID3D12Fence *HL_NAME(fence_create)( int64 value, D3D12_FENCE_FLAGS flags ) {
	ID3D12Fence *f = NULL;
	DXERR(static_driver->device->CreateFence(value,flags, IID_PPV_ARGS(&f)));
	return f;
}

HL_PRIM int64 HL_NAME(fence_get_completed_value)( ID3D12Fence *fence ) {
	return (int64)fence->GetCompletedValue();
}

HL_PRIM void HL_NAME(fence_set_event)( ID3D12Fence *fence, int64 value, HANDLE event ) {
	fence->SetEventOnCompletion(value, event);
}

HL_PRIM HANDLE HL_NAME(waitevent_create)( bool initState ) {
	return CreateEvent(NULL,FALSE,initState,NULL);
}

HL_PRIM bool HL_NAME(waitevent_wait)( HANDLE event, int time ) {
	return WaitForSingleObject(event,time) == 0;
}

#define _EVENT _ABSTRACT(dx_event)
DEFINE_PRIM(_RES, fence_create, _I64 _I32);
DEFINE_PRIM(_I64, fence_get_completed_value, _RES);
DEFINE_PRIM(_VOID, fence_set_event, _RES _I64 _EVENT);
DEFINE_PRIM(_EVENT, waitevent_create, _BOOL);
DEFINE_PRIM(_BOOL, waitevent_wait, _EVENT _I32);


// ---- COMMANDS

HL_PRIM ID3D12CommandAllocator *HL_NAME(command_allocator_create)( D3D12_COMMAND_LIST_TYPE type ) {
	ID3D12CommandAllocator *a = NULL;
	DXERR(static_driver->device->CreateCommandAllocator(type,IID_PPV_ARGS(&a)));
	return a;
}

HL_PRIM void HL_NAME(command_allocator_reset)( ID3D12CommandAllocator *a ) {
	CHKERR(a->Reset());
}

HL_PRIM ID3D12GraphicsCommandList *HL_NAME(command_list_create)( D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *alloc, ID3D12PipelineState *initState ) {
	ID3D12GraphicsCommandList *l = NULL;
	DXERR(static_driver->device->CreateCommandList(CURRENT_NODEMASK,type,alloc,initState,IID_PPV_ARGS(&l)));
	return l;
}

HL_PRIM void HL_NAME(command_list_close)( ID3D12GraphicsCommandList *l ) {
	CHKERR(l->Close());
}

HL_PRIM void HL_NAME(command_list_reset)( ID3D12GraphicsCommandList *l, ID3D12CommandAllocator *alloc, ID3D12PipelineState *state ) {
	CHKERR(l->Reset(alloc,state));
}

HL_PRIM void HL_NAME(command_list_execute)( ID3D12GraphicsCommandList *l ) {
	ID3D12CommandList* const commandLists[] = { l };
	static_driver->commandQueue->ExecuteCommandLists(1, commandLists);
}

HL_PRIM void HL_NAME(command_list_resource_barrier)( ID3D12GraphicsCommandList *l, D3D12_RESOURCE_BARRIER *barrier ) {
	l->ResourceBarrier(1,barrier);
}

HL_PRIM void HL_NAME(command_list_resource_barriers)(ID3D12GraphicsCommandList* l, D3D12_RESOURCE_BARRIER* barrier, int barrierCount) {
	l->ResourceBarrier(barrierCount, barrier);
}

HL_PRIM void HL_NAME(command_list_clear_render_target_view)( ID3D12GraphicsCommandList *l, D3D12_CPU_DESCRIPTOR_HANDLE view, FLOAT *colors ) {
	l->ClearRenderTargetView(view,colors,0,NULL);
}

HL_PRIM void HL_NAME(command_list_clear_depth_stencil_view)( ID3D12GraphicsCommandList *l, D3D12_CPU_DESCRIPTOR_HANDLE view, D3D12_CLEAR_FLAGS flags, FLOAT depth, int stencil ) {
	l->ClearDepthStencilView(view,flags,depth,(UINT8)stencil,0,NULL);
}

HL_PRIM void HL_NAME(command_list_draw_instanced)( ID3D12GraphicsCommandList *l, int vertexCountPerInstance, int instanceCount, int startVertexLocation, int startInstanceLocation ) {
	l->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

HL_PRIM void HL_NAME(command_list_draw_indexed_instanced)( ID3D12GraphicsCommandList *l, int indexCountPerInstance, int instanceCount, int startIndexLocation, int baseVertexLocation, int startInstanceLocation ) {
	l->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

HL_PRIM void HL_NAME(command_list_set_graphics_root_signature)( ID3D12GraphicsCommandList *l, ID3D12RootSignature *sign ) {
	l->SetGraphicsRootSignature(sign);
}

HL_PRIM void HL_NAME(command_list_set_graphics_root32_bit_constants)( ID3D12GraphicsCommandList *l, int index, int numValues, void *data, int destOffset ) {
	l->SetGraphicsRoot32BitConstants(index, numValues, data, destOffset);
}

HL_PRIM void HL_NAME(command_list_set_pipeline_state)( ID3D12GraphicsCommandList *l, ID3D12PipelineState *pipe ) {
	l->SetPipelineState(pipe);
}

HL_PRIM void HL_NAME(command_list_ia_set_vertex_buffers)( ID3D12GraphicsCommandList *l, int startSlot, int numViews, D3D12_VERTEX_BUFFER_VIEW *views ) {
	l->IASetVertexBuffers(startSlot, numViews, views);
}

HL_PRIM void HL_NAME(command_list_ia_set_index_buffer)( ID3D12GraphicsCommandList *l, D3D12_INDEX_BUFFER_VIEW *view ) {
	l->IASetIndexBuffer(view);
}

HL_PRIM void HL_NAME(command_list_ia_set_primitive_topology)( ID3D12GraphicsCommandList *l, D3D12_PRIMITIVE_TOPOLOGY topo ) {
	l->IASetPrimitiveTopology(topo);
}

HL_PRIM void HL_NAME(command_list_copy_buffer_region)( ID3D12GraphicsCommandList *l, ID3D12Resource *dst, int64 dstOffset, ID3D12Resource *src, int64 srcOffset, int64 numBytes ) {
	l->CopyBufferRegion(dst, dstOffset, src, srcOffset, numBytes);
}

HL_PRIM void HL_NAME(command_list_copy_texture_region)( ID3D12GraphicsCommandList *l, D3D12_TEXTURE_COPY_LOCATION *dst, int dstX, int dstY, int dstZ, D3D12_TEXTURE_COPY_LOCATION *src, D3D12_BOX *srcBox ) {
	l->CopyTextureRegion(dst, dstX, dstY, dstZ, src, srcBox);
}

HL_PRIM void HL_NAME(command_list_om_set_render_targets)( ID3D12GraphicsCommandList *l, int count, D3D12_CPU_DESCRIPTOR_HANDLE *handles, BOOL flag, D3D12_CPU_DESCRIPTOR_HANDLE *depthStencils ) {
	l->OMSetRenderTargets(count,handles,flag,depthStencils);
}

HL_PRIM void HL_NAME(command_list_om_set_stencil_ref)( ID3D12GraphicsCommandList *l, int value ) {
	l->OMSetStencilRef(value);
}

HL_PRIM void HL_NAME(command_list_rs_set_viewports)( ID3D12GraphicsCommandList *l, int count, D3D12_VIEWPORT *viewports ) {
	l->RSSetViewports(count, viewports);
}

HL_PRIM void HL_NAME(command_list_rs_set_scissor_rects)( ID3D12GraphicsCommandList *l, int count, D3D12_RECT *rects ) {
	l->RSSetScissorRects(count, rects);
}

HL_PRIM void HL_NAME(command_list_set_descriptor_heaps)( ID3D12GraphicsCommandList *l, varray *heaps ) {
	l->SetDescriptorHeaps(heaps->size,hl_aptr(heaps,ID3D12DescriptorHeap*));
}

HL_PRIM void HL_NAME(command_list_set_graphics_root_constant_buffer_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS address ) {
	l->SetGraphicsRootConstantBufferView(index,address);
}

HL_PRIM void HL_NAME(command_list_set_graphics_root_descriptor_table)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_DESCRIPTOR_HANDLE handle ) {
	l->SetGraphicsRootDescriptorTable(index,handle);
}

HL_PRIM void HL_NAME(command_list_set_graphics_root_shader_resource_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS handle ) {
	l->SetGraphicsRootShaderResourceView(index,handle);
}

HL_PRIM void HL_NAME(command_list_set_graphics_root_unordered_access_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS handle ) {
	l->SetGraphicsRootUnorderedAccessView(index,handle);
}

HL_PRIM void HL_NAME(command_list_execute_indirect)( ID3D12GraphicsCommandList *l, ID3D12CommandSignature *sign, int maxCommandCount, ID3D12Resource *args, int64 argsOffset, ID3D12Resource *count, int64 countOffset  ) {
	l->ExecuteIndirect(sign, maxCommandCount, args, argsOffset, count, countOffset);
}

HL_PRIM void HL_NAME(command_list_begin_query)( ID3D12GraphicsCommandList *l, ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, int index ) {
	l->BeginQuery(heap, type, index);
}

HL_PRIM void HL_NAME(command_list_end_query)( ID3D12GraphicsCommandList *l, ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, int index ) {
	l->EndQuery(heap, type, index);
}

HL_PRIM void HL_NAME(command_list_resolve_query_data)( ID3D12GraphicsCommandList *l, ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, int index, int count, ID3D12Resource *dest, int64 offset ) {
	l->ResolveQueryData(heap,type,index,count,dest,offset);
}

HL_PRIM void HL_NAME(command_list_set_predication)( ID3D12GraphicsCommandList *l, ID3D12Resource *res, int64 offset, D3D12_PREDICATION_OP op ) {
	l->SetPredication(res,offset,op);
}

HL_PRIM void HL_NAME(command_list_set_compute_root_signature)( ID3D12GraphicsCommandList *l, ID3D12RootSignature *sign ) {
	l->SetComputeRootSignature(sign);
}

HL_PRIM void HL_NAME(command_list_set_compute_root32_bit_constants)( ID3D12GraphicsCommandList *l, int index, int numValues, void *data, int destOffset ) {
	l->SetComputeRoot32BitConstants(index, numValues, data, destOffset);
}

HL_PRIM void HL_NAME(command_list_set_compute_root_constant_buffer_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS address ) {
	l->SetComputeRootConstantBufferView(index,address);
}

HL_PRIM void HL_NAME(command_list_set_compute_root_descriptor_table)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_DESCRIPTOR_HANDLE handle ) {
	l->SetComputeRootDescriptorTable(index,handle);
}

HL_PRIM void HL_NAME(command_list_set_compute_root_shader_resource_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS handle ) {
	l->SetComputeRootShaderResourceView(index,handle);
}

HL_PRIM void HL_NAME(command_list_set_compute_root_unordered_access_view)( ID3D12GraphicsCommandList *l, int index, D3D12_GPU_VIRTUAL_ADDRESS handle ) {
	l->SetComputeRootUnorderedAccessView(index,handle);
}

HL_PRIM void HL_NAME(command_list_dispatch)( ID3D12GraphicsCommandList *l, int x, int y, int z ) {
	l->Dispatch(x,y,z);
}

DEFINE_PRIM(_RES, command_allocator_create, _I32);
DEFINE_PRIM(_VOID, command_allocator_reset, _RES);
DEFINE_PRIM(_RES, command_list_create, _I32 _RES _RES);
DEFINE_PRIM(_VOID, command_list_close, _RES);
DEFINE_PRIM(_VOID, command_list_reset, _RES _RES _RES);
DEFINE_PRIM(_VOID, command_list_resource_barrier, _RES _STRUCT);
DEFINE_PRIM(_VOID, command_list_resource_barriers, _RES _ABSTRACT(hl_carray) _I32);
DEFINE_PRIM(_VOID, command_list_execute, _RES);
DEFINE_PRIM(_VOID, command_list_clear_render_target_view, _RES _I64 _STRUCT);
DEFINE_PRIM(_VOID, command_list_clear_depth_stencil_view, _RES _I64 _I32 _F32 _I32);
DEFINE_PRIM(_VOID, command_list_draw_instanced, _RES _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, command_list_draw_indexed_instanced, _RES _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, command_list_set_graphics_root_signature, _RES _RES);
DEFINE_PRIM(_VOID, command_list_set_graphics_root32_bit_constants, _RES _I32 _I32 _BYTES _I32);
DEFINE_PRIM(_VOID, command_list_set_graphics_root_constant_buffer_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_graphics_root_descriptor_table, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_graphics_root_shader_resource_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_graphics_root_unordered_access_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_descriptor_heaps, _RES _ARR);
DEFINE_PRIM(_VOID, command_list_set_pipeline_state, _RES _RES);
DEFINE_PRIM(_VOID, command_list_ia_set_vertex_buffers, _RES _I32 _I32 _STRUCT);
DEFINE_PRIM(_VOID, command_list_ia_set_index_buffer, _RES _STRUCT);
DEFINE_PRIM(_VOID, command_list_ia_set_primitive_topology, _RES _I32);
DEFINE_PRIM(_VOID, command_list_copy_buffer_region, _RES _RES _I64 _RES _I64 _I64);
DEFINE_PRIM(_VOID, command_list_copy_texture_region, _RES _STRUCT _I32 _I32 _I32 _STRUCT _STRUCT);
DEFINE_PRIM(_VOID, command_list_om_set_render_targets, _RES _I32 _BYTES _I32 _BYTES);
DEFINE_PRIM(_VOID, command_list_om_set_stencil_ref, _RES _I32);
DEFINE_PRIM(_VOID, command_list_rs_set_viewports, _RES _I32 _STRUCT);
DEFINE_PRIM(_VOID, command_list_rs_set_scissor_rects, _RES _I32 _STRUCT);
DEFINE_PRIM(_VOID, command_list_execute_indirect, _RES _RES _I32 _RES _I64 _RES _I64);
DEFINE_PRIM(_VOID, command_list_begin_query, _RES _RES _I32 _I32);
DEFINE_PRIM(_VOID, command_list_end_query, _RES _RES _I32 _I32);
DEFINE_PRIM(_VOID, command_list_resolve_query_data, _RES _RES _I32 _I32 _I32 _RES _I64);
DEFINE_PRIM(_VOID, command_list_set_predication, _RES _RES _I64 _I32);

DEFINE_PRIM(_VOID, command_list_set_compute_root_signature, _RES _RES);
DEFINE_PRIM(_VOID, command_list_set_compute_root32_bit_constants, _RES _I32 _I32 _BYTES _I32);
DEFINE_PRIM(_VOID, command_list_set_compute_root_constant_buffer_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_compute_root_descriptor_table, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_compute_root_shader_resource_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_set_compute_root_unordered_access_view, _RES _I32 _I64);
DEFINE_PRIM(_VOID, command_list_dispatch, _RES _I32 _I32 _I32);

//command_list_clear_unordered_access_view_float,
//command_list_clear_unordered_access_view_uint,

HL_PRIM int HL_NAME(get_constant)(int index) {
	return gs_constants[index];
}

DEFINE_PRIM(_I32, get_constant, _I32);