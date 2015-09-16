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

#define Eax 0
#define Ebx 3
#define Ecx 1
#define Edx 2
#define Esp 4
#define Ebp 5
#define Esi 6
#define Edi 7
#define JAlways		0
#define JLt			0x82
#define JGte		0x83
#define JEq			0x84
#define JNeq		0x85
#define JZero		JEq
#define JNotZero	JNeq
#define JLte		0x86
#define JGt			0x87
#define JSignLt		0x8C
#define JSignGte	0x8D
#define JSignLte	0x8E
#define JSignGt		0x8F
#define JOverflow	0x80
#define JCarry		0x82
#define B(bv)	*ctx->buf.b++ = bv
#define W(wv)	*ctx->buf.w++ = wv
#define MOD_RM(mod,reg,rm)		B((mod << 6) | (reg << 3) | rm)
#define IS_SBYTE(c)				( (c) >= -128 && (c) < 128 )
#define OP_RM(op,mod,reg,rm)	{ B(op); MOD_RM(mod,reg,rm); }
#define OP_ADDR(op,addr,reg,rm) { B(op); \
								MOD_RM(((addr) == 0 && reg != Ebp)?0:(IS_SBYTE(addr)?1:2),rm,reg); \
								if( reg == Esp ) B(0x24); \
								if( (addr) == 0 && reg != Ebp ) {} \
								else if IS_SBYTE(addr) B(addr); \
								else W(addr); }
#define XRet()					B(0xC3)
#define XMov_rr(dst,src)		OP_RM(0x8B,3,dst,src)
#define XMov_rc(dst,cst)		B(0xB8+(dst)); W(cst)
#define XMov_rp(dst,reg,idx)	OP_ADDR(0x8B,idx,reg,dst)
#define XMov_ra(dst,addr)		OP_RM(0x8B,0,dst,5); W(addr)
#define XMov_rx(dst,r,idx,mult) OP_RM(0x8B,0,dst,4); SIB(Mult##mult,idx,r)
#define XMov_pr(dst,idx,src)	OP_ADDR(0x89,idx,dst,src)
#define XMov_pc(dst,idx,c)		OP_ADDR(0xC7,idx,dst,0); W(c)
#define XMov_ar(addr,reg)		B(0x3E); if( reg == Eax ) { B(0xA3); } else { OP_RM(0x89,0,reg,5); }; W(addr)
#define XMov_xr(r,idx,mult,src) OP_RM(0x89,0,src,4); SIB(Mult##mult,idx,r)
#define XCall_r(r)				OP_RM(0xFF,3,2,r)
#define XCall_d(delta)			B(0xE8); W(delta)
#define XPush_r(r)				B(0x50+(r))
#define XPush_c(cst)			B(0x68); W(cst)
//XPush_p
#define XPush_p(reg,idx)		OP_ADDR(0xFF,idx,reg,6)
#define XAdd_rc(reg,cst)		if IS_SBYTE(cst) { OP_RM(0x83,3,0,reg); B(cst); } else { OP_RM(0x81,3,0,reg); W(cst); }
#define XAdd_rr(dst,src)		OP_RM(0x03,3,dst,src)
//XAdd_rp
//XAdd_pr
//XAdd_pc
#define XSub_rc(reg,cst)		if IS_SBYTE(cst) { OP_RM(0x83,3,5,reg); B(cst); } else { OP_RM(0x81,3,5,reg); W(cst); }
#define XSub_rr(dst,src)		OP_RM(0x2B,3,dst,src)
//XSub_rp
//XSub_pr
//XSub_pc
#define XCmp_rr(r1,r2)			OP_RM(0x3B,3,r1,r2)
#define XCmp_rc(reg,cst)		if( reg == Eax ) { B(0x3D); } else { OP_RM(0x81,3,7,reg); }; W(cst)
#define XCmp_rb(reg,byte)		OP_RM(0x83,3,7,reg); B(byte)
#define XJump(how,local)		if( (how) == JAlways ) { B(0xE9); } else { B(0x0F); B(how); }; local = ctx->buf.i; W(0)
#define XJump_near(local)		B(0xEB); local = ctx->buf.c; B(0)
#define XJump_r(reg)			OP_RM(0xFF,3,4,reg)
#define XPop_r(reg)				B(0x58 + (reg))
//XPop_p
//XInc_r
//XInc_p
//XDec_r
//XDec_p

//XLea_rp
//XNot_r
//XNot_p
//XNeg_r
//XNeg_p

#define XTest_rc(r,cst)			if( r == Eax ) { B(0xA9); W(cst); } else { B(0xF7); MOD_RM(3,0,r); W(cst); }
#define XTest_rr(r,src)			B(0x85); MOD_RM(3,r,src)
#define XAnd_rc(r,cst)			if( r == Eax ) { B(0x25); W(cst); } else { B(0x81); MOD_RM(3,4,r); W(cst); }
#define XAnd_rr(r,src)			B(0x23); MOD_RM(3,r,src)
//XAnd_rp
//XAnd_rc
//XAnd_pr
//XAnd_pc
#define XOr_rc(r,cst)			if( r == Eax ) { B(0x0D); W(cst); } else { B(0x81); MOD_RM(3,1,r); W(cst); }
#define XOr_rr(r,src)			B(0x0B); MOD_RM(3,r,src)
#define XXor_rc(r,cst)			if( r == Eax ) { B(0x35); W(cst); } else { B(0x81); MOD_RM(3,6,r); W(cst); }
#define XXor_rr(r,src)			B(0x33); MOD_RM(3,r,src)

#define shift_r(r,spec)			B(0xD3); MOD_RM(3,spec,r);
#define shift_c(r,n,spec)		if( (n) == 1 ) { B(0xD1); MOD_RM(3,spec,r); } else { B(0xC1); MOD_RM(3,spec,r); B(n); }

#define XShl_rr(r,src)			if( src != Ecx ) ERROR; shift_r(r,4)
#define XShl_rc(r,n)			shift_c(r,n,4)
//XShl_pr
//XShl_pc
#define XShr_rr(r,src)			if( src != Ecx ) ERROR; shift_r(r,7)
#define XShr_rc(r,n)			shift_c(r,n,7)
#define XUShr_rr(r,src)			if( src != Ecx ) ERROR; shift_r(r,5)
#define XUShr_rc(r,n)			shift_c(r,n,5)

//XMul (unsigned)
//

#define XIMul_rr(dst,src)		B(0x0F); B(0xAF); MOD_RM(3,dst,src)
//XIMul_rp
//XIMul_rrc
//XIMul_rpc
#define XIDiv_r(r)				B(0xF7); MOD_RM(3,7,r)
//XIDiv_p
#define XCdq()					B(0x99);

// FPU
#define XFAddp()				B(0xDE); B(0xC1)
#define XFSubp()				B(0xDE); B(0xE9)
#define XFMulp()				B(0xDE); B(0xC9)
#define XFDivp()				B(0xDE); B(0xF9)
#define XFStp_i(r)				B(0xDD); MOD_RM(0,3,r); if( r == Esp ) B(0x24)
#define XFLd_i(r)				B(0xDD); MOD_RM(0,0,r); if( r == Esp ) B(0x24)
#define XFILd_i(r)				B(0xDB); MOD_RM(0,0,r); if( r == Esp ) B(0x24)

#define MAX_OP_SIZE				64

#define TODO()					printf("TODO(jit.c:%d)\n",__LINE__)

#define LOAD(cpuReg,vReg)		XMov_rp(cpuReg,Ebp,-ctx->regsPos[vReg])
#define STORE(vReg, cpuReg)		XMov_pr(Ebp,-ctx->regsPos[vReg],cpuReg)

typedef struct jlist jlist;
struct jlist {
	int pos;
	int target;
	jlist *next;
};

struct jit_ctx {
	union {
		unsigned char *b;
		unsigned int *w;
		int *i;
	} buf;
	int *regsPos;
	int *regsSize;
	int *opsPos;
	int maxRegs;
	int maxOps;
	unsigned char *startBuf;
	int bufSize;
	int totalRegsSize;
	hl_module *m;
	hl_function *f;
	jlist *jumps;
	hl_alloc alloc;
};

static void jit_buf( jit_ctx *ctx ) {
	if( ctx->buf.b - ctx->startBuf > ctx->bufSize - MAX_OP_SIZE ) {
		int nsize = ctx->bufSize ? (ctx->bufSize * 4) / 3 : ctx->f->nops * 4;
		unsigned char *nbuf = (unsigned char*)malloc(nsize);
		int curpos = ctx->buf.b - ctx->startBuf;
		// TODO : check nbuf
		if( ctx->startBuf ) {
			memcpy(nbuf,ctx->startBuf,curpos);
			free(ctx->startBuf);
		}
		ctx->startBuf = nbuf;
		ctx->buf.b = nbuf + curpos;
		ctx->bufSize = nsize;
	}
}

// TODO : variants when size of register != 32 bits !

static void op_mov( jit_ctx *ctx, int to, int from ) {
	LOAD(Eax,from);
	STORE(to, Eax);
}

static void op_movc( jit_ctx *ctx, int to, void *value ) {
	XMov_pc(Ebp,-ctx->regsPos[to],*(int*)value);
}

static void op_mova( jit_ctx *ctx, int to, void *value ) {
	XMov_ra(Eax,(int)value);
	STORE(to, Eax);
}

static void op_pushr( jit_ctx *ctx, int r ) {
	XPush_p(Ebp,-ctx->regsPos[r]);
}

static void op_callr( jit_ctx *ctx, int r, int rf, int size ) {
	LOAD(Eax, rf);
	XCall_r(Eax);
	XAdd_rc(Esp, size);
	STORE(r, Eax);
}

static void op_enter( jit_ctx *ctx ) {
	XPush_r(Ebp);
	XMov_rr(Ebp, Esp);
	XAdd_rc(Esp, ctx->totalRegsSize);
}

static void op_ret( jit_ctx *ctx, int r ) {
	LOAD(Eax, r);
	XSub_rc(Esp, ctx->totalRegsSize);
	XPop_r(Ebp);
	XRet();
}

static void op_sub( jit_ctx *ctx, int r, int a, int b ) {
	LOAD(Eax, a);
	LOAD(Ecx, b);
	XSub_rr(Eax, Ecx);
	STORE(r, Eax);
}

static void op_add( jit_ctx *ctx, int r, int a, int b ) {
	LOAD(Eax, a);
	LOAD(Ecx, b);
	XAdd_rr(Eax, Ecx);
	STORE(r, Eax);
}

static int *do_jump( jit_ctx *ctx, hl_op op ) {
	int *j;
	switch( op ) {
	case OJAlways:
		XJump(JAlways,j);
		break;
	case OGte:
		XJump(JGte,j);
		break;
	default:
		j = NULL;
		printf("Unknown JUMP %d\n",op);
		break;
	}
	return j;
}

static void patch_jump( jit_ctx *ctx, int *p ) {
	if( p == NULL ) return;
	*p = (int)((int_val)ctx->buf.b - ((int_val)p + 1)) - 3;
}

static void op_cmp( jit_ctx *ctx, hl_opcode *op ) {
	int *p,*e;
	LOAD(Eax, op->p2);
	LOAD(Ecx, op->p3);
	XCmp_rr(Eax, Ecx);
	p = do_jump(ctx,op->op);
	XMov_pc(Ebp,-ctx->regsPos[op->p1],0);
	e = do_jump(ctx,OJAlways);
	XMov_pc(Ebp,-ctx->regsPos[op->p1],1);
	patch_jump(ctx,p);
	patch_jump(ctx,e);
}

static void register_jump( jit_ctx *ctx, int *p, int target ) {
	int pos = (int_val)p - (int_val)ctx->startBuf; 
	jlist *j = (jlist*)hl_malloc(&ctx->alloc, sizeof(jlist));
	j->pos = pos;
	j->target = target;
	j->next = ctx->jumps;
	ctx->jumps = j;
}

jit_ctx *hl_jit_alloc() {
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	if( ctx == NULL ) return NULL;
	memset(ctx,0,sizeof(jit_ctx));
	hl_alloc_init(&ctx->alloc);
	return ctx;
}

void hl_jit_free( jit_ctx *ctx ) {
	free(ctx->regsPos);
	free(ctx->regsSize);
	free(ctx->opsPos);
	free(ctx->startBuf);
	hl_free(&ctx->alloc);
	free(ctx);
}

int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f ) {
	int i, j, size = 0;
	int codePos = ctx->buf.b - ctx->startBuf;
	ctx->m = m;
	ctx->f = f;
	if( f->nregs > ctx->maxRegs ) {
		free(ctx->regsPos);
		free(ctx->regsSize);
		ctx->regsPos = (int*)malloc(sizeof(int) * f->nregs);
		ctx->regsSize = (int*)malloc(sizeof(int) * f->nregs);
		if( ctx->regsPos == NULL || ctx->regsSize == NULL ) {
			ctx->maxRegs = 0;
			return -1;
		}
		ctx->maxRegs = f->nregs;
	}
	if( f->nops >= ctx->maxOps ) {
		free(ctx->opsPos);
		ctx->opsPos = (int*)malloc(sizeof(int) * (f->nops + 1));
		if( ctx->opsPos == NULL ) {
			ctx->maxOps = 0;
			return -1;
		}
		ctx->maxOps = f->nops;
	}
	for(i=0;i<f->nregs;i++) {
		int sz = hl_type_size(f->regs[i]);
		ctx->regsSize[i] = sz;
		ctx->regsPos[i] = size;
		size += sz;
	}
	ctx->totalRegsSize = size;
	jit_buf(ctx);
	op_enter(ctx);
	ctx->opsPos[0] = 0;
	for(i=0;i<f->nops;i++) {
		int *jump;
		hl_opcode *o = f->ops + i;
		jit_buf(ctx);
		switch( o->op ) {
		case OMov:
			op_mov(ctx, o->p1, o->p2);
			break;
		case OInt:
			op_movc(ctx, o->p1, m->code->ints + o->p2);
			break;
		case OFloat:
			op_movc(ctx, o->p1, m->code->floats + o->p2);
			break;
		case OGetGlobal:
			op_mova(ctx, o->p1, m->globals_data + m->globals_indexes[o->p2]);
			break;
		case OCallN:
			size = 0;
			for(j=o->p3-1;j>=0;j--) {
				int r = ((int*)o->extra)[j];
				op_pushr(ctx, r);
				size += 4; // regsize !
			}
			op_callr(ctx, o->p1, o->p2, size);
			break;
		case OSub:
			op_sub(ctx, o->p1, o->p2, o->p3);
			break;
		case OAdd:
			op_add(ctx, o->p1, o->p2, o->p3);
			break;
		case OGte:
			op_cmp(ctx, o);
			break;
		case OJFalse:
			LOAD(Eax,o->p1);
			XTest_rr(Eax,Eax);
			XJump(JZero,jump);
			register_jump(ctx,jump,(i + 1) + o->p2);
			break;
		case OJToAny:
			// NOP
			break;
		case ORet:
			op_ret(ctx, o->p1);
			break;
		default:
			printf("Don't know how to jit op #%d\n",o->op);
			return -1;
		}
		ctx->opsPos[i+1] = ctx->buf.b - ctx->startBuf;
	}
	// patch jumps
	{
		jlist *j = ctx->jumps;
		while( j ) {
			*(int*)(ctx->startBuf + j->pos) = ctx->opsPos[j->target] - j->pos;
			j = j->next;
		}
		ctx->jumps = NULL;
	}
	// reset tmp allocator
	hl_free(&ctx->alloc);
	return codePos;
}

void *hl_alloc_executable_memory( int size );

void *hl_jit_code( jit_ctx *ctx ) {
	int size = ctx->buf.b - ctx->startBuf;
	void *code = hl_alloc_executable_memory(size);
	if( code == NULL ) return NULL;
	memcpy(code,ctx->startBuf,size);
	return code;
}
