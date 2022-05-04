#define HL_NAME(n) dx12_##n
#include <hl.h>

#ifdef HL_WIN_DESKTOP
#include <dxgi.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <dxcapi.h>
#endif

#define DXERR(cmd)	{ HRESULT __ret = cmd; if( __ret == E_OUTOFMEMORY ) return NULL; if( __ret != S_OK ) ReportDxError(__ret,__LINE__); }
#define CHKERR(cmd) { HRESULT __ret = cmd; if( __ret != S_OK ) ReportDxError(__ret,__LINE__); }

typedef struct {
	HWND wnd;
	IDXGIFactory4 *factory;
	IDXGIAdapter1 *adapter;
	IDXGISwapChain4 *swapchain;
	ID3D12Device *device;
	ID3D12CommandQueue *commandQueue;
	ID3D12Debug1 *debug;
    ID3D12DebugDevice *debugDevice;
	ID3D12InfoQueue *infoQueue;
} dx_driver;

static dx_driver *static_driver = NULL;

void dx12_flush_messages();

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


HL_PRIM dx_driver *HL_NAME(create)( HWND window, int flags ) {
	UINT dxgiFlags = 0;
	dx_driver *drv = (dx_driver*)hl_gc_alloc_raw(sizeof(dx_driver));
	memset(drv,0,sizeof(dx_driver));
	drv->wnd = window;

	if( flags & 1 ) {
		ID3D12Debug *debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->QueryInterface(&drv->debug);
		drv->debug->EnableDebugLayer();
		drv->debug->SetEnableGPUBasedValidation(true);
		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
		debugController->Release();
	}
	CHKERR(CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&drv->factory)));

	UINT index = 0;
	IDXGIAdapter1 *adapter = NULL;
	while( drv->factory->EnumAdapters1(index++,&adapter) != DXGI_ERROR_NOT_FOUND ) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) {
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

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;
		CHKERR(drv->device->CreateCommandQueue(&desc,IID_PPV_ARGS(&drv->commandQueue)));
	}

	static_driver = drv;
	return drv;
}

HL_PRIM void HL_NAME(resize)( int width, int height, int buffer_count ) {
	dx_driver *drv = static_driver;
	if( drv->swapchain ) {
		drv->swapchain->ResizeBuffers(buffer_count, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	} else {
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferCount = buffer_count;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.SampleDesc.Count = 1;

		IDXGISwapChain1 *swapchain;
		drv->factory->CreateSwapChainForHwnd(drv->commandQueue,drv->wnd,&desc,NULL,NULL,&swapchain);
		swapchain->QueryInterface(IID_PPV_ARGS(&drv->swapchain));
	}
}

HL_PRIM void HL_NAME(present)( bool vsync ) {
	dx_driver *drv = static_driver;
	UINT syncInterval = vsync ? 1 : 0;
	UINT presentFlags = 0;
	CHKERR(drv->swapchain->Present(syncInterval, presentFlags));
}

int HL_NAME(get_current_back_buffer_index)() {
	return static_driver->swapchain->GetCurrentBackBufferIndex();
}

ID3D12Resource *HL_NAME(get_back_buffer)( int index ) {
	ID3D12Resource *buf = NULL;
	static_driver->swapchain->GetBuffer(index, IID_PPV_ARGS(&buf));
	return buf;
}

void HL_NAME(create_render_target_view)( ID3D12Resource *res, D3D12_RENDER_TARGET_VIEW_DESC *desc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor ) {
	static_driver->device->CreateRenderTargetView(res,desc,descriptor);
}

void HL_NAME(signal)( ID3D12Fence *fence, int64 value ) {
	static_driver->commandQueue->Signal(fence,value);
}

void HL_NAME(resource_release)( IUnknown *res ) {
	res->Release();
}

void HL_NAME(flush_messages)() {
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
}

uchar *HL_NAME(get_device_name)() {
	DXGI_ADAPTER_DESC desc;
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
	return (uchar*)hl_copy_bytes((vbyte*)desc.Description,(int)(ustrlen((uchar*)desc.Description)+1)*2);
}

#define _DRIVER _ABSTRACT(dx_driver)
#define _RES _ABSTRACT(dx_resource)

DEFINE_PRIM(_DRIVER, create, _ABSTRACT(dx_window) _I32);
DEFINE_PRIM(_BOOL, resize, _I32 _I32 _I32);
DEFINE_PRIM(_VOID, present, _BOOL);
DEFINE_PRIM(_I32, get_current_back_buffer_index, _NO_ARG);
DEFINE_PRIM(_VOID, create_render_target_view, _RES _STRUCT _I64);
DEFINE_PRIM(_RES, get_back_buffer, _I32);
DEFINE_PRIM(_VOID, resource_release, _RES);
DEFINE_PRIM(_VOID, signal, _RES _I64);
DEFINE_PRIM(_VOID, flush_messages, _NO_ARG);
DEFINE_PRIM(_BYTES, get_device_name, _NO_ARG);

// ---- SHADERS

typedef struct {
	IDxcLibrary *library;
	IDxcCompiler *compiler;
} dx_compiler;

dx_compiler *HL_NAME(compiler_create)() {
	dx_compiler *comp = (dx_compiler*)hl_gc_alloc_raw(sizeof(dx_compiler));
	memset(comp,0,sizeof(dx_compiler));
	CHKERR(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&comp->library)));
	CHKERR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&comp->compiler)));
	return comp;
}

vbyte *HL_NAME(compiler_compile)( dx_compiler *comp, uchar *source, uchar *profile, varray *args, int *dataLen ) {
	IDxcBlobEncoding *blob = NULL;
	IDxcOperationResult *result = NULL;
	comp->library->CreateBlobWithEncodingFromPinned(source,(int)ustrlen(source)*2,DXC_CP_UTF16,&blob);
	if( blob == NULL )
		hl_error("Could not create blob");
	comp->compiler->Compile(blob,L"",L"main",profile,hl_aptr(args,LPCWSTR),args->size,NULL,0,NULL,&result);
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

#define _COMPILER _ABSTRACT(dx_compiler)
DEFINE_PRIM(_COMPILER, compiler_create, _NO_ARG);
DEFINE_PRIM(_BYTES, compiler_compile, _COMPILER _BYTES _BYTES _ARR _REF(_I32));

// ---- HEAPS

ID3D12DescriptorHeap *HL_NAME(descriptor_heap_create)( D3D12_DESCRIPTOR_HEAP_DESC *desc ) {
	ID3D12DescriptorHeap *heap = NULL;
	DXERR(static_driver->device->CreateDescriptorHeap(desc,IID_PPV_ARGS(&heap)));
	return heap;
}

int HL_NAME(get_descriptor_handle_increment_size)( D3D12_DESCRIPTOR_HEAP_TYPE type ) {
	return static_driver->device->GetDescriptorHandleIncrementSize(type);
}

int64 HL_NAME(descriptor_heap_get_handle)( ID3D12DescriptorHeap *heap, bool gpu ) {
	UINT64 handle = gpu ? heap->GetGPUDescriptorHandleForHeapStart().ptr : heap->GetCPUDescriptorHandleForHeapStart().ptr;
	return handle; 
}

DEFINE_PRIM(_RES, descriptor_heap_create, _STRUCT);
DEFINE_PRIM(_I32, get_descriptor_handle_increment_size, _I32);
DEFINE_PRIM(_I64, descriptor_heap_get_handle, _RES _BOOL);

// ---- SYNCHRO

ID3D12Fence *HL_NAME(fence_create)( int64 value, D3D12_FENCE_FLAGS flags ) {
	ID3D12Fence *f = NULL;
	DXERR(static_driver->device->CreateFence(value,flags, IID_PPV_ARGS(&f)));
	return f;
}

int64 HL_NAME(fence_get_completed_value)( ID3D12Fence *fence ) {
	return (int64)fence->GetCompletedValue();
}

void HL_NAME(fence_set_event)( ID3D12Fence *fence, int64 value, HANDLE event ) {
	fence->SetEventOnCompletion(value, event);
}

HANDLE HL_NAME(waitevent_create)( bool initState ) {
	return CreateEvent(NULL,FALSE,initState,NULL);
}

bool HL_NAME(waitevent_wait)( HANDLE event, int time ) {
	return WaitForSingleObject(event,time) == 0;
}

#define _EVENT _ABSTRACT(dx_event)
DEFINE_PRIM(_RES, fence_create, _I64 _I32);
DEFINE_PRIM(_I64, fence_get_completed_value, _RES);
DEFINE_PRIM(_VOID, fence_set_event, _RES _I64 _EVENT);
DEFINE_PRIM(_EVENT, waitevent_create, _BOOL);
DEFINE_PRIM(_BOOL, waitevent_wait, _EVENT _I32);


// ---- COMMANDS

ID3D12CommandAllocator *HL_NAME(command_allocator_create)( D3D12_COMMAND_LIST_TYPE type ) {
	ID3D12CommandAllocator *a = NULL;
	DXERR(static_driver->device->CreateCommandAllocator(type,IID_PPV_ARGS(&a)));
	return a;
}

void HL_NAME(command_allocator_reset)( ID3D12CommandAllocator *a ) {
	CHKERR(a->Reset());
}

ID3D12GraphicsCommandList *HL_NAME(command_list_create)( int nodeMask, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator *alloc, ID3D12PipelineState *initState ) {
	ID3D12GraphicsCommandList *l = NULL;
	DXERR(static_driver->device->CreateCommandList(nodeMask,type,alloc,initState,IID_PPV_ARGS(&l)));
	return l;
}

void HL_NAME(command_list_close)( ID3D12GraphicsCommandList *l ) {
	CHKERR(l->Close());
}

void HL_NAME(command_list_reset)( ID3D12GraphicsCommandList *l, ID3D12CommandAllocator *alloc, ID3D12PipelineState *state ) {
	CHKERR(l->Reset(alloc,state));
}

void HL_NAME(command_list_execute)( ID3D12GraphicsCommandList *l ) {
	ID3D12CommandList* const commandLists[] = { l };
	static_driver->commandQueue->ExecuteCommandLists(1, commandLists);
}

void HL_NAME(command_list_resource_barrier)( ID3D12GraphicsCommandList *l, D3D12_RESOURCE_BARRIER *barrier ) {
	l->ResourceBarrier(1,barrier);
}

void HL_NAME(command_list_clear_render_target_view)( ID3D12GraphicsCommandList *l, D3D12_CPU_DESCRIPTOR_HANDLE view, FLOAT *colors ) {
	l->ClearRenderTargetView(view,colors,0,NULL);
}

DEFINE_PRIM(_RES, command_allocator_create, _I32);
DEFINE_PRIM(_VOID, command_allocator_reset, _RES);
DEFINE_PRIM(_RES, command_list_create, _I32 _I32 _RES _RES);
DEFINE_PRIM(_VOID, command_list_close, _RES);
DEFINE_PRIM(_VOID, command_list_reset, _RES _RES _RES);
DEFINE_PRIM(_VOID, command_list_resource_barrier, _RES _STRUCT);
DEFINE_PRIM(_VOID, command_list_execute, _RES);
DEFINE_PRIM(_VOID, command_list_clear_render_target_view, _RES _I64 _STRUCT);
