#define HL_NAME(n) fmt_##n
#include <turbojpeg.h>
#include <hl.h>

HL_PRIM bool HL_NAME(jpg_decode)( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags ) {
	tjhandle h = tjInitDecompress();
	int result;
	result = tjDecompress2(h,data,dataLen,out,width,stride,height,format,(flags & 1 ? TJFLAG_BOTTOMUP : 0));
	tjDestroy(h);
	return result == 0;
}

DEFINE_PRIM(_BOOL, jpg_decode, _BYTES _I32 _BYTES _I32 _I32 _I32 _I32 _I32);
