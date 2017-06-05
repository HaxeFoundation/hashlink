#define HL_NAME(n) directx_##n
#include <hl.h>

#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <D3Dcompiler.h>

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

typedef struct {
	hl_type *t;
	D3D11_BOX box;
} dx_box_obj;


typedef ID3D11Buffer dx_buffer;
typedef ID3D11VertexShader dx_vshader;
typedef ID3D11PixelShader dx_pshader;
typedef ID3D11Buffer dx_buffer;
typedef ID3D11InputLayout dx_layout;

static dx_driver *driver = NULL;
static IDXGIFactory *factory = NULL;

static IDXGIFactory *GetDXGI() {
	if( factory == NULL && CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) != S_OK )
		hl_error("Failed to init DXGI");
	return factory;
}

HL_PRIM dx_driver *HL_NAME(create)( HWND window, int flags ) {
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

	d->context->OMSetRenderTargets(1,&d->renderTarget,NULL);

	ID3D11RasterizerState *rs;
	D3D11_RASTERIZER_DESC rdesc;
	rdesc.FillMode = D3D11_FILL_SOLID;
	rdesc.CullMode = D3D11_CULL_NONE;
	rdesc.FrontCounterClockwise = TRUE;
	rdesc.DepthBias = 0;
	rdesc.DepthBiasClamp = 0;
	rdesc.SlopeScaledDepthBias = 0;
	rdesc.DepthClipEnable = FALSE;
	rdesc.ScissorEnable = FALSE;
	rdesc.MultisampleEnable = FALSE;
	rdesc.AntialiasedLineEnable = FALSE;
	d->device->CreateRasterizerState(&rdesc,&rs);
	d->context->RSSetState(rs);

	D3D11_VIEWPORT view;
	view.TopLeftX = 0;
	view.TopLeftY = 0;
	view.Width = (float)r.right;
	view.Height = (float)r.bottom;
	view.MinDepth = 0;
	view.MaxDepth = 1;
	d->context->RSSetViewports(1,&view);

	driver = d;
	return d;
}

HL_PRIM void HL_NAME(clear_color)( double r, double g, double b, double a ) {
	float color[4];
	color[0] = (float)r;
	color[1] = (float)g;
	color[2] = (float)b;
	color[3] = (float)a;
	driver->context->ClearRenderTargetView(driver->renderTarget,color);
}

HL_PRIM void HL_NAME(present)() {
	driver->swapchain->Present(0,0);
}

HL_PRIM int HL_NAME(get_screen_width)() {
	return GetSystemMetrics(SM_CXSCREEN);
}

HL_PRIM int HL_NAME(get_screen_height)() {
	return GetSystemMetrics(SM_CYSCREEN);
}

HL_PRIM const uchar *HL_NAME(get_device_name)() {
	IDXGIAdapter *adapter;
	DXGI_ADAPTER_DESC desc;
	if( GetDXGI()->EnumAdapters(0,&adapter) != S_OK || adapter->GetDesc(&desc) != S_OK )
		return USTR("Unknown");
	adapter->Release();
	return (uchar*)hl_copy_bytes((vbyte*)desc.Description,(ustrlen((uchar*)desc.Description)+1)*2);
}

HL_PRIM double HL_NAME(get_supported_version)() {
	if( driver == NULL ) return 0.;
	return (driver->feature >> 12) + ((driver->feature & 0xFFF) / 2560.);
}

HL_PRIM dx_buffer *HL_NAME(create_buffer)( int size, int usage, int bind, int access, int misc, int stride, vbyte *data ) {
	ID3D11Buffer *buffer;
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA res;
	desc.ByteWidth = size;
	desc.Usage = (D3D11_USAGE)usage;
	desc.BindFlags = bind;
	desc.CPUAccessFlags = access;
	desc.MiscFlags = misc;
	desc.StructureByteStride = stride;
	res.pSysMem = data;
	res.SysMemPitch = 0;
	res.SysMemSlicePitch = 0;
	if( driver->device->CreateBuffer(&desc,data?&res:NULL,&buffer) != S_OK )
		return NULL;
	return buffer;
}

HL_PRIM void HL_NAME(update_subresource)( dx_buffer *b, int index, dx_box_obj *box, vbyte *data, int srcRowPitch, int srcDstPitch ) {
	driver->context->UpdateSubresource(b, index, box ? &box->box : NULL, data, srcRowPitch, srcDstPitch);
}

HL_PRIM void *HL_NAME(buffer_map)( dx_buffer *b, int subRes, int type, bool waitGpu ) {
	D3D11_MAPPED_SUBRESOURCE map;
	if( driver->context->Map(b,subRes,(D3D11_MAP)type,waitGpu?0:D3D11_MAP_FLAG_DO_NOT_WAIT,&map) != S_OK ) 
		return NULL;
	return map.pData;
}

HL_PRIM void HL_NAME(buffer_unmap)( dx_buffer *b, int subRes ) {
	driver->context->Unmap(b, subRes);
}

HL_PRIM void HL_NAME(release_buffer)( dx_buffer *b ) {
	b->Release();
}

HL_PRIM vbyte *HL_NAME(compile_shader)( vbyte *data, int dataSize, char *source, char *entry, char *target, int flags, bool *error, int *size ) {
	ID3DBlob *code;
	ID3DBlob *errorMessage;
	if( D3DCompile(data,dataSize,source,NULL,NULL,entry,target,flags,0,&code,&errorMessage) != S_OK ) {
		*error = true;
		code = errorMessage;
	}
	*size = code->GetBufferSize();
	return hl_copy_bytes((vbyte*)code->GetBufferPointer(),*size);
}

HL_PRIM dx_vshader *HL_NAME(create_vertex_shader)( vbyte *code, int size ) {
	dx_vshader *shader;
	if( driver->device->CreateVertexShader(code, size, NULL, &shader) != S_OK )
		return NULL;
	return shader;
}

HL_PRIM dx_pshader *HL_NAME(create_pixel_shader)( vbyte *code, int size ) {
	dx_pshader *shader;
	if( driver->device->CreatePixelShader(code, size, NULL, &shader) != S_OK )
		return NULL;
	return shader;
}

HL_PRIM void HL_NAME(draw_indexed)( int count, int start, int baseVertex ) {
	driver->context->DrawIndexed(count,start,baseVertex);
}

HL_PRIM void HL_NAME(vs_set_shader)( dx_vshader *s ) {
	driver->context->VSSetShader(s,NULL,0);
}

HL_PRIM void HL_NAME(vs_set_constant_buffers)( int start, int count, varray *a ) {
	driver->context->VSSetConstantBuffers(start,count,hl_aptr(a,dx_buffer*));
}

HL_PRIM void HL_NAME(ps_set_shader)( dx_pshader *s ) {
	driver->context->PSSetShader(s,NULL,0);
}

HL_PRIM void HL_NAME(ps_set_constant_buffers)( int start, int count, varray *a ) {
	driver->context->PSSetConstantBuffers(start,count,hl_aptr(a,dx_buffer*));
}

HL_PRIM void HL_NAME(ia_set_index_buffer)( dx_buffer *b, bool is32, int offset ) {
	driver->context->IASetIndexBuffer(b,is32?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT,offset);
}

HL_PRIM void HL_NAME(ia_set_vertex_buffers)( int start, int count, varray *a, vbyte *strides, vbyte *offsets ) {
	driver->context->IASetVertexBuffers(start,count,hl_aptr(a,dx_buffer*),(UINT*)strides,(UINT*)offsets);
}

HL_PRIM void HL_NAME(ia_set_primitive_topology)( int topology ) {
	driver->context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);
}

HL_PRIM void HL_NAME(ia_set_input_layout)( dx_layout *l ) {
	driver->context->IASetInputLayout(l);
}

typedef struct {
	hl_type *t;
	D3D11_INPUT_ELEMENT_DESC desc;
} input_element;

HL_PRIM dx_layout *HL_NAME(create_input_layout)( varray *arr, vbyte *bytecode, int bytecodeLength ) {
	ID3D11InputLayout *input;
	D3D11_INPUT_ELEMENT_DESC desc[32];
	int i;
	if( arr->size > 32 ) return NULL;
	for(i=0;i<arr->size;i++)
		desc[i] = hl_aptr(arr,input_element*)[i]->desc;
	if( driver->device->CreateInputLayout(desc,arr->size,bytecode,bytecodeLength,&input) != S_OK )
		return NULL;
	return input;
}

#define _DRIVER _ABSTRACT(dx_driver)
#define _BUFFER _ABSTRACT(dx_buffer)
#define _SHADER _ABSTRACT(dx_shader)
#define _LAYOUT _ABSTRACT(dx_layout)

DEFINE_PRIM(_DRIVER, create, _ABSTRACT(dx_window) _I32);
DEFINE_PRIM(_VOID, clear_color, _F64 _F64 _F64 _F64);
DEFINE_PRIM(_VOID, present, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);
DEFINE_PRIM(_BYTES, get_device_name, _NO_ARG);
DEFINE_PRIM(_F64, get_supported_version, _NO_ARG);
DEFINE_PRIM(_BUFFER, create_buffer, _I32 _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_BYTES, buffer_map, _BUFFER _I32 _I32 _BOOL);
DEFINE_PRIM(_VOID, buffer_unmap, _BUFFER _I32);
DEFINE_PRIM(_VOID, release_buffer, _BUFFER);
DEFINE_PRIM(_BYTES, compile_shader, _BYTES _I32 _BYTES _BYTES _BYTES _I32 _REF(_BOOL) _REF(_I32));
DEFINE_PRIM(_SHADER, create_vertex_shader, _BYTES _I32);
DEFINE_PRIM(_SHADER, create_pixel_shader, _BYTES _I32);
DEFINE_PRIM(_VOID, draw_indexed, _I32 _I32 _I32);
DEFINE_PRIM(_VOID, vs_set_shader, _SHADER);
DEFINE_PRIM(_VOID, vs_set_constant_buffers, _I32 _I32 _ARR);
DEFINE_PRIM(_VOID, ps_set_shader, _SHADER);
DEFINE_PRIM(_VOID, ps_set_constant_buffers, _I32 _I32 _ARR);
DEFINE_PRIM(_VOID, update_subresource, _BUFFER _I32 _OBJ(_I32 _I32 _I32 _I32 _I32 _I32) _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, ia_set_index_buffer, _BUFFER _BOOL _I32);
DEFINE_PRIM(_VOID, ia_set_vertex_buffers, _I32 _I32 _ARR _BYTES _BYTES);
DEFINE_PRIM(_VOID, ia_set_primitive_topology, _I32);
DEFINE_PRIM(_VOID, ia_set_input_layout, _LAYOUT);
DEFINE_PRIM(_LAYOUT, create_input_layout, _ARR _BYTES _I32);

