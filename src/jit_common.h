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
#ifndef JIT_COMMON_H
#define JIT_COMMON_H

#include <hlmodule.h>

/*
 * JIT definitions shared between x86 and AArch64 backends.
 */

// Maximum register count across all architectures
// x86/x64: 16 CPU + 16 FPU = 32
// AArch64: 31 CPU + 32 FPU = 63
#define MAX_REG_COUNT 64

// Jump/call patch list
typedef struct jlist jlist;
struct jlist {
	int pos;
	int target;
	jlist *next;
};

// Forward declaration
typedef struct vreg vreg;

// Physical register kinds
typedef enum {
	RCPU = 0,
	RFPU = 1,
	RSTACK = 2,
	RCONST = 3,
	RADDR = 4,
	RMEM = 5,
	RUNUSED = 6,
	RCPU_CALL = 1 | 8,
	RCPU_8BITS = 1 | 16
} preg_kind;

// Physical register
typedef struct {
	preg_kind kind;
	int id;
	int lock;
	vreg *holds;
} preg;

// Virtual register
struct vreg {
	int stackPos;
	int size;
	hl_type *t;
	preg *current;
	preg stack;
	int dirty;  // AArch64: register value differs from stack (needs spill at block boundary)
};

// JIT compilation context
// Note: Uses MAX_REG_COUNT for array sizing. Each backend defines its own
// RCPU_COUNT, RFPU_COUNT, and REG_COUNT based on the target architecture.
struct _jit_ctx {
	union {
		unsigned char *b;
		unsigned int *w;
		unsigned long long *w64;
		int *i;
		double *d;
	} buf;
	vreg *vregs;
	preg pregs[MAX_REG_COUNT];
	vreg *savedRegs[MAX_REG_COUNT];
	int savedLocks[MAX_REG_COUNT];
	int *opsPos;
	int maxRegs;
	int maxOps;
	int bufSize;
	int totalRegsSize;
	int functionPos;
	int allocOffset;
	int currentPos;
	int nativeArgsCount;
	unsigned char *startBuf;
	hl_module *m;
	hl_function *f;
	jlist *jumps;
	jlist *calls;
	jlist *switchs;
	hl_alloc falloc; // cleared per-function
	hl_alloc galloc;
	vclosure *closure_list;
	hl_debug_infos *debug;
	int c2hl;
	int hl2c;
	int longjump;
	void *static_functions[8];
	bool static_function_offset;
#if defined(__aarch64__) || defined(_M_ARM64)
	// Callee-saved register backpatching (AArch64 only)
	unsigned int callee_saved_used;       // Bitmap: bit i = 1 if CPU callee-saved reg i used
	unsigned int fpu_callee_saved_used;   // Bitmap: bit i = 1 if FPU callee-saved reg i used
	int stp_positions[5];                 // CPU STP positions (5 pairs: X19-X28)
	int ldp_positions[5];                 // CPU LDP positions
	int stp_fpu_positions[4];             // FPU STP positions (4 pairs: V8-V15)
	int ldp_fpu_positions[4];             // FPU LDP positions
#endif
};

// Portable macros
#define REG_AT(i)		(ctx->pregs + (i))
#define ID2(a,b)		((a) | ((b)<<8))
#define R(id)			(ctx->vregs + (id))
#define IS_FLOAT(r)		((r)->t->kind == HF64 || (r)->t->kind == HF32)
#define RLOCK(r)		if( (r)->lock < ctx->currentPos ) (r)->lock = ctx->currentPos
#define RUNLOCK(r)		if( (r)->lock == ctx->currentPos ) (r)->lock = 0

#define BUF_POS()		((int)(ctx->buf.b - ctx->startBuf))
#define RTYPE(r)		r->t->kind

#define MAX_OP_SIZE		256

// Global unused register
extern preg _unused;
extern preg *UNUSED;

// Error handling macros (to be defined by each backend)
#ifndef jit_exit
#define jit_exit() { hl_debug_break(); exit(-1); }
#endif

#ifndef jit_error
#define jit_error(msg)	_jit_error(ctx,msg,__LINE__)
#endif

#ifndef ASSERT
#define ASSERT(i)	{ printf("JIT ERROR %d (jit.c line %d)\n",i,(int)__LINE__); jit_exit(); }
#endif

// Shared utility functions (implemented in jit_shared.c)
void jit_buf(jit_ctx *ctx);

// AArch64: Emit a 32-bit instruction to the code buffer
// Ensures buffer space is available before writing
#if defined(__aarch64__) || defined(_M_ARM64)
#define EMIT32(ctx, val) do { \
	jit_buf(ctx); \
	*((ctx)->buf.w++) = (unsigned int)(val); \
} while(0)
#endif

#endif // JIT_COMMON_H
