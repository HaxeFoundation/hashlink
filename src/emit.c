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
	PHI,
	ALLOC_STACK,
	FREE_STACK,
	NATIVE_REG,
	PREFETCH,
	DEBUG_BREAK,
} emit_op;

static const char *op_names[] = {
	"load-addr",
	"load-imm",
	"load-arg",
	"store",
	"lea",
	"test",
	"cmp",
	"jcond",
	"jump",
	"jump-table",
	"binop",
	"unop",
	"conv",
	"conv-unsigned",
	"ret",
	"call",
	"call",
	"call",
	"phi",
	"alloc-stack",
	"free-stack",
	"native-reg",
	"prefetch",
	"debug-break",
};

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
	hl_type *t;
	int id;
	ereg current;
	bool written;
} vreg;

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

#define MAX_TMP_ARGS	32
#define MAX_TRAPS		32
#define MAX_REFS		512 // TODO : different impl

typedef struct _linked_inf linked_inf;
typedef struct _emit_block emit_block;

struct _linked_inf {
	int id;
	void *ptr;
	linked_inf *next;
};

struct _emit_block {
	int id;
	int start_pos;
	int wait_nexts;
	linked_inf *preds;
	linked_inf *written_vars;
	linked_inf *incomplete_phis;
};

typedef struct {
	int *data;
	int max;
	int cur;
} int_alloc;

struct _emit_ctx {
	hl_module *mod;
	hl_function *fun;
	einstr *instrs;
	vreg *vregs;
	int max_instrs;
	int max_regs;
	int emit_pos;
	int op_pos;

	ereg tmp_args[MAX_TMP_ARGS];
	ereg traps[MAX_TRAPS];
	struct {
		ereg r;
		int reg;
	} refs[MAX_REFS];
	int *pos_map;
	int pos_map_size;
	int trap_count;
	int ref_count;

	int_alloc args_data;
	int_alloc jump_regs;
	int_alloc block_writes;
	int_alloc phi_gather;
	
	hl_alloc falloc;
	emit_block *current_block;
	linked_inf *arrival_points;
	void *closure_list; // TODO : patch with good addresses
}; 

#define R(i)	(ctx->vregs + (i))

#define emit_error(msg)	_emit_error(msg,__LINE__)
#define emit_assert()	emit_error("")

#define LOAD(r) emit_load_reg(ctx, r)
#define STORE(r, v) emit_store_reg(ctx, r, v)
#define LOAD_CONST(v, t) emit_load_const(ctx, (uint64)(v), t)
#define LOAD_CONST_PTR(v) LOAD_CONST(v,&hlt_bytes)
#define LOAD_MEM(v, offs, t) emit_load_mem(ctx, v, offs, t)
#define LOAD_MEM_PTR(v, offs) LOAD_MEM(v, offs, &hlt_bytes)
#define STORE_MEM(to, offs, v) emit_store_mem(ctx, to, offs, v)
#define LOAD_OBJ_METHOD(obj,id) LOAD_MEM_PTR(LOAD_MEM_PTR(LOAD_MEM_PTR(obj,0),HL_WSIZE*2),HL_WSIZE*(id))
#define OFFSET(base,index,mult,offset) emit_gen_ext(ctx, LEA, base, index, M_PTR, (mult) | ((offset) << 8))
#define BREAK() emit_gen(ctx, DEBUG_BREAK, ENULL, ENULL, 0)
#define CUR_REG() __current_reg(ctx)

#define HDYN_VALUE 8

#define IS_FLOAT(t)	((t)->kind == HF64 || (t)->kind == HF32)

static hl_type hlt_ui8 = { HUI8, 0 };
static hl_type hlt_ui16 = { HUI16, 0 };
static ereg ENULL = {-1};
static emit_ctx *current_ctx = NULL;

//#define BLOCK_DEBUG

static void _emit_error( const char *msg, int line ) {
	printf("*** EMIT ERROR line %d (%s) ****\n", line, msg);
	if( current_ctx ) hl_emit_dump(current_ctx);
	hl_debug_break();
	fflush(stdout);
	exit(-1);
}

static void hl_stub_null_field_access() { emit_assert(); }
static void hl_stub_null_access() { emit_assert(); }
static void hl_stub_assert() { emit_assert(); }

static void int_alloc_reset( int_alloc *a ) {
	a->cur = 0;
}

static void int_alloc_free( int_alloc *a ) {
	free(a->data);
	a->cur = 0;
	a->max = 0;
	a->data = NULL;
}

static int *int_alloc_get( int_alloc *a, int count ) {
	while( a->cur + count > a->max ) {
		int next_size = a->max ? a->max << 1 : 128; 
		int *new_data = (int*)malloc(sizeof(int) * next_size);
		if( new_data == NULL ) emit_error("Out of memory");
		memcpy(new_data, a->data, sizeof(int) * a->cur);
		free(a->data);
		a->data = new_data;
		a->max = next_size;
	}
	int *ptr = a->data + a->cur;
	a->cur += count;
	return ptr;
}

static void int_alloc_store( int_alloc *a, int v ) {
	*int_alloc_get(a,1) = v;
}

static void int_alloc_store_unique( int_alloc *a, int v ) {
	int i = 0;
	while( i < a->cur ) {
		if( a->data[i] == v ) return;
		i++;
	}
	*int_alloc_get(a,1) = v;
}

static linked_inf *link_add( emit_ctx *ctx, int id, void *ptr, linked_inf *head ) {
	linked_inf *l = hl_malloc(&ctx->falloc,sizeof(linked_inf));
	l->id = id;
	l->ptr = ptr;
	l->next = head;
	return l;
}

static linked_inf *link_add_sort_unique( emit_ctx *ctx, int id, void *ptr, linked_inf *head ) {
	linked_inf *prev = NULL;
	linked_inf *cur = head;
	while( cur && cur->id < id ) {
		prev = cur;
		cur = cur->next;
	}
	// check duplicate
	while( cur && cur->id == id ) {
		if( cur->ptr == ptr )
			return head;
		cur = cur->next;
	}
	// insert
	linked_inf *l = hl_malloc(&ctx->falloc,sizeof(linked_inf));
	l->id = id;
	l->ptr = ptr;
	if( !prev ) {
		l->next = head;
		return l;
	} else {
		l->next = prev->next;
		prev->next = l;
		return head;
	}
}

static void *link_sort_lookup( linked_inf *head, int id ) {
	while( head && head->id < id )
		head = head->next;
	if( head && head->id == id )
		return head->ptr;
	return NULL;
}

static emit_mode hl_type_mode( hl_type *t ) {
	if( t->kind == HVOID )
		return M_VOID;
	if( t->kind < HBOOL )
		return (emit_mode)t->kind;
	if( t->kind == HBOOL )
		return sizeof(bool) == 1 ? M_UI8 : M_I32;
	if( t->kind == HGUID )
		return M_I64;
	return M_PTR;
}

static void mark_no_return( emit_ctx *ctx ) {
	ctx->instrs[ctx->emit_pos-1].mode = M_NORET;
}

void hl_jit_patch_method( void*fun, void**newt ) {
	emit_assert();
}

static ereg __current_reg( emit_ctx *ctx ) {
	ereg r = {ctx->emit_pos-1};
	return r;
}

static ereg *get_tmp_args( emit_ctx *ctx, int count ) {
	if( count > MAX_TMP_ARGS ) emit_error("Too many arguments");
	return ctx->tmp_args;
}

static ereg resolve_ref( emit_ctx *ctx, int reg ) {
	for(int i=0;i<ctx->ref_count;i++) {
		if( ctx->refs[i].reg == reg )
			return ctx->refs[i].r;
	}
	return ENULL;
}

static einstr *emit_instr( emit_ctx *ctx, emit_op op ) {
	if( ctx->emit_pos == ctx->max_instrs ) {
		int pos = ctx->emit_pos;
		int next_size = ctx->max_instrs ? (ctx->max_instrs << 1) : 256;
		einstr *instrs = (einstr*)malloc(sizeof(einstr) * next_size);
		if( instrs == NULL ) emit_error("Out of memory");
		memcpy(instrs, ctx->instrs, pos * sizeof(einstr));
		memset(instrs + pos, 0, (next_size - pos) * sizeof(einstr));
		free(ctx->instrs);
		ctx->instrs = instrs;
		ctx->max_instrs = next_size;
	} else if( (ctx->emit_pos & 0xFF) == 0 )
		memset(ctx->instrs + ctx->emit_pos, 0, 256 * sizeof(einstr));
	einstr *e = ctx->instrs + ctx->emit_pos++;
	e->op = op;
	return e;
}

static void emit_store_mem( emit_ctx *ctx, ereg to, int offs, ereg from ) {
	einstr *e = emit_instr(ctx, STORE);
	e->mode = ctx->instrs[from.index].mode;
	e->size_offs = offs;
	e->a = to;
	e->b = from;
}

static void store_args( emit_ctx *ctx, einstr *e, ereg *args, int count ) {
	if( count < 0 ) emit_assert();
	if( e->op == PHI ) {
		if( count > 0x200 ) emit_error("Too many branches");
	} else {
		if( count > 64 ) emit_error("Too many arguments");
	}
	e->nargs = (unsigned char)count;
	if( count == 0 ) return;
	if( count == 1 ) {
		e->size_offs = args[0].index;
		return;
	}
	if( e->op == PHI && count <= 3 ) {
		memcpy(&e->size_offs, args, sizeof(int) * count);
		return;
	}
	int *args_data = int_alloc_get(&ctx->args_data, count);
	e->size_offs = (int)(args_data - ctx->args_data.data);
	memcpy(args_data, args, sizeof(int) * count);
}

ereg *hl_emit_get_args( emit_ctx *ctx, einstr *e ) {
	if( e->nargs == 0 )
		return NULL;
	if( e->nargs == 1 )
		return (ereg*)&e->size_offs;
	if( e->op == PHI && e->nargs <= 3 )
		return (ereg*)&e->size_offs;
	return (ereg*)(ctx->args_data.data + e->size_offs);
}

static ereg emit_gen_ext( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode, int size_offs ) {
	einstr *e = emit_instr(ctx, op);
	if( (unsigned char)mode != mode ) emit_assert();
	e->mode = (unsigned char)mode;
	e->size_offs = size_offs;
	e->a = a;
	e->b = b;
	return CUR_REG();
}

static ereg emit_gen( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode ) {
	return emit_gen_ext(ctx,op,a,b,mode,0);
}

static ereg emit_gen_size( emit_ctx *ctx, emit_op op, int size_offs ) {
	return emit_gen_ext(ctx,op,ENULL,ENULL,op==ALLOC_STACK ? M_PTR : 0,size_offs);
}

static ereg emit_phi( emit_ctx *ctx, ereg v1, ereg v2 ) {
	unsigned char mode = ctx->instrs[v1.index].mode;
	if( mode != ctx->instrs[v2.index].mode ) emit_assert();
	einstr *e = emit_instr(ctx, PHI);
	e->mode = mode;
	ereg args[2];
	args[0] = v1;
	args[1] = v2;
	store_args(ctx, e, args, 2);
	return CUR_REG();
}

static int emit_jump( emit_ctx *ctx, bool cond ) {
	int p = ctx->emit_pos;
	emit_gen(ctx, cond ? JCOND : JUMP, ENULL, ENULL, 0);
	return p;
}

static void patch_jump( emit_ctx *ctx, int jpos ) {
	ctx->instrs[jpos].size_offs = ctx->emit_pos - (jpos + 1); 
}

static emit_block *alloc_block( emit_ctx *ctx ) {
	return hl_zalloc(&ctx->falloc, sizeof(emit_block));
}

static void block_add_prec( emit_ctx *ctx, emit_block *b, emit_block *p ) {
	b->preds = link_add(ctx,0,p,b->preds);
#	ifdef BLOCK_DEBUG
	printf("  PRED %d\n",p->id);
#	endif
}

static void split_block( emit_ctx *ctx ) {
	// flush previous block
	int i = 0;
	emit_block *cur = ctx->current_block;
	while( i < ctx->block_writes.cur ) {
		vreg *r = R(ctx->block_writes.data[i++]);
		cur->written_vars = link_add_sort_unique(ctx,r->id,(void*)(int_val)(r->current.index+1),cur->written_vars);
#		ifdef BLOCK_DEBUG
		printf("  WRITES R%d @%X\n", r->id, r->current.index);
#		endif
		r->current.index = -1;
		r->written = false;
	}
	int_alloc_reset(&ctx->block_writes);
	// split
	emit_block *b = alloc_block(ctx);
	b->id = ctx->current_block->id + 1;
	b->start_pos = ctx->emit_pos;
#	ifdef BLOCK_DEBUG
	printf("BLOCK #%d@%X\n",b->id,ctx->op_pos);
#	endif
	einstr *eprev = &ctx->instrs[ctx->emit_pos-1];
	if( (eprev->op != JUMP && eprev->op != RET) || ctx->fun->ops[ctx->op_pos].op == OTrap )
		block_add_prec(ctx, b, ctx->current_block);
	while( ctx->arrival_points && ctx->arrival_points->id == ctx->op_pos ) {
		block_add_prec(ctx, b, (emit_block*)ctx->arrival_points->ptr);
		ctx->arrival_points = ctx->arrival_points->next;
	}
	ctx->current_block = b;
}

static void register_jump( emit_ctx *ctx, int jpos, int offs ) {
	int target = offs + ctx->op_pos + 1;
	int_alloc_store(&ctx->jump_regs, jpos);
	int_alloc_store(&ctx->jump_regs, target);
	if( offs > 0 ) {
		ctx->arrival_points = link_add_sort_unique(ctx, target, ctx->current_block, ctx->arrival_points);
		if( ctx->arrival_points->id != ctx->op_pos + 1 && ctx->fun->ops[ctx->op_pos].op != OSwitch )
			split_block(ctx);
	}
}

static ereg emit_load_const( emit_ctx *ctx, uint64 value, hl_type *size_t ) {
	einstr *e = emit_instr(ctx, LOAD_IMM);
	e->mode = hl_type_mode(size_t);
	e->value = value;
	return CUR_REG();
}

static ereg emit_load_mem( emit_ctx *ctx, ereg v, int offset, hl_type *size_t ) {
	einstr *e = emit_instr(ctx, LOAD_ADDR);
	e->mode = hl_type_mode(size_t);
	e->a = v;
	e->b.index = offset;
	return CUR_REG();
}

static void emit_store_reg( emit_ctx *ctx, vreg *to, ereg v ) {
	if( to->t->kind == HVOID ) return;
	if( v.index < 0 ) emit_assert();
	if( !to->written ) {
		to->written = true;
		int_alloc_store(&ctx->block_writes,to->id);
	}
	to->current = v;
}

static ereg emit_native_call( emit_ctx *ctx, void *native_ptr, ereg args[], int nargs, hl_type *ret ) {
	einstr *e = emit_instr(ctx, CALL_PTR);
	e->mode = hl_type_mode(ret);
	e->value = (int_val)native_ptr;
	store_args(ctx, e, args, nargs);
	return CUR_REG();
}

static ereg emit_dyn_call( emit_ctx *ctx, ereg f, ereg args[], int nargs, hl_type *ret ) {
	einstr *e = emit_instr(ctx, CALL_REG);
	e->mode = hl_type_mode(ret);
	e->a = f;
	store_args(ctx, e, args, nargs);
	return CUR_REG();
}

static void emit_test( emit_ctx *ctx, ereg v, hl_op o ) {
	emit_gen_ext(ctx, TEST, v, ENULL, ctx->instrs[v.index].mode, o);
}

static void emit_gather_phi_rec( emit_ctx *ctx, emit_block *b, vreg *r ) {
	if( b->wait_nexts > 0 ) {
		if( ctx->instrs[ctx->emit_pos-1].op != PHI ) emit_assert();
		int_alloc_store_unique(&ctx->phi_gather, -1);
		b->incomplete_phis = link_add_sort_unique(ctx, ctx->emit_pos - 1, r, b->incomplete_phis);
		return;
	}
	int eid = (int)(int_val)link_sort_lookup(b->written_vars, r->id) - 1;
	if( eid >= 0 ) {
		if( eid != ctx->emit_pos - 1 )
			int_alloc_store_unique(&ctx->phi_gather, eid);
	} else {
		b->written_vars = link_add_sort_unique(ctx, r->id, (void*)(int_val)ctx->emit_pos, b->written_vars);
		linked_inf *l = b->preds;
		while( l ) {
			emit_gather_phi_rec(ctx, (emit_block*)l->ptr, r);
			l = l->next;
		}
	}
}

static ereg emit_load_reg( emit_ctx *ctx, vreg *r ) {
	if( r->current.index < 0 ) {
		ereg ref = resolve_ref(ctx, r->id);
		if( ref.index >= 0 )
			return LOAD_MEM(ref,0,r->t);
		einstr *p = emit_instr(ctx, PHI);
		p->mode = hl_type_mode(r->t);
		int_alloc_reset(&ctx->phi_gather);
		emit_gather_phi_rec(ctx, ctx->current_block, r);
		if( ctx->phi_gather.cur == 1 && ctx->phi_gather.data[0] != -1 ) {
#			ifdef BLOCK_DEBUG
			printf("  SKIP-PHI @%X\n",ctx->emit_pos-1);
#			endif
			// remove phi
			ctx->emit_pos--;
			r->current.index = ctx->phi_gather.data[0];
		} else {
			if( ctx->phi_gather.cur == 0 ) emit_assert();
			store_args(ctx, p, (ereg*)ctx->phi_gather.data, ctx->phi_gather.cur);
			r->current = CUR_REG();
		}
	}
	return r->current;
}

static void emit_call_fun( emit_ctx *ctx, vreg *dst, int findex, int count, int *args_regs ) {
	hl_module *m = ctx->mod;
	int fid = m->functions_indexes[findex];
	bool isNative = fid >= m->code->nfunctions;
	ereg *args = get_tmp_args(ctx, count);
	for(int i=0;i<count;i++)
		args[i] = LOAD(R(args_regs[i]));
	if( isNative )
		STORE(dst, emit_native_call(ctx, m->functions_ptrs[findex], args, count, dst->t));
	else {
		einstr *e = emit_instr(ctx, CALL_FUN);
		e->mode = hl_type_mode(dst->t);
		e->a.index = findex;
		store_args(ctx, e, args, count);
		STORE(dst, CUR_REG());
	}
}

static vclosure *alloc_static_closure( emit_ctx *ctx, int fid ) {
	hl_module *m = ctx->mod;
	vclosure *c = hl_malloc(&m->ctx.alloc,sizeof(vclosure));
	int fidx = m->functions_indexes[fid];
	c->hasValue = 0;
	if( fidx >= m->code->nfunctions ) {
		// native
		c->t = m->code->natives[fidx - m->code->nfunctions].t;
		c->fun = m->functions_ptrs[fid];
		c->value = NULL;
	} else {
		c->t = m->code->functions[fidx].type;
		c->fun = (void*)(int_val)fid;
		c->value = ctx->closure_list;
		ctx->closure_list = c;
	}
	return c;
}

static void *get_dynget( hl_type *t ) {
	switch( t->kind ) {
	case HF32:
		return hl_dyn_getf;
	case HF64:
		return hl_dyn_getd;
	case HI64:
	case HGUID:
		return hl_dyn_geti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_geti;
	default:
		return hl_dyn_getp;
	}
}

static void *get_dynset( hl_type *t ) {
	switch( t->kind ) {
	case HF32:
		return hl_dyn_setf;
	case HF64:
		return hl_dyn_setd;
	case HI64:
	case HGUID:
		return hl_dyn_seti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_seti;
	default:
		return hl_dyn_setp;
	}
}

static void *get_dyncast( hl_type *t ) {
	switch( t->kind ) {
	case HF32:
		return hl_dyn_castf;
	case HF64:
		return hl_dyn_castd;
	case HI64:
	case HGUID:
		return hl_dyn_casti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_casti;
	default:
		return hl_dyn_castp;
	}
}

static void emit_store_size( emit_ctx *ctx, ereg dst, int dst_offset, ereg src, int src_offset, int total_size ) {
	int offset = 0;
	while( offset < total_size) {
		int remain = total_size - offset;
		hl_type *ct = remain >= HL_WSIZE ? &hlt_bytes : (remain >= 4 ? &hlt_i32 : &hlt_ui8);
		STORE_MEM(dst, dst_offset+offset, LOAD_MEM(src,src_offset+offset,ct));
		offset += hl_type_size(ct);
	}
}

static ereg emit_conv( emit_ctx *ctx, ereg v, emit_mode mode, bool _unsigned ) {
	return emit_gen(ctx, _unsigned ? CONV_UNSIGNED : CONV, v, ENULL, mode);
}

static bool dyn_need_type( hl_type *t ) {
	return !(IS_FLOAT(t) || t->kind == HI64 || t->kind == HGUID);
}

static ereg emit_dyn_cast( emit_ctx *ctx, ereg v, hl_type *t, hl_type *dt ) {
	if( t->kind == HNULL && t->tparam->kind == dt->kind ) {
		emit_test(ctx, v, OJNotNull);
		int jnot = emit_jump(ctx, false);
		ereg v1 = LOAD_CONST(0,dt);
		int jend = emit_jump(ctx, true);
		patch_jump(ctx, jnot);
		ereg v2 = LOAD_MEM(v,0,dt);
		patch_jump(ctx, jend);
		return emit_phi(ctx, v1, v2);
	}
	bool need_dyn = dyn_need_type(dt);
	ereg st = emit_gen_size(ctx, ALLOC_STACK, 1);
	STORE_MEM(st, 0, v);
	ereg args[3];
	args[0] = st;
	args[1] = LOAD_CONST_PTR(t);
	if( need_dyn ) args[2] = LOAD_CONST_PTR(dt);
	ereg r = emit_native_call(ctx, get_dyncast(dt), args, need_dyn ? 3 : 2, dt);
	emit_gen_size(ctx, FREE_STACK, 1);
	return r;
}

static void emit_opcode( emit_ctx *ctx, hl_opcode *o );

static void hl_emit_flush( emit_ctx *ctx ) {
	int i = 0;
	while( i < ctx->jump_regs.cur ) {
		int pos = ctx->jump_regs.data[i++];
		einstr *e = ctx->instrs + pos;
		int target = ctx->jump_regs.data[i++];
		e->size_offs = ctx->pos_map[target] - (pos + 1);
	}
	ctx->pos_map[ctx->fun->nops] = -1;
}

int hl_emit_function( emit_ctx *ctx, hl_module *m, hl_function *f ) {
	int i;
	ctx->mod = m;
	ctx->fun = f;
	ctx->emit_pos = 0;
	ctx->trap_count = 0;
	ctx->ref_count = 0;
	int_alloc_reset(&ctx->args_data);
	int_alloc_reset(&ctx->jump_regs);
	int_alloc_reset(&ctx->block_writes);
	current_ctx = ctx;
	hl_free(&ctx->falloc);
	ctx->current_block = alloc_block(ctx);
	ctx->arrival_points = NULL;
#	ifdef BLOCK_DEBUG
	printf("---- begin ----\n");
#	endif
	if( f->nregs > ctx->max_regs ) {
		free(ctx->vregs);
		ctx->vregs = (vreg*)malloc(sizeof(vreg) * (f->nregs + 1));
		if( ctx->vregs == NULL )
			return false;
		for(i=0;i<f->nregs;i++)
			R(i)->id = i;
		ctx->max_regs = f->nregs;
	}

	for(i=0;i<f->nregs;i++) {
		vreg *r = R(i);
		r->t = f->regs[i];
		r->current = ENULL;
		r->written = false;
	}

	for(i=0;i<f->type->fun->nargs;i++) {
		STORE(R(i), emit_gen(ctx, LOAD_ARG, ENULL, ENULL, hl_type_mode(f->type->fun->args[i])));
	}

	for(i=f->nops-1;i>=0;i--) {
		hl_opcode *o = f->ops + i;
		if( o->op == ORef ) {
			ereg ref = resolve_ref(ctx, o->p2);
			if( ref.index >= 0 ) continue;
			if( ctx->ref_count == MAX_REFS ) emit_error("Too many refs");
			ctx->refs[ctx->ref_count].r = emit_gen_size(ctx, ALLOC_STACK, hl_type_size(R(o->p2)->t));
			ctx->refs[ctx->ref_count].reg = o->p2;
			ctx->ref_count++;
		}
	}

	if( f->nops >= ctx->pos_map_size ) {
		free(ctx->pos_map);
		ctx->pos_map = (int*)malloc(sizeof(int) * (f->nops+1));
		if( ctx->pos_map == NULL )
			return false;
		ctx->pos_map_size = f->nops;
	}

	for(int op_pos=0;op_pos<f->nops;op_pos++) {
		ctx->op_pos = op_pos;
		ctx->pos_map[op_pos] = ctx->emit_pos;
		if( ctx->arrival_points && ctx->arrival_points->id == op_pos )
			split_block(ctx);
		emit_opcode(ctx,f->ops + op_pos);
	}

	hl_emit_flush(ctx);
	current_ctx = NULL;
	return true;
}

emit_ctx *hl_emit_alloc() {
	emit_ctx *ctx = (emit_ctx*)malloc(sizeof(emit_ctx));
	if( ctx == NULL ) return NULL;
	memset(ctx,0,sizeof(emit_ctx));
	if( sizeof(einstr) != 16 ) emit_assert();
	hl_alloc_init(&ctx->falloc);
	return ctx;
}

void hl_emit_free( emit_ctx *ctx, h_bool can_reset ) {
	hl_free(&ctx->falloc);
	free(ctx->vregs);
	free(ctx->instrs);
	free(ctx->pos_map);
	int_alloc_free(&ctx->jump_regs);
	int_alloc_free(&ctx->args_data);
	int_alloc_free(&ctx->block_writes);
	free(ctx);
}

void hl_emit_init( emit_ctx *ctx, hl_module *m ) {
}

void hl_emit_reset( emit_ctx *ctx, hl_module *m ) {
}

static bool seal_block_rec( emit_ctx *ctx, emit_block *b, int target ) {
	if( b->start_pos < target )
		return false;
	if( b->start_pos == target ) {
		b->wait_nexts--;
		b->preds = link_add(ctx,0,ctx->current_block,b->preds);
		split_block(ctx);
		if( b->wait_nexts == 0 ) {
#			ifdef BLOCK_DEBUG
			printf("  SEAL #%d\n",b->id);
#			endif
			// seal block
			linked_inf *l = b->incomplete_phis;
			while( l ) {
				einstr *e = ctx->instrs + l->id;
				vreg *r = (vreg*)l->ptr;
				if( e->op != PHI ) emit_assert();
				int prev_emit = ctx->emit_pos;
				ctx->emit_pos = l->id + 1;
				int_alloc_reset(&ctx->phi_gather);
				emit_gather_phi_rec(ctx, b, r);
				ctx->emit_pos = prev_emit;
				store_args(ctx, e, (ereg*)ctx->phi_gather.data, ctx->phi_gather.cur);
				l = l->next;
			}
			b->incomplete_phis = NULL;
		}
		return true;
	}
	linked_inf *l = b->preds;
	while( l ) {
		emit_block *n = (emit_block*)l->ptr;
		if( n->start_pos < b->start_pos && seal_block_rec(ctx,n,target) )
			return true;
		l = l->next;
	}
	return false;
}

static void register_block_jump( emit_ctx *ctx, int offs, bool cond ) {
	int jidx = emit_jump(ctx, cond);
	register_jump(ctx, jidx, offs);
	if( offs < 0 ) {
		int target = ctx->pos_map[ctx->op_pos + 1 + offs];
		emit_block *b = ctx->current_block;
		if( !seal_block_rec(ctx, b, target) ) emit_assert();
	}
}

static void emit_opcode( emit_ctx *ctx, hl_opcode *o ) {
	vreg *dst = R(o->p1);
	vreg *ra = R(o->p2);
	vreg *rb = R(o->p3);
	hl_module *m = ctx->mod;
#ifdef HL_DEBUG
	int uid = (ctx->fun->findex << 16) | ctx->op_pos;
#endif
	switch( o->op ) {
	case OMov:
	case OUnsafeCast:
		STORE(dst, LOAD(ra));
		break;
	case OInt:
		STORE(dst, LOAD_CONST(m->code->ints[o->p2], dst->t));
		break;
	case OBool:
		STORE(dst, LOAD_CONST(o->p2, &hlt_bool));
		break;
	case ONull:
		STORE(dst, LOAD_CONST(0, dst->t));
		break;
	case OFloat:
		{
			union {
				float f;
				double d;
				uint64 i;
			} v;
			if( dst->t->kind == HF32 )
				v.f = (float)m->code->floats[o->p2];
			else
				v.d = m->code->floats[o->p2];
			STORE(dst, LOAD_CONST(v.i, dst->t));
		}
		break;
	case OString:
		STORE(dst, LOAD_CONST_PTR(hl_get_ustring(m->code,o->p2)));
		break;
	case OBytes:
		{
			char *b = m->code->version >= 5 ? m->code->bytes + m->code->bytes_pos[o->p2] : m->code->strings[o->p2];
			STORE(dst,LOAD_CONST_PTR(b));
		}
		break;
	case OGetGlobal:
		{
			void *addr = m->globals_data + m->globals_indexes[o->p2];
			STORE(dst, LOAD_MEM_PTR(LOAD_CONST_PTR(addr),0));
		}
		break;
	case OSetGlobal:
		{
			void *addr = m->globals_data + m->globals_indexes[o->p1];
			STORE_MEM(LOAD_CONST_PTR(addr),0,LOAD(ra));
		}
		break;
	case OCall0:
		emit_call_fun(ctx, dst, o->p2, 0, NULL);
		break;
	case OCall1:
		emit_call_fun(ctx, dst, o->p2, 1, &o->p3);
		break;
	case OCall2:
		{
			int args[2] = { o->p3, (int)(int_val)o->extra };
			emit_call_fun(ctx, dst, o->p2, 2, args);
		}
		break;
	case OCall3:
		{
			int args[3] = { o->p3, o->extra[0], o->extra[1] };
			emit_call_fun(ctx, dst, o->p2, 3, args);
		}
		break;
	case OCall4:
		{
			int args[4] = { o->p3, o->extra[0], o->extra[1], o->extra[2] };
			emit_call_fun(ctx, dst, o->p2, 4, args);
		}
		break;
	case OCallN:
		emit_call_fun(ctx, dst, o->p2, o->p3, o->extra);
		break;
	case OSub:
	case OAdd:
	case OMul:
	case OSDiv:
	case OUDiv:
	case OShl:
	case OSShr:
	case OUShr:
	case OAnd:
	case OOr:
	case OXor:
	case OSMod:
	case OUMod:
		{
			ereg va = LOAD(ra);
			ereg vb = LOAD(rb);
			STORE(dst, emit_gen_ext(ctx, BINOP, va, vb, hl_type_mode(dst->t), o->op));
		}
		break;
	case ONeg:
	case ONot:
		STORE(dst, emit_gen_ext(ctx, UNOP, LOAD(ra), ENULL, hl_type_mode(dst->t), o->op));
		break;
	case OJFalse:
	case OJTrue:
	case OJNotNull:
	case OJNull:
		{
			emit_test(ctx, LOAD(dst), o->op);
			register_block_jump(ctx, o->p2, true);
		}
		break;
	case OJEq:
	case OJNotEq:
	case OJSLt:
	case OJSGte:
	case OJSLte:
	case OJSGt:
	case OJULt:
	case OJUGte:
	case OJNotLt:
	case OJNotGte:
		{
			emit_gen_ext(ctx, CMP, LOAD(dst), LOAD(ra), hl_type_mode(dst->t), o->op);
			register_block_jump(ctx, o->p3, true);
		}
		break;
	case OJAlways:
		register_block_jump(ctx, o->p1, false);
		break;
	case OToDyn:
		if( ra->t->kind == HBOOL ) {
			ereg arg = LOAD(ra);
			STORE(dst, emit_native_call(ctx,hl_alloc_dynbool,&arg,1,&hlt_dyn));
		} else {
			ereg arg = LOAD_CONST_PTR(ra->t);
			ereg ret = emit_native_call(ctx,hl_alloc_dynamic,&arg,1,&hlt_dyn);
			STORE_MEM(ret,HDYN_VALUE,LOAD(ra));
			STORE(dst, ret);
		}
		break;
	case OToSFloat:
	case OToInt:
	case OToUFloat:
		STORE(dst, emit_conv(ctx,LOAD(ra),hl_type_mode(dst->t), o->op == OToUFloat));
		break;
	case ORet:
		if( dst->t->kind == HVOID )
			emit_gen(ctx,RET,ENULL,ENULL,M_VOID);
		else
			emit_gen(ctx, RET, LOAD(dst), ENULL, hl_type_mode(dst->t));
		break;
	case OIncr:
	case ODecr:
		{
			if( IS_FLOAT(dst->t) ) {
				emit_assert();
			} else {
				STORE(dst, emit_gen_ext(ctx,UNOP,LOAD(dst),ENULL,hl_type_mode(dst->t),o->op));
			}
		}
		break;
	case ONew:
		{
			ereg arg = ENULL;
			void *allocFun = NULL;
			int nargs = 1;
			switch( dst->t->kind ) {
			case HOBJ:
			case HSTRUCT:
				allocFun = hl_alloc_obj;
				break;
			case HDYNOBJ:
				allocFun = hl_alloc_dynobj;
				nargs = 0;
				break;
			case HVIRTUAL:
				allocFun = hl_alloc_virtual;
				break;
			default:
				emit_assert();
			}
			if( nargs ) arg = LOAD_CONST_PTR(dst->t);
			STORE(dst, emit_native_call(ctx,allocFun,&arg,nargs,dst->t));
		}
		break;
	case OInstanceClosure:
		{
			ereg args[3];
			args[0] = LOAD_CONST_PTR(m->code->functions[m->functions_indexes[o->p2]].type);
			// TODO : WRITE (emit_pos + op_count) to process later and replace address !
			args[1] = LOAD_CONST_PTR(0);
			args[2] = LOAD(rb);
			STORE(dst, emit_native_call(ctx,hl_alloc_closure_ptr,args,3,dst->t));
		}
		break;
	case OVirtualClosure:
		{
			hl_type *t = NULL;
			hl_type *ot = ra->t;
			while( t == NULL ) {
				int i;
				for(i=0;i<ot->obj->nproto;i++) {
					hl_obj_proto *pp = ot->obj->proto + i;
					if( pp->pindex == o->p3 ) {
						t = m->code->functions[m->functions_indexes[pp->findex]].type;
						break;
					}
				}
				ot = ot->obj->super;
			}
			ereg args[3];
			ereg obj = LOAD(ra);
			args[0] = LOAD_CONST_PTR(t);
			args[1] = LOAD_OBJ_METHOD(obj,o->p3);
			args[2] = obj;
			STORE(dst, emit_native_call(ctx,hl_alloc_closure_ptr,args,3,dst->t));
		}
		break;
	case OCallClosure:
		if( ra->t->kind == HDYN ) {
			int i;
			ereg st = emit_gen_size(ctx, ALLOC_STACK, o->p3);
			for(i=0;i<o->p3;i++) {
				vreg *r = R(o->extra[i]);
				if( !hl_is_dynamic(r->t) ) emit_assert();
				STORE_MEM(st,i*HL_WSIZE,LOAD(r));
			}
			ereg args[3];
			args[0] = LOAD(ra);
			args[1] = st;
			args[2] = LOAD_CONST(o->p3,&hlt_i32);
			STORE(dst, emit_dyn_cast(ctx,emit_native_call(ctx,hl_dyn_call,args,3,dst->t),ra->t,dst->t));
			emit_gen_size(ctx, FREE_STACK, o->p3);
		} else {
			ereg r = LOAD(ra);
			ereg *args = get_tmp_args(ctx,o->p3+1);
			// Code for if( c->hasValue ) c->fun(c->value,args) else c->fun(args)
			ereg has = LOAD_MEM(r,HL_WSIZE*2,&hlt_i32);
			emit_test(ctx, has, OJNull);
			int jidx = emit_jump(ctx, true);
			int i;
			args[0] = LOAD_MEM_PTR(r,HL_WSIZE * 3);
			for(i=0;i<o->p3;i++)
				args[i+1] = LOAD(R(o->extra[i]));
			ereg v1 = emit_dyn_call(ctx,LOAD_MEM_PTR(r,HL_WSIZE),args,o->p3 + 1,dst->t);
			int jend = emit_jump(ctx, false);			
			patch_jump(ctx, jidx);
			for(i=0;i<o->p3;i++)
				args[i] = LOAD(R(o->extra[i]));
			ereg v2 = emit_dyn_call(ctx,LOAD_MEM_PTR(r,HL_WSIZE),args,o->p3,dst->t);
			patch_jump(ctx, jend);
			STORE(dst, emit_phi(ctx,v1,v2));
		}
		break;
	case OStaticClosure:
		{
			vclosure *c = alloc_static_closure(ctx,o->p2);
			STORE(dst, LOAD_CONST_PTR(c));
		}
		break;
	case OField:
		{
			switch( ra->t->kind ) {
			case HOBJ:
			case HSTRUCT:
				{
					hl_runtime_obj *rt = hl_get_obj_rt(ra->t);
					ereg r = LOAD(ra);
					if( dst->t->kind == HSTRUCT ) {
						hl_type *ft = hl_obj_field_fetch(ra->t,o->p3)->t;
						if( ft->kind == HPACKED ) {
							STORE(dst,OFFSET(r, ENULL, 0, rt->fields_indexes[o->p3]));
							break;
						}
					}
					STORE(dst, LOAD_MEM(r,rt->fields_indexes[o->p3],dst->t));
				}
				break;
			case HVIRTUAL:
				// code for : if( hl_vfields(o)[f] ) r = *hl_vfields(o)[f]; else r = hl_dyn_get(o,hash(field),vt)
				{
					ereg obj = LOAD(ra);
					ereg field = LOAD_MEM_PTR(obj,sizeof(vvirtual)+HL_WSIZE*o->p3);
					emit_test(ctx, field, OJNull);
					int jidx = emit_jump(ctx, true);
					ereg v1 = LOAD_MEM(field,0,dst->t);
					int jend = emit_jump(ctx, false);
					patch_jump(ctx, jidx);
					bool need_type = dyn_need_type(dst->t);
					ereg args[3];
					args[0] = obj;
					args[1] = LOAD_CONST(ra->t->virt->fields[o->p3].hashed_name,&hlt_i32);
					if( need_type ) args[2] = LOAD_CONST_PTR(dst->t);
					ereg v2 = emit_native_call(ctx,get_dynget(dst->t),args,need_type?3:2,dst->t);
					patch_jump(ctx, jend);
					STORE(dst, emit_phi(ctx, v1, v2));
				}
				break;
			default:
				emit_assert();
				break;
			}
		}
		break;
	case OSetField:
		{
			switch( dst->t->kind ) {
			case HOBJ:
			case HSTRUCT:
				{
					ereg obj = LOAD(dst);
					ereg val = LOAD(rb);
					hl_runtime_obj *rt = hl_get_obj_rt(dst->t);
					int field_pos = rt->fields_indexes[o->p2];
					if( rb->t->kind == HSTRUCT ) {
						hl_type *ft = hl_obj_field_fetch(dst->t,o->p2)->t;
						if( ft->kind == HPACKED ) {
							emit_store_size(ctx,obj,field_pos,val,0,hl_get_obj_rt(ft->tparam)->size);
							break;
						}
					}
					STORE_MEM(obj,field_pos, val);
				}
				break;
			case HVIRTUAL:
				// code for : if( hl_vfields(o)[f] ) *hl_vfields(o)[f] = v; else hl_dyn_set(o,hash(field),vt,v)
				{
					ereg obj = LOAD(dst);
					ereg val = LOAD(rb);
					ereg field = LOAD_MEM_PTR(obj,sizeof(vvirtual)+HL_WSIZE*o->p2);
					emit_test(ctx, field, OJNull);
					int jidx = emit_jump(ctx, true);
					STORE_MEM(field, 0, val);
					int jend = emit_jump(ctx, false);
					patch_jump(ctx, jidx);
					bool need_type = dyn_need_type(dst->t);
					ereg args[4];
					args[0] = obj;
					args[1] = LOAD_CONST(dst->t->virt->fields[o->p2].hashed_name,&hlt_i32);
					if( need_type ) {
						args[2] = LOAD_CONST_PTR(rb->t);
						args[3] = val;
					} else {
						args[2] = val;
					}
					emit_native_call(ctx,get_dynset(dst->t),args,need_type?4:3,dst->t);
					patch_jump(ctx, jend);
				}
				break;
			default:
				emit_assert();
				break;
			}
		}
		break;
	case OGetThis:
		{
			vreg *r = R(0);
			ereg obj = LOAD(r);
			hl_runtime_obj *rt = hl_get_obj_rt(r->t);
			int field_pos = rt->fields_indexes[o->p2];
			if( dst->t->kind == HSTRUCT ) {
				hl_type *ft = hl_obj_field_fetch(r->t,o->p2)->t;
				if( ft->kind == HPACKED ) {
					STORE(dst, OFFSET(obj, ENULL, 0, field_pos));
					break;
				}
			}
			STORE(dst, LOAD_MEM(obj, field_pos, dst->t));
		}
		break;
	case OSetThis:
		{
			vreg *r = R(0);
			ereg obj = LOAD(r);
			ereg val = LOAD(ra);
			hl_runtime_obj *rt = hl_get_obj_rt(r->t);
			int field_pos = rt->fields_indexes[o->p1];
			if( ra->t->kind == HSTRUCT ) {
				hl_type *ft = hl_obj_field_fetch(r->t,o->p1)->t;
				if( ft->kind == HPACKED ) {
					emit_store_size(ctx, obj, field_pos, val, 0, hl_get_obj_rt(ft->tparam)->size);
					break;
				}
			}
			STORE_MEM(obj,field_pos,val);
		}
		break;
	case OCallThis:
		{
			int i;
			int nargs = o->p3 + 1;
			ereg obj = LOAD(R(0));
			ereg *args = get_tmp_args(ctx, nargs);
			args[0] = obj;
			for(i=1;i<nargs;i++)
				args[i] = LOAD(R(o->extra[i-1]));
			ereg fun = LOAD_OBJ_METHOD(obj, o->p2);
			STORE(dst, emit_dyn_call(ctx,fun,args,nargs,dst->t));
		}
		break;
	case OCallMethod:
		{
			vreg *r = R(o->extra[0]);
			ereg obj = LOAD(r);
			switch( r->t->kind ) {
			case HOBJ:
				{
					int i;
					int nargs = o->p3;
					ereg *args = get_tmp_args(ctx, nargs);
					for(i=0;i<nargs;i++)
						args[i] = LOAD(R(o->extra[i]));
					ereg fun = LOAD_OBJ_METHOD(obj, o->p2);
					STORE(dst, emit_dyn_call(ctx,fun,args,nargs,dst->t));
				}
				break;
			case HVIRTUAL:
				// code for : if( hl_vfields(o)[f] ) dst = *hl_vfields(o)[f](o->value,args...); else dst = hl_dyn_call_obj(o->value,field,args,&ret)
				{
					vreg *_o = R(o->extra[0]);
					ereg obj = LOAD(_o);
					ereg field = LOAD_MEM_PTR(obj,sizeof(vvirtual)+HL_WSIZE*o->p2);
					emit_test(ctx, field, OJNull);
					int jidx = emit_jump(ctx, true);

					int nargs = o->p3;
					ereg *args = get_tmp_args(ctx, nargs);
					int i;
					args[0] = LOAD_MEM_PTR(obj,HL_WSIZE);
					for(i=1;i<nargs;i++)
						args[i] = LOAD(R(o->extra[i]));
					ereg v1 = emit_dyn_call(ctx,LOAD_MEM_PTR(field,0),args,nargs,dst->t);

					int jend = emit_jump(ctx, false);
					patch_jump(ctx, jidx);

					nargs = o->p3 - 1;
					ereg eargs = emit_gen_size(ctx, ALLOC_STACK, nargs);
					for(i=0;i<nargs;i++)
						STORE_MEM(eargs,i*HL_WSIZE,LOAD(R(o->extra[i+1])));
					bool need_dyn = !hl_is_ptr(dst->t) && dst->t->kind != HVOID;
					int dyn_size = sizeof(vdynamic)/HL_WSIZE;
					ereg edyn = need_dyn ? emit_gen_size(ctx, ALLOC_STACK, dyn_size) : LOAD_CONST_PTR(NULL);

					args = get_tmp_args(ctx, 4);
					args[0] = LOAD_MEM_PTR(obj,HL_WSIZE);
					args[1] = LOAD_CONST(_o->t->virt->fields[o->p2].hashed_name,&hlt_i32);
					args[2] = eargs;
					args[3] = edyn;

					ereg v2 = emit_native_call(ctx, hl_dyn_call_obj, args, 4, dst->t);

					emit_gen_size(ctx, FREE_STACK, o->p3 + (need_dyn ? dyn_size : 0));
					patch_jump(ctx, jend);

					STORE(dst, emit_phi(ctx, v1, v2));
				}
				break;
			default:
				emit_assert();
				break;
			}
		}
		break;
	case OThrow:
	case ORethrow:
		{
			ereg arg = LOAD(dst);
			emit_native_call(ctx, o->op == OThrow ? hl_throw : hl_rethrow, &arg, 1, &hlt_void);
			mark_no_return(ctx);
		}
		break;
	case OLabel:
		// NOP
		{
			if( ctx->current_block->start_pos != ctx->emit_pos )
				split_block(ctx);
			int i;
			for(i=ctx->op_pos+1;i<ctx->fun->nops;i++) {
				hl_opcode *op = &ctx->fun->ops[i];
				int offs = 0;
				switch( op->op ) {
				case OJFalse:
				case OJTrue:
				case OJNotNull:
				case OJNull:
					offs = op->p2;
					break;
				case OJAlways:
					offs = op->p1;
					break;
				case OJEq:
				case OJNotEq:
				case OJSLt:
				case OJSGte:
				case OJSLte:
				case OJSGt:
				case OJULt:
				case OJUGte:
				case OJNotLt:
				case OJNotGte:
					offs = op->p3;
					break;
				default:
					break;
				}
				if( offs < 0 && i + 1 + offs == ctx->op_pos ) {
#					ifdef BLOCK_DEBUG
					printf("  WAIT @%X\n",i);
#					endif
					ctx->current_block->wait_nexts++;
				}
			}
		}
		break;
	case OGetI8:
	case OGetI16:
	case OGetMem:
		{
			ereg offs = OFFSET(LOAD(ra),LOAD(rb),1,0);
			ereg val = LOAD_MEM(offs, 0, dst->t);
			if( o->op != OGetMem ) val = emit_conv(ctx, val, M_I32, false);
			STORE(dst, val);
		}
		break;
	case OSetI8:
	case OSetI16:
	case OSetMem:
		{
			ereg offs = OFFSET(LOAD(dst), LOAD(ra),1,0);
			ereg val = LOAD(rb);
			if( o->op != OSetMem ) val = emit_conv(ctx, val, M_I32, false);
			STORE_MEM(offs, 0, val);
		}
		break;
	case OType:
		STORE(dst, LOAD_CONST_PTR(m->code->types + o->p2));
		break;
	case OGetType:
		{
			ereg r = LOAD(ra);
			emit_test(ctx, r, OJNotNull);
			int jidx = emit_jump(ctx, true);
			ereg v1 = LOAD_CONST_PTR(&hlt_void);
			int jend = emit_jump(ctx, false);
			patch_jump(ctx, jidx);
			ereg v2 = LOAD_MEM_PTR(r,0);
			patch_jump(ctx, jend);
			STORE(dst, emit_phi(ctx, v1, v2));
		}
		break;
	case OGetArray:
		{
			if( ra->t->kind == HABSTRACT ) {
				int osize;
				bool isPtr = dst->t->kind != HOBJ && dst->t->kind != HSTRUCT;
				if( isPtr )
					osize = HL_WSIZE; // a pointer into the carray
				else {
					hl_runtime_obj *rt = hl_get_obj_rt(dst->t);
					osize = rt->size; // a mem offset into it
				}
				ereg pos = OFFSET(LOAD(ra), LOAD(rb), osize, 0);
				ereg val = isPtr ? LOAD_MEM_PTR(pos,0) : pos;
				STORE(dst, val);
			} else {
				ereg pos = OFFSET(LOAD(ra), LOAD(rb), hl_type_size(dst->t), sizeof(varray));
				STORE(dst, LOAD_MEM_PTR(pos,0));
			}
		}
		break;
	case OSetArray:
		{
			if( dst->t->kind == HABSTRACT ) {
				int osize;
				bool isPtr = rb->t->kind != HOBJ && rb->t->kind != HSTRUCT;
				if( isPtr) {
					osize = HL_WSIZE;
				} else {
					hl_runtime_obj *rt = hl_get_obj_rt(rb->t);
					osize = rt->size;
				}
				ereg pos = OFFSET(LOAD(dst), LOAD(ra), osize, 0);
				emit_store_size(ctx, pos, 0, LOAD(rb), 0, osize);
			} else  {
				ereg pos = OFFSET(LOAD(dst), LOAD(ra), hl_type_size(dst->t), sizeof(varray));
				STORE_MEM(pos, 0, LOAD(rb));
			}
		}
		break;
	case OArraySize:
		STORE(dst, LOAD_MEM(LOAD(ra),HL_WSIZE*2,&hlt_i32));
		break;
	case ORef:
		{
			ereg ref = resolve_ref(ctx, ra->id);
			if( ref.index < 0 ) emit_assert();
			if( ra->current.index >= 0 ) STORE_MEM(ref, 0, LOAD(ra));
			ra->current.index = -1; // ref will be modified
			STORE(dst, ref);
		}
		break;
	case OUnref:
		STORE(dst, LOAD_MEM(LOAD(ra),0,dst->t));
		break;
	case OSetref:
		STORE_MEM(LOAD(dst),0,LOAD(ra));
		break;
	case ORefData:
		switch( ra->t->kind ) {
		case HARRAY:
			STORE(dst, OFFSET(LOAD(ra),ENULL,0,sizeof(varray)));
			break;
		default:
			emit_assert();
		}
		break;
	case ORefOffset:
		STORE(dst, OFFSET(LOAD(ra),LOAD(rb), hl_type_size(dst->t->tparam),0));
		break;
	case OToVirtual:
		{
			ereg args[2];
			args[0] = LOAD(ra);
			args[1] = LOAD_CONST_PTR(dst->t);
			STORE(dst, emit_native_call(ctx,hl_to_virtual,args,2, dst->t));
		}
		break;
	case OMakeEnum:
		{
			ereg args[2];
			args[0] = LOAD_CONST_PTR(dst->t);
			args[1] = LOAD_CONST(o->p2,&hlt_i32);
			ereg en = emit_native_call(ctx, hl_alloc_enum, args, 2, dst->t);
			STORE(dst, en);
			hl_enum_construct *c = &dst->t->tenum->constructs[o->p2];
			for(int i=0;i<c->nparams;i++)
				STORE_MEM(en, c->offsets[i], LOAD(R(o->extra[i])));			
		}
		break;
	case OEnumAlloc:
		{
			ereg args[2];
			args[0] = LOAD_CONST_PTR(dst->t);
			args[1] = LOAD_CONST(o->p2,&hlt_i32);
			STORE(dst, emit_native_call(ctx, hl_alloc_enum, args, 2, dst->t));
		}
		break;
	case OEnumField:
		{
			hl_enum_construct *c = &ra->t->tenum->constructs[o->p3];
			int slot = (int)(int_val)o->extra;
			STORE(dst, LOAD_MEM(LOAD(ra),c->offsets[slot], dst->t));
		}
		break;
	case OEnumIndex:
		STORE(dst, LOAD_MEM(LOAD(ra),HL_WSIZE,dst->t));
		break;
	case OSetEnumField:
		{
			hl_enum_construct *c = &dst->t->tenum->constructs[0];
			STORE_MEM(LOAD(dst), c->offsets[o->p2], LOAD(rb));
		}
		break;
	case ONullCheck:
		{
			emit_test(ctx, LOAD(dst), OJNotNull);
			int jok = emit_jump(ctx, true);

			// ----- DETECT FIELD ACCESS ----------------
			hl_function *f = ctx->fun;
			hl_opcode *next = f->ops + ctx->op_pos + 1;
			bool null_field_access = false;
			int hashed_name = 0;
			// skip const and operation between nullcheck and access
			while( (next < f->ops + f->nops - 1) && (next->op >= OInt && next->op <= ODecr) ) {
				next++;
			}
			if( (next->op == OField && next->p2 == o->p1) || (next->op == OSetField && next->p1 == o->p1) ) {
				int fid = next->op == OField ? next->p3 : next->p2;
				hl_obj_field *f = NULL;
				if( dst->t->kind == HOBJ || dst->t->kind == HSTRUCT )
					f = hl_obj_field_fetch(dst->t, fid);
				else if( dst->t->kind == HVIRTUAL )
					f = dst->t->virt->fields + fid;
				if( f == NULL ) emit_assert();
				null_field_access = true;
				hashed_name = f->hashed_name;
			} else if( (next->op >= OCall1 && next->op <= OCallN) && next->p3 == o->p1 ) {
				int fid = next->p2 < 0 ? -1 : m->functions_indexes[next->p2];
				hl_function *cf = m->code->functions + fid;
				const uchar *name = fun_field_name(cf);
				null_field_access = true;
				hashed_name = hl_hash_gen(name, true);
			}
			// -----------------------------------------
			ereg arg = null_field_access ? LOAD_CONST(hashed_name,&hlt_i32) : ENULL;
			emit_native_call(ctx, null_field_access ? hl_stub_null_field_access : hl_stub_null_access, &arg, null_field_access ? 1 : 0, &hlt_void);
			mark_no_return(ctx);
			patch_jump(ctx, jok);
		}
		break;
	case OSafeCast:
		STORE(dst, emit_dyn_cast(ctx, LOAD(ra), ra->t, dst->t));
		break;
	case ODynGet:
		{
			bool need_type = dyn_need_type(dst->t);
			ereg args[3];
			args[0] = LOAD(ra);
			args[1] = LOAD_CONST(hl_hash_utf8(m->code->strings[o->p3]),&hlt_i32);
			if( need_type ) args[2] = LOAD_CONST_PTR(dst->t);
			STORE(dst, emit_native_call(ctx, get_dynget(dst->t), args, need_type ? 3 : 2, dst->t));
		}
		break;
	case ODynSet:
		{
			bool need_type = dyn_need_type(dst->t);
			ereg args[4];
			args[0] = LOAD(dst);
			args[1] = LOAD_CONST(hl_hash_utf8(m->code->strings[o->p2]),&hlt_i32);
			if( need_type ) {
				args[2] = LOAD_CONST_PTR(rb->t);
				args[3] = LOAD(rb);
			} else
				args[2] = LOAD(rb);
			emit_native_call(ctx, get_dynset(rb->t), args, need_type ? 4 : 3, &hlt_void);
		}
		break;
	case OTrap:
		{
			ereg st = emit_gen_size(ctx, ALLOC_STACK, sizeof(hl_trap_ctx));
			
			ereg thread, current_addr;
			static hl_thread_info *tinf = NULL;
			static hl_trap_ctx *trap = NULL;
#			ifndef HL_THREADS
			if( tinf == NULL ) tinf = hl_get_thread();
			current_addr = LOAD_CONST_PTR(&tinf->trap_current);
#			else
			thread = emit_native_call(ctx, hl_get_thread, NULL, 0, &hlt_bytes);
			current_addr = OFFSET(thread, ENULL, 0, (int)(int_val)&tinf->trap_current);
#			endif
			STORE_MEM(st, (int)(int_val)&trap->prev, LOAD_MEM_PTR(current_addr,0));
			STORE_MEM(current_addr, 0, st);


			/*
				trap E,@catch
				catch g
				catch g2
				...
				@:catch

				// Before haxe 5
				This is a bit hackshish : we want to detect the type of exception filtered by the catch so we check the following
				sequence of HL opcodes:

				trap E,@catch
				...
				@catch:
				global R, _
				call _, ???(R,E)

				??? is expected to be hl.BaseType.check
			*/
			hl_function *f = ctx->fun;
			hl_opcode *cat = f->ops + ctx->op_pos + 1;
			hl_opcode *next = f->ops + ctx->op_pos + 1 + o->p2;
			hl_opcode *next2 = f->ops + ctx->op_pos + 2 + o->p2;
			void *addr = NULL;
			if( cat->op == OCatch || (next->op == OGetGlobal && next2->op == OCall2 && next2->p3 == next->p1 && dst->id == (int)(int_val)next2->extra) ) {
				int gindex = cat->op == OCatch ? cat->p1 : next->p2;
				hl_type *gt = m->code->globals[gindex];
				while( gt->kind == HOBJ && gt->obj->super ) gt = gt->obj->super;
				if( gt->kind == HOBJ && gt->obj->nfields && gt->obj->fields[0].t->kind == HTYPE )
					addr = m->globals_data + m->globals_indexes[gindex];
			}
			STORE_MEM(st, (int)(int_val)&trap->tcheck, addr ? LOAD_MEM_PTR(LOAD_CONST_PTR(addr),0) : LOAD_CONST_PTR(NULL));

			void *fun = setjmp;
			ereg args[2];
			int nargs = 1;
			args[0] = OFFSET(st, ENULL, 0, (int)(int_val)&trap->buf);
#if defined(HL_WIN) && defined(HL_64)
			// On Win64 setjmp actually takes two arguments
			// the jump buffer and the frame pointer (or the stack pointer if there is no FP)
			nargs = 2;
			args[1] = emit_gen(ctx, NATIVE_REG, ENULL, ENULL, REG_RBP);
#endif
#ifdef HL_MINGW
			fun = _setjmp;
#endif
			ereg ret = emit_native_call(ctx, fun, args, nargs, &hlt_i32);
			emit_test(ctx, ret, OJNull);
			int jskip = emit_jump(ctx, true);
			emit_gen_size(ctx, FREE_STACK, sizeof(hl_trap_ctx));
			STORE(dst, tinf ? LOAD_CONST_PTR(&tinf->exc_value) : LOAD_MEM_PTR(thread,(int)(int_val)&tinf->exc_value));

			int jtrap = emit_jump(ctx, false);
			register_jump(ctx, jtrap, o->p2);
			patch_jump(ctx, jskip);

			if( ctx->trap_count == MAX_TRAPS ) emit_error("Too many try/catch depth");
			ctx->traps[ctx->trap_count++] = st;
		}
		break;
	case OEndTrap:
		{
			if( ctx->trap_count == 0 ) emit_assert();
			ereg st = ctx->traps[ctx->trap_count - 1];

			ereg thread, current_addr;
			static hl_thread_info *tinf = NULL;
			static hl_trap_ctx *trap = NULL;
#			ifndef HL_THREADS
			if( tinf == NULL ) tinf = hl_get_thread();
			current_addr = LOAD_CONST_PTR(&tinf->trap_current);
#			else
			thread = emit_native_call(ctx, hl_get_thread, NULL, 0, &hlt_bytes);
			current_addr = OFFSET(thread, ENULL, 0, (int)(int_val)&tinf->trap_current);
#			endif
			
			STORE_MEM(current_addr, 0, LOAD_MEM_PTR(st,(int)(int_val)&trap->prev));

#			ifdef HL_WIN
			// erase eip (prevent false positive in exception stack)
			{
				_JUMP_BUFFER *b = NULL;
#				ifdef HL_64
				int offset = (int)(int_val)&(b->Rip);
#				else
				int offset = (int)(int_val)&(b->Eip);
#				endif
				STORE_MEM(st, offset, LOAD_CONST_PTR(NULL));
			}
#			endif

			emit_gen_size(ctx, FREE_STACK, sizeof(hl_trap_ctx));
		}
		break;
	case OSwitch:
		{
			ereg v = LOAD(dst);
			int count = o->p2;
			emit_gen_ext(ctx, CMP, v, LOAD_CONST(count,&hlt_i32), M_I32, OJUGte);
			int jdefault = emit_jump(ctx, true);
			emit_gen_ext(ctx, JUMP_TABLE, v, ENULL, 0, count);
			for(int i=0; i<count; i++) {
				int j = emit_jump(ctx, false);
				register_jump(ctx, j, o->extra[i]);
			}
			patch_jump(ctx, jdefault);
		}
		break;
	case OGetTID:
		STORE(dst, LOAD_MEM(LOAD(ra),0,&hlt_i32));
		break;
	case OAssert:
		emit_native_call(ctx, hl_stub_assert, NULL, 0, &hlt_void);
		break;
	case ONop:
		break;
	case OPrefetch:
		{
			ereg r = LOAD(dst);
			if( o->p2 > 0 ) {
				switch( dst->t->kind ) {
				case HOBJ:
				case HSTRUCT:
					{
						hl_runtime_obj *rt = hl_get_obj_rt(dst->t);
						r = OFFSET(r, ENULL, 0, rt->fields_indexes[o->p2-1]);
					}
					break;
				default:
					emit_assert();
					break;
				}
			}
			emit_gen(ctx, PREFETCH, r, ENULL, o->p3);
		}
		break;
	case OAsm:
		emit_assert();
		break;
	case OCatch:
		// Only used by OTrap typing
		break;
	default:
		emit_error(hl_op_name(o->op));
		break;
	}
}

// -------------------- CODE ----------------------------------------------------------

void *hl_emit_code( emit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous ) {
	printf("TODO:emit_code\n");
	exit(0);
	return NULL;
}

// -------------------- DUMP ----------------------------------------------------------

static void hl_dump_arg( emit_ctx *ctx, int fmt, int val, char sep ) {
	if( fmt == 0 ) return;
	printf("%c", sep);
	switch( fmt ) {
	case 1:
	case 2:
		printf("R%d", val);
		if( val < 0 || val >= ctx->fun->nregs ) printf("?");
		break;
	case 3:
		printf("%d", val);
		break;
	case 4:
		printf("[%d]", val);
		break;
	case 5:
	case 6:
		printf("@%X", val + ctx->op_pos + 1);
		break;
	default:
		printf("?#%d", fmt);
		break;
	}
}

#define OP(_,_a,_b,_c) ((_a) | (((_b)&0xFF) << 8) | (((_c)&0xFF) << 16)),
#define OP_BEGIN static int hl_op_fmt[] = {
#define OP_END };
#undef R
#include "opcodes.h"

static void hl_dump_op( emit_ctx *ctx, hl_opcode *op ) {
	printf("%s", hl_op_name(op->op) + 1);
	int fmt = hl_op_fmt[op->op];
	hl_dump_arg(ctx, fmt & 0xFF, op->p1, ' ');
	if( ((fmt >> 8) & 0xFF) == 5 ) {
		int count = (fmt >> 16) & 0xFF;
		printf(" [");
		if( count == 4 ) {
			printf("%d", op->p2);
			printf(",%d", op->p3);
			printf(",%d", (int)(int_val)op->extra);
		} else if( op->op == OSwitch ) {
			for(int i=0;i<op->p2;i++) {
				if( i != 0 ) printf(",");
				printf("@%X", (op->extra[i] + ctx->op_pos + 1));
			}
			printf(",def=@%X", op->p3 + ctx->op_pos + 1);
		} else {
			if( count == 0xFF ) count = op->p3;
			for(int i=0;i<count;i++) {
				if( i != 0 ) printf(",");
				printf("%d", op->extra[i]);
			}
		}
		printf("]");
	} else {
		hl_dump_arg(ctx, (fmt >> 8) & 0xFF, op->p2,',');
		hl_dump_arg(ctx, fmt >> 16, op->p3,',');
	}
}

static const char *emit_mode_str( emit_mode mode ) {
	switch( mode ) {
	case M_UI8: return "-ui8";
	case M_UI16: return "-ui16";
	case M_I32: return "-i32";
	case M_I64: return "-i64";
	case M_F32: return "-f32";
	case M_F64: return "-f64";
	case M_PTR: return "";
	case M_VOID: return "-void";
	case M_NORET: return "-noret";
	default:
		static char buf[50];
		sprintf(buf,"?%d",mode);
		return buf;
	}
}

static void dump_value( emit_ctx *ctx, uint64 value, emit_mode mode ) {
	union {
		uint64 v;
		double d;
		float f;
	} tmp;
	hl_module *mod = ctx->mod;
	hl_code *code = ctx->mod->code;
	switch( mode ) {
	case M_NONE:
		printf("?0x%llX",value);
		break;
	case M_UI8:
	case M_UI16:
	case M_I32:
		if( (int)value >= -0x10000 && (int)value <= 0x10000 )
			printf("%d",(int)value);
		else
			printf("0x%X",(int)value);
		break;
	case M_F32:
		tmp.v = value;
		printf("%f",tmp.f);
		break;
	case M_F64:
		tmp.v = value;
		printf("%g",tmp.d);
		break;
	default:
		if( value == 0 )
			printf("NULL");
		else if( mode == M_PTR && value >= (uint64)code->types && value < (uint64)(code->types + code->ntypes) )
			uprintf(USTR("<%s>"),hl_type_str((hl_type*)value));
		else if( mode == M_PTR && value >= (uint64)mod->globals_data && value < (uint64)(mod->globals_data + mod->globals_size) )
			printf("<global@%X>",(int)(value - (uint64)mod->globals_data));
		else if( value == (uint64)&hlt_void )
			printf("<void>");
		else
			printf("0x%llX",value);
		break;
	}
}

static void hl_dump_fun_name( hl_function *f ) {
	if( f->obj )
		uprintf(USTR("%s.%s"),f->obj->name,f->field.name);
	else if( f->field.ref )
		uprintf(USTR("%s.~%s.%d"),f->field.ref->obj->name, f->field.ref->field.name, f->ref);
	printf("[%X]", f->findex);
}

static void hl_dump_args( emit_ctx *ctx, einstr *e ) {
	ereg *v = hl_emit_get_args(ctx, e);
	printf("(");
	for(int i=0;i<e->nargs;i++) {
		if( i != 0 ) printf(",");
		printf("@%X", v[i].index);
	}
	printf(")");
}


typedef struct { const char *name; void *ptr; } named_ptr;
static void hl_dump_ptr_name( emit_ctx *ctx, void *ptr ) {
#	define N(v)	ptr_names[i].name = #v; ptr_names[i].ptr = v; i++
#	define N2(n,v)	ptr_names[i].name = n; ptr_names[i].ptr = v; i++
#	define DYN(p) N2("dyn_get" #p, hl_dyn_get##p); N2("dyn_set" #p, hl_dyn_set##p); N2("dyn_cast" #p, hl_dyn_cast##p)
	static named_ptr ptr_names[256] = { NULL };
	int i = 0;
	if( !ptr_names[0].ptr ) {
		N(hl_alloc_dynbool);
		N(hl_alloc_dynamic);
		N(hl_alloc_obj);
		N(hl_alloc_dynobj);
		N(hl_alloc_virtual);
		N(hl_alloc_closure_ptr);
		N(hl_dyn_call);
		N(hl_dyn_call_obj);
		N(hl_throw);
		N(hl_rethrow);
		N(hl_to_virtual);
		N(hl_alloc_enum);
		DYN(f);
		DYN(d);
		DYN(i64);
		DYN(i);
		DYN(p);
		N2("null_field",hl_stub_null_field_access);
		N2("null_access",hl_stub_null_access);
		N(hl_get_thread);
		N(setjmp);
		N(_setjmp);
		N2("assert",hl_stub_assert);
		i = 0;
	}
#	undef N
#	undef N2
	while( true ) {
		named_ptr p = ptr_names[i++];
		if( !p.ptr ) break;
		if( p.ptr == ptr ) {
			printf("<%s>",p.name);
			return;
		}
	}
	for(i=0;i<ctx->mod->code->nnatives;i++) {
		hl_native *n = ctx->mod->code->natives + i;
		if( ctx->mod->functions_ptrs[n->findex] == ptr ) {
			printf("<%s.%s>",n->lib[0] == '?' ? n->lib + 1 : n->lib,n->name);
			return;
		}
	}
	printf("<?0x%llX>",(uint64)ptr);
}

void hl_emit_dump( emit_ctx *ctx ) {
	int i;
	int cur_op = 0;
	hl_function *f = ctx->fun;
	int nargs = f->type->fun->nargs;
	hl_emit_flush(ctx); // if it not was not before (in case of dump during emit)
	printf("function ");
	hl_dump_fun_name(f);
	printf("(");
	for(i=0;i<nargs;i++) {
		if( i > 0 ) printf(",");
		uprintf(USTR("R%d"), i);
	}
	printf(")\n");
	for(i=0;i<f->nregs;i++)
		uprintf(USTR("\tR%d : %s\n"),i, hl_type_str(f->regs[i]));
	for(i=0;i<ctx->emit_pos;i++) {
		while( ctx->pos_map[cur_op] == i ) {
			printf("@%X ", cur_op);
			ctx->op_pos = cur_op;
			hl_dump_op(ctx, f->ops + cur_op);
			printf("\n");
			cur_op++;
		}
		einstr *e = ctx->instrs + i;
		printf("\t\t@%X %s", i, op_names[e->op]);
		switch( e->op ) {
		case TEST:
		case CMP:
			printf("-%s", hl_op_name(e->size_offs)+2);
			break;
		case BINOP:
		case UNOP:
			printf("-%s", hl_op_name(e->size_offs)+1);
			break;
		default:
			break;
		}
		if( e->mode && e->op != PHI )
			printf("%s", emit_mode_str(e->mode));
		switch( e->op ) {
		case CALL_FUN:
			printf(" ");
			{
				int fid = ctx->mod->functions_indexes[e->a.index];
				hl_code *code = ctx->mod->code;
				if( fid < code->nfunctions ) {
					hl_dump_fun_name(&code->functions[fid]);
				} else {
					printf("???");
				}
			}
			hl_dump_args(ctx,e);
			break;
		case CALL_REG:
			printf(" @%X", e->a.index);
			hl_dump_args(ctx,e);
			break;
		case CALL_PTR:
			printf(" ");
			hl_dump_ptr_name(ctx, (void*)e->value);
			hl_dump_args(ctx,e);
			break;
		case PHI:
			hl_dump_args(ctx,e);
			{
				int i;
				ereg *args = hl_emit_get_args(ctx, e);
				for(i=0;i<e->nargs;i++) {
					einstr *a = ctx->instrs + args[i].index;
					if( a->mode != e->mode ) {
						printf(" ???");
						break;
					}
				}
			}
			break;
		case JUMP:
		case JCOND:
			printf(" @%X", i + 1 + e->size_offs);
			break;
		case LOAD_IMM:
			printf(" ");
			dump_value(ctx, e->value, e->mode);
			break;
		case ALLOC_STACK:
			printf(" %d", e->size_offs);
			break;
		case LOAD_ADDR:
			if( (e->b.index>>8) )
				printf(" @%X[%Xh]", e->a.index, e->b.index);
			else
				printf(" @%X[%d]", e->a.index, e->b.index);
			break;
		case STORE:
			{
				int offs = e->size_offs;
				if( offs == 0 )
					printf(" [@%X] = @%X", e->a.index, e->b.index);
				else
					printf(" @%X[%d] = @%X", e->a.index, offs, e->b.index);
				if( e->mode == 0 || e->mode != ctx->instrs[e->b.index].mode )
					printf(" ???");
			}
			break;
		default:
			if( e->a.index >= 0 ) {
				printf(" @%X", e->a.index);
				if( e->b.index >= 0 ) printf(", @%X", e->b.index);
			}
			break;
		}
		printf("\n");
	}
	// interrupted
	if( cur_op < f->nops ) {
		printf("@%X ", cur_op);
		ctx->op_pos = cur_op;
		hl_dump_op(ctx, f->ops + cur_op);
		printf("\n\t\t...\n");
	}
	printf("\n\n");
	fflush(stdout);
}
