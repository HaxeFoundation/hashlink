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
#ifndef JIT_H
#define JIT_H

#include <hlmodule.h>

typedef enum {
	LOAD_ADDR,
	LOAD_IMM,
	LOAD_ARG,
	STORE,
	LEA,
	TEST,
	CMP,
	JCOND,
	JUMP,
	JUMP_TABLE,
	BINOP,
	UNOP,
	CONV,
	CONV_UNSIGNED,
	RET,
	CALL_PTR,
	CALL_REG,
	CALL_FUN,
	MOV,
	ALLOC_STACK,
	FREE_STACK,
	NATIVE_REG,
	PREFETCH,
	DEBUG_BREAK,
} emit_op;

typedef enum {
	REG_RBP,
} native_reg;

typedef enum {
	M_NONE,
	M_UI8,
	M_UI16,
	M_I32,
	M_I64,
	M_F32,
	M_F64,
	M_PTR,
	M_VOID,
	M_NORET,
} emit_mode;

typedef struct {
	int index;
} ereg;

typedef struct {
	union {
		struct {
			unsigned char op;
			unsigned char mode;
			unsigned char nargs;
			unsigned char _unused;
		};
		int header;
	};
	int size_offs;
	union {
		struct {
			ereg a;
			ereg b;
		};
		uint64 value;
	};
} einstr;

#define VAL_NULL 0x80000000
#define IS_NULL(e) ((e).index == VAL_NULL)

typedef struct {
	int *data;
	int max;
	int cur;
} int_alloc;

typedef struct _ephi ephi;

struct _ephi {
	ereg value;
	int nvalues;
	ereg *values;
};

typedef struct _eblock {
	int id;
	int start_pos;
	int end_pos;
	int next_count;
	int pred_count;
	int phi_count;
	int *nexts;
	int *preds;
	ephi *phis;
} eblock;

typedef struct _emit_ctx emit_ctx;

typedef struct _jit_ctx ji_ctx;

struct _jit_ctx {
	hl_module *mod;
	hl_function *fun;
	hl_alloc falloc;
	emit_ctx *emit;
	// emit output
	int instr_count;
	int block_count;
	int value_count;
	einstr *instrs;
	eblock *blocks;
	int *values_writes;
	int *emit_pos_map;
};

jit_ctx *hl_jit_alloc();
void hl_jit_free( jit_ctx *ctx, h_bool can_reset );
void hl_jit_reset( jit_ctx *ctx, hl_module *m );
void hl_jit_init( jit_ctx *ctx, hl_module *m );
int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f );

void hl_jit_null_field_access();
void hl_jit_null_access();
void hl_jit_assert();

void int_alloc_reset( int_alloc *a );
void int_alloc_free( int_alloc *a );
int *int_alloc_get( int_alloc *a, int count );
void int_alloc_store( int_alloc *a, int v );

void hl_emit_dump( jit_ctx *ctx );
const char *hl_emit_regstr( ereg v );
ereg *hl_emit_get_args( emit_ctx *ctx, einstr *e );

#define val_str(v) hl_emit_regstr(v)


#ifdef HL_DEBUG
#	define JIT_DEBUG
#endif

#define jit_error(msg)	{ hl_jit_error(msg,__func__,__LINE__); hl_debug_break(); exit(-1); }
#define jit_assert()	jit_error("")

#ifdef JIT_DEBUG
#	define jit_debug(...)	printf(__VA_ARGS__)
#else
#	define jit_debug(...)
#endif

void hl_jit_error( const char *msg, const char *func, int line );

void *hl_jit_code( jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous );
void hl_jit_patch_method( void *old_fun, void **new_fun_table );

#endif
