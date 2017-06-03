#define HL_NAME(n) directx_##n
#include <hl.h>

#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <DirectXMath.h>

#define INIT_ERROR __LINE__
#define CHECK(call) if( (call) != S_OK ) return INIT_ERROR

typedef struct {
	ID3D11Device *device;
	ID3D11DeviceContext *context;
	IDXGISwapChain *swapchain;
	ID3D11RenderTargetView *renderTarget;
	D3D_FEATURE_LEVEL feature;
	int init_flags;
} dx_driver;

static dx_driver *driver = NULL;
static IDXGIFactory *factory = NULL;

static IDXGIFactory *GetDXGI() {
	if( factory == NULL && CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) != S_OK )
		hl_error("Failed to init DXGI");
	return factory;
}

HL_PRIM dx_driver *HL_NAME(dx_create)( HWND window, int flags ) {
	DWORD result;
	static D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	dx_driver *d = (dx_driver*)hl_gc_alloc_noptr(sizeof(dx_driver));
	ZeroMemory(d,sizeof(dx_driver));

	d->init_flags = flags;
	result = D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,flags,levels,7,D3D11_SDK_VERSION,&d->device,&d->feature,&d->context);
	if( result == E_INVALIDARG ) // most likely no DX11.1 support, try again
		result = D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,flags,NULL,0,D3D11_SDK_VERSION,&d->device,&d->feature,&d->context);
	if( result != S_OK )
		return NULL;

	// create the SwapChain
	DXGI_SWAP_CHAIN_DESC desc;
	RECT r;
	GetClientRect(window,&r);
	ZeroMemory(&desc,sizeof(desc));
	desc.BufferDesc.Width = r.right;
	desc.BufferDesc.Height = r.bottom;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1; // NO AA for now
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = 1;
	desc.Windowed = true;
	desc.OutputWindow = window;
	if( GetDXGI()->CreateSwapChain(d->device,&desc,&d->swapchain) != S_OK )
		return NULL;

	// create the backbuffer
	ID3D11Texture2D *backBuffer;
	if( d->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) != S_OK )
		return NULL;
	if( d->device->CreateRenderTargetView(backBuffer, NULL, &d->renderTarget) != S_OK )
		return NULL;
	backBuffer->Release();
	driver = d;
	return d;
}

HL_PRIM void HL_NAME(dx_clear_color)( double r, double g, double b, double a ) {
	float color[4];
	color[0] = (float)r;
	color[1] = (float)g;
	color[2] = (float)b;
	color[3] = (float)a;
	driver->context->ClearRenderTargetView(driver->renderTarget,color);
}

HL_PRIM void HL_NAME(dx_present)() {
	driver->swapchain->Present(0,0);
}

HL_PRIM int HL_NAME(dx_get_screen_width)() {
	return GetSystemMetrics(SM_CXSCREEN);
}

HL_PRIM int HL_NAME(dx_get_screen_height)() {
	return GetSystemMetrics(SM_CYSCREEN);
}

HL_PRIM const uchar *HL_NAME(dx_get_device_name)() {
	IDXGIAdapter *adapter;
	DXGI_ADAPTER_DESC desc;
	if( GetDXGI()->EnumAdapters(0,&adapter) != S_OK || adapter->GetDesc(&desc) != S_OK )
		return USTR("Unknown");
	adapter->Release();
	return (uchar*)hl_copy_bytes((vbyte*)desc.Description,(ustrlen((uchar*)desc.Description)+1)*2);
}

HL_PRIM double HL_NAME(dx_get_supported_version)() {
	if( driver == NULL ) return 0.;
	return (driver->feature >> 12) + ((driver->feature & 0xFFF) / 2560.);
}

#define _DRIVER _ABSTRACT(dx_driver)
DEFINE_PRIM(_DRIVER, dx_create, _ABSTRACT(dx_window) _I32);
DEFINE_PRIM(_VOID, dx_clear_color, _F64 _F64 _F64 _F64);
DEFINE_PRIM(_VOID, dx_present, _NO_ARG);
DEFINE_PRIM(_I32, dx_get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, dx_get_screen_height, _NO_ARG);
DEFINE_PRIM(_BYTES, dx_get_device_name, _NO_ARG);
DEFINE_PRIM(_F64, dx_get_supported_version, _NO_ARG);