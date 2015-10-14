#include <hl.h>

static void bblit( char *dst, int dpos, char *src, int spos, int len ) {
	memcpy(dst + dpos,src+spos,len);
}

DEFINE_PRIM_WITH_NAME(_ARR(_DYN),hl_alloc_array,_TYPE _I32,aalloc);
DEFINE_PRIM_WITH_NAME(_BYTES,hl_alloc_bytes,_I32,balloc);
DEFINE_PRIM(_VOID,bblit,_BYTES _I32 _BYTES _I32 _I32);
