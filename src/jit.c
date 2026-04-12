/*
 * Copyright (C)2015-2016 Haxe Foundation
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
#include <jit.h>

static jit_ctx *current_ctx = NULL;

void hl_jit_error( const char *msg, const char *func, int line ) {
	printf("*** JIT ERROR %s:%d (%s)****\n", func, line, msg);
	if( current_ctx  ) {
		jit_ctx *ctx = current_ctx;
		current_ctx = NULL;
		hl_emit_dump(ctx);
	}
	fflush(stdout);
}

void hl_jit_null_field_access() { jit_assert(); }
void hl_jit_null_access() { jit_assert(); }
void hl_jit_assert() { jit_assert(); }

void int_alloc_reset( int_alloc *a ) {
	a->cur = 0;
}

void int_alloc_free( int_alloc *a ) {
	free(a->data);
	a->cur = 0;
	a->max = 0;
	a->data = NULL;
}

int *int_alloc_get( int_alloc *a, int count ) {
	while( a->cur + count > a->max ) {
		int next_size = a->max ? a->max << 1 : 128; 
		int *new_data = (int*)malloc(sizeof(int) * next_size);
		if( new_data == NULL ) jit_error("Out of memory");
		memcpy(new_data, a->data, sizeof(int) * a->cur);
		free(a->data);
		a->data = new_data;
		a->max = next_size;
	}
	int *ptr = a->data + a->cur;
	a->cur += count;
	return ptr;
}

void int_alloc_store( int_alloc *a, int v ) {
	*int_alloc_get(a,1) = v;
}

void hl_emit_alloc( jit_ctx *jit );
void hl_emit_free( jit_ctx *jit );
void hl_emit_function( jit_ctx *jit );


void hl_regs_alloc( jit_ctx *jit );
void hl_regs_free( jit_ctx *jit );
void hl_regs_function( jit_ctx *jit );


jit_ctx *hl_jit_alloc() {
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	memset(ctx,0,sizeof(jit_ctx));
	hl_alloc_init(&ctx->falloc);
	hl_emit_alloc(ctx);
	hl_regs_alloc(ctx);
	return ctx;
}

void hl_jit_init( jit_ctx *ctx, hl_module *m ) {
}

void hl_jit_free( jit_ctx *ctx, h_bool can_reset ) {
	hl_regs_free(ctx);
	hl_emit_free(ctx);
	hl_free(&ctx->falloc);
	free(ctx);
}

void hl_jit_reset( jit_ctx *ctx, hl_module *m ) {
}

int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f ) {
	hl_free(&ctx->falloc);
	ctx->mod = m;
	ctx->fun = f;
	current_ctx = ctx;
	hl_emit_function(ctx);
	hl_regs_function(ctx);
	current_ctx = NULL;
	return 0;
}

void *hl_jit_code( jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous ) {
	printf("TODO:emit_code\n");
	exit(0);
	return NULL;
}

void hl_jit_patch_method( void*fun, void**newt ) {
	jit_assert();
}
