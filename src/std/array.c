#include <hl.h>

HL_PRIM varray *hl_aalloc( hl_type *at, int size ) {
	int esize = hl_type_size(at);
	varray *a = (varray*)hl_gc_alloc(sizeof(varray) + esize*size + sizeof(hl_type));
	a->t = &hlt_array;
	a->at = at;
	a->size = size;
	memset(a+1,0,size*esize);
	return a;
}

HL_PRIM void hl_ablit( varray *dst, int dpos, varray *src, int spos, int len ) {
	memcpy( (void**)(dst + 1) + dpos, (void**)(src + 1) + spos, len * sizeof(void*)); 
}

HL_PRIM hl_type *hl_atype( varray *a ) {
	return a->at;
}

DEFINE_PRIM(_ARR(_DYN),hl_aalloc,_TYPE _I32);
DEFINE_PRIM(_VOID,hl_ablit,_ARR(_DYN) _I32 _ARR(_DYN) _I32 _I32);
DEFINE_PRIM(_TYPE,hl_atype,_ARR);
