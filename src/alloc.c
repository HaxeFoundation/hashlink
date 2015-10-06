/*
 * Copyright (C)2015 Haxe Foundation
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
#include "hl.h"
#ifdef HL_WIN
#	include <windows.h>
#else
#	include <sys/types.h>
#	include <sys/mman.h>
#endif

struct hl_alloc_block {
	int size;
	hl_alloc_block *next;
	unsigned char *p;
};

void hl_alloc_init( hl_alloc *a ) {
	a->cur = NULL;
}

void *hl_malloc( hl_alloc *a, int size ) {
	hl_alloc_block *b = a->cur;
	void *p;
	if( b == NULL || b->size <= size ) {
		int alloc = size < 4096-sizeof(hl_alloc_block) ? 4096-sizeof(hl_alloc_block) : size;
		b = (hl_alloc_block *)malloc(sizeof(hl_alloc_block) + alloc);
		if( b == NULL ) {
			printf("Out of memory");
			exit(99);
		}
		b->p = ((unsigned char*)b) + sizeof(hl_alloc_block);
		b->size = alloc;
		b->next = a->cur;
		a->cur = b;
	}
	p = b->p;
	b->p += size;
	b->size -= size;
	return p;
}

void *hl_zalloc( hl_alloc *a, int size ) {
	void *p = hl_malloc(a,size);
	if( p ) memset(p,0,size);
	return p;
}

void hl_free( hl_alloc *a ) {
	hl_alloc_block *b = a->cur;
	int_val prev = 0;
	int size = 0;
	while( b ) {
		hl_alloc_block *n = b->next;
		size = (int)(b->p + b->size - ((unsigned char*)b));
		prev = (int_val)b;
		free(b);
		b = n;
	}
	// check if our allocator was not part of the last free block
	if( (int_val)a < prev || (int_val)a > prev+size )
		a->cur = NULL;
}

void *hl_alloc_executable_memory( int size ) {
#ifdef HL_WIN
	return VirtualAlloc(NULL,size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
#else
	void *p;
	p = mmap(NULL,size,PROT_READ|PROT_WRITE|PROT_EXEC,(MAP_PRIVATE|MAP_ANON),-1,0);
	return p;
#endif
}

void hl_free_executable_memory( void *c, int size ) {
#ifdef HL_WIN
	VirtualFree(c,0,MEM_RELEASE);
#else
	munmap(c, size);
#endif
}

vdynamic *hl_alloc_dynamic( hl_type **t ) {
	vdynamic *d = (vdynamic*)malloc(sizeof(vdynamic));
	d->t = t;
	d->v.ptr = NULL;
	return d;
}

vobj *hl_alloc_obj( hl_module *m, hl_type *t ) {
	vobj *o;
	int size;
	hl_runtime_obj *rt = t->obj->rt;
	if( rt == NULL || rt->proto == NULL ) rt = hl_get_obj_proto(m,t);
	size = rt->size;
	if( size & (HL_WSIZE-1) ) size += HL_WSIZE - (size & (HL_WSIZE-1));
	o = (vobj*)malloc(size);
	memset(o,0,size);
	o->proto = rt->proto;
	return o;
}

vclosure *hl_alloc_closure_void( hl_module *m, int_val fid ) {
	vclosure *c = (vclosure*)malloc(sizeof(vclosure));
	c->t = &m->code->functions[m->functions_indexes[fid]].type;
	c->fun = m->functions_ptrs[fid];
	c->bits = 0;
	return c;
}

vclosure *hl_alloc_closure_i32( hl_module *m, int_val fid, int v32 ) {
	vclosure *c = (vclosure*)malloc(sizeof(vclosure));
	c->t = &m->code->functions[m->functions_indexes[fid]].type;
	c->fun = m->functions_ptrs[fid];
	c->bits = CL_HAS_V32;
	c->v32 = v32;
	return c;
}

vclosure *hl_alloc_closure_i64( hl_module *m, int_val fid, int_val v64 ) {
	vclosure *c = (vclosure*)malloc(sizeof(vclosure));
	c->t = &m->code->functions[m->functions_indexes[fid]].type;
	c->fun = m->functions_ptrs[fid];
	c->bits = CL_HAS_V64;
	c->v64 = v64;
	return c;
}
