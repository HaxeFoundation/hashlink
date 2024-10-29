/*
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <hl.h>

HL_PRIM varray *hl_alloc_array( hl_type *at, int size ) {
	if( size == 0 && at->kind == HDYN ) {
		static varray empty_array = { &hlt_array, &hlt_dyn };
		return &empty_array;
	}
	int esize = hl_type_size(at);
	varray *a;
	if( size < 0 ) hl_error("Invalid array size");
	a = (varray*)hl_gc_alloc_gen(&hlt_array, sizeof(varray) + esize*size, (hl_is_ptr(at) ? MEM_KIND_DYNAMIC : MEM_KIND_NOPTR) | MEM_ZERO);
	a->t = &hlt_array;
	a->at = at;
	a->size = size;
	return a;
}

HL_PRIM void hl_array_blit( varray *dst, int dpos, varray *src, int spos, int len ) {
	int size = hl_type_size(dst->at);
	memmove( hl_aptr(dst,vbyte) + dpos * size, hl_aptr(src,vbyte) + spos * size, len * size);
}

HL_PRIM hl_type *hl_array_type( varray *a ) {
	return a->at;
}

HL_PRIM vbyte *hl_array_bytes( varray *a ) {
	return hl_aptr(a,vbyte);
}

DEFINE_PRIM(_ARR,alloc_array,_TYPE _I32);
DEFINE_PRIM(_VOID,array_blit,_ARR _I32 _ARR _I32 _I32);
DEFINE_PRIM(_TYPE,array_type,_ARR);
DEFINE_PRIM(_BYTES,array_bytes,_ARR);

HL_PRIM void *hl_alloc_carray( hl_type *at, int size ) {
	if( at->kind != HOBJ && at->kind != HSTRUCT )
		hl_error("Invalid array type");
	if( size < 0 )
		hl_error("Invalid array size");

	hl_runtime_obj *rt = at->obj->rt;
	if( rt == NULL || rt->methods == NULL ) rt = hl_get_obj_proto(at);
	char *arr = hl_gc_alloc_gen(at, size * rt->size, (rt->hasPtr ? MEM_KIND_RAW : MEM_KIND_NOPTR) | MEM_ZERO);
	if( at->kind == HOBJ || rt->nbindings ) {
		int i,k;
		for(k=0;k<size;k++) {
			char *o = arr + rt->size * k;
			if( at->kind == HOBJ )
				((vobj*)o)->t = at;
			for(i=0;i<rt->nbindings;i++) {
				hl_runtime_binding *b = rt->bindings + i;
				*(void**)(o + rt->fields_indexes[b->fid]) = b->closure ? hl_alloc_closure_ptr(b->closure,b->ptr,o) : b->ptr;
			}
		}
	}
	return arr;
}

#define _CARRAY _ABSTRACT(hl_carray)
DEFINE_PRIM(_CARRAY,alloc_carray,_TYPE _I32);
