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
#define W64(wv)	*ctx->buf.w64++ = wv
#define REX_W	(64|8)
#define REX_R	(64|4)
#define REX_X	(64|2)
#define REX_B	(64|1)
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

#define XMov_rc32(dst,cst)		B(0xB8+(dst)); W(cst)
#define XMov_rc64(dst,cst)		B(REX_W); B(0xB8+(dst)); W64(cst)
#define XMov_ra32(dst,addr)		OP_RM(0x8B,0,dst,5); W(addr)
#define XMov_ra64(dst,addr)		B(REX_W); OP_RM(0x8B,0,dst,5); W64(addr)

#define XMov_rp(dst,reg,idx)	OP_ADDR(0x8B,idx,reg,dst)
#define XMov_rp64(dst,reg,idx)	B(REX_W); OP_ADDR(0x8B,idx,reg,dst)

#define XMov_rx(dst,r,idx,mult) OP_RM(0x8B,0,dst,4); SIB(Mult##mult,idx,r)
#define XMov_pr(dst,idx,src)	OP_ADDR(0x89,idx,dst,src)
#define XMov_pc(dst,idx,c)		OP_ADDR(0xC7,idx,dst,0); W(c)
#define XMov_ar(addr,reg)		B(0x3E); if( reg == Eax ) { B(0xA3); } else { OP_RM(0x89,0,reg,5); }; W(addr)
#define XMov_xr(r,idx,mult,src) OP_RM(0x89,0,src,4); SIB(Mult##mult,idx,r)
#define XCall_r(r)				OP_RM(0xFF,3,2,r)
#define XCall_d(delta)			B(0xE8); W(delta)
#define XPush_r(r)				B(0x50+(r))
#define XPush_c(cst)			B(0x68); W(cst)
#define XPush_p(reg,idx)		OP_ADDR(0xFF,idx,reg,6)
#define XAdd_rc(reg,cst)		if IS_SBYTE(cst) { OP_RM(0x83,3,0,reg); B(cst); } else { OP_RM(0x81,3,0,reg); W(cst); }
#define XAdd_rr(dst,src)		OP_RM(0x03,3,dst,src)

#define XSub_rc(reg,cst)		if IS_SBYTE(cst) { OP_RM(0x83,3,5,reg); B(cst); } else { OP_RM(0x81,3,5,reg); W(cst); }
#define XSub_rr(dst,src)		OP_RM(0x2B,3,dst,src)
#define XSub_pr(reg,idx,src)	OP_ADDR(0x29,idx,reg,src)
#define XAdd_pr(dst,idx,src)	OP_ADDR(0x01,idx,dst,src)
#define XAdd_rp(dst,reg,idx)	OP_ADDR(0x03,idx,reg,dst)

#define XCmp_rp(dst,reg,idx)	OP_ADDR(0x3B,idx,reg,dst)
#define XCmp_pr(dst,reg,idx)	OP_ADDR(0x39,idx,reg,dst)
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

#define XNop()					B(0x90)				

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

#define BUF_POS()				((int)(ctx->buf.b - ctx->startBuf))
#define RTYPE(r)				r->t->kind

#ifdef HL_64
#	define XMov_rc	XMov_rc64
#	define XMov_ra	XMov_ra64
#else
#	define XMov_rc	XMov_rc32
#	define XMov_ra	XMov_ra32
#endif

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
	RCONST = 3
} preg_kind;

typedef struct {
	int id;
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
#else
#	define RCPU_COUNT	6
#	define RFPU_COUNT	8
#	define RCPU_SCRATCH_COUNT	3
static int CPU_REGS[] = { Eax, Ecx, Edx, Ebx, Esi, Edi };
#	define PEAX			REG_AT(0)
#endif

#define REG_COUNT	(RCPU_COUNT + RFPU_COUNT)

#define ID2(a,b)	((a) | ((b)<<8))
#define R(id)		(ctx->vregs + (id))
#define ASSERT(i)	{ printf("JIT ERROR %d@%d\n",i,__LINE__); exit(-1); }

struct jit_ctx {
	union {
		unsigned char *b;
		unsigned int *w;
		unsigned long long *w64;
		int *i;
	} buf;
	vreg *vregs;
	preg pregs[REG_COUNT];
	int *opsPos;
	int maxRegs;
	int maxOps;
	unsigned char *startBuf;
	int bufSize;
	int totalRegsSize;
	int *globalToFunction;
	int functionPos;
	int allocOffset;
	hl_module *m;
	hl_function *f;
	jlist *jumps;
	jlist *calls;
	hl_alloc falloc; // cleared per-function
	hl_alloc galloc;
};

static void jit_buf( jit_ctx *ctx ) {
	if( BUF_POS() > ctx->bufSize - MAX_OP_SIZE ) {
		int nsize = ctx->bufSize ? (ctx->bufSize * 4) / 3 : ctx->f->nops * 4;
		unsigned char *nbuf;
		int curpos;
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

static void patch_jump( jit_ctx *ctx, int *p ) {
	if( p == NULL ) return;
	*p = (int)((int_val)ctx->buf.b - ((int_val)p + 1)) - 3;
}

static preg *alloc_reg( jit_ctx *ctx, preg_kind k ) {
	int i, start, count;
	int off = ctx->allocOffset++;
	if( k == RCPU ) {
		start = 0;
		count = RCPU_COUNT;
	} else if( k == RFPU ) {
		start = RCPU_COUNT;
		count = RFPU_COUNT;
	} else
		ASSERT(k);
	for(i=0;i<count;i++) {
		int r = start + ((i + off)%count);
		if( ctx->pregs[r].holds == NULL )
			return ctx->pregs + r;
	}
	i = start + (off%count);
	return ctx->pregs + i;
}

static preg *fetch( vreg *r ) {
	if( r->current )
		return r->current;
	return &r->stack;
}

static void scratch( preg *r ) {
	if( r->holds ) {
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
	if( r->kind != RCPU ) ASSERT(r->kind);
	switch( from->kind ) {
	case RCPU:
		XMov_rr(CPU_REGS[r->id],CPU_REGS[from->id]);
		break;
	case RSTACK:
		XMov_rp(CPU_REGS[r->id],Ebp,R(from->id)->stackPos);
		break;
	default:
		ASSERT(from->kind);
	}
}

static preg *alloc_cpu( jit_ctx *ctx, vreg *r ) {
	preg *p = fetch(r);
	p = alloc_reg(ctx, RCPU);
	load(ctx, p, r);
	return p;
}

static void store( jit_ctx *ctx, vreg *r, preg *v ) {
	if( r->current && r->current != v ) {
		r->current->holds = NULL;
		r->current = NULL;
	}
	switch( r->size ) {
	case 0:
		break;
	case 4:
		switch( v->kind ) {
		case RCPU:
			XMov_pr(Ebp,r->stackPos,CPU_REGS[v->id]);
			if( v->holds == NULL ) {
				r->current = v;
				v->holds = r;
			}
			break;
		case RSTACK:
			store(ctx,r,alloc_cpu(ctx, R(v->id)));
			break;
		default:
			ASSERT(v->kind);
		}
		break;
	default:
		ASSERT(r->size);
	}
}

static void op_mov( jit_ctx *ctx, vreg *to, vreg *from ) {
	preg *r = fetch(from);
	store(ctx, to, r);
}

static void store_const( jit_ctx *ctx, vreg *r, int v, bool useTmpReg ) {
	if( r->size != 4 ) ASSERT(r->size);
	if( useTmpReg ) {
		if( !r->current || r->current->kind != RCPU ) {
			if( r->current )
				r->current->holds = NULL;
			r->current = alloc_reg(ctx, RCPU);
		}
		XMov_rc(CPU_REGS[r->current->id],v);
		store(ctx,r,r->current);
	} else {
		if( r->current ) {
			r->current->holds = NULL;
			r->current = NULL;
		}
		XMov_pc(Ebp, r->stackPos, v); 
	}
}

static void op_mova( jit_ctx *ctx, vreg *to, void *value ) {
	preg *r = alloc_cpu(ctx, to);
	XMov_ra(CPU_REGS[r->id],(int_val)value);
	store(ctx, to, r);
}

static void discard_regs( jit_ctx *ctx, int native_call ) {
	int i;
	for(i=(native_call ? RCPU_SCRATCH_COUNT : 0);i<RCPU_COUNT;i++) {
		preg *r = ctx->pregs + i;
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
			ASSERT(0); // Need FPU stack reset ?
		}
	}
}

static int pad_stack( jit_ctx *ctx, int size ) {
	int total = size + ctx->totalRegsSize + HL_WSIZE * 2; // EIP+EBP
	if( total & 15 ) {
		int pad = 16 - (total & 15);
		XSub_rc(Esp,pad);
		size += pad;
	}
	return size;
}

static void stack_align_error( int r ) {
	printf("STACK ALIGN ERROR %d !\n",r&15);
	exit(1);
}

//#define CHECK_STACK_ALIGN

static void op_callg( jit_ctx *ctx, vreg *dst, int g, int count, int *args ) {
	int fid = ctx->globalToFunction[g];
	int i, size = 0;
	for(i=0;i<count;i++) {
		// TODO : padding ?
		size += R(args[i])->size;
	}
	size = pad_stack(ctx,size);
	for(i=0;i<count;i++) {
		// RTL
		vreg *r = R(args[count - (i + 1)]);
		if( (i & 7) == 0 ) jit_buf(ctx);
		if( r->current && r->current->kind == RCPU )
			XPush_r(CPU_REGS[r->current->id]);
		else
			XPush_p(Ebp,r->stackPos);
	}
	if( fid < 0 ) {
		// not a static function or native, load it at runtime
		XMov_ra(Eax, (int_val)(ctx->m->globals_data + ctx->m->globals_indexes[g]));
		XCall_r(Eax);
		discard_regs(ctx, 1);
	} else if( fid >= ctx->m->code->nfunctions ) {
#		ifdef HL_64
		// TODO ! native x64 calling convention are not __cdecl !
		// args needs to be passed in registers
		XMov_rp64(Ecx, Esp, 0);
#		endif
		// native function, already resolved
		XMov_rc(Eax, *(int_val*)(ctx->m->globals_data + ctx->m->globals_indexes[g]));
#		ifdef CHECK_STACK_ALIGN
		{
			int *j;
			XTest_rc(Esp,15);
			XJump(JZero,j);
			XPush_r(Esp);
			XMov_rc(Eax, (int_val)stack_align_error);
			patch_jump(ctx,j);
		}
#		endif
		XCall_r(Eax);
		discard_regs(ctx, 0);
	} else {
		int cpos = BUF_POS();
		if( ctx->m->functions_ptrs[fid] ) {
			// already compiled
			XCall_d((int)ctx->m->functions_ptrs[fid] - (cpos + 5));
		} else if( ctx->m->code->functions + fid == ctx->f ) {
			// our current function
			XCall_d(ctx->functionPos - (cpos + 5));
		} else {
			// stage for later
			jlist *j = (jlist*)hl_malloc(&ctx->galloc,sizeof(jlist));
			j->pos = cpos;
			j->target = g;
			j->next = ctx->calls;
			ctx->calls = j;
			XCall_d(0);
		}
		discard_regs(ctx, 1);
	}
	XAdd_rc(Esp, size);
	store(ctx, dst, PEAX);
}

static void op_enter( jit_ctx *ctx ) {
	XPush_r(Ebp);
	XMov_rr(Ebp, Esp);
	XSub_rc(Esp, ctx->totalRegsSize);
}

static void op_ret( jit_ctx *ctx, vreg *r ) {
	load(ctx, PEAX, r);
	XAdd_rc(Esp, ctx->totalRegsSize);
	XPop_r(Ebp);
	XRet();
}

static preg *op_binop( jit_ctx *ctx, vreg *dst, vreg *a, vreg *b, hl_opcode *op ) {
	preg *pa = fetch(a), *pb = fetch(b), *out = NULL;
	switch( RTYPE(a) ) {
	case HI32:
		switch( ID2(pa->kind, pb->kind) ) {
		case ID2(RCPU,RCPU):
			switch( op->op ) {
			case OAdd:
				XAdd_rr(CPU_REGS[pa->id],CPU_REGS[pb->id]);
				scratch(pa);
				out = pa;
				break;
			case OSub:
				XSub_rr(CPU_REGS[pa->id],CPU_REGS[pb->id]);
				scratch(pa);
				out = pa;
				break;
			case OGte:
			case OLt:
			case OJLt:
			case OJGte:
				XCmp_rr(CPU_REGS[pa->id],CPU_REGS[pb->id]);
				break;
			default:
				printf("%s\n", hl_op_name(op->op));
				ASSERT(op->op);
			}
			break;
		case ID2(RCPU,RSTACK):
			switch( op->op ) {
			case OGte:
			case OLt:
			case OJLt:
			case OJGte:
				XCmp_rp(CPU_REGS[pa->id],Ebp,R(pb->id)->stackPos);
				break;
			case OAdd:
				XAdd_rp(CPU_REGS[pa->id],Ebp,R(pb->id)->stackPos);
				scratch(pa);
				out = pa;
				break;
			default:
				printf("%s\n", hl_op_name(op->op));
				ASSERT(op->op);
			}
			break;
		case ID2(RSTACK,RCPU):
			if( dst == a ) {
				switch( op->op ) {
				case OSub:
					XSub_pr(Ebp,R(pa->id)->stackPos,CPU_REGS[pb->id]);
					break;
				case OAdd:
					XAdd_pr(Ebp,R(pa->id)->stackPos,CPU_REGS[pb->id]);
					break;
				default:
					printf("%s\n", hl_op_name(op->op));
					ASSERT(op->op);
				}
				dst = NULL;
				out = pa;
			} else {
				alloc_cpu(ctx,a);
				return op_binop(ctx,dst,a,b,op);
			}
			break;
		case ID2(RSTACK,RSTACK):
			alloc_cpu(ctx, a);
			return op_binop(ctx, dst, a, b, op);
		default:
			printf("%s(%d,%d)\n", hl_op_name(op->op), pa->kind, pb->kind);
			ASSERT(ID2(pa->kind, pb->kind));
		}
		if( dst ) store(ctx, dst, out);
		return out;
	default:
		ASSERT(RTYPE(dst));
	}
	return NULL;
}

static int *do_jump( jit_ctx *ctx, hl_op op ) {
	int *j;
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
		j = NULL;
		printf("Unknown JUMP %d\n",op);
		break;
	}
	return j;
}

static void op_cmp( jit_ctx *ctx, hl_opcode *op ) {
	int *p, *e;
	op_binop(ctx,NULL,R(op->p2),R(op->p3),op);
	p = do_jump(ctx,op->op);
	store_const(ctx, R(op->p1), 0, false);
	e = do_jump(ctx,OJAlways);
	patch_jump(ctx,p);
	store_const(ctx, R(op->p1), 1, false);
	patch_jump(ctx,e);
}

static void register_jump( jit_ctx *ctx, int *p, int target ) {
	int pos = (int)((int_val)p - (int_val)ctx->startBuf); 
	jlist *j = (jlist*)hl_malloc(&ctx->falloc, sizeof(jlist));
	j->pos = pos;
	j->target = target;
	j->next = ctx->jumps;
	ctx->jumps = j;
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
	free(ctx->globalToFunction);
	hl_free(&ctx->falloc);
	hl_free(&ctx->galloc);
	free(ctx);
}

int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f ) {
	int i, size = 0;
	int codePos = BUF_POS();
	int nargs = m->code->globals[f->global]->nargs;
	ctx->m = m;
	ctx->f = f;
	if( !ctx->globalToFunction ) {
		ctx->globalToFunction = (int*)malloc(sizeof(int)*m->code->nglobals);
		memset(ctx->globalToFunction,0xFF,sizeof(int)*m->code->nglobals);
		for(i=0;i<m->code->nfunctions;i++)
			ctx->globalToFunction[(m->code->functions + i)->global] = i;
		for(i=0;i<m->code->nnatives;i++)
			ctx->globalToFunction[(m->code->natives + i)->global] = i + m->code->nfunctions;
	}
	if( f->nregs > ctx->maxRegs ) {
		free(ctx->vregs);
		ctx->vregs = (vreg*)malloc(sizeof(vreg) * f->nregs);
		if( ctx->vregs == NULL ) {
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
	size = 0;
	for(i=0;i<f->nregs;i++) {
		vreg *r = R(i);
		r->t = f->regs[i];
		r->current = NULL;
		r->size = hl_type_size(r->t);
		// TODO : hl_pad_size ?
		if( i >= nargs ) {
			if( i == nargs ) size = 0;
			r->stackPos = -(size + HL_WSIZE);
			size += r->size;
		} else {
			size += r->size;
			r->stackPos = size + HL_WSIZE;
		}
		r->stack.holds = NULL;
		r->stack.id = i;
		r->stack.kind = RSTACK;
	}
	if( f->nregs == nargs ) size = 0;
	ctx->totalRegsSize = size;
	jit_buf(ctx);
	ctx->functionPos = BUF_POS();
	op_enter(ctx);
	ctx->opsPos[0] = 0;
	for(i=0;i<f->nops;i++) {
		int *jump;
		hl_opcode *o = f->ops + i;
		jit_buf(ctx);
		switch( o->op ) {
		case OMov:
			op_mov(ctx, R(o->p1), R(o->p2));
			break;
		case OInt:
			{
				preg *r = R(o->p1)->current;
				if( r == NULL || r->kind != RCPU ) r = alloc_reg(ctx, RCPU);
				XMov_rc(CPU_REGS[r->id], m->code->ints[o->p2]);
				store(ctx,R(o->p1),r);
			}
			break;
		case OGetGlobal:
			op_mova(ctx, R(o->p1), m->globals_data + m->globals_indexes[o->p2]);
			break;
		case OCall3:
		case OCall4:
		case OCallN:
			op_callg(ctx, R(o->p1), o->p2, o->p3, o->extra);
			break;
		case OCall0:
			op_callg(ctx, R(o->p1), o->p2, 0, NULL);
			break;
		case OCall1:
			op_callg(ctx, R(o->p1), o->p2, 1, &o->p3);
			break;
		case OCall2:
			{
				int args[2];
				args[0] = o->p3;
				args[1] = (int)o->extra;
				op_callg(ctx, R(o->p1), o->p2, 2, args);
			}
			break;
		case OSub:
		case OAdd:
		case OMul:
		case ODiv:
			op_binop(ctx, R(o->p1), R(o->p2), R(o->p3), o);
			break;
		case OGte:
			op_cmp(ctx, o);
			break;
		case OJFalse:
			{
				preg *r = alloc_cpu(ctx, R(o->p1));
				XTest_rr(CPU_REGS[r->id],CPU_REGS[r->id]);
				XJump(JZero,jump);
				register_jump(ctx,jump,(i + 1) + o->p2);
			}
			break;
		case OJLt:
			op_binop(ctx, NULL, R(o->p1), R(o->p2), o);
			jump = do_jump(ctx,o->op);
			register_jump(ctx,jump,(i + 1) + o->p3);
			break;
		case OToAny:
			op_mov(ctx,R(o->p1),R(o->p2)); // TODO
			break;
		case ORet:
			op_ret(ctx, R(o->p1));
			break;
		default:
			printf("Don't know how to jit %s(%d)\n",hl_op_name(o->op),o->op);
			return -1;
		}
		ctx->opsPos[i+1] = BUF_POS();
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
		XNop();
	// reset tmp allocator
	hl_free(&ctx->falloc);
	return codePos;
}

void *hl_alloc_executable_memory( int size );

void *hl_jit_code( jit_ctx *ctx, hl_module *m ) {
	jlist *c;
	int size = BUF_POS();
	unsigned char *code = (unsigned char*)hl_alloc_executable_memory(size);
	if( code == NULL ) return NULL;
	memcpy(code,ctx->startBuf,size);
	// patch calls
	c = ctx->calls;
	while( c ) {
		int fid = ctx->globalToFunction[c->target];
		int fpos = (int)m->functions_ptrs[fid];
		*(int*)(code + c->pos + 1) = fpos - (c->pos + 5);
		c = c->next;
	}
	return code;
}

