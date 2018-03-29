#define HL_NAME(n) directx_##n
#include <hl.h>

#ifdef HL_WIN_DESKTOP
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#else
#include <xbo_directx.h>
#endif
#include <assert.h>
#include "directx.h"

#define DXERR(cmd)	{ HRESULT __ret = cmd; if( __ret == E_OUTOFMEMORY ) return NULL; if( __ret != S_OK ) ReportDxError(__ret,__LINE__); }

template <typename T> class dx_struct {
	hl_type *t;
public:
	T value;
};

typedef ID3D11Resource dx_resource;
typedef ID3D11DeviceChild dx_pointer;

static dx_driver *driver = NULL;
static IDXGIFactory *factory = NULL;

static IDXGIFactory *GetDXGI() {
	if( factory == NULL && CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) != S_OK )
		hl_error("Failed to init DXGI");
	return factory;
}

static void ReportDxError( HRESULT err, int line ) {
	if( err == DXGI_ERROR_DEVICE_REMOVED && driver ){
		err = driver->device->GetDeviceRemovedReason();
		hl_error_msg(USTR("DXGI_ERROR_DEVICE_REMOVED reason 0x%X line %d"),(DWORD)err,line);
	}else{
		hl_error_msg(USTR("DXERROR %X line %d"),(DWORD)err,line);
	}
}

HL_PRIM dx_driver *HL_NAME(create)( HWND window, int format, int flags, int restrictLevel ) {
	static D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};
	static int maxLevels = sizeof(levels) / sizeof(D3D_FEATURE_LEVEL);
	DWORD result;
	DXGI_SWAP_CHAIN_DESC desc;
	RECT r;
	dx_driver *d = (dx_driver*)hl_gc_alloc_noptr(sizeof(dx_driver));
	ZeroMemory(d,sizeof(dx_driver));
	GetClientRect(window, &r);
	ZeroMemory(&desc, sizeof(desc));
	desc.BufferDesc.Width = r.right;
	desc.BufferDesc.Height = r.bottom;
	desc.BufferDesc.Format = (DXGI_FORMAT)format;
	desc.SampleDesc.Count = 1; // NO AA for now
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
#ifdef HL_WIN_DESKTOP
	desc.BufferCount = 1;
	desc.Windowed = true;
#else
	desc.BufferCount = 2;
	desc.Windowed = false;
#endif
	desc.OutputWindow = window;
	if( restrictLevel >= maxLevels ) restrictLevel = maxLevels - 1;
	d->init_flags = flags;
	result = D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,flags,levels + restrictLevel,maxLevels - restrictLevel,D3D11_SDK_VERSION,&desc,&d->swapchain,&d->device,&d->feature,&d->context);

#ifdef HL_WIN_DESKTOP
	if( result == DXGI_ERROR_SDK_COMPONENT_MISSING && (flags & D3D11_CREATE_DEVICE_DEBUG) != 0 ) {
		// no debug driver available, retry
		flags &= ~D3D11_CREATE_DEVICE_DEBUG;
		d->init_flags = flags;
		result = E_INVALIDARG;
	}
#endif

	if( result == E_INVALIDARG ) // most likely no DX11.1 support, try again
		result = D3D11CreateDeviceAndSwapChain(NULL,D3D_DRIVER_TYPE_HARDWARE,NULL,flags,NULL,0,D3D11_SDK_VERSION, &desc, &d->swapchain, &d->device, &d->feature, &d->context);

	DXERR(result);

	driver = d;
	return d;
}

HL_PRIM dx_driver *HL_NAME(get_driver)(){
	return driver;
}

HL_PRIM dx_resource *HL_NAME(get_back_buffer)() {
	ID3D11Texture2D *backBuffer;
	DXERR( driver->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer) );
	return backBuffer;
}

HL_PRIM bool HL_NAME(resize)(int width, int height, int format) {
#ifdef HL_WIN_DESKTOP
	HRESULT res = driver->swapchain->ResizeBuffers(1, width, height, (DXGI_FORMAT)format, 0); assert(res == S_OK);
	return res == S_OK;
#else
	return TRUE; //Should not be called if the window is not resized (in the case here it will never happen)
#endif
}

HL_PRIM dx_pointer *HL_NAME(create_render_target_view)( dx_resource *r, dx_struct<D3D11_RENDER_TARGET_VIEW_DESC> *desc ) {
	ID3D11RenderTargetView *rt;
	DXERR( driver->device->CreateRenderTargetView(r, desc ? &desc->value : NULL, &rt) );
	return rt;
}

HL_PRIM void HL_NAME(om_set_render_targets)( int count, dx_pointer **arr, dx_pointer *depth ) {
	driver->context->OMSetRenderTargets(count,(ID3D11RenderTargetView**)arr,(ID3D11DepthStencilView*)depth);
}

HL_PRIM dx_pointer *HL_NAME(create_rasterizer_state)( dx_struct<D3D11_RASTERIZER_DESC> *desc ) {
	ID3D11RasterizerState *rs;
	DXERR( driver->device->CreateRasterizerState(&desc->value,&rs) );
	return rs;
}

HL_PRIM void HL_NAME(rs_set_state)( dx_pointer *rs ) {
	driver->context->RSSetState((ID3D11RasterizerState*)rs);
}

HL_PRIM void HL_NAME(rs_set_viewports)( int count, vbyte *data ) {
	driver->context->RSSetViewports(count,(D3D11_VIEWPORT*)data);
}

HL_PRIM void HL_NAME(rs_set_scissor_rects)( int count, vbyte *data ) {
	driver->context->RSSetScissorRects(count,(D3D11_RECT*)data);
}

HL_PRIM void HL_NAME(clear_color)( dx_pointer *rt, double r, double g, double b, double a ) {
	float color[4];
	color[0] = (float)r;
	color[1] = (float)g;
	color[2] = (float)b;
	color[3] = (float)a;
	driver->context->ClearRenderTargetView((ID3D11RenderTargetView*)rt,color);
}

HL_PRIM void HL_NAME(present)( int interval, int flags ) {
	HRESULT ret = driver->swapchain->Present(interval, flags);
	if (ret != S_OK && ret != DXGI_STATUS_OCCLUDED) ReportDxError(ret, __LINE__);
}

HL_PRIM const uchar *HL_NAME(get_device_name)() {
	IDXGIAdapter *adapter;
	DXGI_ADAPTER_DESC desc;
	if( GetDXGI()->EnumAdapters(0,&adapter) != S_OK || adapter->GetDesc(&desc) != S_OK )
		return USTR("Unknown");
	adapter->Release();
	return (uchar*)hl_copy_bytes((vbyte*)desc.Description,(int)(ustrlen((uchar*)desc.Description)+1)*2);
}

HL_PRIM double HL_NAME(get_supported_version)() {
	if( driver == NULL ) return 0.;
	return (driver->feature >> 12) + ((driver->feature & 0xFFF) / 2560.);
}

HL_PRIM dx_resource *HL_NAME(create_buffer)( int size, int usage, int bind, int access, int misc, int stride, vbyte *data ) {
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
	if( size == 0 )
		return NULL;
	DXERR( driver->device->CreateBuffer(&desc,data?&res:NULL,&buffer) );
	return buffer;
}

HL_PRIM dx_resource *HL_NAME(create_texture_2d)( dx_struct<D3D11_TEXTURE2D_DESC> *desc, vbyte *data ) {
	ID3D11Texture2D *tex;
	D3D11_SUBRESOURCE_DATA res;
	res.pSysMem = data;
	res.SysMemPitch = 0;
	res.SysMemSlicePitch = 0;
	DXERR( driver->device->CreateTexture2D(&desc->value,data?&res:NULL,&tex) );
	return tex;
}

HL_PRIM void HL_NAME(update_subresource)( dx_resource *r, int index, dx_struct<D3D11_BOX> *box, vbyte *data, int srcRowPitch, int srcDstPitch ) {
	driver->context->UpdateSubresource(r, index, box ? &box->value : NULL, data, srcRowPitch, srcDstPitch);
}

HL_PRIM void HL_NAME(copy_subresource_region)( dx_resource *r, int index, int dx, int dy, int dz, dx_resource *src, int sindex, dx_struct<D3D11_BOX> *box ) {
	driver->context->CopySubresourceRegion(r, index, dx, dy, dz, src, sindex, box ? &box->value : NULL);
}

HL_PRIM void *HL_NAME(map)( dx_resource *r, int subRes, int type, bool waitGpu, int *pitch ) {
	D3D11_MAPPED_SUBRESOURCE map;
	DXERR( driver->context->Map(r,subRes,(D3D11_MAP)type,waitGpu?0:D3D11_MAP_FLAG_DO_NOT_WAIT,&map) );
	if( pitch )
		*pitch = map.RowPitch;
	return map.pData;
}

HL_PRIM void HL_NAME(unmap)( dx_resource *r, int subRes ) {
	driver->context->Unmap(r, subRes);
}

HL_PRIM void HL_NAME(copy_resource)( dx_resource *to, dx_resource *from ) {
	driver->context->CopyResource(to,from);
}

HL_PRIM vbyte *HL_NAME(compile_shader)( vbyte *data, int dataSize, char *source, char *entry, char *target, int flags, bool *error, int *size ) {
	ID3DBlob *code;
	ID3DBlob *errorMessage;
	vbyte *ret;
	if( D3DCompile(data,dataSize,source,NULL,NULL,entry,target,flags,0,&code,&errorMessage) != S_OK ) {
		*error = true;
		code = errorMessage;
	}
	*size = (int)code->GetBufferSize();
	ret = hl_copy_bytes((vbyte*)code->GetBufferPointer(),*size);
	code->Release();
	return ret;
}

HL_PRIM vbyte *HL_NAME(disassemble_shader)( vbyte *data, int dataSize, int flags, vbyte *comments, int *size ) {
	ID3DBlob *out;
	vbyte *ret;
	if( D3DDisassemble(data,dataSize,flags,(char*)comments,&out) != S_OK )
		return NULL;
	*size = (int)out->GetBufferSize();
	ret = hl_copy_bytes((vbyte*)out->GetBufferPointer(),*size);
	out->Release();
	return ret;
}

HL_PRIM dx_pointer *HL_NAME(create_vertex_shader)( vbyte *code, int size ) {
	ID3D11VertexShader *shader;
	DXERR( driver->device->CreateVertexShader(code, size, NULL, &shader) );
	return shader;
}

HL_PRIM dx_pointer *HL_NAME(create_pixel_shader)( vbyte *code, int size ) {
	ID3D11PixelShader *shader;
	DXERR( driver->device->CreatePixelShader(code, size, NULL, &shader) );
	return shader;
}

HL_PRIM void HL_NAME(draw_indexed)( int count, int start, int baseVertex ) {
	driver->context->DrawIndexed(count,start,baseVertex);
}

HL_PRIM void HL_NAME(vs_set_shader)( dx_pointer *s ) {
	driver->context->VSSetShader((ID3D11VertexShader*)s,NULL,0);
}

HL_PRIM void HL_NAME(vs_set_constant_buffers)( int start, int count, dx_resource **a ) {
	driver->context->VSSetConstantBuffers(start,count,(ID3D11Buffer**)a);
}

HL_PRIM void HL_NAME(ps_set_shader)( dx_pointer *s ) {
	driver->context->PSSetShader((ID3D11PixelShader*)s,NULL,0);
}

HL_PRIM void HL_NAME(ps_set_constant_buffers)( int start, int count, dx_resource **a ) {
	driver->context->PSSetConstantBuffers(start,count,(ID3D11Buffer**)a);
}

HL_PRIM void HL_NAME(ia_set_index_buffer)( dx_resource *r, bool is32, int offset ) {
	driver->context->IASetIndexBuffer((ID3D11Buffer*)r,is32?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_R16_UINT,offset);
}

HL_PRIM void HL_NAME(ia_set_vertex_buffers)( int start, int count, dx_resource **a, vbyte *strides, vbyte *offsets ) {
	driver->context->IASetVertexBuffers(start,count,(ID3D11Buffer**)a,(UINT*)strides,(UINT*)offsets);
}

HL_PRIM void HL_NAME(ia_set_primitive_topology)( int topology ) {
	driver->context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topology);
}

HL_PRIM void HL_NAME(ia_set_input_layout)( dx_pointer *l ) {
	driver->context->IASetInputLayout((ID3D11InputLayout*)l);
}

HL_PRIM void HL_NAME(release_pointer)( dx_pointer *p ) {
	p->Release();
}

HL_PRIM void HL_NAME(release_resource)( dx_resource *r ) {
	r->Release();
}

HL_PRIM dx_pointer *HL_NAME(create_input_layout)( varray *arr, vbyte *bytecode, int bytecodeLength ) {
	ID3D11InputLayout *input;
	D3D11_INPUT_ELEMENT_DESC desc[32];
	int i;
	if( arr->size > 32 ) return NULL;
	for(i=0;i<arr->size;i++)
		desc[i] = hl_aptr(arr,dx_struct<D3D11_INPUT_ELEMENT_DESC>*)[i]->value;
	DXERR( driver->device->CreateInputLayout(desc,arr->size,bytecode,bytecodeLength,&input) );
	return input;
}

HL_PRIM dx_pointer *HL_NAME(create_depth_stencil_view)( dx_resource *tex, int format ) {
	ID3D11DepthStencilView *view;
	D3D11_DEPTH_STENCIL_VIEW_DESC  desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Format = (DXGI_FORMAT)format;
	desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	DXERR( driver->device->CreateDepthStencilView(tex,&desc,&view) );
	return view;
}

HL_PRIM dx_pointer *HL_NAME(create_depth_stencil_state)( dx_struct<D3D11_DEPTH_STENCIL_DESC> *desc ) {
	ID3D11DepthStencilState *state;
	DXERR( driver->device->CreateDepthStencilState(&desc->value,&state) );
	return state;
}

HL_PRIM void HL_NAME(om_set_depth_stencil_state)( dx_pointer *s, int ref )  {
	driver->context->OMSetDepthStencilState((ID3D11DepthStencilState*)s,ref);
}

HL_PRIM void HL_NAME(clear_depth_stencil_view)( dx_pointer *view, vdynamic *depth, vdynamic *stencil ) {
	driver->context->ClearDepthStencilView((ID3D11DepthStencilView*)view, (depth?D3D11_CLEAR_DEPTH:0) | (stencil?D3D11_CLEAR_STENCIL:0), depth ? (FLOAT)depth->v.d : 0.f, stencil ? (UINT8)stencil->v.i : 0);
}

HL_PRIM void HL_NAME(om_set_blend_state)( dx_pointer *state, vbyte *factors, int sampleMask ) {
	driver->context->OMSetBlendState((ID3D11BlendState*)state,(FLOAT*)factors,sampleMask);
}

HL_PRIM dx_pointer *HL_NAME(create_blend_state)( bool alphaToCoverage, bool independentBlend, varray *arr, int count ) {
	ID3D11BlendState *s;
	D3D11_BLEND_DESC desc;
	int i;
	ZeroMemory(&desc,sizeof(desc));
	desc.AlphaToCoverageEnable = alphaToCoverage;
	desc.IndependentBlendEnable = independentBlend;
	for(i=0;i<count;i++)
		desc.RenderTarget[i] = hl_aptr(arr,dx_struct<D3D11_RENDER_TARGET_BLEND_DESC>*)[i]->value;
	DXERR( driver->device->CreateBlendState(&desc,&s) );
	return s;
}

HL_PRIM dx_pointer *HL_NAME(create_sampler_state)( dx_struct<D3D11_SAMPLER_DESC> *desc ) {
	ID3D11SamplerState *s;
	DXERR( driver->device->CreateSamplerState(&desc->value,&s) );
	return s;
}

HL_PRIM dx_pointer *HL_NAME(create_shader_resource_view)( dx_resource *res, dx_struct<D3D11_SHADER_RESOURCE_VIEW_DESC> *desc ) {
	ID3D11ShaderResourceView *view;
	DXERR( driver->device->CreateShaderResourceView(res,&desc->value,&view) );
	return view;
}

HL_PRIM void HL_NAME(ps_set_samplers)( int start, int count, dx_pointer **arr ) {
	driver->context->PSSetSamplers(start,count,(ID3D11SamplerState**)arr);
}

HL_PRIM void HL_NAME(vs_set_samplers)( int start, int count, dx_pointer **arr ) {
	driver->context->VSSetSamplers(start,count,(ID3D11SamplerState**)arr);
}

HL_PRIM void HL_NAME(ps_set_shader_resources)( int start, int count, dx_pointer **arr ) {
	driver->context->PSSetShaderResources(start, count, (ID3D11ShaderResourceView**)arr);
}

HL_PRIM void HL_NAME(vs_set_shader_resources)( int start, int count, dx_pointer **arr ) {
	driver->context->VSSetShaderResources(start, count, (ID3D11ShaderResourceView**)arr);
}

HL_PRIM void HL_NAME(generate_mips)( dx_pointer *t ) {
	driver->context->GenerateMips((ID3D11ShaderResourceView*)t);
}

HL_PRIM bool HL_NAME(set_fullscreen_state)( bool fs ) {
	return driver->swapchain->SetFullscreenState(fs,NULL) == S_OK;
}

HL_PRIM bool HL_NAME(get_fullscreen_state)() {
	BOOL ret;
	if( driver->swapchain->GetFullscreenState(&ret,NULL) != S_OK )
		return false;
	return ret != 0;
}

HL_PRIM void HL_NAME(debug_print)( vbyte *b ) {
	OutputDebugString((LPCWSTR)b);
}

#define _DRIVER _ABSTRACT(dx_driver)
#define _POINTER _ABSTRACT(dx_pointer)
#define _RESOURCE _ABSTRACT(dx_resource)

DEFINE_PRIM(_DRIVER, create, _ABSTRACT(dx_window) _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, resize, _I32 _I32 _I32);
DEFINE_PRIM(_RESOURCE, get_back_buffer, _NO_ARG);
DEFINE_PRIM(_POINTER, create_render_target_view, _RESOURCE _DYN);
DEFINE_PRIM(_VOID, om_set_render_targets, _I32 _REF(_POINTER) _POINTER);
DEFINE_PRIM(_POINTER, create_rasterizer_state, _DYN);
DEFINE_PRIM(_VOID, rs_set_state, _POINTER);
DEFINE_PRIM(_VOID, rs_set_viewports, _I32 _BYTES);
DEFINE_PRIM(_VOID, rs_set_scissor_rects, _I32 _BYTES);
DEFINE_PRIM(_VOID, clear_color, _POINTER _F64 _F64 _F64 _F64);
DEFINE_PRIM(_VOID, present, _I32 _I32);
DEFINE_PRIM(_BYTES, get_device_name, _NO_ARG);
DEFINE_PRIM(_F64, get_supported_version, _NO_ARG);
DEFINE_PRIM(_RESOURCE, create_buffer, _I32 _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_BYTES, map, _RESOURCE _I32 _I32 _BOOL _REF(_I32));
DEFINE_PRIM(_VOID, unmap, _RESOURCE _I32);
DEFINE_PRIM(_VOID, copy_resource, _RESOURCE _RESOURCE);
DEFINE_PRIM(_BYTES, compile_shader, _BYTES _I32 _BYTES _BYTES _BYTES _I32 _REF(_BOOL) _REF(_I32));
DEFINE_PRIM(_BYTES, disassemble_shader, _BYTES _I32 _I32 _BYTES _REF(_I32));
DEFINE_PRIM(_POINTER, create_vertex_shader, _BYTES _I32);
DEFINE_PRIM(_POINTER, create_pixel_shader, _BYTES _I32);
DEFINE_PRIM(_VOID, draw_indexed, _I32 _I32 _I32);
DEFINE_PRIM(_VOID, vs_set_shader, _POINTER);
DEFINE_PRIM(_VOID, vs_set_constant_buffers, _I32 _I32 _REF(_RESOURCE));
DEFINE_PRIM(_VOID, ps_set_shader, _POINTER);
DEFINE_PRIM(_VOID, ps_set_constant_buffers, _I32 _I32 _REF(_RESOURCE));
DEFINE_PRIM(_VOID, update_subresource, _RESOURCE _I32 _DYN _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, ia_set_index_buffer, _RESOURCE _BOOL _I32);
DEFINE_PRIM(_VOID, ia_set_vertex_buffers, _I32 _I32 _REF(_RESOURCE) _BYTES _BYTES);
DEFINE_PRIM(_VOID, ia_set_primitive_topology, _I32);
DEFINE_PRIM(_VOID, ia_set_input_layout, _POINTER);
DEFINE_PRIM(_POINTER, create_input_layout, _ARR _BYTES _I32);
DEFINE_PRIM(_RESOURCE, create_texture_2d, _DYN _BYTES);
DEFINE_PRIM(_POINTER, create_depth_stencil_view, _RESOURCE _I32);
DEFINE_PRIM(_POINTER, create_depth_stencil_state, _DYN);
DEFINE_PRIM(_VOID, om_set_depth_stencil_state, _POINTER);
DEFINE_PRIM(_VOID, clear_depth_stencil_view, _POINTER _NULL(_F64) _NULL(_I32));
DEFINE_PRIM(_POINTER, create_blend_state, _BOOL _BOOL _ARR _I32);
DEFINE_PRIM(_VOID, om_set_blend_state, _POINTER _BYTES _I32);
DEFINE_PRIM(_VOID, release_pointer, _POINTER);
DEFINE_PRIM(_VOID, release_resource, _RESOURCE);
DEFINE_PRIM(_POINTER, create_sampler_state, _DYN);
DEFINE_PRIM(_POINTER, create_shader_resource_view, _RESOURCE _DYN);
DEFINE_PRIM(_VOID, ps_set_samplers, _I32 _I32 _REF(_POINTER));
DEFINE_PRIM(_VOID, vs_set_samplers, _I32 _I32 _REF(_POINTER));
DEFINE_PRIM(_VOID, ps_set_shader_resources, _I32 _I32 _REF(_POINTER));
DEFINE_PRIM(_VOID, vs_set_shader_resources, _I32 _I32 _REF(_POINTER));
DEFINE_PRIM(_VOID, generate_mips, _POINTER);
DEFINE_PRIM(_BOOL, set_fullscreen_state, _BOOL);
DEFINE_PRIM(_BOOL, get_fullscreen_state, _NO_ARG);
DEFINE_PRIM(_VOID, debug_print, _BYTES);
DEFINE_PRIM(_VOID, copy_subresource_region, _RESOURCE _I32 _I32 _I32 _I32 _RESOURCE _I32 _DYN);
