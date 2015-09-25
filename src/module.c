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

hl_module *hl_module_alloc( hl_code *c ) {
	int i;
	int gsize = 0;
	hl_module *m = (hl_module*)malloc(sizeof(hl_module));
	if( m == NULL )
		return NULL;	
	memset(m,0,sizeof(hl_module));
	m->code = c;
	m->globals_indexes = (int*)malloc(sizeof(int)*c->nglobals);
	if( m == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	for(i=0;i<c->nglobals;i++) {
		gsize += hl_pad_size(gsize, c->globals[i]);
		m->globals_indexes[i] = gsize;
		gsize += hl_type_size(c->globals[i]);
	}
	m->globals_data = (unsigned char*)malloc(gsize);
	if( m->globals_data == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->globals_data,0,gsize);
	m->functions_ptrs = (void**)malloc(sizeof(void*)*c->nfunctions);
	if( m->functions_ptrs == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->functions_ptrs,0,sizeof(void*)*c->nfunctions);
	return m;
}

static void null_function() {
	// TODO : throw an error instead
	printf("Null function ptr\n");
}

static void do_log( vdynamic *v ) {
	switch( v->t->kind ) {
	case HI32:
		printf("%di\n",v->v.i);
		break;
	case HF64:
		printf("%df\n",v->v.d);
		break;
	default:
		printf("%llXH\n",v->v.ptr);
		break;
	}
}

int hl_module_init( hl_module *m ) {
	int i;
	jit_ctx *ctx;
	// RESET globals
	for(i=0;i<m->code->nglobals;i++) {
		hl_type *t = m->code->globals[i];
		if( t->kind == HFUN ) *(fptr*)(m->globals_data + m->globals_indexes[i]) = null_function;
	}
	// INIT natives
	for(i=0;i<m->code->nnatives;i++) {
		hl_native *n = m->code->natives + i;
		*(void**)(m->globals_data + m->globals_indexes[n->global]) = do_log;
	}
	// JIT
	ctx = hl_jit_alloc();
	if( ctx == NULL )
		return 0;
	for(i=0;i<m->code->nfunctions;i++) {
		int f = hl_jit_function(ctx, m, m->code->functions+i);
		if( f < 0 ) {
			hl_jit_free(ctx);
			return 0;
		}
		m->functions_ptrs[i] = (void*)f;
	}
	m->jit_code = hl_jit_code(ctx, m);
	for(i=0;i<m->code->nfunctions;i++)
		m->functions_ptrs[i] = ((unsigned char*)m->jit_code) + ((int_val)m->functions_ptrs[i]);
	hl_jit_free(ctx);
	// INIT functions
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		*(void**)(m->globals_data + m->globals_indexes[f->global]) = m->functions_ptrs[i];
	}
	return 1;
}

void hl_module_free( hl_module *m ) {
	free(m->functions_ptrs);
	free(m->globals_indexes);
	free(m->globals_data);
	free(m);
}
