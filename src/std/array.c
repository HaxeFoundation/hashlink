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

typedef struct {
	size_t size;
	hl_type* type;
} hl_carray_header;

HL_PRIM void *hl_alloc_carray( hl_type *at, int size ) {
	if( at->kind != HOBJ && at->kind != HSTRUCT )
		hl_error("Invalid array type");
	if( size < 0 )
		hl_error("Invalid array size");

	hl_runtime_obj *rt = at->obj->rt;
	if( rt == NULL || rt->methods == NULL ) rt = hl_get_obj_proto(at);
	void *ptr = hl_gc_alloc_gen(at, sizeof(hl_carray_header) + size * rt->size, (rt->hasPtr ? MEM_KIND_RAW : MEM_KIND_NOPTR) | MEM_ZERO);
	((hl_carray_header*)ptr)->size = size;
	((hl_carray_header*)ptr)->type = at;
	char *arr = (char*)ptr + sizeof(hl_carray_header);
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

HL_PRIM void hl_carray_blit( void *dst, hl_type *at, int dpos, void *src, int spos, int len ) {
	if( at->kind != HOBJ && at->kind != HSTRUCT )
		hl_error("Invalid array type");
	if( dpos < 0 || spos < 0 || len < 0 )
		hl_error("Invalid array pos or length");
	hl_runtime_obj *rt = at->obj->rt;
	if( rt == NULL || rt->methods == NULL ) rt = hl_get_obj_proto(at);
	int size = rt->size;
	memmove( (vbyte*)dst + dpos * size, (vbyte*)src + spos * size, len * size);
}

#define _CARRAY _ABSTRACT(hl_carray)
DEFINE_PRIM(_CARRAY,alloc_carray,_TYPE _I32);
DEFINE_PRIM(_VOID,carray_blit,_CARRAY _TYPE _I32 _CARRAY _I32 _I32);

// For backwards compatibility with HL 1.13

HL_PRIM int hl_carray_length( void *arr ) {
	if (!hl_is_gc_ptr((char*)arr - sizeof(hl_carray_header))) {
		hl_error("Cannot call hl_carray_length with external carray");
	}
	hl_carray_header* header = (hl_carray_header*)((char*)arr - sizeof(hl_carray_header));
	return header->size;
}

HL_PRIM vdynamic *hl_carray_get( void *arr, int pos ) {
	if (!hl_is_gc_ptr((char*)arr - sizeof(hl_carray_header))) {
		hl_error("Cannot call hl_carray_get with external carray");
	}
	hl_carray_header* header = (hl_carray_header*)((char*)arr - sizeof(hl_carray_header));
	if (pos < 0 || pos >= header->size) return NULL;
	hl_type* type = header->type;
	void* element = (void*)((char*)arr + pos * type->obj->rt->size);
	if (type->kind == HSTRUCT) {
		vdynamic* dyn = hl_alloc_dynamic(type);
		dyn->v.ptr = element;
		return dyn;
	}
	return (vdynamic*)element;
}

DEFINE_PRIM(_DYN,carray_get,_CARRAY _I32);
DEFINE_PRIM(_I32,carray_length,_CARRAY);
