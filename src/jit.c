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

void hl_emit_alloc( jit_ctx *jit );
void hl_emit_free( jit_ctx *jit );
void hl_emit_function( jit_ctx *jit );
void hl_emit_final( jit_ctx *jit );

void hl_regs_alloc( jit_ctx *jit );
void hl_regs_free( jit_ctx *jit );
void hl_regs_function( jit_ctx *jit );

void hl_codegen_alloc( jit_ctx *jit );
void hl_codegen_free( jit_ctx *jit );
void hl_codegen_function( jit_ctx *jit );
void hl_codegen_final( jit_ctx *jit );

jit_ctx *hl_jit_alloc() {
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	memset(ctx,0,sizeof(jit_ctx));
	hl_alloc_init(&ctx->falloc);
	hl_emit_alloc(ctx);
	hl_regs_alloc(ctx);
	hl_codegen_alloc(ctx);
	return ctx;
}

void hl_jit_init( jit_ctx *ctx, hl_module *m ) {
}

void hl_jit_free( jit_ctx *ctx, h_bool can_reset ) {
	hl_codegen_free(ctx);
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
	ctx->reg_instr_count = 0;
	ctx->code_size = 0;
	current_ctx = ctx;
	hl_emit_function(ctx);
	hl_regs_function(ctx);
	hl_codegen_function(ctx);
	int pos = ctx->out_pos;
	if( pos + ctx->code_size > ctx->out_max ) {
		int nsize = ctx->out_max ? ctx->out_max * 3 : 4096;
		while( pos + ctx->code_size > nsize ) nsize *= 3;
		unsigned char *nout = malloc(nsize);
		if( !nout ) return -1;
		memcpy(nout,ctx->output,pos);
		free(ctx->output);
		ctx->output = nout;
		ctx->out_max = nsize;
	}
	memcpy(ctx->output + pos, ctx->code_instrs, ctx->code_size);
	ctx->out_pos += ctx->code_size;
	current_ctx = NULL;
	return pos;
}

void *hl_jit_code( jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous ) {
	int size = ctx->out_pos;
	if( size & 4095 ) size += 4096 - (size&4095);
	unsigned char *code = (unsigned char*)hl_alloc_executable_memory(size);
	if( code == NULL ) return NULL;
	memcpy(code,ctx->output,size);
	*codesize = size;
	*debug = NULL;
	ctx->final_code = code;
	hl_emit_final(ctx);
	hl_codegen_final(ctx);
	return code;
}

void hl_jit_patch_method( void*fun, void**newt ) {
	jit_assert();
}
