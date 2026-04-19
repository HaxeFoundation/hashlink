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
	LOAD_CONST,
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
	PUSH_CONST,
	PUSH,
	POP,
	ALLOC_STACK,
	PREFETCH,
	DEBUG_BREAK,
	BLOCK,
	ENTER,
	STACK_OFFS,
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
	M_PTR,
	M_F64,
	M_F32,
	M_VOID,
	M_NORET,
} emit_mode;

typedef int ereg;

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

#define FL_NATMASK	0x70000000
#define FL_NATREG	0x40000000
#define FL_MEMPTR	0x20000000
#define FL_STACK	0x10000000
#define FL_STACKREG (FL_NATREG | FL_MEMPTR | FL_STACK)
#define FL_STACKOFFS (FL_NATREG | FL_STACK)
#define IS_NULL(e) ((e) == 0)
#define IS_NATREG(e) (((e) & (0x80000000 | FL_NATREG)) == FL_NATREG)
#define IS_PURE(e)		((e) != UNUSED && ((e)&(FL_MEMPTR | FL_STACK)) == 0)
#define MK_STACK_REG(v)	(((v)&0xFFFFFFF) | FL_STACKREG)
#define MK_STACK_OFFS(v)(((v)&0xFFFFFFF) | FL_STACKOFFS)
#define GET_STACK_OFFS(v) ((int)(((v) & 0x8000000) ? ((v) | 0xF0000000) : ((v)&0xFFFFFFF)))
#define IS_CALL(op)	((op) == CALL_PTR || (op) == CALL_REG || (op) == CALL_FUN)
#define UNUSED		((ereg)0)

typedef struct {
	int *data;
	int max;
	int cur;
} int_alloc;

typedef struct _ephi ephi;

struct _ephi {
	ereg value;
	int nvalues;
	emit_mode mode;
	ereg *values;
};

typedef struct _eblock {
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
typedef struct _regs_ctx regs_ctx;
typedef struct _code_ctx code_ctx;
typedef struct _jit_ctx jit_ctx;

typedef struct {
	int nscratchs;
	int npersists;
	int nargs;
	ereg ret;
	ereg *scratch;
	ereg *persist;
	ereg *arg;
} reg_config;

typedef struct {
	reg_config regs;
	reg_config floats;
	ereg stack_reg;
	ereg stack_pos;
	int stack_align;
	ereg req_bit_shifts;
	ereg req_div_a;
	ereg req_div_b;
} regs_config;

struct _jit_ctx {
	hl_module *mod;
	hl_function *fun;
	hl_alloc falloc;
	hl_alloc galloc;
	emit_ctx *emit;
	regs_ctx *regs;
	code_ctx *code;
	// emit output
	int instr_count;
	int block_count;
	int value_count;
	int phi_count;
	einstr *instrs;
	eblock *blocks;
	int *values_writes;
	int *emit_pos_map;
	// regs output
	int reg_instr_count;
	einstr *reg_instrs;
	ereg *reg_writes;
	int *reg_pos_map;
	// codegen output
	int code_size;
	unsigned char *code_instrs;
	int *code_pos_map;
	// accum output
	int out_pos;
	int out_max;
	unsigned char *output;
	unsigned char *final_code;
};

jit_ctx *hl_jit_alloc();
void hl_jit_free( jit_ctx *ctx, h_bool can_reset );
void hl_jit_reset( jit_ctx *ctx, hl_module *m );
void hl_jit_init( jit_ctx *ctx, hl_module *m );
int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f );

void hl_jit_null_field_access();
void hl_jit_null_access();
void hl_jit_assert();

// emit & dump
void hl_emit_dump( jit_ctx *ctx );
const char *hl_emit_regstr( ereg v, emit_mode m );
ereg *hl_emit_get_args( emit_ctx *ctx, einstr *e );
ereg **hl_emit_get_regs( einstr *e, int *count );
void hl_emit_reg_iter( jit_ctx *jit, einstr *e, void *ctx, void (*iter_reg)( void *, ereg * ) );
extern int hl_emit_mode_sizes[];
extern bool hl_jit_dump_bin;
#define val_str(v,m) hl_emit_regstr(v,m)

#ifdef HL_DEBUG
#	define JIT_DEBUG
#endif

#define jit_error(msg)	{ hl_jit_error(msg,__func__,__LINE__); hl_debug_break(); exit(-1); }
#define jit_assert()	jit_error("")

#if defined(JIT_DEBUG)
#	define jit_debug(...)	printf(__VA_ARGS__)
#else
#	define jit_debug(...)
#endif

#define DEF_ALLOC &ctx->jit->falloc

#define jit_pad_size(size,k)	((k == 0) ? 0 : ((-(size)) & (k - 1)))

static void __ignore( void *value ) {}

void hl_jit_error( const char *msg, const char *func, int line );

void *hl_jit_code( jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous );
void hl_jit_patch_method( void *old_fun, void **new_fun_table );

#endif
