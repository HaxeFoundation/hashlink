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

//#define OP_LOG

typedef enum {
	Eax = 0,
	Ecx = 1,
	Edx = 2,
	Ebx = 3,
	Esp = 4,
	Ebp = 5,
	Esi = 6,
	Edi = 7,
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
	_LAST = 0xFF
} CpuReg;

typedef enum {
	MOV,
	PUSH,
	ADD,
	SUB,
	POP,
	RET,
	CALL,
	AND,
	XOR,
	CMP,
	NOP,
	// SSE
	MOVSD,
	COMISD,
	ADDSD,
	SUBSD,
	// 8 bits
	MOV8,
	// --
	_CPU_LAST
} CpuOp;

#define JAlways		0
#define JOverflow	0x80
#define JLt			0x82
#define JGte		0x83
#define JEq			0x84
#define JNeq		0x85
#define JLte		0x86
#define JGt			0x87
#define JSignLt		0x8C
#define JSignGte	0x8D
#define JSignLte	0x8E
#define JSignGt		0x8F

#define JCarry		JLt
#define JZero		JEq
#define JNotZero	JNeq

#define B(bv)	*ctx->buf.b++ = bv
#define W(wv)	*ctx->buf.w++ = wv

#ifdef HL_64
#	define W64(wv)	*ctx->buf.w64++ = wv
#else
#	define W64(wv)	W(wv)
#endif

#define MOD_RM(mod,reg,rm)		B(((mod) << 6) | (((reg)&7) << 3) | ((rm)&7))
#define IS_SBYTE(c)				( (c) >= -128 && (c) < 128 )

#define AddJump(how,local)		{ if( (how) == JAlways ) { B(0xE9); } else { B(0x0F); B(how); }; local = BUF_POS(); W(0); }
#ifdef OP_LOG
#	define XJump(how,local)		{ AddJump(how,local); printf("%s\n",(how) == JAlways ? "JUMP" : JUMP_NAMES[(how)&0x7F]); }
#else
#	define XJump(how,local)		AddJump(how,local)
#endif

#define MAX_OP_SIZE				64

#define TODO()					printf("TODO(jit.c:%d)\n",__LINE__)

#define BUF_POS()				((int)(ctx->buf.b - ctx->startBuf))
#define RTYPE(r)				r->t->kind

typedef struct jlist jlist;
struct jlist {
	int pos;
	int target;
	jlist *next;
};

typedef struct vreg vreg;

typedef enum {
	RCPU = 0,
	RFPU = 1,
	RSTACK = 2,
	RCONST = 3,
	RADDR = 4,
	RMEM = 5,
	RUNUSED = 6
} preg_kind;

typedef struct {
	int id;
	int lock;
	preg_kind kind;
	vreg *holds;
} preg;

struct vreg {
	int stackPos;
	int size;
	hl_type *t;
	preg *current;
	preg stack;
};

#define REG_AT(i)		(ctx->pregs + (i))

#ifdef HL_64
#	define RCPU_COUNT	14
#	define RFPU_COUNT	16
#	define RCPU_SCRATCH_COUNT	7
#	define RFPU_SCRATCH_COUNT	6
static int RCPU_SCRATCH_REGS[] = { Eax, Ecx, Edx, R8, R9, R10, R11 };
#else
#	define RCPU_COUNT	6
#	define RFPU_COUNT	8
#	define RCPU_SCRATCH_COUNT	3
#	define RFPU_SCRATCH_COUNT	8
static int RCPU_SCRATCH_REGS[] = { Eax, Ecx, Edx };
#endif

#define XMM(i)			((i) + RCPU_COUNT)
#define PXMM(i)			REG_AT(XMM(i))

#define PEAX			REG_AT(Eax)
#define PESP			REG_AT(Esp)
#define PEBP			REG_AT(Ebp)

#define REG_COUNT	(RCPU_COUNT + RFPU_COUNT)

#define ID2(a,b)	((a) | ((b)<<8))
#define R(id)		(ctx->vregs + (id))
#define ASSERT(i)	{ printf("JIT ERROR %d (jic.c line %d)\n",i,__LINE__); jit_exit(); }
#define IS_FLOAT(r)	((r)->t->kind == HF64)
#define LOCK(r)		if( (r)->lock < ctx->currentPos ) (r)->lock = ctx->currentPos

#define BREAK()		B(0xCC)

static preg _unused = { 0, 0, RUNUSED, NULL };
static preg *UNUSED = &_unused;

struct jit_ctx {
	union {
		unsigned char *b;
		unsigned int *w;
		unsigned long long *w64;
		int *i;
		double *d;
	} buf;
	vreg *vregs;
	preg pregs[REG_COUNT];
	int *opsPos;
	int maxRegs;
	int maxOps;
	int bufSize;
	int totalRegsSize;
	int functionPos;
	int allocOffset;
	int currentPos;
	unsigned char *startBuf;
	hl_module *m;
	hl_function *f;
	jlist *jumps;
	jlist *calls;
	hl_alloc falloc; // cleared per-function
	hl_alloc galloc;
};

static void jit_exit() {
	exit(-1);
}

static preg *pmem( preg *r, CpuReg reg, int regOrOffset, int mult ) {
	r->kind = RMEM;
	r->id = mult | (reg << 4) | (regOrOffset << 8);
	return r;
}

static preg *pcodeaddr( preg *r, int offset ) {
	r->kind = RMEM;
	r->id = 15 | (offset << 4);
	return r;
}

static preg *pconst( preg *r, int c ) {
	r->kind = RCONST;
	r->holds = NULL;
	r->id = c;
	return r;
}

static preg *pconst64( preg *r, int_val c ) {
	r->kind = RCONST;
	r->id = 0xC064C064;
	r->holds = (vreg*)c;
	return r;
}

static preg *paddr( preg *r, void *p ) {
	r->kind = RADDR;
	r->holds = (vreg*)p;
	return r;
}

static void jit_buf( jit_ctx *ctx ) {
	if( BUF_POS() > ctx->bufSize - MAX_OP_SIZE ) {
		int nsize = ctx->bufSize * 4 / 3;
		unsigned char *nbuf;
		int curpos;
		if( nsize == 0 ) {
			int i;
			for(i=0;i<ctx->m->code->nfunctions;i++)
				nsize += ctx->m->code->functions[i].nops;
			nsize *= 4;
		}
		if( nsize < ctx->bufSize + MAX_OP_SIZE * 4 ) nsize = ctx->bufSize + MAX_OP_SIZE * 4;
		curpos = BUF_POS();
		nbuf = (unsigned char*)malloc(nsize);
		if( nbuf == NULL ) ASSERT(nsize);
		if( ctx->startBuf ) {
			memcpy(nbuf,ctx->startBuf,curpos);
			free(ctx->startBuf);
		}
		ctx->startBuf = nbuf;
		ctx->buf.b = nbuf + curpos;
		ctx->bufSize = nsize;
	}
}

static const char *KNAMES[] = { "cpu","fpu","stack","const","addr","mem","unused" };
#define ERRIF(c)	if( c ) { printf("%s(%s,%s)\n",f?f->name:"???",KNAMES[a->kind], KNAMES[b->kind]); ASSERT(0); }

typedef struct {
	const char *name;
	int r_mem;		// r32 / r/m32 
	int mem_r;		// r/m32 / r32
	int r_const;	// r32 / imm32
	int r_i8;		// r32 / imm8
	int mem_const;	// r/m32 / imm32
} opform;

#define RM(op,id) ((op) | (((id)+1)<<8))
#define GET_RM(op)	(((op) >> 8) & 7)
#define SBYTE(op) ((op) << 16)

static opform OP_FORMS[_CPU_LAST] = {
	{ "MOV", 0x8B, 0x89, 0xB8, 0, RM(0xC7,0) },
	{ "PUSH", 0x50, RM(0xFF,6), 0x68, 0x6A },
	{ "ADD", 0x03, 0x01, RM(0x81,0), RM(0x83,0) },
	{ "SUB", 0x2B, 0x29, RM(0x81,5), RM(0x83,5) },
	{ "POP", 0x58, RM(0x8F,0) },
	{ "RET", 0xC3 },
	{ "CALL", RM(0xFF,2), 0, 0xE8 },
	{ "AND" },
	{ "XOR", 0x33, 0x31, RM(0x81,6), RM(0x83,6) },
	{ "CMP", 0x3B, 0x39, RM(0x81,7), RM(0x83,7) },
	{ "NOP", 0x90 },
	// SSE
	{ "MOVSD", 0xF20F10, 0xF20F11  },
	{ "COMISD", 0x660F2F },
	{ "ADDSD", 0xF20F58 },
	{ "SUBSD", 0xF20F5C },
	// 8 bits,
	{ "MOV8", 0x8A, 0x88, 0, 0xB0, RM(0xC6,0) },
};

static const char *REG_NAMES[] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }; 
static const char *JUMP_NAMES[] = { "JOVERFLOW", "J???", "JLT", "JGTE", "JEQ", "JNEQ", "JLTE", "JGT", "J?8", "J?9", "J?A", "J?B", "JSLT", "JSGTE", "JSLTE", "JSGT" };

static const char *preg_str( jit_ctx *ctx, preg *r, bool mode64 ) {
	static char buf[64];
	switch( r->kind ) {
	case RCPU:
		if( r->id < 8 )
			sprintf(buf,"%c%s",mode64?'r':'e',REG_NAMES[r->id]);
		else
			sprintf(buf,"r%d%s",r->id,mode64?"":"d");
		break;
	case RFPU:
		sprintf(buf,"xmm%d",r->id,mode64?"":"f");
		break;
	case RSTACK:
		{
			int sp = R(r->id)->stackPos;
			sprintf(buf,"@%d[%s%Xh]",r->id,sp < 0 ? "-" : "", sp < 0 ? -sp : sp);
		}
		break;
	case RCONST:
		{
			int_val v = r->holds ? (int_val)r->holds : r->id;
			if( IS_64 )
				sprintf(buf,"%s%llXh",v < 0 ? "-" : "", v < 0 ? -v : v);
			else
				sprintf(buf,"%s%Xh",v < 0 ? "-" : "", v < 0 ? -v : v);
		}
		break;
	case RMEM:
		{
			int mult = r->id & 0xF;
			int regOrOffs = mult == 15 ? r->id >> 4 : r->id >> 8;  
			CpuReg reg = (r->id >> 4) & 0xF;
			if( mult == 15 ) {
				sprintf(buf,"%s ptr[%c%Xh]",mode64 ? "qword" : "dword", regOrOffs<0?'-':'+',regOrOffs<0?-regOrOffs:regOrOffs);
			} else if( mult == 0 ) {
				int off = regOrOffs;
				if( reg < 8 )
					sprintf(buf,"[%c%s %c %Xh]",mode64?'r':'e',REG_NAMES[reg], off < 0 ? '-' : '+', off < 0 ? -off : off);
				else
					sprintf(buf,"[r%d%s %c %Xh]",reg,mode64?"":"d",off < 0 ? '-' : '+', off < 0 ? -off : off);
			} else {
				return "TODO";
			}
		}
		break;
	case RADDR:
		if( IS_64 )
			sprintf(buf, "%s ptr[%llXh]", mode64 ? "qword" : "dword", r->holds);
		else
			sprintf(buf, "%s ptr[%Xh]", mode64 ? "qword" : "dword", r->holds);
		break;
	default:
		return "???";
	}
	return buf;
}

#ifdef HL_64
#	define REX()	if( r64 ) B(r64 | 0x40)
#else
#	define REX()
#endif

#define	OP(b)	if( (b) > 0xFFFF ) {  B((b)>>16); if( r64 ) B(r64 | 0x40); B((b)>>8); B(b); } else { REX(); B(b); }

static void log_op( jit_ctx *ctx, CpuOp o, preg *a, preg *b, bool mode64 ) {
	opform *f = &OP_FORMS[o];
	printf("@%d %s%s",ctx->currentPos, f->name,mode64?"64":"");
	if( a->kind != RUNUSED )
		printf(" %s",preg_str(ctx, a, mode64));
	if( b->kind != RUNUSED )
		printf(",%s",preg_str(ctx, b, mode64));
	printf("\n");
}

static void op( jit_ctx *ctx, CpuOp o, preg *a, preg *b, bool mode64 ) {
	opform *f = &OP_FORMS[o];
	int r64 = mode64 && (o != PUSH && o != POP && o != CALL) ? 8 : 0;
#	ifdef OP_LOG
	log_op(ctx,o,a,b,mode64 && IS_64);
#	endif
	switch( ID2(a->kind,b->kind) ) {
	case ID2(RUNUSED,RUNUSED):
		ERRIF(f->r_mem == 0);
		OP(f->r_mem);
		break;
	case ID2(RCPU,RCPU):
	case ID2(RFPU,RFPU):
		ERRIF( f->r_mem == 0 );
		if( a->id > 7 ) r64 |= 4;
		if( b->id > 7 ) r64 |= 1;
		OP(f->r_mem);
		MOD_RM(3,a->id,b->id);
		break;
	case ID2(RCPU,RUNUSED):
		ERRIF( f->r_mem == 0 );
		if( a->id > 7 ) r64 |= 1;
		if( GET_RM(f->r_mem) > 0 ) {
			OP(f->r_mem);
			MOD_RM(3, GET_RM(f->r_mem)-1, a->id); 
		} else
			OP(f->r_mem + (a->id&7));
		break;
	case ID2(RSTACK,RUNUSED):
		ERRIF( f->mem_r == 0 || GET_RM(f->mem_r) == 0 );
		{
			int stackPos = R(a->id)->stackPos;
			OP(f->mem_r);
			if( IS_SBYTE(stackPos) ) {
				MOD_RM(1,GET_RM(f->mem_r)-1,Ebp);
				B(stackPos);
			} else {
				MOD_RM(2,GET_RM(f->mem_r)-1,Ebp);
				W(stackPos);
			}
		}
		break;
	case ID2(RCPU,RCONST):
		ERRIF( f->r_const == 0 );
		if( a->id > 7 ) r64 |= 1;
		{
			int_val cval = b->holds ? (int_val)b->holds : b->id;
			// short byte form
			if( f->r_i8 && IS_SBYTE(cval) ) {
				OP(f->r_i8);
				MOD_RM(3,GET_RM(f->r_i8)-1,a->id);
				B((int)cval);
			} else if( GET_RM(f->r_const) > 0 ) {
				OP(f->r_const&0xFF);
				MOD_RM(3,GET_RM(f->r_const)-1,a->id);
				if( mode64 && IS_64 ) W64(cval); else W((int)cval);
			} else {
				OP((f->r_const&0xFF) + (a->id&7));
				if( mode64 && IS_64 ) W64(cval); else W((int)cval);
			}
		}
		break;
	case ID2(RSTACK,RCPU):
	case ID2(RSTACK,RFPU):
		ERRIF( f->mem_r == 0 );
		if( b->id > 7 ) r64 |= 4;
		{
			int stackPos = R(a->id)->stackPos;
			OP(f->mem_r);
			if( IS_SBYTE(stackPos) ) {
				MOD_RM(1,b->id,Ebp);
				B(stackPos);
			} else {
				MOD_RM(2,b->id,Ebp);
				W(stackPos);
			}
		}
		break;
	case ID2(RCPU,RSTACK):
	case ID2(RFPU,RSTACK):
		ERRIF( f->r_mem == 0 );
		if( a->id > 7 ) r64 |= 4;
		{
			int stackPos = R(b->id)->stackPos;
			OP(f->r_mem);
			if( IS_SBYTE(stackPos) ) {
				MOD_RM(1,a->id,Ebp);
				B(stackPos);
			} else {
				MOD_RM(2,a->id,Ebp);
				W(stackPos);
			}
		}
		break;
	case ID2(RCONST,RUNUSED):
		ERRIF( f->r_const == 0 );
		{
			int_val cval = a->holds ? (int_val)a->holds : a->id;
			OP(f->r_const);
			if( mode64 && IS_64 ) W64(cval); else W((int)cval);
		}
		break;
	case ID2(RMEM,RUNUSED):
		ERRIF( f->r_mem == 0 );
		{
			int mult = a->id & 0xF;
			int regOrOffs = mult == 15 ? a->id >> 4 : a->id >> 8;  
			CpuReg reg = (a->id >> 4) & 0xF;
			if( mult == 15 ) {
				ERRIF(1);
			} else if( mult == 0 ) {
				if( reg > 7 ) r64 |= 1;
				OP(f->r_mem);
				if( regOrOffs == 0 && (reg&7) != Ebp ) {
					MOD_RM(0,GET_RM(f->r_mem)-1,reg);
					if( (reg&7) == Esp ) B(0x24);
				} else if( IS_SBYTE(regOrOffs) ) {
					MOD_RM(1,GET_RM(f->r_mem)-1,reg);
					if( (reg&7) == Esp ) B(0x24);
					B(regOrOffs);
				} else {
					MOD_RM(2,GET_RM(f->r_mem)-1,reg);
					if( (reg&7) == Esp ) B(0x24);
					W(regOrOffs);
				}
			} else {
				// [eax + ebx * M]
				ERRIF(1);
			}
		}
		break;
	case ID2(RCPU, RMEM):
	case ID2(RFPU, RMEM):
		ERRIF( f->r_mem == 0 );
		{
			int mult = b->id & 0xF;
			int regOrOffs = mult == 15 ? b->id >> 4 : b->id >> 8;  
			CpuReg reg = (b->id >> 4) & 0xF;
			if( mult == 15 ) {
				int pos;
				if( a->id > 7 ) r64 |= 4;
				OP(f->r_mem);
				MOD_RM(0,a->id,5);
				if( IS_64 ) {
					// offset wrt current code
					pos = BUF_POS() + 4;
					W(regOrOffs - pos);
				} else {
					ERRIF(1);
				}
			} else if( mult == 0 ) {
				if( a->id > 7 ) r64 |= 4;
				if( reg > 7 ) r64 |= 1;
				OP(f->r_mem);
				if( regOrOffs == 0 && (reg&7) != Ebp ) {
					MOD_RM(0,a->id,reg);
					if( (reg&7) == Esp ) B(0x24);
				} else if( IS_SBYTE(regOrOffs) ) {
					MOD_RM(1,a->id,reg);
					if( (reg&7) == Esp ) B(0x24);
					B(regOrOffs);
				} else {
					MOD_RM(2,a->id,reg);
					if( (reg&7) == Esp ) B(0x24);
					W(regOrOffs);
				}
			} else {
				// [eax + ebx * M]
				ERRIF(1);
			}
		}
		break;
#	ifndef HL_64
	case ID2(RFPU,RADDR):
#	endif
	case ID2(RCPU,RADDR):
		ERRIF( f->r_mem == 0 );
		if( a->id > 7 ) r64 |= 4;
		OP(f->r_mem);
		MOD_RM(0,a->id,5);
		if( IS_64 )
			W64((int_val)b->holds);
		else
			W((int)b->holds);
		break;
#	ifndef HL_64
	case ID2(RADDR,RFPU):
#	endif
	case ID2(RADDR,RCPU):
		ERRIF( f->mem_r == 0 );
		if( b->id > 7 ) r64 |= 4;
		OP(f->mem_r);
		MOD_RM(0,b->id,5);
		if( IS_64 )
			W64((int_val)a->holds);
		else
			W((int)a->holds);
		break;
	case ID2(RMEM, RCPU):
	case ID2(RMEM, RFPU):
		ERRIF( f->mem_r == 0 );
		{
			int mult = a->id & 0xF;
			int regOrOffs = mult == 15 ? a->id >> 4 : a->id >> 8;  
			CpuReg reg = (a->id >> 4) & 0xF;
			if( mult == 15 ) {
				int pos;
				if( b->id > 7 ) r64 |= 4;
				OP(f->mem_r);
				MOD_RM(0,b->id,5);
				if( IS_64 ) {
					// offset wrt current code
					pos = BUF_POS() + 4;
					W(regOrOffs - pos);
				} else {
					ERRIF(1);
				}
			} else if( mult == 0 ) {
				if( b->id > 7 ) r64 |= 4;
				if( reg > 7 ) r64 |= 1;
				OP(f->mem_r);
				if( regOrOffs == 0 && (reg&7) != Ebp ) {
					MOD_RM(0,b->id,reg);
					if( (reg&7) == Esp ) B(0x24);
				} else if( IS_SBYTE(regOrOffs) ) {
					MOD_RM(1,b->id,reg);
					if( (reg&7) == Esp ) B(0x24);
					B(regOrOffs);
				} else {
					MOD_RM(2,b->id,reg);
					if( (reg&7) == Esp ) B(0x24);
					W(regOrOffs);
				}
			} else {
				// [eax + ebx * M]
				ERRIF(1);
			}
		}
		break;
	default:
		ERRIF(1);
	}
}

static void op32( jit_ctx *ctx, CpuOp o, preg *a, preg *b ) {
	op(ctx,o,a,b,false);
}

static void op64( jit_ctx *ctx, CpuOp o, preg *a, preg *b ) {
#ifndef HL_64
	op(ctx,o,a,b,false);
#else
	op(ctx,o,a,b,true);
#endif
}

static void patch_jump( jit_ctx *ctx, int p ) {
	if( p == 0 ) return;
	*(int*)(ctx->startBuf + p) = BUF_POS() - (p + 4);
}

static preg *alloc_reg( jit_ctx *ctx, preg_kind k ) {
	int i;
	preg *p;
	switch( k ) {
	case RCPU:
		{
			int off = ctx->allocOffset++;
			const int count = RCPU_SCRATCH_COUNT;
			for(i=0;i<count;i++) {
				int r = RCPU_SCRATCH_REGS[(i + off)%count];
				p = ctx->pregs + r;
				if( p->holds == NULL ) return p;
			}
			for(i=0;i<count;i++) {
				preg *p = ctx->pregs + RCPU_SCRATCH_REGS[(i + off)%count];
				if( p->lock >= ctx->currentPos ) continue;
				if( p->holds ) {
					p->holds->current = NULL;
					p->holds = NULL;
					return p;
				}
			}
		}
		break;
	case RFPU:
		{
			int off = ctx->allocOffset++;
			const int count = RFPU_SCRATCH_COUNT;
			for(i=0;i<count;i++) {
				preg *p = PXMM((i + off)%count);
				if( p->holds == NULL ) return p;
			}
			for(i=0;i<count;i++) {
				preg *p = PXMM((i + off)%count);
				if( p->lock >= ctx->currentPos ) continue;
				if( p->holds ) {
					p->holds->current = NULL;
					p->holds = NULL;
					return p;
				}
			}
		}
		break;
	default:
		ASSERT(k);
	}
	ASSERT(0); // out of registers !
	return NULL;
}

static preg *fetch( vreg *r ) {
	if( r->current )
		return r->current;
	return &r->stack;
}

static void scratch( preg *r ) {
	if( r && r->holds ) {
		r->holds->current = NULL;
		r->holds = NULL;
	}
}

static void load( jit_ctx *ctx, preg *r, vreg *v ) {
	preg *from = fetch(v);
	if( from == r || v->size == 0 ) return;
	if( r->holds ) r->holds->current = NULL;
	r->holds = v;
	v->current = r;
	if( IS_FLOAT(v) )
		op64(ctx,MOVSD,r,from);
	else if( v->size > 4 )
		op64(ctx,MOV,r,from);
	else
		op32(ctx,MOV,r,from);
}

static preg *alloc_fpu( jit_ctx *ctx, vreg *r, bool andLoad ) {
	preg *p = fetch(r);
	if( p->kind != RFPU ) {
		if( r->size != 8 ) ASSERT(r->size);
		p = alloc_reg(ctx, RFPU);
		if( andLoad )
			load(ctx,p,r);
		else {
			if( r->current )
				r->current->holds = NULL;
			r->current = p;
			p->holds = r;
		}
	}
	return p;
}

static preg *alloc_cpu( jit_ctx *ctx, vreg *r, bool andLoad ) {
	preg *p = fetch(r);
	if( p->kind != RCPU ) {
#		ifndef HL_64
		if( r->size > 4 ) ASSERT(r->size);
#		endif
		p = alloc_reg(ctx, RCPU);
		if( andLoad )
			load(ctx,p,r);
		else {
			if( r->current )
				r->current->holds = NULL;
			r->current = p;
			p->holds = r;
		}
	}
	return p;
}

static void store( jit_ctx *ctx, vreg *r, preg *v, bool bind ) {
	if( r->current && r->current != v ) {
		r->current->holds = NULL;
		r->current = NULL;
	}
	switch( r->size ) {
	case 0:
		break;
	case 1:
		if( v->kind == RSTACK )
			store(ctx,r,alloc_cpu(ctx, R(v->id), true), bind);
		else
			op32(ctx,MOV8,&r->stack,v);
		break;
	case 4:
		if( v->kind == RSTACK )
			store(ctx,r,alloc_cpu(ctx, R(v->id), true), bind);
		else
			op32(ctx,MOV,&r->stack,v);
		break;
	case 8:
		if( IS_FLOAT(r) || v->kind == RFPU ) {
			if( v->kind == RSTACK )
				store(ctx,r,alloc_fpu(ctx, R(v->id), true), bind);
			else 
				op64(ctx,MOVSD,&r->stack,v);
		} else if( IS_64 ) {
			if( v->kind == RSTACK )
				store(ctx,r,alloc_cpu(ctx, R(v->id), true), bind);
			else
				op64(ctx,MOV,&r->stack,v);
		} else {
			ASSERT(r->size);
		}
		break;
	default:
		ASSERT(r->size);
	}
	if( bind && r->current != v ) {
		scratch(v);
		r->current = v;
		v->holds = r;
	}
}

static void op_mov( jit_ctx *ctx, vreg *to, vreg *from ) {
	preg *r = fetch(from);
	store(ctx, to, r, true);
}

static void store_const( jit_ctx *ctx, vreg *r, int c, bool useTmpReg ) {
	preg p;
	if( r->size > 4 )
		ASSERT(r->size);
	if( useTmpReg ) {
		if( c == 0 )
			op32(ctx,XOR,alloc_cpu(ctx,r,false),alloc_cpu(ctx,r,false));
		else
			op32(ctx,MOV,alloc_cpu(ctx,r,false),pconst(&p,c));
		store(ctx,r,r->current,false);
	} else {
		scratch(r->current);
		op32(ctx,MOV,&r->stack,pconst(&p,c)); 
	}
}

static void discard_regs( jit_ctx *ctx, bool native_call ) {
	int i;
	for(i=0;i<RCPU_SCRATCH_COUNT;i++) {
		preg *r = ctx->pregs + RCPU_SCRATCH_REGS[i];
		if( r->holds ) {
			r->holds->current = NULL;
			r->holds = NULL;
		}
	}
	for(i=0;i<RFPU_COUNT;i++) {
		preg *r = ctx->pregs + (i + RCPU_COUNT);
		if( r->holds ) {
			r->holds->current = NULL;
			r->holds = NULL;
		}
	}
}

static int pad_stack( jit_ctx *ctx, int size ) {
	int total = size + ctx->totalRegsSize + HL_WSIZE * 2; // EIP+EBP
	if( total & 15 ) {
		int pad = 16 - (total & 15);
		preg p;
		if( pad ) op64(ctx,SUB,PESP,pconst(&p,pad));
		size += pad;
	}
	return size;
}

static void stack_align_error( int r ) {
	printf("STACK ALIGN ERROR %d !\n",r&15);
	exit(1);
}

//#define CHECK_STACK_ALIGN

#ifdef HL_64
static CpuReg CALL_REGS[] = { Ecx, Edx, R8, R9 };
#endif

static int prepare_call_args( jit_ctx *ctx, int count, int *args, vreg *vregs, bool isNative ) {
	int i;
	int stackRegs = count;
	int size = 0, paddedSize;
	preg p;
#ifdef HL_64
	if( isNative ) {
		for(i=0;i<count;i++) {
			vreg *v = vregs + args[i];
			preg *vr = fetch(v);
			preg *r = REG_AT(CALL_REGS[i]);
#			ifdef HL_VCC
			if( i == 4 ) break;
#			endif
			if( vr != r ) {
				switch( v->size ) {
				case 1:
				case 2:
				case 4:
					op32(ctx,MOV,r,vr);
					break;
				case 8:
					if( IS_FLOAT(v) ) {
						ASSERT(0); // TODO : use XMM register !
					} else
						op64(ctx,MOV,r,vr);
					break;
				default:
					ASSERT(v->size);
					break;
				}
			}
			LOCK(r);
		}
		stackRegs = count - i;
	}
#endif
	for(i=0;i<stackRegs;i++) {
		vreg *r = vregs + args[i];
		size += hl_pad_size(size + ctx->totalRegsSize,r->t);
		size += r->size;
	}
	paddedSize = pad_stack(ctx,size);
	size = paddedSize - size;
	for(i=0;i<stackRegs;i++) {
		// RTL
		vreg *r = vregs + args[count - (i + 1)];
		int pad;
		size += r->size;
		pad = hl_pad_size(size + ctx->totalRegsSize,r->t);
		if( (i & 7) == 0 ) jit_buf(ctx);
		if( pad ) {
			op64(ctx,SUB,PESP,pconst(&p,pad));
			size += pad;
		}
		switch( r->size ) {
		case 4:
			if( !IS_64 )
				op32(ctx,PUSH,fetch(r),UNUSED);
			else {
				// pseudo push32 (not available)
				op64(ctx,SUB,PESP,pconst(&p,4));
				op32(ctx,MOV,pmem(&p,Esp,0,0),alloc_cpu(ctx,r,true));
			}
			break;
		case 8:
			if( fetch(r)->kind == RFPU ) {
				op64(ctx,SUB,PESP,pconst(&p,8));
				op64(ctx,MOVSD,pmem(&p,Esp,0,0),fetch(r));
			} else if( IS_64 )
				op64(ctx,PUSH,fetch(r),UNUSED);
			else
				ASSERT(r->t->kind);
			break;
		default:
			ASSERT(r->size);
		}
	}
	return paddedSize;
}

static void call_native( jit_ctx *ctx, void *nativeFun, int size ) {
	preg p;
#	if defined(HL_VCC) && defined(HL_64)
	// MSVC requires 32bytes of free space here
	op64(ctx,SUB,PESP,pconst(&p,32));
	size += 32;
#	endif
	// native function, already resolved
	op64(ctx,MOV,PEAX,pconst64(&p,(int_val)nativeFun));
	op64(ctx,CALL,PEAX,UNUSED);
	discard_regs(ctx, true);
	op64(ctx,ADD,PESP,pconst(&p,size));
}

static void op_call_fun( jit_ctx *ctx, vreg *dst, int findex, int count, int *args ) {
	int fid = findex < 0 ? -1 : ctx->m->functions_indexes[findex];
	int isNative = fid >= ctx->m->code->nfunctions;
	int i = 0, size = prepare_call_args(ctx,count,args,ctx->vregs,isNative);
	preg p;
	if( fid < 0 ) {
		ASSERT(fid);
	} else if( isNative ) {
		call_native(ctx,ctx->m->functions_ptrs[findex],size);
		size = 0;
	} else {
		int cpos = BUF_POS();
		if( ctx->m->functions_ptrs[findex] ) {
			// already compiled
			op32(ctx,CALL,pconst(&p,(int)ctx->m->functions_ptrs[findex] - (cpos + 5)), UNUSED);
		} else if( ctx->m->code->functions + fid == ctx->f ) {
			// our current function
			op32(ctx,CALL,pconst(&p, ctx->functionPos - (cpos + 5)), UNUSED);
		} else {
			// stage for later
			jlist *j = (jlist*)hl_malloc(&ctx->galloc,sizeof(jlist));
			j->pos = cpos;
			j->target = findex;
			j->next = ctx->calls;
			ctx->calls = j;
			op32(ctx,CALL,pconst(&p,0),UNUSED);
		}
		discard_regs(ctx, false);
	}
	if( size ) op64(ctx,ADD,PESP,pconst(&p,size));
	store(ctx, dst, IS_FLOAT(dst) ? PXMM(0) : PEAX, true);
}

static void op_call_closure( jit_ctx *ctx, vreg *dst, vreg *f, int nargs, int *rargs ) {
	/*
		We don't know in avance what is the size of the first argument of the closure, so we need 
		a dynamic check here.
		
		Note : sizes != 4/8 and closures of natives functions are directly handled into the compiler
		by creating a trampoline function.
	*/
	preg p;
	preg *r = alloc_cpu(ctx, f, true);
	preg *tmp;
	int i, has_param, double_param, end1, end2;
	LOCK(r);
	tmp = alloc_reg(ctx, RCPU);
	// read bits
	op32(ctx,MOV,tmp,pmem(&p,(CpuReg)r->id,HL_WSIZE*2,0));
	op32(ctx,CMP,tmp,pconst(&p,0));
	XJump(JNeq,has_param);
	{
		// no argument call
		int size = prepare_call_args(ctx,nargs,rargs,ctx->vregs,false);
		op64(ctx,CALL,pmem(&p,(CpuReg)r->id,HL_WSIZE,0),UNUSED);
		if( size ) op64(ctx,ADD,PESP,pconst(&p,size));
		XJump(JAlways,end1);
	}
	patch_jump(ctx,has_param);
	op32(ctx,CMP,tmp,pconst(&p,1));
	XJump(JNeq,double_param);
	{
		// int32 argument call
		int *args;
		vreg *vargs;
		int size;

		// load our first arg into a fake vreg
		vreg fake;
		hl_type ti32;
		ti32.kind = HI32;
		fake.size = 4;
		fake.t = &ti32;
		fake.current = alloc_reg(ctx,RCPU);
		LOCK(fake.current);
		op32(ctx,MOV,fake.current,pmem(&p,(CpuReg)r->id,HL_WSIZE*3,0));

		// prepare the args
		vargs = (vreg*)hl_malloc(&ctx->falloc,sizeof(vreg)*(nargs+1));
		args = (int*)hl_malloc(&ctx->falloc,sizeof(int)*(nargs+1));
		vargs[0] = fake;
		args[0] = 0;
		for(i=0;i<nargs;i++) {
			vargs[i+1] = ctx->vregs[rargs[i]];
			args[i+1] = i + 1;
		}

		// call
		size = prepare_call_args(ctx,nargs+1,args,vargs,false);
		op64(ctx,CALL,pmem(&p,(CpuReg)r->id,HL_WSIZE,0),UNUSED);
		op64(ctx,ADD,PESP,pconst(&p,size));
		XJump(JAlways,end2);
	}
	patch_jump(ctx,double_param);
	{
		// int64 argument call
		int *args;
		vreg *vargs;
		int size;

		// load our first arg into a fake vreg
		vreg fake;
		hl_type ti64;
		ti64.kind = HF64;
		fake.size = 8;
		fake.t = &ti64;
		fake.current = alloc_reg(ctx,RFPU);
		LOCK(fake.current);

		op64(ctx,MOVSD,fake.current,pmem(&p,(CpuReg)r->id,HL_WSIZE*3,0));

		// prepare the args
		vargs = (vreg*)hl_malloc(&ctx->falloc,sizeof(vreg)*(nargs+1));
		args = (int*)hl_malloc(&ctx->falloc,sizeof(int)*(nargs+1));
		vargs[0] = fake;
		args[0] = 0;
		for(i=0;i<nargs;i++) {
			vargs[i+1] = ctx->vregs[rargs[i]];
			args[i+1] = i + 1;
		}

		// call
		size = prepare_call_args(ctx,nargs+1,args,vargs,false);
		op64(ctx,CALL,pmem(&p,(CpuReg)r->id,HL_WSIZE,0),UNUSED);
		op64(ctx,ADD,PESP,pconst(&p,size));
	}
	patch_jump(ctx,end1);
	patch_jump(ctx,end2);
	discard_regs(ctx, false);
	store(ctx, dst, IS_FLOAT(dst) ? PXMM(0) : PEAX, true);
}

static void op_enter( jit_ctx *ctx ) {
	preg p;
	op64(ctx, PUSH, PEBP, UNUSED);
	op64(ctx, MOV, PEBP, PESP);
	if( ctx->totalRegsSize ) op64(ctx, SUB, PESP, pconst(&p,ctx->totalRegsSize));
}

static void op_ret( jit_ctx *ctx, vreg *r ) {
	preg p;
	load(ctx, IS_FLOAT(r) ? PXMM(0) : PEAX, r);
	if( ctx->totalRegsSize ) op64(ctx, ADD, PESP, pconst(&p, ctx->totalRegsSize));
	op64(ctx, POP, PEBP, UNUSED);
	op64(ctx, RET, UNUSED, UNUSED);
}

static preg *op_binop( jit_ctx *ctx, vreg *dst, vreg *a, vreg *b, hl_opcode *op ) {
	preg *pa = fetch(a), *pb = fetch(b), *out = NULL;
	CpuOp o;
	if( IS_FLOAT(a) ) {
		switch( op->op ) {
		case OAdd: o = ADDSD; break;
		case OSub: o = SUBSD; break;
		case OGte:
		case OLt:
		case OJLt:
		case OJGte:
			o = COMISD;
			break;
		default:
			printf("%s\n", hl_op_name(op->op));
			ASSERT(op->op);
		}
	} else {
		switch( op->op ) {
		case OAdd: o = ADD; break;
		case OSub: o = SUB; break;
		case OGte:
		case OLt:
		case OJLt:
		case OJGte:
			o = CMP;
			break;
		default:
			printf("%s\n", hl_op_name(op->op));
			ASSERT(op->op);
		}
	}
	switch( RTYPE(a) ) {
	case HI32:
		switch( ID2(pa->kind, pb->kind) ) {
		case ID2(RCPU,RCPU):
		case ID2(RCPU,RSTACK):
			op32(ctx, o, pa, pb);
			scratch(pa);
			out = pa;
			break;
		case ID2(RSTACK,RCPU):
			if( dst == a ) {
				op32(ctx, o, pa, pb);
				dst = NULL;
				out = pa;
			} else {
				alloc_cpu(ctx,a, true);
				return op_binop(ctx,dst,a,b,op);
			}
			break;
		case ID2(RSTACK,RSTACK):
			alloc_cpu(ctx, a, true);
			return op_binop(ctx, dst, a, b, op);
		default:
			printf("%s(%d,%d)\n", hl_op_name(op->op), pa->kind, pb->kind);
			ASSERT(ID2(pa->kind, pb->kind));
		}
		if( dst ) store(ctx, dst, out, true);
		return out;
	case HF64:
		pa = alloc_fpu(ctx, a, true);
		pb = alloc_fpu(ctx, b, true);
		switch( ID2(pa->kind, pb->kind) ) {
		case ID2(RFPU,RFPU):
			op64(ctx,o,pa,pb);
			scratch(pa);
			out = pa;
			break;
		default:
			printf("%s(%d,%d)\n", hl_op_name(op->op), pa->kind, pb->kind);
			ASSERT(ID2(pa->kind, pb->kind));
		}
		if( dst ) store(ctx, dst, out, true);
		return out;
	default:
		ASSERT(RTYPE(a));
	}
	return NULL;
}

static int do_jump( jit_ctx *ctx, hl_op op ) {
	int j;
	switch( op ) {
	case OJAlways:
		XJump(JAlways,j);
		break;
	case OGte:
	case OJGte:
		XJump(JGte,j);
		break;
	case OLt:
	case OJLt:
		XJump(JLt,j);
		break;
	case OEq:
	case OJEq:
		XJump(JEq,j);
		break;
	case ONotEq:
	case OJNeq:
		XJump(JNeq,j);
		break;
	default:
		j = 0;
		printf("Unknown JUMP %d\n",op);
		break;
	}
	return j;
}

static void op_cmp( jit_ctx *ctx, hl_opcode *op ) {
	int p, e;
	op_binop(ctx,NULL,R(op->p2),R(op->p3),op);
	p = do_jump(ctx,op->op);
	store_const(ctx, R(op->p1), 0, false);
	e = do_jump(ctx,OJAlways);
	patch_jump(ctx,p);
	store_const(ctx, R(op->p1), 1, false);
	patch_jump(ctx,e);
}

static void register_jump( jit_ctx *ctx, int pos, int target ) {
	jlist *j = (jlist*)hl_malloc(&ctx->falloc, sizeof(jlist));
	j->pos = pos;
	j->target = target;
	j->next = ctx->jumps;
	ctx->jumps = j;
	if( target == 0 || ctx->opsPos[target] > 0 ) ASSERT(target);
	ctx->opsPos[target] = -1;
}

static void call_native_consts( jit_ctx *ctx, void *nativeFun, int_val *args, int nargs ) {
	int size = pad_stack(ctx, IS_64 ? 0 : HL_WSIZE*nargs);
	preg p;
	int i;
#	ifdef HL_64
	for(i=0;i<nargs;i++)
		op64(ctx, MOV, REG_AT(CALL_REGS[i]), pconst64(&p, args[i]));
#	else
	for(i=nargs-1;i>=0;i--)
		op32(ctx, PUSH, pconst64(&p, args[i]), UNUSED);
#	endif
	call_native(ctx, nativeFun, size);
}

jit_ctx *hl_jit_alloc() {
	int i;
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	if( ctx == NULL ) return NULL;
	memset(ctx,0,sizeof(jit_ctx));
	hl_alloc_init(&ctx->falloc);
	hl_alloc_init(&ctx->galloc);
	for(i=0;i<RCPU_COUNT;i++) {
		preg *r = REG_AT(i);
		r->id = i;
		r->kind = RCPU;
	}
	for(i=0;i<RFPU_COUNT;i++) {
		preg *r = REG_AT(i + RCPU_COUNT);
		r->id = i;
		r->kind = RFPU;
	}
	return ctx;
}

void hl_jit_free( jit_ctx *ctx ) {
	free(ctx->vregs);
	free(ctx->opsPos);
	free(ctx->startBuf);
	hl_free(&ctx->falloc);
	hl_free(&ctx->galloc);
	free(ctx);
}

void hl_jit_init( jit_ctx *ctx, hl_module *m ) {
	int i;
	ctx->m = m;
	for(i=0;i<m->code->nfloats;i++) {
		jit_buf(ctx);
		*ctx->buf.d++ = m->code->floats[i];
	}
	while( BUF_POS() & 15 )
		op32(ctx, NOP, UNUSED, UNUSED);
}

int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f ) {
	int i, size = 0, opCount;
	int codePos = BUF_POS();
	int nargs = f->type->fun->nargs;
	preg p;
	ctx->f = f;
	if( f->nregs > ctx->maxRegs ) {
		free(ctx->vregs);
		ctx->vregs = (vreg*)malloc(sizeof(vreg) * f->nregs);
		if( ctx->vregs == NULL ) {
			ctx->maxRegs = 0;
			return -1;
		}
		ctx->maxRegs = f->nregs;
	}
	if( f->nops > ctx->maxOps ) {
		free(ctx->opsPos);
		ctx->opsPos = (int*)malloc(sizeof(int) * (f->nops + 1));
		if( ctx->opsPos == NULL ) {
			ctx->maxOps = 0;
			return -1;
		}
		ctx->maxOps = f->nops;
	}
	memset(ctx->opsPos,0,(f->nops+1)*sizeof(int));
	for(i=0;i<f->nregs;i++) {
		vreg *r = R(i);
		r->t = f->regs[i];
		r->size = hl_type_size(r->t);
		r->current = NULL;
		r->stack.holds = NULL;
		r->stack.id = i;
		r->stack.kind = RSTACK;
	}
	size = 0;
	for(i=0;i<nargs;i++) {
		vreg *r = R(i);
		size += hl_pad_size(size,r->t);
		r->stackPos = size + HL_WSIZE * 2;
		size += r->size;
	}
	size = 0;
	for(i=nargs;i<f->nregs;i++) {
		vreg *r = R(i);
		size += r->size;
		size += hl_pad_size(size + HL_WSIZE,r->t);
		r->stackPos = -(size + HL_WSIZE);
	}
	ctx->totalRegsSize = size;
	jit_buf(ctx);
	ctx->functionPos = BUF_POS();
	op_enter(ctx);
	ctx->opsPos[0] = 0;
	for(opCount=0;opCount<f->nops;opCount++) {
		int jump;
		hl_opcode *o = f->ops + opCount;
		ctx->currentPos = opCount;
		jit_buf(ctx);
		switch( o->op ) {
		case OMov:
			op_mov(ctx, R(o->p1), R(o->p2));
			break;
		case OInt:
			store_const(ctx, R(o->p1), m->code->ints[o->p2], true);
			break;
		case OBool:
			store_const(ctx, R(o->p1), o->p2, true);
			break;
		case OGetGlobal:
			{
				vreg *to = R(o->p1);
				preg *r = IS_FLOAT(to) ? alloc_fpu(ctx,to,false) : alloc_cpu(ctx, to, false);
				void *addr = m->globals_data + m->globals_indexes[o->p2];
				if( IS_FLOAT(to) )
					op64(ctx,MOVSD,r,paddr(&p,addr));
				else if( IS_64 ) {
					op64(ctx,MOV,r,pconst64(&p,(int_val)addr));
					op32(ctx,MOV,r,pmem(&p,r->id,0,0));
				} else
					op64(ctx,MOV,r,paddr(&p,addr));
				store(ctx, to, r, false);
			}
			break;
		case OSetGlobal:
			{
				vreg *r = R(o->p2);
				preg *v = IS_FLOAT(r) ? alloc_fpu(ctx,r,true) : alloc_cpu(ctx,r,true);
				void *addr = m->globals_data + m->globals_indexes[o->p1];
				LOCK(v);
				if( IS_FLOAT(r) )
					op64(ctx,MOVSD,paddr(&p,addr),v);
				else if( IS_64 ) {
					preg *tmp = alloc_reg(ctx, RCPU);
					op64(ctx,MOV,tmp,pconst64(&p,(int_val)addr));
					op32(ctx,MOV,pmem(&p,(CpuReg)tmp->id,0,0),v);
				} else
					op32(ctx,MOV,paddr(&p,addr),v);
			}
			break;
		case OCall3:
		case OCall4:
		case OCallN:
			op_call_fun(ctx, R(o->p1), o->p2, o->p3, o->extra);
			break;
		case OCall0:
			op_call_fun(ctx, R(o->p1), o->p2, 0, NULL);
			break;
		case OCall1:
			op_call_fun(ctx, R(o->p1), o->p2, 1, &o->p3);
			break;
		case OCall2:
			{
				int args[2];
				args[0] = o->p3;
				args[1] = (int)o->extra;
				op_call_fun(ctx, R(o->p1), o->p2, 2, args);
			}
			break;
		case OSub:
		case OAdd:
		case OMul:
		case ODiv:
			op_binop(ctx, R(o->p1), R(o->p2), R(o->p3), o);
			break;
		case OGte:
		case OLt:
			op_cmp(ctx, o);
			break;
		case OJFalse:
		case OJTrue:
			{
				preg *r = alloc_cpu(ctx, R(o->p1), true);
				op32(ctx, AND, r, r);
				XJump( o->op == OJFalse ? JZero : JNotZero,jump);
				register_jump(ctx,jump,(opCount + 1) + o->p2);
			}
			break;
		case OJLt:
		case OJGte:
			op_binop(ctx, NULL, R(o->p1), R(o->p2), o);
			jump = do_jump(ctx,o->op);
			register_jump(ctx,jump,(opCount + 1) + o->p3);
			break;
		case OToAny:
			{
				vreg *r = R(o->p2);
				int_val rt = (int_val)&f->regs[o->p1];
				call_native_consts(ctx, hl_alloc_dynamic, &rt, 1);
				// copy value to dynamic
				if( IS_FLOAT(r) && !IS_64 ) {
					preg *tmp = REG_AT(RCPU_SCRATCH_REGS[1]);
					op64(ctx,MOV,tmp,&r->stack);
					op64(ctx,MOV,pmem(&p,Eax,8,0),tmp);
					r->stackPos += 4;
					op64(ctx,MOV,tmp,&r->stack);
					op64(ctx,MOV,pmem(&p,Eax,12,0),tmp);
					r->stackPos -= 4;
				} else {
					preg *tmp = REG_AT(RCPU_SCRATCH_REGS[1]);
					op64(ctx,MOV,tmp,&r->stack);
					op64(ctx,MOV,pmem(&p,Eax,8,0),tmp);
				}
				store(ctx, R(o->p1), PEAX, true);
			}
			break;
		case ORet:
			op_ret(ctx, R(o->p1));
			break;
		case OFloat:
			{
				vreg *r = R(o->p1);
				if( r->size != 8 ) ASSERT(r->size);
#				ifdef HL_64
				op64(ctx,MOVSD,alloc_fpu(ctx,r,false),pcodeaddr(&p,o->p2 * 8));
#				else
				op64(ctx,MOVSD,alloc_fpu(ctx,r,false),paddr(&p,m->code->floats + o->p2));
#				endif
				store(ctx,r,r->current,false); 
			}
			break;
		case OString:
			{
				vreg *r = R(o->p1);
				op64(ctx,MOV,alloc_cpu(ctx, r, false),pconst64(&p,(int_val)m->code->strings[o->p2]));
				store(ctx,r,r->current,false);
			}
			break;
		case ONull:
			{
				vreg *r = R(o->p1);
				op64(ctx,XOR,alloc_cpu(ctx, r, false),alloc_cpu(ctx, r, false));
				store(ctx,r,r->current,false);
			}
			break;
		case ONew:
			{
				vreg *r = R(o->p1);
				int_val args[2] = { (int_val)m, (int_val)r->t }; 
				if( r->t->kind != HOBJ ) ASSERT(r->t->kind);
				call_native_consts(ctx, hl_alloc_obj, args, 2);
				store(ctx, r, PEAX, true);
			}
			break;
		case OGetFunction:
			{
				vreg *r = R(o->p1);
				int_val args[2] = { (int_val)m, (int_val)o->p2 };
				call_native_consts(ctx, hl_alloc_closure_void, args, 2);
				store(ctx, r, PEAX, true);
			}
			break;
		case OClosure:
			{
				vreg *arg = R(o->p3);
#				ifdef HL_64
				if( arg->size == 4 ) {
					TODO();
				} else {
					int size = pad_stack(ctx, 0);
					if( arg->current && arg->current->kind == RFPU ) scratch(arg->current);
					op64(ctx,MOV,REG_AT(CALL_REGS[0]),pconst64(&p,(int_val)m));
					op64(ctx,MOV,REG_AT(CALL_REGS[1]),pconst64(&p,(int_val)o->p2));
					op64(ctx,MOV,REG_AT(CALL_REGS[2]),fetch(arg));
					call_native(ctx,hl_alloc_closure_i64,size);
				}
#				else
				if( arg->size == 8 ) {
					TODO();
				} else {
					int size = pad_stack(ctx, HL_WSIZE*2 + 4);
					op64(ctx,PUSH,fetch(arg),UNUSED);
					op64(ctx,PUSH,pconst(&p,o->p2),UNUSED);
					op64(ctx,PUSH,pconst(&p,(int)m),UNUSED);
					call_native(ctx,hl_alloc_closure_i32,size);
				}
#				endif
				discard_regs(ctx,true);
				store(ctx, R(o->p1), PEAX, true);
			}
			break;
		case OCallClosure:
			op_call_closure(ctx,R(o->p1),R(o->p2),o->p3,o->extra);
			break;
		case OField:
			{
				vreg *r = R(o->p2);
				hl_runtime_obj *rt = hl_get_obj_rt(m, r->t);
				preg *rr = alloc_cpu(ctx,r, true);
				preg *rv;
				LOCK(rr);
				rv = alloc_reg(ctx,RCPU);
				if( rr == rv ) ASSERT(0);
				// TODO : copy data
				op32(ctx, MOV, rv, pmem(&p, (CpuReg)rr->id, rt->fields_indexes[o->p3], 0));
				store(ctx, R(o->p1), rv, true);
			}
			break;
		case OSetField:
			{
				vreg *r = R(o->p1);
				hl_runtime_obj *rt = hl_get_obj_rt(m, r->t);
				preg *rr = alloc_cpu(ctx, r, true);
				preg *rv;
				LOCK(rr);
				rv = alloc_cpu(ctx, R(o->p3), true);
				if( rr == rv ) ASSERT(0);
				// TODO : copy data
				op32(ctx, MOV, pmem(&p, (CpuReg)rr->id, rt->fields_indexes[o->p2], 0), rv);
			}
			break;
		case OGetThis:
			{
				vreg *r = R(0);
				hl_runtime_obj *rt = hl_get_obj_rt(m, r->t);
				preg *rr = alloc_cpu(ctx,r, true);
				preg *rv;
				LOCK(rr);
				rv = alloc_reg(ctx,RCPU);
				if( rr == rv ) ASSERT(0);
				// TODO : copy data
				op32(ctx, MOV, rv, pmem(&p, (CpuReg)rr->id, rt->fields_indexes[o->p2], 0));
				store(ctx, R(o->p1), rv, true);
			}
			break;
		case OSetThis:
			{
				vreg *r = R(0);
				hl_runtime_obj *rt = hl_get_obj_rt(m, r->t);
				preg *rr = alloc_cpu(ctx, r, true);
				preg *rv;
				LOCK(rr);
				rv = alloc_cpu(ctx, R(o->p2), true);
				if( rr == rv ) ASSERT(0);
				// TODO : copy data
				op32(ctx, MOV, pmem(&p, (CpuReg)rr->id, rt->fields_indexes[o->p1], 0), rv);
			}
			break;
		case OCallThis:
			{
				int nargs = o->p3 + 1;
				int *args = (int*)hl_malloc(&ctx->falloc,sizeof(int) * nargs);
				int size;
				vreg *dst = R(o->p1);
				preg *r = alloc_cpu(ctx, R(0), true);
				preg *tmp;
				LOCK(r);
				tmp = alloc_reg(ctx, RCPU);
				LOCK(tmp);
				op64(ctx,MOV,tmp,pmem(&p,r->id,0,0)); // read proto
				args[0] = 0;
				for(i=1;i<nargs;i++)
					args[i] = o->extra[i-1];
				size = prepare_call_args(ctx,nargs,args,ctx->vregs,false);
				op64(ctx,CALL,pmem(&p,tmp->id,(o->p2 + 1)*HL_WSIZE,0),UNUSED);
				discard_regs(ctx, false);
				op64(ctx,ADD,PESP,pconst(&p,size));
				store(ctx, dst, IS_FLOAT(dst) ? PXMM(0) : PEAX, true);
			}
			break;
		case OCallMethod:
			{
				int size;
				vreg *dst = R(o->p1);
				preg *r = alloc_cpu(ctx, R(o->extra[0]), true);
				preg *tmp;
				LOCK(r);
				tmp = alloc_reg(ctx, RCPU);
				LOCK(tmp);
				// TODO : check r == NULL
				op64(ctx,MOV,tmp,pmem(&p,r->id,0,0)); // read proto
				size = prepare_call_args(ctx,o->p3,o->extra,ctx->vregs,false);
				op64(ctx,CALL,pmem(&p,tmp->id,(o->p2 + 1)*HL_WSIZE,0),UNUSED);
				discard_regs(ctx, false);
				op64(ctx,ADD,PESP,pconst(&p,size));
				store(ctx, dst, IS_FLOAT(dst) ? PXMM(0) : PEAX, true);
			}
			break;
		case OThrow:
			{
				// TODO
				BREAK();
			}
			break;
		default:
			printf("Don't know how to jit %s\n",hl_op_name(o->op));
			break;
		}
		// we are landing at this position, assume we have lost our registers
		if( ctx->opsPos[opCount+1] == -1 )
			discard_regs(ctx,true);
		ctx->opsPos[opCount+1] = BUF_POS();
	}
	// patch jumps
	{
		jlist *j = ctx->jumps;
		while( j ) {
			*(int*)(ctx->startBuf + j->pos) = ctx->opsPos[j->target] - (j->pos + 4);
			j = j->next;
		}
		ctx->jumps = NULL;
	}
	// add nops padding
	while( BUF_POS() & 15 )
		op32(ctx, NOP, UNUSED, UNUSED);
	// clear regs
	for(i=0;i<REG_COUNT;i++) {
		preg *r = REG_AT(i);
		r->holds = NULL;
		r->lock = 0;
	}
	// reset tmp allocator
	hl_free(&ctx->falloc);
	return codePos;
}

void *hl_jit_code( jit_ctx *ctx, hl_module *m ) {
	jlist *c;
	int size = BUF_POS();
	unsigned char *code = (unsigned char*)hl_alloc_executable_memory(size);
	if( code == NULL ) return NULL;
	memcpy(code,ctx->startBuf,size);
	// patch calls
	c = ctx->calls;
	while( c ) {
		int fpos = (int)m->functions_ptrs[c->target];
		*(int*)(code + c->pos + 1) = fpos - (c->pos + 5);
		c = c->next;
	}
	return code;
}

