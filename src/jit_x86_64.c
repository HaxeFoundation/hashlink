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

#define S_TYPE			value_arr
#define S_NAME(name)	value_arr_##name
#define S_VALUE			uint64
#include "data_struct.c"

#define S_SORTED
#define S_MAP
#define S_TYPE			value_map
#define S_NAME(name)	value_map_##name
#define S_KEY			uint64
#define S_VALUE			int
#define S_DEFVAL		-1
#include "data_struct.c"

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

#define R(id)		MK_REG(id,R_REG)
#define MMX(id)		MK_REG((id)+64,R_REG)

typedef enum {
	_MOV,
	_LEA,
	_PUSH,
	ADD,
	SUB,
	IMUL,	// only overflow flag changes compared to MUL
	DIV,
	IDIV,
	NEG,
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
	MOVSXD,
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
	XORPS,
	XORPD,
	CVTSI2SD,
	CVTSI2SS,
	CVTTSD2SI,
	CVTSD2SS,
	CVTSS2SD,
	CVTTSS2SI,
	STMXCSR,
	LDMXCSR,
	STC,
	CLC,
	// 8-16 bits
	ADD8,
	SUB8,
	MOV8,
	MOVZX8,
	MOVSX8,
	CMP8,
	TEST8,
	PUSH8,
	ADD16,
	SUB16,
	IMUL16,
	MOV16,
	MOVZX16,
	MOVSX16,
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
#define OP16(op)	((op) | FLAG_16B)
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
	{ "NEG", RM(0xF7,3) },
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
	{ "MOVSXD", 0x63 },
	// FPU
	{ "FSTP", 0, RM(0xDD,3) },
	{ "FSTP32", 0, RM(0xD9,3) },
	{ "FLD", 0, RM(0xDD,0) },
	{ "FLD32", 0, RM(0xD9,0) },
	{ "FLDCW", 0, RM(0xD9, 5) },
	// SSE
	{ "MOVSD", 0xF20F10, 0xF20F11  },
	{ "MOVSS", 0xF30F10, 0xF30F11  },
	{ "COMISD", LONG_RM(0x660F2F,1) },
	{ "COMISS", LONG_RM(0x0F2F,1) },
	{ "ADDSD", 0xF20F58 },
	{ "SUBSD", 0xF20F5C },
	{ "MULSD", 0xF20F59 },
	{ "DIVSD", 0xF20F5E },
	{ "ADDSS", 0xF30F58 },
	{ "SUBSS", 0xF30F5C },
	{ "MULSS", 0xF30F59 },
	{ "DIVSS", 0xF30F5E },
	{ "XORPS", LONG_OP(0x0F57) },
	{ "XORPD", 0x660F57 },
	{ "CVTSI2SD", 0xF20F2A },
	{ "CVTSI2SS", 0xF30F2A },
	{ "CVTTSD2SI", 0xF20F2C },
	{ "CVTSD2SS", 0xF20F5A },
	{ "CVTSS2SD", 0xF30F5A },
	{ "CVTTSS2SI", 0xF30F2C },
	{ "STMXCSR", 0, LONG_RM(0x0FAE,3) },
	{ "LDMXCSR", 0, LONG_RM(0x0FAE,2) },
	{ "STC", 0xF9 },
	{ "CLC", 0xF8 },
	// 8 bits,
	{ "ADD8", 0, RM(0x00,3) },
	{ "SUB8", 0, 0x28 },
	{ "MOV8", 0x8A, 0x88, 0, RM(0xC6,0) },
	{ "MOVZX8", LONG_OP(0x0FB6) },
	{ "MOVSX8", LONG_OP(0x0FBE) },
	{ "CMP8", 0x3A, 0x38, 0, RM(0x80,7) },
	{ "TEST8", 0x84, 0x84, RM(0xF6,0) },
	{ "PUSH8", FLAG_DEF64, 0, 0x6A | FLAG_8B },
	{ "ADD16", 0, OP16(0x01) },
	{ "SUB16", 0, OP16(0x29) },
	{ "IMUL16", OP16(LONG_OP(0x0FAF)) },
	{ "MOV16", OP16(0x8B), OP16(0x89), OP16(0xB8) },
	{ "MOVZX16", LONG_OP(0x0FB7) },
	{ "MOVSX16", LONG_OP(0x0FBF) },
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
		}\
		if( (b) & FLAG_LONGOP ) B((b)>>8); \
		B(b); \
	}

struct _code_ctx {
	jit_ctx *jit;
	byte_arr code;
	int_arr funs;
	int_arr short_jumps;
	int_arr near_jumps;
	value_map const_table_lookup;
	byte_arr const_table;
	int_arr const_refs;
	int_arr const_addr;
	int *pos_map;
	int cur_op;
	bool flushed;
	int const_table_pos;
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
	static const char *regs_str8[] = { "AL", "CL", "DL", "BL", "SPL", "BPL", "SIL", "DIL" };
	CpuReg r = REG_REG(reg);
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
			sprintf(out,"%s",regs_str8[r]);
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

static ereg scratch_not_param[] = { R(RAX), R(R10), R(R11) };

void hl_jit_init_regs( regs_config *cfg ) {
	// exclude R11 at it's use as temporary for various ops
#	ifdef HL_WIN_CALL
	static int scratch_regs[] = { R(RAX), R(RCX), R(RDX), R(R8), R(R9), R(R10), /*R(R11)*/ };
	static int free_regs[] = { R(RSI), R(RDI), R(RBX), R(R12), R(R13), R(R14), R(R15) };
	static int call_regs[] = { R(RCX), R(RDX), R(R8), R(R9) };
#	else
	static int scratch_regs[] = { R(RAX), R(RCX), R(RDX), R(RSI), R(RDI), R(R8), R(R9), R(R10), /*R(R11)*/ };
	static int free_regs[] = { R(RBX), R(R12), R(R13), R(R14), R(R15) };
	static int call_regs[] = { R(RDI), R(RSI), R(RDX), R(RCX), R(R8), R(R9) };
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
	cfg->req_div_b = R(RCX);
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
	int64 value;
} preg;

#define ERRIF(v)	if( v ) jit_assert()

static preg make_reg( ereg r, uint64 value ) {
	preg p;
	if( IS_NULL(r) ) {
		p.kind = RUNUSED;
		return p;
	}
	if( r == VAL_CONST ) {
		p.kind = RCONST;
		p.value = value;
		return p;
	}
	p.reg = REG_REG(r);
	p.value = REG_VALUE(r);
	switch( REG_KIND(r) ) {
	case R_REG:
		if( p.reg >= 64 ) {
			p.kind = RFPU;
			p.reg -= 64;
		} else
			p.kind = RCPU;
		break;
	case R_REG_PTR:
		if( p.reg == RBP )
			p.kind = RSTACK;
		else
			p.kind = RMEM;
		break;
	case R_CONST:
		p.kind = RCONST;
		break;
	default:
		jit_assert();
		break;
	}
	if( p.reg < 0 || p.reg > 15 ) jit_assert();
	return p;
}

static void emit_ext( code_ctx *ctx, CpuOp op, ereg _a, ereg _b, emit_mode mode, int_val _value ) {
	opform *f = &OP_FORMS[op];
	int mode64 = mode == M_PTR && (f->r_mem&FLAG_DEF64) == 0 ? 8 : 0;
	int r64 = mode64;
	preg a = make_reg(_a,_value), b = make_reg(_b,_value);
	switch( ID2(a.kind,b.kind) ) {
	case ID2(RUNUSED,RUNUSED):
		ERRIF(f->r_mem == 0);
		OP(f->r_mem);
		break;
	case ID2(RCPU,RCPU):
	case ID2(RFPU,RFPU):
		if( f->mem_r ) {
			// canonical form
			if( a.reg & 8 ) r64 |= 1;
			if( b.reg & 8 ) r64 |= 4;
			OP(f->mem_r);
			MOD_RM(3,b.reg,a.reg);
		} else {
			ERRIF( f->r_mem == 0 );
			if( a.reg & 8 ) r64 |= 4;
			if( b.reg & 8 ) r64 |= 1;
			OP(f->r_mem);
			MOD_RM(3,a.reg,b.reg);
		}
		break;
	case ID2(RCPU,RFPU):
	case ID2(RFPU,RCPU):
		ERRIF( (f->r_mem>>16) == 0 );
		if( a.reg & 8 ) r64 |= 4;
		if( b.reg & 8 ) r64 |= 1;
		OP(f->r_mem);
		MOD_RM(3,a.reg,b.reg);
		break;
	case ID2(RCPU,RUNUSED):
		ERRIF( f->r_mem == 0 );
		if( a.reg & 8 ) r64 |= 1;
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
			W((int)a.value);
		}
		break;
	case ID2(RCPU,RCONST):
		ERRIF( f->r_const == 0 && f->r_i8 == 0 );
		if( a.reg & 8 ) r64 |= 1;
		if( f->r_i8 && IS_SBYTE(b.value) ) {
			if( (f->r_i8&FLAG_DUAL) && (a.reg & 8) ) r64 |= 4;
			OP(f->r_i8);
			if( (f->r_i8&FLAG_DUAL) ) MOD_RM(3,a.reg,a.reg); else MOD_RM(3,GET_RM(f->r_i8)-1,a.reg);
			B(b.value);
		} else if( GET_RM(f->r_const) > 0 || (f->r_const&FLAG_DUAL) ) {
			if( (f->r_i8&FLAG_DUAL) && (a.reg & 8) ) r64 |= 4;
			OP(f->r_const&0xFF);
			if( (f->r_i8&FLAG_DUAL) ) MOD_RM(3,a.reg,a.reg); else MOD_RM(3,GET_RM(f->r_const)-1,a.reg);
			if( mode64 && IS_64 && op == _MOV ) W64(b.value); else W((int)b.value);
		} else {
			ERRIF( f->r_const == 0);
			OP((f->r_const&0xFF) + (a.reg&7));
			if( mode64 && IS_64 && op == _MOV ) W64(b.value); else W((int)b.value);
		}
		break;
	case ID2(RSTACK,RCPU):
	case ID2(RSTACK,RFPU):
		ERRIF( f->mem_r == 0 );
		if( b.reg & 8 ) r64 |= 4;
		OP(f->mem_r);
		if( IS_SBYTE(a.value) ) {
			MOD_RM(1,b.reg,RBP);
			B(a.value);
		} else {
			MOD_RM(2,b.reg,RBP);
			W((int)a.value);
		}
		break;
	case ID2(RCPU,RSTACK):
	case ID2(RFPU,RSTACK):
		ERRIF( f->r_mem == 0 );
		if( a.reg & 8 ) r64 |= 4;
		OP(f->r_mem);
		if( IS_SBYTE(b.value) ) {
			MOD_RM(1,a.reg,RBP);
			B(b.value);
		} else {
			MOD_RM(2,a.reg,RBP);
			W((int)b.value);
		}
		break;
	case ID2(RCONST,RUNUSED):
		ERRIF( f->r_const == 0 );
		OP(f->r_const);
		if( f->r_const & FLAG_8B ) B(a.value); else W((int)a.value);
		break;
	case ID2(RMEM,RUNUSED):
		ERRIF( f->mem_r == 0 );
		if( a.reg & 8 ) r64 |= 1;
		OP(f->mem_r);
		if( a.value == 0 && (a.reg&7) != RBP ) {
			MOD_RM(0,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(a.value) ) {
			MOD_RM(1,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			B(a.value);
		} else {
			MOD_RM(2,GET_RM(f->mem_r)-1,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			W((int)a.value);
		}
		break;
	case ID2(RCPU, RMEM):
	case ID2(RFPU, RMEM):
		ERRIF( f->r_mem == 0 );
		if( a.reg & 8 ) r64 |= 4;
		if( b.reg & 8 ) r64 |= 1;
		OP(f->r_mem);
		if( b.value == 0 && (b.reg&7) != RBP ) {
			MOD_RM(0,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(b.value) ) {
			MOD_RM(1,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
			B(b.value);
		} else {
			MOD_RM(2,a.reg,b.reg);
			if( (b.reg&7) == RSP ) B(0x24);
			W((int)b.value);
		}
		break;
	case ID2(RMEM, RCPU):
	case ID2(RMEM, RFPU):
		ERRIF( f->mem_r == 0 );
		if( a.reg & 8 ) r64 |= 1;
		if( b.reg & 8 ) r64 |= 4;
		OP(f->mem_r);
		if( a.value == 0 && (a.reg&7) != RBP ) {
			MOD_RM(0,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
		} else if( IS_SBYTE(a.value) ) {
			MOD_RM(1,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			B(a.value);
		} else {
			MOD_RM(2,b.reg,a.reg);
			if( (a.reg&7) == RSP ) B(0x24);
			W((int)a.value);
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
	op_mult = 10;
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

static void emit_mov( code_ctx *ctx, ereg out, ereg val, emit_mode mode ) {
	if( out == val )
		return;
	if( !IS_REG(out) && (!IS_REG(val) || REG_VALUE(val) != 0) ) {
		ereg tmp = get_tmp(mode);
		emit_mov(ctx, tmp, val, mode);
		emit_mov(ctx, out, tmp, mode);
	} else if( IS_REG(val) && REG_VALUE(val) != 0 ) {
		emit_ext(ctx,_LEA,out,REG_PTR(val),M_PTR,0);
	} else {
		static CpuOp MOV_OP[] = {_MOV,MOV8,MOV16,_MOV,_MOV,MOVSD,MOVSS,_MOV,_MOV};
		CpuOp op = MOV_OP[mode];
		if( (mode == M_UI8 || mode == M_UI16) && IS_REG(out) ) {
			op++; // MOVZX
			mode = M_PTR;
		}
		emit_ext(ctx,op,out,val,mode,0);
	}
}

static int jump_near( code_ctx *ctx, int mode ) {
	int pos = byte_count(ctx->code);
	if( mode < 0 ) {
		// backwards
		int target = -mode;
		B(JAlways_short);
		B(target - (pos + 2)); 
	} else {
		B(mode == JAlways ? JAlways_short : mode - 0x10);
		B(0);
	}
	return pos;
}

static void patch_jump_near( code_ctx *ctx, int jpos ) {
	if( !jpos ) return;
	ctx->code.values[jpos + 1] = (unsigned char)(byte_count(ctx->code) - (jpos + 2));
}

static void emit_div_mod( code_ctx *ctx, hl_op op, ereg out, ereg a, ereg b, emit_mode mode ) {
	if( IS_FLOAT(mode) ) {
		BREAK();
		return;
	}
	ereg bas = R(RAX), div = R(RDX);
	if( out != bas ) EMIT(_PUSH,bas,UNUSED,M_PTR);
	if( out != div ) EMIT(_PUSH,div,UNUSED,M_PTR);
	if( b == bas || b == div || !IS_REG(b) ) {
		EMIT(_MOV,RTMP,b,mode);
		b = RTMP;
	}
	if( a != bas ) EMIT(_MOV,bas,a,mode);
		
	// check for div = 0
	EMIT(_TEST,b,b,mode);
	int jz = jump_near(ctx,JZero);
	int jz1 = 0;
	// Prevent MIN/-1 overflow exception
	// OSMod: r = (b == 0 || b == -1) ? 0 : a % b
	// OSDiv: r = (b == 0 || b == -1) ? a * b : a / b
	if( op == OSMod || op == OSDiv ) {
		EMIT(_CMP,b,MK_CONST(-1),mode);
		jz1 = jump_near(ctx,JZero);
	}
	bool unsign = op == OUDiv || op == OUMod;
	if( unsign )
		EMIT(XOR,div,div,mode);
	else
		EMIT(CDQ, UNUSED, UNUSED, mode);
	EMIT(unsign ? DIV : IDIV, b, UNUSED, mode);
	ereg res = (op == OUDiv || op == OSDiv) ? bas : div;
	int jn = jump_near(ctx,JAlways);
	patch_jump_near(ctx,jz);
	patch_jump_near(ctx,jz1);
	if( op != OSDiv ) {
		EMIT(XOR, res, res, mode);
	} else {
		if( res != bas ) EMIT(_MOV,res,bas,mode);
		EMIT(IMUL,res,b,mode);
	}
	patch_jump_near(ctx,jn);
	if( out != res ) EMIT(_MOV,out,res,mode);
	if( out != div ) EMIT(_POP,div,UNUSED,M_PTR);	
	if( out != bas ) EMIT(_POP,bas,UNUSED,M_PTR);
}

static void emit_anyop( code_ctx *ctx, hl_op op, ereg out, ereg a, ereg b, emit_mode mode ) {
	CpuOp cop;
	int mask = 0;
#	define F_OP(iop,f32,f64) cop = mode == M_F32 ? f32 : (mode == M_F64 ? f64 : iop);
#	define DECL_OP(i8,i16,iop,f32,f64) static CpuOp ops_##iop[] = {-1,i8,i16,iop,iop,f64,f32,-1,-1}; cop = ops_##iop[mode]
	switch( op ) {
	case OAdd:
		DECL_OP(ADD8,ADD16,ADD,ADDSS,ADDSD);
		break;
	case OSub:
		DECL_OP(SUB8,SUB16,SUB,SUBSS,SUBSD);
		break;
	case OMul:
		DECL_OP(IMUL16/*NO IMUL8*/,IMUL16,IMUL,MULSS,MULSD);
		if( mode == M_UI8 ) mask = 0xFF;
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
					EMIT(_MOV,RTMP,a,mode);
					a = RTMP;
				}
				if( out == f ) {
					EMIT(_MOV,f,b,mode);
					emit_anyop(ctx, op, RTMP, RTMP, f, mode);
					EMIT(_MOV,f,RTMP,mode);
				} else {
					EMIT(_PUSH,f,UNUSED,M_PTR);
					EMIT(_MOV,f,b,mode);
					emit_anyop(ctx, op, out, a, f, mode);
					EMIT(_POP,f,UNUSED,M_PTR);
				}
				return;
			}
		}
		if( out == b ) {
			ereg r = get_tmp(mode);
			emit_anyop(ctx,op,r,a,b,mode);
			emit_mov(ctx,out,r,mode);
			return;
		}
		b = UNUSED;
		cop = (op == OShl ? SHL : (op == OSShr ? SAR : SHR));
		break;
	case OSDiv:
		F_OP(0,DIVSS,DIVSD);
		if( IS_FLOAT(mode) ) break;
	case OSMod:
	case OUMod:
	case OUDiv:
		emit_div_mod(ctx,op,out,a,b,mode);
		return;
	case ONot:
		if( IS_REG(a) ) {
			EMIT(XOR,a,MK_CONST(1),M_I32);
		} else {			
			BREAK();
		}
		return;
	case ONeg:
		if( IS_FLOAT(mode) ) {
			if( out != a && IS_REG(out) ) {
				EMIT(mode == M_F32 ? XORPS : XORPD, out, out, mode);
				EMIT(mode == M_F32 ? SUBSS : SUBSD, out, a, mode);
			} else {
				ereg tmp = get_tmp(mode);
				EMIT(mode == M_F32 ? XORPS : XORPD, tmp, tmp, mode);
				EMIT(mode == M_F32 ? SUBSS : SUBSD, tmp, a, mode);
				EMIT(mode == M_F32 ? MOVSS : MOVSD, out, tmp, mode);
			}
			return;
		}
		cop = NEG;
		break;
	default:
		jit_assert();
		break;
	}

	if( out == a && IS_REG(a) ) {
		EMIT(cop,out,b,mode);
	} else if( !IS_REG(out) || out == b ) {
		ereg tmp = get_tmp(mode);
		emit_mov(ctx, tmp, a, mode);
		EMIT(cop,tmp,b,mode);
		if( mask ) {
			EMIT(AND,tmp,MK_CONST(mask),M_I32);
			mask = 0;
		}
		emit_mov(ctx, out, tmp, mode);
	} else {
		emit_mov(ctx, out, a, mode);
		EMIT(cop,out,b,mode);
	}
	if( mask ) EMIT(AND,out,MK_CONST(mask),M_I32);
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

#define CALC_REX(w,a,b) (((w)&8) ? 4 : 0) | (((b)&8) ? 2 : 0) | (((a) & 8) ? 1 : 0)

#define REX64(out,a,b)	B(0x48 | CALC_REX(out,a,b))
#define REX32(out,a,b)	{ int v = CALC_REX(out,a,b); if( v ) B(v|0x40); }

static void emit_lea( code_ctx *ctx, ereg out, einstr *_e ) {
	einstr e = *_e;

	int mult = e.size_offs & 0xFF;
	int offs = e.size_offs >> 8;
	if( mult != 0 && (mult < 0 || mult > 8 || (mult & (mult - 1)) != 0) ) jit_assert();

	if( IS_REG(e.a) )
		offs += REG_VALUE(e.a);

	if( !IS_REG(e.a) ) {
		// a is always a mem address !
		emit_mov(ctx, RTMP, e.a, M_PTR);
		e.a = RTMP;
		if( e.b && !IS_REG(e.b) ) {
			if( !IS_REG(out) ) jit_assert();
			emit_mov(ctx, out, e.b, M_I32);
			e.b = out;
		}
	} else if( e.b && !IS_REG(e.b) ) {
		// b is always an int index !
		emit_mov(ctx, RTMP, e.b, M_I32);
		e.b = RTMP;
	}

	if( mult == 0 ) {
		if( REG_KIND(e.a) != R_REG ) jit_assert();
		// no index
		emit_ext(ctx,_LEA,out,MK_ADDR(e.a,offs),M_PTR,0);
		return;
	}

	bool use_offs = offs != 0 || (e.a&7) == RBP;
	REX64(out,e.a,e.b);
	B(0x8D);
	MOD_RM(use_offs ? 1 : 0,out,4);
	SIB(mult,e.b,e.a);
	if( use_offs ) {
		if( !IS_SBYTE(offs) ) jit_assert();
		B(offs);
	}
}

static void align_function( code_ctx *ctx ) {
	while( byte_count(ctx->code) & 15 )
		emit_nop(ctx,16 - (byte_count(ctx->code) & 15));
}

static int reserve_const_segment( code_ctx *ctx, int size, int align ) {
	int pos = byte_count(ctx->const_table);
	if( align ) {
		int k = pos & (align-1);
		if( k ) {
			byte_reserve_impl(&ctx->jit->galloc,&ctx->const_table,align - k);
			pos = byte_count(ctx->const_table);
		}
	}
	byte_reserve_impl(&ctx->jit->galloc,&ctx->const_table,size);
	return pos;
}

static void alloc_const( code_ctx *ctx, uint64 value ) {
	int pos = value_map_find(ctx->const_table_lookup, value);
	if( pos < 0 ) {
		pos = reserve_const_segment(ctx,8,8);
		*(uint64*)byte_addr(ctx->const_table,pos) = value;
		value_map_add_impl(&ctx->jit->galloc,&ctx->const_table_lookup,value,pos);
	}
	int_arr_add_impl(&ctx->jit->galloc,&ctx->const_refs,ctx->jit->out_pos + byte_count(ctx->code) - 4);
	int_arr_add_impl(&ctx->jit->galloc,&ctx->const_refs,pos);
}

static int emit_lea_rel( code_ctx *ctx, ereg out ) {
	B(0x48 + ((out & 8) ? 4 : 0));
	B(0x8D);
	MOD_RM(0,out&7,5);
	int pos = ctx->jit->out_pos + byte_count(ctx->code);
	W(0);
	return pos;
}

static int get_cond_jump( code_ctx *ctx ) {
	int prev = 0;
	einstr *p;
	do {
		p = ctx->jit->reg_instrs + ctx->cur_op - (++prev);
	} while( p->op == MOV || p->op == JCOND || p->op == CMOV || p->op == XCHG || p->op == CXCHG );
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
	case 0:
		if( p->op == DEBUG_BREAK ) {
			// found a debug break !
			BREAK();
			op = JZero;
			break;
		}
		// fallback
	default:
		jit_assert();
		break;
	}
	return op;
}

static void emit_cmov( code_ctx *ctx, ereg out, ereg r, int cond, emit_mode m ) {
	if( IS_FLOAT(m) ) jit_assert();
	if( hl_emit_mode_sizes[m] == 8 )
		REX64(out,r,UNUSED);
	else
		REX32(out,r,UNUSED);
	B(0x0F);
	B(cond - 0x40);
	MOD_RM(3,out,r);
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
	int const_addr_prev = int_arr_count(ctx->const_addr);
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
			emit_mov(ctx, out, e->a, e->mode);
			break;
		case XCHG:
			{
				ereg tmp = get_tmp(e->mode);
				if( !IS_REG(e->a) && !IS_REG(e->b) )
					jit_assert();
				emit_mov(ctx, tmp, e->a, e->mode);
				emit_mov(ctx, e->a, e->b, e->mode);
				emit_mov(ctx, e->b, tmp, e->mode);
			}
			break;
		case STORE:
			if( !IS_REG(e->a) && !IS_REG(e->b) ) {
				if( e->mode != M_PTR ) {
					// no push/pop 32 bit
					ereg tmp2 = R(RAX);
					emit_mode mode = e->mode == M_F64 ? M_PTR : e->mode == M_F32 ? M_I32 : e->mode; 
					EMIT(_PUSH,tmp2,UNUSED,M_PTR);
					emit_mov(ctx, RTMP, e->a, M_PTR);
					emit_mov(ctx, tmp2, e->b, mode);
					emit_mov(ctx, MK_ADDR(RTMP,e->size_offs), tmp2, mode);
					EMIT(_POP,tmp2,UNUSED,M_PTR);
				} else {
					if( IS_FLOAT(e->mode) ) BREAK();
					EMIT(_PUSH,e->b,UNUSED,e->mode);
					emit_mov(ctx, RTMP, e->a, M_PTR);
					emit_ext(ctx, _POP,REG_ADD_OFFSET(REG_PTR(RTMP),e->size_offs), UNUSED, e->mode, 0);
				}
			} else if( !IS_REG(e->a) ) {
				emit_mov(ctx, RTMP, e->a, M_PTR);
				emit_mov(ctx, MK_ADDR(RTMP,e->size_offs), e->b, e->mode);
			} else
				emit_mov(ctx, REG_ADD_OFFSET(REG_PTR(e->a),e->size_offs), e->b, e->mode);
			break;
		case PUSH:
			if( IS_FLOAT(e->mode) ) {
				if( !IS_REG(e->a) )
					EMIT(_PUSH,e->a,UNUSED,M_PTR);
				else {
					EMIT(SUB,R(RSP),MK_CONST(8),M_PTR);
					EMIT(e->mode == M_F32 ? MOVSS : MOVSD,REG_PTR(R(RSP)),e->a,e->mode);
				}
			} else if( IS_REG(e->a) && REG_VALUE(e->a) != 0 ) {
				emit_mov(ctx, RTMP, e->a, e->mode);
				EMIT(_PUSH, RTMP, UNUSED, M_PTR);
			} else
				EMIT(_PUSH, e->a, UNUSED, M_PTR);
			break;
		case POP:
			if( IS_FLOAT(e->mode) ) jit_assert();
			EMIT(_POP, e->a, UNUSED, M_PTR);
			break;
		case PUSH_CONST:
			if( e->mode != M_PTR ) jit_assert();
			if( (e->value&0xFF) == e->value )
				emit_ext(ctx,PUSH8, VAL_CONST, UNUSED, M_PTR, e->value);
			else if( (e->value&0xFFFFFFFF) == e->value )
				emit_ext(ctx,_PUSH, VAL_CONST, UNUSED, M_I32, e->value); // will push 64bits
			else
				emit_ext(ctx,_PUSH, VAL_CONST, UNUSED, M_PTR, e->value);
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
				emit_mode mode = e->mode;
				if( !IS_REG(out) )
					mode = (mode == M_F32 ? M_I32 : mode == M_F64 ? M_PTR : mode); // don't use FP for stack ops
				ereg w = IS_REG(out) ? out : get_tmp(mode);
				if( e->value == 0 )
					EMIT(mode == M_F32 ? XORPS : mode == M_F64 ? XORPD : XOR, w, w, mode);
				else if( IS_FLOAT(mode) ) {
					// MOVSS / MOVSD with data relative
					B(e->mode == M_F32 ? 0xF3 : 0xF2);
					if( out&8 ) B(0x44);
					B(0x0F);
					B(0x10);
					MOD_RM(0,out&7,5);
					W(0);					
					alloc_const(ctx, e->value);
				} else if( mode == M_PTR && (e->value&0xFFFFFFFF) == e->value )
					emit_ext(ctx, _MOV, w, VAL_CONST, M_I32, e->value);
				else
					emit_ext(ctx, _MOV, w, VAL_CONST, mode, e->value);
				if( w != out )
					emit_mov(ctx, out, w, mode);
			}
			break;
		case LOAD_ADDR:
			if( IS_REG(e->a) && e->nargs == e->mode ) {
				emit_mov(ctx, out, REG_ADD_OFFSET(REG_PTR(e->a),e->size_offs), e->nargs);
			} else {
				ereg tmp = IS_REG(out) || (e->nargs == e->mode) ? out : RTMP;
				emit_mov(ctx, RTMP, e->a, M_PTR);
				emit_mov(ctx, tmp, MK_ADDR(RTMP,e->size_offs), e->nargs);
				if( out != tmp )
					emit_mov(ctx, out, tmp, e->mode);
			}
			break;
		case LOAD_FUN:
			{
				ereg w = IS_REG(out) ? out : RTMP;
				int pos = emit_lea_rel(ctx,w);
				int fid = e->size_offs;
				int_arr_add_impl(&ctx->jit->galloc,&ctx->funs,pos);
				int_arr_add_impl(&ctx->jit->galloc,&ctx->funs,fid);
				if( w != out )
					emit_mov(ctx, out, w, M_PTR);
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
				// call near indirect
				B(0xFF);
				B(0x15);
				W(0);
				alloc_const(ctx, (uint64)e->value);
				if( e->mode == M_UI8 || e->mode == M_UI16 ) {
					// clear value upper bits
					EMIT(e->mode == M_UI8 ? MOVZX8 : MOVZX16,R(RAX),R(RAX),M_PTR);
				}
			}
			break;
		case CALL_REG:
			EMIT(_CALL, e->a, UNUSED, M_NONE);
			break;
		case TEST:
			if( IS_FLOAT(e->mode) )
				jit_assert();
			if( !IS_REG(e->a) ) {
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
				ereg a = e->a;
				if( !IS_REG(e->a) && (IS_FLOAT(e->mode) || !IS_REG(e->b)) ) {
					ereg tmp = get_tmp(e->mode);
					emit_mov(ctx, tmp, e->a, e->mode);
					a = tmp;
				}
				EMIT(op,a,e->b,e->mode);
				if( IS_FLOAT(e->mode) && e->size_offs != OJSGt && e->size_offs != OJNull && e->size_offs != OJNotNull ) {
					// handle NaNs
					int jnotnan = jump_near(ctx,JNParity);
					switch( e->size_offs ) {
					case OJSLt:
					case OJNotLt:
						// set CF=0, ZF=1
						EMIT(XOR,RTMP,RTMP,M_I32);
						break;
					case OJSGte:
					case OJNotGte:
						// set ZF=0, CF=1
						EMIT(XOR,RTMP,RTMP,M_I32);
						EMIT(STC,UNUSED,UNUSED,0);
						break;
					case OJNotEq:
					case OJEq:
						// set ZF=0, CF=?
					case OJSLte:
						// set ZF=0, CF=0
						EMIT(TEST,R(RSP),R(RSP),M_PTR);
						break;
					default:
						jit_assert();
					}
					patch_jump_near(ctx,jnotnan);
				}
			}
			break;
		case JCOND:
			{
				int jump = get_cond_jump(ctx);
				emit_jump(ctx, jump, e->size_offs);
			}
			break;
		case JUMP:
			emit_jump(ctx, JAlways, e->size_offs);
			break;
		case JUMP_TABLE:
			{
				int start = reserve_const_segment(ctx,HL_WSIZE * e->nargs,16);
				int pos = emit_lea_rel(ctx, RTMP);
				int_arr_add_impl(&ctx->jit->galloc,&ctx->const_refs,pos);
				int_arr_add_impl(&ctx->jit->galloc,&ctx->const_refs,start);
				ereg a = RTMP;
				ereg b = e->a;
				if( IS_REG(b) ) {
					// jump [a+b*8]
					B(0x40 | ((a&8)?1:0) | ((b&8)?2:0));
					B(0xFF);
					B(0x24);
					SIB(3,(b&7),(a&7));
				} else {
					ereg save = R(RAX);
					EMIT(_PUSH,save,UNUSED,M_PTR);
					EMIT(_MOV,save,b,M_I32);
					// lea tmp, [tmp+save*8]
					einstr etmp;
					etmp.a = a;
					etmp.b = save;
					etmp.size_offs = 8;
					emit_lea(ctx, RTMP, &etmp);
					EMIT(_POP,save,UNUSED,M_PTR);
					// jump [tmp]
					B(0x40 | ((RTMP&8)?1:0));
					B(0xFF);
					MOD_RM(0,4,RTMP&7);
				}
				ereg *args = hl_emit_get_args(jit->emit,e);
				for(int k=0;k<e->nargs;k++) {
					int_arr_add_impl(&jit->galloc,&ctx->const_addr,start + k * HL_WSIZE);
					int_arr_add_impl(&jit->galloc,&ctx->const_addr,ctx->cur_op + (int)args[k] + 1);
				}
			}
			break;
		case CONV_UNSIGNED:
		case CONV:
			{
				emit_mode in_mode = e->size_offs;
				ereg r = IS_REG(e->a) ? e->a : get_tmp(in_mode);
				if( r != e->a ) emit_mov(ctx, r, e->a, in_mode);
				CpuOp op = -1;
				switch( ID2(e->mode,in_mode) ) {
				case ID2(M_F32,M_UI8):
				case ID2(M_F32,M_UI16):
				case ID2(M_F32,M_I32):
				case ID2(M_F32,M_PTR):
					op = CVTSI2SS;
					break;
				case ID2(M_F64,M_UI8):
				case ID2(M_F64,M_UI16):
				case ID2(M_F64,M_I32):
				case ID2(M_F64,M_PTR):
					op = CVTSI2SD;
					break;
				case ID2(M_UI8,M_F32):
				case ID2(M_UI16,M_F32):
				case ID2(M_I32,M_F32):
				case ID2(M_PTR,M_F32):
					op = CVTTSS2SI;
					break;
				case ID2(M_UI8,M_F64):
				case ID2(M_UI16,M_F64):
				case ID2(M_I32,M_F64):
				case ID2(M_PTR,M_F64):
					op = CVTTSD2SI;
					break;
				case ID2(M_F32,M_F64):
					op = CVTSD2SS;
					break;
				case ID2(M_F64,M_F32):
					op = CVTSS2SD;
					break;
				case ID2(M_PTR,M_I32):
					// sign extend 32-64 bit conv
					op = MOVSXD;
					break;
				case ID2(M_UI16,M_UI8):
				case ID2(M_I32,M_UI8):
				case ID2(M_PTR,M_UI8):
				case ID2(M_UI8, M_UI16):
				case ID2(M_UI8, M_I32):
				case ID2(M_UI8, M_PTR):
					op = MOVZX8;
					break;
				case ID2(M_I32,M_UI16):
				case ID2(M_PTR,M_UI16):
				case ID2(M_UI16, M_I32):
				case ID2(M_UI16, M_PTR):
					op = MOVZX16;
					break;
				case ID2(M_I32,M_PTR):
					op = _MOV;
					break;
				default:
					jit_assert();
					break;
				}
				if( IS_REG(out) || op == _MOV )
					EMIT(op,out,r,e->op == CONV_UNSIGNED ? M_PTR : e->mode);
				else {
					ereg r2 = get_tmp(e->mode);
					EMIT(op,r2,r,e->op == CONV_UNSIGNED ? M_PTR : e->mode);
					emit_mov(ctx,out,r2,e->mode);
				}
			}
			break;
		case BINOP:
		case UNOP:
			emit_anyop(ctx, e->size_offs, out, e->a, e->b, e->mode);
			break;
		case LEA:
			if( !IS_REG(out) ) {
				ereg tmp = get_tmp(e->mode);
				emit_lea(ctx,tmp,e);
				emit_mov(ctx,out,tmp,e->mode);
			} else
				emit_lea(ctx,out,e);
			break;
		case STACK_OFFS:
			if( e->size_offs >= 0 )
				EMIT(ADD,R(RSP),MK_CONST(e->size_offs),M_PTR);
			else
				EMIT(SUB,R(RSP),MK_CONST(-e->size_offs),M_PTR);
			break;
		case PREFETCH:
			{
				CpuOp op;
				switch( e->size_offs ) {
				case 0: op = PREFETCHT0; break;
				case 1: op = PREFETCHT1; break;
				case 2: op = PREFETCHT2; break;
				case 3: op = PREFETCHNTA; break;
				case 4: op = PREFETCHW; break;
				default: jit_assert();
				}
				ereg a = e->a;
				if( !IS_REG(e->a) ) {
					emit_mov(ctx,RTMP,e->a,M_PTR);
					a = RTMP;
				}
				EMIT(op,REG_PTR(a),UNUSED,M_PTR);
			}
			break;
		case CMOV:
			{
				int cond = get_cond_jump(ctx);
				if( !IS_REG(out) ) jit_assert();
				if( IS_REG(e->a) ) {
					emit_cmov(ctx,out,e->a,cond,M_PTR);					
				} else {
					emit_mov(ctx,RTMP,e->a,e->mode);
					emit_cmov(ctx,out,RTMP,cond,M_PTR);
				}
			}
			break;
		case CXCHG:
			BREAK();
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
	for(int i=const_addr_prev;i<int_arr_count(ctx->const_addr);i+=2) {
		int target = int_arr_get(ctx->const_addr,i+1);
		int offs = jit->out_pos + ctx->pos_map[target];
		ctx->const_addr.values[i+1] = offs;
	}
}

void hl_codegen_alloc( jit_ctx *jit ) {
	code_ctx *ctx = (code_ctx*)malloc(sizeof(code_ctx));
	memset(ctx,0,sizeof(code_ctx));
	jit->code = ctx;
	ctx->jit = jit;
}

static void flush_function( code_ctx *ctx, int start ) {
	hl_jit_define_function(ctx->jit, start, ctx->jit->out_pos + byte_count(ctx->code) - start);
	align_function(ctx);
	if( byte_count(ctx->code) > ctx->code.max ) jit_assert();
}

void hl_codegen_init( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	byte_reserve(ctx->code,1024);
	ctx->code.cur -= 1024;

	// generate hl_null_access stub
	ctx->null_access_pos = jit->out_pos + byte_count(ctx->code);
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);
	EMIT(SUB,R(RSP),MK_CONST(0x20),M_PTR);
	emit_ext(ctx,_MOV,R(RAX),VAL_CONST,M_PTR,(int_val)hl_null_access);
	EMIT(_CALL,R(RAX),UNUSED,M_PTR);
	BREAK();
	flush_function(ctx, ctx->null_access_pos);
	
	// generate hl_null_field access stub
	ctx->null_field_pos = jit->out_pos + byte_count(ctx->code);
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);
	EMIT(SUB,R(RSP),MK_CONST(0x28),M_PTR);
	EMIT(_MOV,jit->cfg.regs.arg[0],MK_ADDR(RBP,HL_WSIZE*2),M_I32);
	emit_ext(ctx,_MOV,R(RAX),VAL_CONST,M_PTR,(int_val)hl_jit_null_field_access);
	EMIT(_CALL,R(RAX),UNUSED,M_PTR);
	BREAK();
	flush_function(ctx, ctx->null_field_pos);

	// generate c2hl stub
	jit->code_funs.c2hl = jit->out_pos + byte_count(ctx->code);
	regs_config *cfg = &jit->cfg;
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);

	ereg fptr = scratch_not_param[0];
	ereg vargs = scratch_not_param[1];
	ereg nargs = scratch_not_param[2];
	EMIT(_MOV,fptr,cfg->regs.arg[0],M_PTR);
	EMIT(_MOV,vargs,cfg->regs.arg[1],M_PTR);
	EMIT(_MOV,nargs,cfg->regs.arg[2],M_I32);

	for(int i=0;i<cfg->regs.nargs;i++)
		EMIT(_MOV, cfg->regs.arg[i], MK_ADDR(vargs,i*8), M_PTR);
	for(int i=0;i<cfg->floats.nargs;i++)
		EMIT(MOVSD, cfg->floats.arg[i]-64, MK_ADDR(vargs,(i + cfg->regs.nargs) * 8), M_PTR);

	EMIT(ADD,vargs,MK_CONST((MAX_ARGS - 1) * HL_WSIZE),M_PTR);
	int begin = byte_count(ctx->code);
	EMIT(_TEST,nargs,nargs,M_I32);
	int pos = jump_near(ctx,JZero);
	EMIT(_PUSH,MK_ADDR(vargs,0),UNUSED,M_PTR);
	EMIT(DEC,nargs,UNUSED,M_I32);
	jump_near(ctx,-begin);
	patch_jump_near(ctx,pos);

	if( IS_WINCALL64 ) EMIT(SUB,R(RSP),MK_CONST(0x20),M_PTR);
	EMIT(_CALL, fptr, UNUSED, M_NONE);

	EMIT(_MOV,R(RSP),R(RBP),M_PTR);
	EMIT(_POP,R(RBP),UNUSED,M_PTR);
	EMIT(_RET,UNUSED,UNUSED,M_NONE);
	
	flush_function(ctx, jit->code_funs.c2hl);

	// generate hl2c stub
	jit->code_funs.hl2c = jit->out_pos + byte_count(ctx->code);
	ereg cl = cfg->regs.arg[0];
	ereg tmp = cfg->regs.arg[1];
	EMIT(_PUSH,R(RBP),UNUSED,M_PTR);
	EMIT(_MOV,R(RBP),R(RSP),M_PTR);
	EMIT(SUB,R(RSP),MK_CONST(cfg->floats.nargs*8),M_PTR);

	// push all possible call registers
	for(int i=0;i<cfg->floats.nargs;i++)
		EMIT(MOVSD,MK_ADDR(RSP,i*8),cfg->floats.arg[cfg->floats.nargs - 1 - i],M_F64);
	for(int i=0;i<cfg->regs.nargs;i++)
		EMIT(_PUSH,cfg->regs.arg[cfg->regs.nargs - 1 - i],UNUSED,M_PTR);

	// opcodes for:
	//		switch( arg0->t->fun->ret->kind ) {
	//		case HF32: case HF64: return jit_wrapper_d(arg0,&args);
	//		default: return jit_wrapper_ptr(arg0,&args);
	//		}
	hl_type_fun *ft = NULL;
	ereg fun_ptr = scratch_not_param[0];

	EMIT(_MOV,tmp,MK_ADDR(cl,0),M_PTR); // ->t
	EMIT(_MOV,tmp,MK_ADDR(tmp,HL_WSIZE),M_PTR); // ->fun
	EMIT(_MOV,tmp,MK_ADDR(tmp,(int)(int_val)&ft->ret),M_PTR); // ->rets
	EMIT(_MOV,tmp,MK_ADDR(tmp,0),M_I32); // ->kind

	EMIT(_CMP,tmp,MK_CONST(HF64),M_I32);
	int float1 = jump_near(ctx,JEq);
	EMIT(_CMP,tmp,MK_CONST(HF32),M_I32);
	int float2 = jump_near(ctx,JEq);
	emit_ext(ctx,_MOV,fun_ptr,VAL_CONST,M_PTR,(int_val)hl_jit_wrapper_ptr);
	
	int jexit = jump_near(ctx, JAlways);
	patch_jump_near(ctx, float1);
	patch_jump_near(ctx, float2);
	emit_ext(ctx,_MOV,fun_ptr,VAL_CONST,M_PTR,(int_val)hl_jit_wrapper_d);
	patch_jump_near(ctx, jexit);

	int stack_args_pos = HL_WSIZE * (IS_64?2:3);
	if( IS_WINCALL64 ) {
		stack_args_pos += 0x20;
		EMIT(SUB,R(RSP),MK_CONST(0x20),M_PTR);
	}
	EMIT(_LEA,cfg->regs.arg[1],MK_ADDR(R(RBP),stack_args_pos),M_PTR);
	EMIT(_LEA,cfg->regs.arg[2],MK_ADDR(R(RBP),-(cfg->floats.nargs * 8 + cfg->regs.nargs * HL_WSIZE)),M_PTR);
	EMIT(_CALL,fun_ptr,UNUSED,M_PTR);

	if( IS_WINCALL64 )
		EMIT(ADD,R(RSP),MK_CONST(0x20),M_PTR);

	EMIT(_MOV,R(RSP),R(RBP),M_PTR);
	EMIT(_POP,R(RBP),UNUSED,M_PTR);
	EMIT(_RET,UNUSED,UNUSED,M_NONE);
	
	flush_function(ctx, jit->code_funs.hl2c);

	
	hl_codegen_flush(jit);
}

void hl_codegen_free( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	free(ctx->pos_map);
	free(ctx);
}

void hl_codegen_flush_consts( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	// patch function offsets
	for(int i=0;i<int_arr_count(ctx->funs);i+=2) {
		int pos = int_arr_get(ctx->funs,i);
		int fid = int_arr_get(ctx->funs,i+1);
		int offset = (int)(int_val)jit->mod->functions_ptrs[fid] - (pos + 4);
		*(int*)(jit->output + pos) = offset;
	}
	int_arr_reset(&ctx->funs);
	// emit constant table
	jit->code_size = byte_count(ctx->const_table);
	jit->code_instrs = ctx->const_table.values;
	ctx->const_table_pos = jit->out_pos;
	// patch constant offsets
	for(int i=0;i<int_arr_count(ctx->const_refs);i+=2) {
		int pos = int_arr_get(ctx->const_refs,i);
		int coffs = int_arr_get(ctx->const_refs,i+1);
		int offset = (ctx->const_table_pos + coffs) - (pos + 4);
		*(int*)(jit->output + pos) = offset;
	}
	int_arr_reset(&ctx->const_refs);
	// cleanup
	byte_free(&ctx->const_table);
	value_map_free(&ctx->const_table_lookup);
}

void hl_codegen_final( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	// patch absolute addresses
	for(int i=0;i<int_arr_count(ctx->const_addr);i+=2) {
		int pos = int_arr_get(ctx->const_addr,i);
		int offs = int_arr_get(ctx->const_addr,i+1);
		*(void**)(jit->final_code + ctx->const_table_pos + pos) = jit->final_code + offs;
	}
	int_arr_free(&ctx->const_addr);
}
