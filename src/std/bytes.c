#include <hl.h>

static void bblit( char *dst, int dpos, char *src, int spos, int len ) {
	memcpy(dst + dpos,src+spos,len);
}

static void ablit( varray *dst, int dpos, varray *src, int spos, int len ) {
	memcpy( (void**)(dst + 1) + dpos, (void**)(src + 1) + spos, len * sizeof(void*)); 
}

DEFINE_PRIM(_VOID,ablit,_ARR(_DYN) _I32 _ARR(_DYN) _I32 _I32);
DEFINE_PRIM_WITH_NAME(_ARR(_DYN),hl_alloc_array,_TYPE _I32,aalloc);
DEFINE_PRIM_WITH_NAME(_BYTES,hl_alloc_bytes,_I32,balloc);
DEFINE_PRIM(_VOID,bblit,_BYTES _I32 _BYTES _I32 _I32);
