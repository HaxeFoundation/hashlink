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
#include <hlmodule.h>
#include <jit.h>
#include "data_struct.h"

#ifdef HL_DEBUG
#	define GEN_DEBUG
#endif

#define S_TYPE			byte_arr
#define S_NAME(name)	byte_##name
#define S_VALUE			unsigned char
#include "data_struct.c"
#define byte_reserve(set,count)	byte_reserve_impl(DEF_ALLOC,&set,count)
#define VAL_CONST		0x80000000
#define VAL_MEM(reg)	(FL_MEMPTR | (reg))

#define IS_FLOAT(mode)	((mode) == M_F64 || (mode) == M_F32)

typedef enum {
	RAX = 0,
	RCX = 1,
	RDX = 2,
	RBX = 3,
	RSP = 4,
	RBP = 5,
	RSI = 6,
	RDI = 7,
#ifdef HL_64
	R8 = 8,
	R9 = 9,
	R10	= 10,
	R11	= 11,
	R12	= 12,
	R13	= 13,
	R14	= 14,
	R15	= 15,
#endif
	_UNUSED = 0xFF
} CpuReg;

#define R(id)		((id) | FL_NATREG)
#define X(id)		R(id)
#define MMX(id)		R(id+64)

typedef enum {
	_MOV,
	_LEA,
	_PUSH,
	ADD,
	SUB,
	IMUL,	// only overflow flag changes compared to MUL
	DIV,
	IDIV,
	CDQ,
	CDQE,
	_POP,
	_RET,
	_CALL,
	AND,
	OR,
	XOR,
	_CMP,
	_TEST,
	NOP,
	SHL,
	SHR,
	SAR,
	INC,
	DEC,
	JMP,
	// FPU
	FSTP,
	FSTP32,
	FLD,
	FLD32,
	FLDCW,
	// SSE
	MOVSD,
	MOVSS,
	COMISD,
	COMISS,
	ADDSD,
	SUBSD,
	MULSD,
	DIVSD,
	ADDSS,
	SUBSS,
	MULSS,
	DIVSS,
	XORPD,
	CVTSI2SD,
	CVTSI2SS,
	CVTSD2SI,
	CVTSD2SS,
	CVTSS2SD,
	CVTSS2SI,
	STMXCSR,
	LDMXCSR,
	// 8-16 bits
	MOV8,
	CMP8,
	TEST8,
	PUSH8,
	MOV16,
	CMP16,
	TEST16,
	// prefetchs
	PREFETCHT0,
	PREFETCHT1,
	PREFETCHT2,
	PREFETCHNTA,
	PREFETCHW,
	// --
	_CPU_LAST
} CpuOp;

#define JAlways			0xE9
#define JAlways_short	0xEB
#define JOverflow	0x80
#define JULt		0x82
#define JUGte		0x83
#define JEq			0x84
#define JNeq		0x85
#define JULte		0x86
#define JUGt		0x87
#define JParity		0x8A
#define JNParity	0x8B
#define JSLt		0x8C
#define JSGte		0x8D
#define JSLte		0x8E
#define JSGt		0x8F

#define JCarry		JLt
#define JZero		JEq
#define JNotZero	JNeq

#define FLAG_LONGOP	0x80000000
#define FLAG_16B	0x40000000
#define FLAG_8B		0x20000000
#define FLAG_DUAL   0x10000000
#define FLAG_DEF64	0x08000000

#define RM(op,id) ((op) | (((id)+1)<<8))
#define GET_RM(op)	(((op) >> ((op) < 0 ? 24 : 8)) & 15)
#define SBYTE(op) ((op) << 16)
#define LONG_OP(op)	((op) | FLAG_LONGOP)
#define OP16(op)	LONG_OP((op) | FLAG_16B)
#define LONG_RM(op,id)	LONG_OP(op | (((id) + 1) << 24))

typedef struct {
	const char *name;						// single operand
	int r_mem;		// r32 / r/m32				r32
	int mem_r;		// r/m32 / r32				r/m32
	int r_const;	// r32 / imm32				imm32
	int r_i8;		// r32 / imm8				imm8
} opform;

static opform OP_FORMS[] = {
	{ "MOV", 0x8B, 0x89, 0xB8, 0 },
	{ "LEA", 0x8D },
	{ "PUSH", 0x50 | FLAG_DEF64, RM(0xFF,6), 0x68, 0x6A },
	{ "ADD", 0x03, 0x01, RM(0x81,0), RM(0x83,0) },
	{ "SUB", 0x2B, 0x29, RM(0x81,5), RM(0x83,5) },
	{ "IMUL", LONG_OP(0x0FAF), 0, 0x69 | FLAG_DUAL, 0x6B | FLAG_DUAL },
	{ "DIV", RM(0xF7,6), RM(0xF7,6) },
	{ "IDIV", RM(0xF7,7), RM(0xF7,7) },
	{ "CDQ", 0x99 },
	{ "CDQE", 0x98 },
	{ "POP", 0x58 | FLAG_DEF64, RM(0x8F,0) },
	{ "RET", 0xC3 },
	{ "CALL", RM(0xFF,2) | FLAG_DEF64, RM(0xFF,2), 0xE8 },
	{ "AND", 0x23, 0x21, RM(0x81,4), RM(0x83,4) },
	{ "OR", 0x0B, 0x09, RM(0x81,1), RM(0x83,1) },
	{ "XOR", 0x33, 0x31, RM(0x81,6), RM(0x83,6) },
	{ "CMP", 0x3B, 0x39, RM(0x81,7), RM(0x83,7) },
	{ "TEST", 0x85, 0x85/*SWP?*/, RM(0xF7,0) },
	{ "NOP", 0x90 },
	{ "SHL", RM(0xD3,4), 0, 0, RM(0xC1,4) },
	{ "SHR", RM(0xD3,5), 0, 0, RM(0xC1,5) },
	{ "SAR", RM(0xD3,7), 0, 0, RM(0xC1,7) },
	{ "INC", IS_64 ? RM(0xFF,0) : 0x40, RM(0xFF,0) },
	{ "DEC", IS_64 ? RM(0xFF,1) : 0x48, RM(0xFF,1) },
	{ "JMP", RM(0xFF,4) },
	// FPU
	{ "FSTP", 0, RM(0xDD,3) },
	{ "FSTP32", 0, RM(0xD9,3) },
	{ "FLD", 0, RM(0xDD,0) },
	{ "FLD32", 0, RM(0xD9,0) },
	{ "FLDCW", 0, RM(0xD9, 5) },
	// SSE
	{ "MOVSD", 0xF20F10, 0xF20F11  },
	{ "MOVSS", 0xF30F10, 0xF30F11  },
	{ "COMISD", 0x660F2F },
	{ "COMISS", LONG_OP(0x0F2F) },
	{ "ADDSD", 0xF20F58 },
	{ "SUBSD", 0xF20F5C },
	{ "MULSD", 0xF20F59 },
	{ "DIVSD", 0xF20F5E },
	{ "ADDSS", 0xF30F58 },
	{ "SUBSS", 0xF30F5C },
	{ "MULSS", 0xF30F59 },
	{ "DIVSS", 0xF30F5E },
	{ "XORPD", 0x660F57 },
	{ "CVTSI2SD", 0xF20F2A },
	{ "CVTSI2SS", 0xF30F2A },
	{ "CVTSD2SI", 0xF20F2D },
	{ "CVTSD2SS", 0xF20F5A },
	{ "CVTSS2SD", 0xF30F5A },
	{ "CVTSS2SI", 0xF30F2D },
	{ "STMXCSR", 0, LONG_RM(0x0FAE,3) },
	{ "LDMXCSR", 0, LONG_RM(0x0FAE,2) },
	// 8 bits,
	{ "MOV8", 0x8A, 0x88, 0, RM(0xC6,0) },
	{ "CMP8", 0x3A, 0x38, 0, RM(0x80,7) },
	{ "TEST8", 0x84, 0x84, RM(0xF6,0) },
	{ "PUSH8", FLAG_DEF64, 0, 0x6A | FLAG_8B },
	{ "MOV16", OP16(0x8B), OP16(0x89), OP16(0xB8) },
	{ "CMP16", OP16(0x3B), OP16(0x39) },
	{ "TEST16", OP16(0x85) },
	// prefetchs
	{ "PREFETCHT0", FLAG_DEF64, LONG_RM(0x0F18,1) },
	{ "PREFETCHT1", FLAG_DEF64, LONG_RM(0x0F18,2) },
	{ "PREFETCHT2", FLAG_DEF64, LONG_RM(0x0F18,3) },
	{ "PREFETCHNTA", FLAG_DEF64, LONG_RM(0x0F18,0) },
	{ "PREFETCHW", FLAG_DEF64, LONG_RM(0x0F0D,1) },
};

#ifdef HL_64
#	define REX()	if( r64 ) B(r64 | 0x40)
#else
#	define REX()
#endif

static const int SIB_MULT[] = {-1, 0, 1, -1, 2, -1, -1, -1, 3};

#define B(v)					ctx->code.values[ctx->code.cur++] = (unsigned char)(v)
#define W(wv)					*(int*)&ctx->code.values[_incr(&ctx->code.cur,4)] = wv
#define W64(v64)				*(int_val*)&ctx->code.values[_incr(&ctx->code.cur,8)] = v64

#define MOD_RM(mod,reg,rm)		B(((mod) << 6) | (((reg)&7) << 3) | ((rm)&7))
#define SIB(mult,rmult,rbase)	B((SIB_MULT[mult]<<6) | (((rmult)&7)<<3) | ((rbase)&7))
#define IS_SBYTE(c)				( (c) >= -128 && (c) < 128 )

#define BREAK()					B(0xCC)

#define	OP(b)	\
	if( (b) & 0xFF0000 ) { \
		B((b)>>16); \
		if( r64 ) B(r64 | 0x40); /* also in 32 bits mode */ \
		B((b)>>8); \
		B(b); \
	} else { \
		if( (b) & FLAG_16B ) { \
			B(0x66); \
			REX(); \
		} else {\
			REX(); \
			if( (b) & FLAG_LONGOP ) B((b)>>8); \
		}\
		B(b); \
	}

struct _code_ctx {
	jit_ctx *jit;
	byte_arr code;
	int_arr funs;
	int_arr short_jumps;
	int_arr near_jumps;
	int *pos_map;
	int cur_op;
	bool flushed;
	int null_access_pos;
	int null_field_pos;
};

static int _incr( int*v, int n ) {
	int k = *v;
	*v += n;
	return k;
}

const char *hl_natreg_str( int reg, emit_mode m ) {
	static char out[16];
	static const char *regs_str[] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	CpuReg r = (reg & 0xFF);
	switch( m ) {
	case M_I32:
		if( r < 8 )
			sprintf(out,"E%s",regs_str[r]);
		else
			sprintf(out,"R%dD%s",r,r<16?"":"???");
		break;
	case M_UI16:
		if( r < 8 )
			sprintf(out,"%s",regs_str[r]);
		else
			sprintf(out,"R%dW%s",r,r<16?"":"???");
		break;
	case M_UI8:
		if( r < 8 )
			sprintf(out,"%s",regs_str[r]);
		else
			sprintf(out,"R%dB%s",r,r<16?"":"???");
		break;
	case M_F32:
		r -= 64;
		sprintf(out,"XMM%df%s",r,r >= 0 && r < 16 ? "" : "???");
		break;
	case M_F64:
		r -= 64;
		sprintf(out,"XMM%d%s",r,r >= 0 && r < 16 ? "" : "???");
		break;
	default:
		if( r < 8 )
			sprintf(out,"R%s",regs_str[r]);
		else
			sprintf(out,"R%d%s",r,r<16?"":"???");
		break;
	}
	return out;
}

static int scratch_float_reg = -1;

void hl_jit_init_regs( regs_config *cfg ) {
	// exclude R11 at it's use as temporary for various ops
#	ifdef HL_WIN_CALL
	static int scratch_regs[] = { X(RAX), X(RCX), X(RDX), X(R8), X(R9), X(R10), /*X(R11)*/ };
	static int free_regs[] = { X(RSI), X(RDI), X(RBX), X(R12), X(R13), X(R14), X(R15) };
	static int call_regs[] = { X(RCX), X(RDX), X(R8), X(R9) };
#	else
	static int scratch_regs[] = { X(RAX), X(RCX), X(RDX), X(RSI), X(RDI), X(R8), X(R9), X(R10), /*X(R11)*/ };
	static int free_regs[] = { X(RBX), X(R12), X(R13), X(R14), X(R15) };
	static int call_regs[] = { X(RDI), X(RSI), X(RDX), X(RCX), X(R8), X(R9) };
#	endif
	cfg->regs.ret = scratch_regs[0];
	cfg->regs.nscratchs = sizeof(scratch_regs) / sizeof(int);
	cfg->regs.npersists = sizeof(free_regs) / sizeof(int);
	cfg->regs.nargs = sizeof(call_regs) / sizeof(int);
	cfg->regs.scratch = (ereg*)scratch_regs;
	cfg->regs.persist = (ereg*)free_regs;
	cfg->regs.arg = (ereg*)call_regs;
	// floats
	static int floats[] = {
		MMX(0), MMX(1), MMX(2), MMX(3), 
		MMX(4), MMX(5), MMX(6), MMX(7), 
		MMX(8), MMX(9), MMX(10), MMX(11), 
		MMX(12), MMX(13), MMX(14), MMX(15)
	};
#	ifdef HL_WIN_CALL
	cfg->floats.nargs = 4;
	cfg->floats.nscratchs = 6;
#	else
	cfg.floats.nargs = 8;
	cfg.floats.nscratchs = 16;
#	endif
	scratch_float_reg = cfg->floats.nscratchs - 1;
	cfg->floats.nscratchs--;
	cfg->floats.ret = floats[0];
	cfg->floats.scratch = (ereg*)floats;
	cfg->floats.arg = (ereg*)floats;
	cfg->floats.persist = (ereg*)floats + cfg->floats.nscratchs + 1;
	cfg->floats.npersists = 15 - cfg->floats.nscratchs;
	// extra
	cfg->req_bit_shifts = R(RCX);
	cfg->req_div_a = R(RAX);
	cfg->req_div_b = R(RDX);
	cfg->stack_reg = R(RSP);
	cfg->stack_pos = R(RBP);
	cfg->stack_align = 16;
#	ifdef GEN_DEBUG
	cfg->debug_prefix_size = 6;
#	endif
}

#define EMIT(op,a,b,mode) emit_ext(ctx,op,a,b,mode,0)
#define ID2(a,b)	((a) | ((b)<<8))

typedef enum {
	RCPU = 0,
	RFPU = 1,
	RSTACK = 2,
	RCONST = 3,
	RMEM = 4,
	RUNUSED = 5,
} preg_kind;

typedef struct {
	preg_kind kind;
	CpuReg reg;
	int value;
} preg;

#define ERRIF(v)	if( v ) jit_assert()

static preg make_reg( ereg r, emit_mode m ) {
	preg p;
	if( IS_NULL(r) ) {
		p.kind = RUNUSED;
		return p;
	}
	if( r == VAL_CONST ) {
		p.kind = RCONST;
		return p;
	}
	if( (r & FL_NATMASK) == (FL_NATREG | FL_MEMPTR) ) {
		p.kind = RMEM;
		p.reg = (r&0xFFFF);
		p.value = 0;
		return p;
	}
	ERRIF(!IS_NATREG(r));
	ERRIF((r&FL_NATMASK) == FL_STACKOFFS);
	if( (r & FL_NATMASK) == FL_STACKREG ) {
		p.kind = RSTACK;
		p.value = GET_STACK_OFFS(r);
	} else if( m == M_F32 || m == M_F64 ) {
		p.kind = RFPU;
		p.reg = (r&0xFF) - 64;
	} else {
		p.kind = RCPU;
		p.reg = (r&0xFF);
	}
	return p;
}

static void emit_ext( code_ctx *ctx, CpuOp op, ereg _a, ereg _b, emit_mode mode, int_val value ) {
	opform *f = &OP_FORMS[op];
	int mode64 = (mode == M_PTR || mode == M_I64) && (f->r_mem&FLAG_DEF64) == 0 ? 8 : 0;
	int r64 = mode64;
	preg a = make_reg(_a,mode), b = make_reg(_b,mode);
	switch( ID2(a.kind,b.kind) ) {
	case ID2(RUNUSED,RUNUSED):
		ERRIF(f->r_mem == 0);
		OP(f->r_mem);
		break;
	case ID2(RCPU,RCPU):
	case ID2(RFPU,RFPU):
		if( f->mem_r ) {
			// canonical form
			if( a.reg > 7 ) r64 |= 1;
			if( b.reg > 7 ) r64 |= 4;
			OP(f->mem_r);
			MOD_RM(3,b.reg,a.reg);
		} else {
			ERRIF( f->r_mem == 0 );
			if( a.reg > 7 ) r64 |= 4;
			if( b.reg > 7 ) r64 |= 1;
			OP(f->r_mem);
			MOD_RM(3,a.reg,b.reg);
		}
		break;
	case ID2(RCPU,RFPU):
	case ID2(RFPU,RCPU):
		ERRIF( (f->r_mem>>16) == 0 );
		if( a.reg > 7 ) r64 |= 4;
		if( b.reg > 7 ) r64 |= 1;
		OP(f->r_mem);
		MOD_RM(3,a.reg,b.reg);
		break;
	case ID2(RCPU,RUNUSED):
		ERRIF( f->r_mem == 0 );
		if( a.reg > 7 ) r64 |= 1;
		if( GET_RM(f->r_mem) > 0 ) {
			OP(f->r_mem);
			MOD_RM(3, GET_RM(f->r_mem)-1, a.reg);
		} else
			OP(f->r_mem + (a.reg&7));
		break;
	case ID2(RSTACK,RUNUSED):
		ERRIF( f->mem_r == 0 || GET_RM(f->mem_r) == 0 );
		OP(f->mem_r);
		if( IS_SBYTE(a.value) ) {
			MOD_RM(1,GET_RM(f->mem_r)-1,RBP);
			B(a.value);
		} else {
			MOD_RM(2,GET_RM(f->mem_r)-1,RBP);
			W(a.value);
		}
		break;
	case ID2(RCPU,RCONST):
		ERRIF( f->r_const == 0 && f->r_i8 == 0 );
		if( a.reg > 7 ) r64 |= 1;
		if( f->r_i8 && IS_SBYTE(value) ) {
			if( (f->r_i8&FLAG_DUAL) && a.reg > 7 ) r64 |= 4;
			OP(f->r_i8);
			if( (f->r_i8&FLAG_DUAL) ) MOD_RM(3,a.reg,a.reg); else MOD_RM(3,GET_RM(f->r_i8)-1,a.reg);
			B(value);
		} else if( GET_RM(f->r_const) > 0 || (f->r_const&FLAG_DUAL) ) {
			if( (f->r_i8&FLAG_DUAL) && a.reg > 7 ) r64 |= 4;
			OP(f->r_const&0xFF);
			if( (f->r_i8&FLAG_DUAL) ) MOD_RM(3,a.reg,a.reg); else MOD_RM(3,GET_RM(f->r_const)-1,a.reg);
			if( mode64 && IS_64 && op == _MOV ) W64(value); else W((int)value);
		} else {
			ERRIF( f->r_const == 0);
			OP((f->r_const&0xFF) + (a.reg&7));
			if( mode64 && IS_64 && op == _MOV ) W64(value); else W((int)value);
		}
		break;
	case ID2(RSTACK,RCPU):
	case ID2(RSTACK,RFPU):
		ERRIF( f->mem_r == 0 );
		if( b.reg > 7 ) r64 |= 4;
		OP(f->mem_r);
		if( IS_SBYTE(a.value) ) {
			MOD_RM(1,b.reg,RBP);
			B(a.value);
		} else {
			MOD_RM(2,b.reg,RBP);
			W(a.value);
		}
		break;
	case ID2(RCPU,RSTACK):
	case ID2(RFPU,RSTACK):
		ERRIF( f->r_mem == 0 );
		if( a.reg > 7 ) r64 |= 4;
		OP(f->r_mem);
		if( IS_SBYTE(b.value) ) {
			MOD_RM(1,a.reg,RBP);
			B(b.value);
		} else {
			MOD_RM(2,a.reg,RBP);
			W(b.value);
		}
		break;
	case ID2(RCONST,RUNUSED):
		ERRIF( f->r_const == 0 );
		OP(f->r_const);
		if( f->r_const & FLAG_8B ) B(value); else W((int)value);
		break;
	case ID2(RMEM,RUNUSED):
		ERRIF( f->mem_r == 0 );
		if( a.reg > 7 ) r64 |= 1;
		OP(f->mem_r);
		if( value == 0 && (a.reg&7) != RBP ) {
			MOD_RM(0,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(value) ) {
			MOD_RM(1,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			B(value);
		} else {
			MOD_RM(2,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			W((int)value);
		}
		break;
	case ID2(RCPU, RMEM):
	case ID2(RFPU, RMEM):
		ERRIF( f->r_mem == 0 );
		if( a.reg > 7 ) r64 |= 4;
		if( b.reg > 7 ) r64 |= 1;
		OP(f->r_mem);
		if( value == 0 && (b.reg&7) != RBP ) {
			MOD_RM(0,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(value) ) {
			MOD_RM(1,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
			B(value);
		} else {
			MOD_RM(2,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
			W((int)value);
		}
		break;
	case ID2(RMEM, RCPU):
	case ID2(RMEM, RFPU):
		ERRIF( f->mem_r == 0 );
		if( a.reg > 7 ) r64 |= 1;
		if( b.reg > 7 ) r64 |= 4;
		OP(f->mem_r);
		if( value == 0 && (a.reg&7) != RBP ) {
			MOD_RM(0,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(value) ) {
			MOD_RM(1,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			B(value);
		} else {
			MOD_RM(2,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			W((int)value);
		}
		break;
	default:
		ERRIF(1);
	}
}

static void emit_jump( code_ctx *ctx, int mode, int offset ) {
	int op_mult;
#	ifdef GEN_DEBUG
	op_mult = 16; // additional debug info per op
#	else
	op_mult = 8;
#	endif
	if( IS_SBYTE(offset*op_mult) ) {
		// assume it's ok to use short jump
		B(mode == JAlways ? JAlways_short : mode - 0x10);
		int_arr_add(ctx->short_jumps, byte_count(ctx->code));
		int_arr_add(ctx->short_jumps, ctx->cur_op + offset + 1);
		B(-2);
	} else {
		if( mode != JAlways ) B(0x0F);
		B(mode);
		int_arr_add(ctx->near_jumps, byte_count(ctx->code));
		int_arr_add(ctx->near_jumps, ctx->cur_op + offset + 1);
		W(-5);
	}
}

#define RTMP R(R11)
static ereg get_tmp( emit_mode mode ) {
	if( IS_FLOAT(mode) ) 
		return MMX(scratch_float_reg);
	return RTMP;
}

static void emit_mov_ext( code_ctx *ctx, ereg out, int out_val, ereg val, int in_val, emit_mode mode ) {
	if( out == val )
		return;
	if( (val & FL_NATMASK) == FL_STACKOFFS ) {
		emit_mov_ext(ctx,out,out_val,VAL_MEM(R(RBP)),in_val + GET_STACK_OFFS(val),mode);
		return;
	}
	if( (out & FL_NATMASK) == FL_STACKOFFS ) {
		emit_mov_ext(ctx,VAL_MEM(R(RBP)),out_val + GET_STACK_OFFS(val),val,in_val,mode);
		return;
	}
	if( !IS_PURE(out) && !IS_PURE(val) ) {
		ereg tmp = get_tmp(mode);
		emit_mov_ext(ctx, tmp, 0, val, in_val, mode);
		emit_mov_ext(ctx, out, out_val, tmp, 0, mode);
	} else {
		static CpuOp MOV_OP[] = {_MOV,MOV8,MOV16,_MOV,_MOV,_MOV,MOVSD,MOVSS,_MOV,_MOV};
		emit_ext(ctx, MOV_OP[mode],out,val,mode,IS_PURE(out) ? in_val : out_val);
	}
}

static void emit_mov( code_ctx *ctx, ereg out, ereg val, emit_mode mode ) {
	emit_mov_ext(ctx,out,0,val,0,mode);
}

static void emit_anyop( code_ctx *ctx, hl_op op, ereg out, ereg a, ereg b, emit_mode mode ) {
	CpuOp cop;
#	define F_OP(iop,f32,f64) cop = mode == M_F32 ? f32 : (mode == M_F64 ? f64 : iop);
	switch( op ) {
	case OAdd:
		F_OP(ADD,ADDSS,ADDSD);
		break;
	case OSub:
		F_OP(SUB,SUBSS,SUBSD);
		break;
	case OMul:
		F_OP(IMUL,MULSS,MULSD);
		break;
	case OIncr:
		cop = INC;
		break;
	case ODecr:
		cop = DEC;
		break;
	case OAnd:
		cop = AND;
		break;
	case OOr:
		cop = OR;
		break;
	case OXor:
		cop = XOR;
		break;
	case OShl:
	case OSShr:
	case OUShr:
		{
			ereg f = R(RCX);
			if( b != f ) {
				if( a == f || out == f ) {
					EMIT(_MOV,RTMP,a,M_I32);
					a = RTMP;
				}
				if( out == f ) {
					EMIT(_MOV,f,b,M_I32);
					emit_anyop(ctx, op, RTMP, RTMP, f, mode);
					EMIT(_MOV,f,RTMP,M_I32);
				} else {
					EMIT(_PUSH,f,UNUSED,M_I32);
					EMIT(_MOV,f,b,M_I32);
					emit_anyop(ctx, op, out, a, f, mode);
					EMIT(_POP,f,UNUSED,M_I32);
				}
				return;
			}
		}
		b = UNUSED;
		cop = (op == OShl ? SHL : (op == OSShr ? SAR : SHR));
		break;
	case OSMod:
	case OSDiv:
	case OUMod:
	case OUDiv:
		BREAK();
		return;
	default:
		jit_assert();
		break;
	}
	if( !IS_PURE(out) ) {
		ereg tmp = get_tmp(mode);
		emit_mov(ctx, tmp, a, mode);
		EMIT(cop,tmp,b,mode);
		emit_mov(ctx, out, tmp, mode);
	} else {
		emit_mov(ctx, out, a, mode);
		EMIT(cop,out,b,mode);
	}
}

void hl_codegen_flush( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	jit->code_size = ctx->code.cur;
	jit->code_instrs = ctx->code.values;
	jit->code_pos_map = ctx->pos_map;
	if( ctx->pos_map ) ctx->pos_map[ctx->cur_op+1] = ctx->code.cur;
}

static void emit_nop( code_ctx *ctx, int size ) {
	byte_reserve(ctx->code,size);
	ctx->code.cur -= size;
	if( size >= 8 ) {
		W(0x841F0F);
		W(0);
		return;
	}
	if( size >= 5 ) {
		W(0x441F0F);
		B(0);
		return;
	}
	if( size >= 4 ) {
		W(0x401F0F);
		return;
	}
	if( size >= 3 ) {
		B(0x0F);
		B(0x1F);
		B(0x00);
		return;
	}
	if( size >= 2 ) {
		B(0x66);
		B(0x90);
		return;
	}
	B(0x90);
}

static void emit_lea( code_ctx *ctx, ereg out, einstr *_e ) {
	einstr e = *_e;
	if( e.a && (e.a & FL_NATMASK) == FL_STACKOFFS ) {
		e.value += GET_STACK_OFFS(e.a);
		e.a = R(RBP);
	}
	if( !IS_PURE(e.a) ) {
		// a is always a mem address !
		emit_mov(ctx, RTMP, e.a, M_PTR);
		e.a = RTMP;
		if( !IS_PURE(e.b) ) {
			BREAK(); // TODO !
			return;
		}
	} else if( e.b && !IS_PURE(e.b) ) {
		// b is always an int index !
		emit_mov(ctx, RTMP, e.b, M_I32);
		e.b = RTMP;
	}
	int mult = e.size_offs & 0xFF;
	int offs = e.size_offs >> 8;
	if( mult == 0 ) {
		BREAK();
		emit_ext(ctx,_LEA,out,e.a,M_PTR,offs);
		return;
	}
	int r64 = 8 | ((out&8) ? 4 : 0) | ((e.b&8) ? 2 : 0) | ((e.a & 8) ? 1 : 0);
	REX();
	B(0x8D);
	MOD_RM(1,out&7,4);
	switch( mult ) {
	case 1: mult = 0; break;
	case 2: mult = 1; break;
	case 4: mult = 2; break;
	case 8: mult = 3; break;
	default: jit_assert();
	}
	SIB(mult,e.b&7,e.a&7);
	if( IS_SBYTE(offs) )
		B(offs);
	else
		jit_assert();
}

static void align_function( code_ctx *ctx ) {
	while( byte_count(ctx->code) & 15 )
		emit_nop(ctx,16 - (byte_count(ctx->code) & 15));
}

void hl_codegen_function( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	ctx->flushed = false;
	byte_free(&ctx->code);
	int_arr_free(&ctx->near_jumps);
	int_arr_free(&ctx->short_jumps);
	free(ctx->pos_map);
	ctx->pos_map = (int*)malloc((jit->reg_instr_count + 1) * sizeof(int));
	ctx->pos_map[0] = 0;
	byte_reserve(ctx->code,64);
	ctx->code.cur -= 64;
#	ifdef GEN_DEBUG
	int reg_index = 0;
	int emit_index = 0;
#	endif
	for(int cur_pos=0;cur_pos<jit->reg_instr_count;cur_pos++) {
		einstr *e = jit->reg_instrs + cur_pos;
		ereg out = jit->reg_writes[cur_pos];
		byte_reserve(ctx->code,64);
		ctx->code.cur -= 64;
		ctx->cur_op = cur_pos;
		if( cur_pos > 0 ) ctx->pos_map[cur_pos] = ctx->code.cur;
#		ifdef GEN_DEBUG
		int rid = cur_pos | (jit->fun->findex << 16);
		while( reg_index < jit->instr_count && jit->reg_pos_map[reg_index] <= cur_pos ) reg_index++;
		int uid;
		while( emit_index < jit->fun->nops && jit->emit_pos_map[emit_index] < reg_index ) {
			uid = emit_index | (jit->fun->findex << 16);
			__ignore(&uid);
			__ignore(&rid);
			emit_index++;
			if( emit_index >= jit->fun->nops || jit->emit_pos_map[emit_index] >= reg_index )
				emit_ext(ctx,_MOV,RTMP,VAL_CONST,M_I32,uid);
		}
#		endif
		switch( e->op ) {
		case LOAD_ARG:
			continue; // nop
		case MOV:
			if( (e->a & FL_NATMASK) == FL_STACKOFFS ) {
				if( !IS_PURE(out) ) jit_assert();
				emit_ext(ctx,_LEA,out,VAL_MEM(R(RBP)),M_PTR,GET_STACK_OFFS(e->a));
			} else
				emit_mov(ctx, out, e->a, e->mode);
			break;
		case XCHG:
			{
				ereg tmp = get_tmp(e->mode);
				if( !IS_PURE(e->a) && !IS_PURE(e->b) )
					jit_assert();
				emit_mov(ctx, tmp, e->a, e->mode);
				emit_mov(ctx, e->a, e->b, e->mode);
				emit_mov(ctx, e->b, tmp, e->mode);
			}
			break;
		case STORE:
			if( !IS_PURE(e->a) && !IS_PURE(e->b) && (e->a & FL_NATMASK) != FL_STACKOFFS ) {
				EMIT(_PUSH,e->b,UNUSED,e->mode);
				emit_mov(ctx, RTMP, e->a, M_PTR);
				emit_ext(ctx, _POP,VAL_MEM(RTMP), UNUSED, e->mode, e->size_offs);
			} else if( (e->a & FL_NATMASK) == FL_STACKREG ) {
				emit_mov(ctx, RTMP, e->a, M_PTR);
				emit_mov_ext(ctx, VAL_MEM(RTMP), e->size_offs, e->b, 0, e->mode);
			} else
				emit_mov_ext(ctx, VAL_MEM(e->a), e->size_offs, e->b, 0, e->mode);
			break;
		case PUSH:
			emit_ext(ctx, _PUSH, e->a, UNUSED, e->mode, 0);
			break;
		case POP:
			emit_ext(ctx, _POP, e->a, UNUSED, e->mode, 0);
			break;
		case PUSH_CONST:
			emit_ext(ctx, (e->value&0xFF) == e->value && e->mode == M_PTR ? PUSH8 : _PUSH, VAL_CONST, UNUSED, e->mode, e->value);
			break;
		case DEBUG_BREAK:
			BREAK();
			break;
		case RET:
			if( !IS_NULL(e->a) ) {
				ereg ret = IS_FLOAT(e->mode) ? MMX(0) : R(RAX);
				if( e->a != ret ) emit_mov(ctx, ret, e->a, e->mode);
			}
			EMIT(_RET, UNUSED, UNUSED, M_NONE);
			break;
		case LOAD_CONST:
			{
				ereg w = IS_PURE(out) ? out : get_tmp(e->mode);
				if( e->value == 0 )
					EMIT(IS_FLOAT(e->mode) ? XORPD : XOR, w, w, e->mode);
				else
					emit_ext(ctx, _MOV, w, VAL_CONST, e->mode, e->value);
				if( w != out )
					emit_mov(ctx, out, w, e->mode);
			}
			break;
		case LOAD_ADDR:
			{
				ereg addr;
				if( (e->a & FL_STACKREG) == FL_STACKREG ) {
					emit_mov(ctx,RTMP,e->a,M_PTR);
					addr = VAL_MEM(RTMP);
				} else {
					addr = VAL_MEM(e->a);
				}
				emit_mov_ext(ctx,out,0,addr,e->size_offs,e->mode);
			}
			break;
		case CALL_FUN:
			B(0xE8);
			{
				int pos = jit->out_pos + byte_count(ctx->code);
				int fid = e->a;
				int_arr_add_impl(&ctx->jit->galloc,&ctx->funs,pos);
				int_arr_add_impl(&ctx->jit->galloc,&ctx->funs,fid);
				W(0);
			}
			break;
		case CALL_PTR:
			if( e->value == (uint64)hl_null_access || e->value == (uint64)hl_jit_null_field_access ) {
				// call near
				int target = e->value == (uint64)hl_null_access ? ctx->null_access_pos : ctx->null_field_pos;
				B(0xE8);
				W(target - (jit->out_pos + byte_count(ctx->code) + 4));
			} else {
				emit_ext(ctx, _MOV, R(RAX), VAL_CONST, M_PTR, e->value);
				EMIT(_CALL, R(RAX), UNUSED, M_NONE);
			}
			break;
		case CALL_REG:
			EMIT(_CALL, e->a, UNUSED, M_NONE);
			break;
		case TEST:
			if( IS_FLOAT(e->mode) )
				jit_assert();
			if( !IS_PURE(e->a) ) {
				ereg tmp = get_tmp(e->mode);
				emit_mov(ctx, tmp, e->a, e->mode);
				EMIT(_TEST,tmp,tmp,e->mode);
			} else
				EMIT(_TEST,e->a,e->a,e->mode);
			break;
		case CMP:
			{
				CpuOp op;
				switch( e->mode ) {
				case M_UI8: op = CMP8; break;
				case M_UI16: op = CMP16; break;
				case M_F32: op = COMISS; break;
				case M_F64: op = COMISD; break;
				default: op = _CMP; break;
				}
				if( !IS_PURE(e->a) && !IS_PURE(e->b) ) {
					ereg tmp = get_tmp(e->mode);
					emit_mov(ctx, tmp, e->b, e->mode);
					EMIT(op,e->a,tmp,e->mode);
				} else
					EMIT(op,e->a,e->b,e->mode);
			}
			break;
		case JCOND:
			{
				int prev = 0;
				einstr *p;
				do {
					p = &e[--prev];
				} while( p->op == MOV );
				int op;
				switch( p->size_offs ) {
				case OJFalse:
				case OJNull:
					op = JZero;
					break;
				case OJTrue:
				case OJNotNull:
					op = JNotZero;
					break;
				case OJSGte:
					op = IS_FLOAT(p->mode) ? JUGte : JSGte;
					break;
				case OJSGt:
					op = IS_FLOAT(p->mode) ? JUGt : JSGt;
					break;
				case OJUGte:
					op = JUGte;
					break;
				case OJSLt:
					op = IS_FLOAT(p->mode) ? JULt : JSLt;
					break;
				case OJSLte:
					op = IS_FLOAT(p->mode) ? JULte : JSLte;
					break;
				case OJULt:
					op = JULt;
					break;
				case OJEq:
					op = JEq;
					break;
				case OJNotEq:
					op = JNeq;
					break;
				case OJNotLt:
					op = JUGte;
					break;
				case OJNotGte:
					op = JULt;
					break;
				default:
					jit_assert();
					break;
				}
				emit_jump(ctx, op, e->size_offs);
			}
			break;
		case JUMP:
			emit_jump(ctx, JAlways, e->size_offs);
			break;
		case JUMP_TABLE:
			BREAK();
			break;
		case CONV:
			BREAK();
			break;
		case BINOP:
		case UNOP:
			emit_anyop(ctx, e->size_offs, out, e->a, e->b, e->mode);
			break;
		case LEA:
			if( !IS_PURE(out) ) {
				ereg tmp = get_tmp(e->mode);
				emit_lea(ctx,tmp,e);
				emit_mov(ctx,out,tmp,e->mode);
			} else
				emit_lea(ctx,out,e);
			break;
		case STACK_OFFS:
			if( e->size_offs >= 0 )
				emit_ext(ctx,ADD,R(RSP),VAL_CONST,M_PTR,e->size_offs);
			else
				emit_ext(ctx,SUB,R(RSP),VAL_CONST,M_PTR,-e->size_offs);
			break;
		default:
			jit_assert();
			break;
		}
		if( ctx->code.cur > ctx->code.max ) jit_assert();
	}
	align_function(ctx);
	hl_codegen_flush(jit);
	for(int i=0;i<int_arr_count(ctx->short_jumps);i+=2) {
		int pos = int_arr_get(ctx->short_jumps,i);
		int target = int_arr_get(ctx->short_jumps,i+1);
		int offset = ctx->pos_map[target] - (pos + 1);
		if( !IS_SBYTE(offset) ) jit_assert();
		*(char*)&ctx->code.values[pos] = (char)offset;
	}
	for(int i=0;i<int_arr_count(ctx->near_jumps);i+=2) {
		int pos = int_arr_get(ctx->near_jumps,i);
		int target = int_arr_get(ctx->near_jumps,i+1);
		int offset = ctx->pos_map[target] - (pos + 4);
		*(int*)&ctx->code.values[pos] = offset;
	}
}


void hl_codegen_alloc( jit_ctx *jit ) {
	code_ctx *ctx = (code_ctx*)malloc(sizeof(code_ctx));
	memset(ctx,0,sizeof(code_ctx));
	jit->code = ctx;
	ctx->jit = jit;
}

void hl_codegen_init( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	byte_reserve(ctx->code,256);
	ctx->code.cur -= 256;
	ctx->null_access_pos = jit->out_pos + byte_count(ctx->code);
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);
	emit_ext(ctx,SUB,R(RSP),VAL_CONST,M_PTR,0x20);
	emit_ext(ctx,_MOV,R(RAX),VAL_CONST,M_PTR,(int_val)hl_null_access);
	EMIT(_CALL,R(RAX),UNUSED,M_PTR);
	BREAK();
	hl_jit_define_function(jit, ctx->null_access_pos, jit->out_pos + byte_count(ctx->code) - ctx->null_access_pos);
	align_function(ctx);
	ctx->null_field_pos = jit->out_pos + byte_count(ctx->code);
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);
	emit_ext(ctx,SUB,R(RSP),VAL_CONST,M_PTR,0x28);
	emit_ext(ctx,_MOV,R(RCX),VAL_MEM(R(RBP)),M_I32,HL_WSIZE*2);
	emit_ext(ctx,_MOV,R(RAX),VAL_CONST,M_PTR,(int_val)hl_jit_null_field_access);
	EMIT(_CALL,R(RAX),UNUSED,M_PTR);
	BREAK();
	hl_jit_define_function(jit, ctx->null_field_pos, jit->out_pos + byte_count(ctx->code) - ctx->null_field_pos);
	align_function(ctx);
	if( byte_count(ctx->code) > ctx->code.max ) jit_assert();
	hl_codegen_flush(jit);
}

void hl_codegen_free( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	free(ctx->pos_map);
	free(ctx);
}

void hl_codegen_final( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	for(int i=0;i<int_arr_count(ctx->funs);i+=2) {
		int pos = int_arr_get(ctx->funs,i);
		int fid = int_arr_get(ctx->funs,i+1);
		int offset = (int)(int_val)jit->mod->functions_ptrs[fid] - (pos + 4);
		*(int*)(jit->final_code + pos) = offset;
	}
	int_arr_reset(&ctx->funs);
}
