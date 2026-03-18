#define HL_NAME(n) dx12debug_##n
#include <hl.h>
#undef _GUID

#ifdef HL_WIN_DESKTOP
#include <d3d12.h>
#define USE_PIX
#include <pix3.h>
#endif

#define _RES _ABSTRACT(dx_resource)

HL_PRIM void HL_NAME(command_list_pix_begin_event)(ID3D12GraphicsCommandList* l, UINT64 color, wchar_t const* formatString) {
	PIXBeginEvent(l, color, formatString);
}

HL_PRIM void HL_NAME(command_list_pix_end_event)(ID3D12GraphicsCommandList* l) {
	PIXEndEvent(l);
}

DEFINE_PRIM(_VOID, command_list_pix_begin_event, _RES _I64 _BYTES);
DEFINE_PRIM(_VOID, command_list_pix_end_event, _RES);
