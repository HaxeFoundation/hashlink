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

//#define EMIT_DEBUG

#ifdef EMIT_DEBUG
#	define emit_debug	jit_debug
#else
#	define emit_debug(...)
#endif

int hl_emit_mode_sizes[] = {0,1,2,4,8,HL_WSIZE,8,4,0,0};

typedef struct {
	hl_type *t;
	int id;
	ereg ref;
} vreg;

#define MAX_TMP_ARGS	32
#define MAX_TRAPS		32

typedef struct _linked_inf linked_inf;
typedef struct _emit_block emit_block;
typedef struct _tmp_phi tmp_phi;

#define S_TYPE			blocks
#define S_NAME(name)	blocks_##name
#define S_VALUE			emit_block*
#include "data_struct.c"
#define blocks_add(set,v)		blocks_add_impl(DEF_ALLOC,&(set),v)

#define S_TYPE			phi_arr
#define S_NAME(name)	phi_##name
#define S_VALUE			tmp_phi*
#include "data_struct.c"
#define phi_add(set,v)		phi_add_impl(DEF_ALLOC,&(set),v)

#define S_SORTED

#define S_TYPE			ereg_map
#define S_NAME(name)	ereg_##name
#define S_VALUE			ereg
#include "data_struct.c"
#define ereg_add(set,v)		ereg_add_impl(DEF_ALLOC,&(set),v)

#define S_MAP

#define S_TYPE			vreg_map
#define S_NAME(name)	vreg_##name
#define S_KEY			int
#define S_VALUE			ereg
#include "data_struct.c"
#define vreg_replace(set,k,v) vreg_replace_impl(DEF_ALLOC,&(set),k,v)

struct _linked_inf {
	int id;
	void *ptr;
	linked_inf *next;
};

struct _emit_block {
	int id;
	int start_pos;
	int end_pos;
	int wait_nexts;
	bool sealed;
	blocks nexts;
	blocks preds;
	vreg_map written_vars;
	phi_arr phis;
	emit_block *wait_seal_next;
};

struct _tmp_phi {
	ereg value;
	vreg *r;
	ereg target;
	int final_id;
	bool locked;
	bool opt;
	unsigned char mode;
	emit_block *b;
	ereg_map vals;
	phi_arr ref_phis;
	linked_inf *ref_blocks;
};

struct _emit_ctx {
	hl_module *mod;
	hl_function *fun;
	jit_ctx *jit;

	einstr *instrs;
	vreg *vregs;
	tmp_phi **phis;
	int max_instrs;
	int max_regs;
	int max_phis;
	int emit_pos;
	int op_pos;
	int phi_count;
	int phi_depth;
	bool flushed;

	ereg tmp_args[MAX_TMP_ARGS];
	ereg traps[MAX_TRAPS];
	int *pos_map;
	int pos_map_size;
	int trap_count;

	int_arr args_data;
	int_arr jump_regs;
	int_arr values;

	blocks blocks;
	emit_block *current_block;
	emit_block *wait_seal;
	linked_inf *arrival_points;
	vclosure *closure_list;
};

#define R(i)	(ctx->vregs + (i))

#define LOAD(r) emit_load_reg(ctx, r)
#define STORE(r, v) emit_store_reg(ctx, r, v)
#define LOAD_CONST(v, t) emit_load_const(ctx, (uint64)(v), t)
#define LOAD_CONST_PTR(v) LOAD_CONST(v,&hlt_bytes)
#define LOAD_MEM(v, offs, t) emit_load_mem(ctx, v, offs, t)
#define LOAD_MEM_PTR(v, offs) LOAD_MEM(v, offs, &hlt_bytes)
#define STORE_MEM(to, offs, v) emit_store_mem(ctx, to, offs, v)
#define LOAD_OBJ_METHOD(obj,id) LOAD_MEM_PTR(LOAD_MEM_PTR(LOAD_MEM_PTR(obj,0),HL_WSIZE*2),HL_WSIZE*(id))
#define OFFSET(base,index,mult,offset) emit_gen_ext(ctx, LEA, base, index, M_PTR, (mult) | ((offset) << 8))
#define BREAK() emit_gen(ctx, DEBUG_BREAK, UNUSED, UNUSED, 0)
#define GET_MODE(r) emit_get_mode(ctx,r)
#define GET_PHI(r) ctx->phis[-(r)-1]
#define HDYN_VALUE 8

#define IS_FLOAT(t)	((t)->kind == HF64 || (t)->kind == HF32)

static hl_type hlt_ui8 = { HUI8, 0 };
static hl_type hlt_ui16 = { HUI16, 0 };

static linked_inf *link_add( emit_ctx *ctx, int id, void *ptr, linked_inf *head ) {
	linked_inf *l = hl_malloc(&ctx->jit->falloc,sizeof(linked_inf));
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
	linked_inf *l = hl_malloc(&ctx->jit->falloc,sizeof(linked_inf));
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

static linked_inf *link_add_sort_replace( emit_ctx *ctx, int id, void *ptr, linked_inf *head ) {
	linked_inf *prev = NULL;
	linked_inf *cur = head;
	while( cur && cur->id < id ) {
		prev = cur;
		cur = cur->next;
	}
	// replace duplicate
	if( cur && cur->id == id ) {
		cur->ptr = ptr;
		return head;
	}
	// insert
	linked_inf *l = hl_malloc(&ctx->jit->falloc,sizeof(linked_inf));
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

static linked_inf *link_sort_remove( linked_inf *head, int id ) {
	linked_inf *prev = NULL;
	linked_inf *cur = head;
	while( cur && cur->id < id ) {
		prev = cur;
		cur = cur->next;
	}
	if( cur && cur->id == id ) {
		if( !prev )
			return cur->next;
		prev->next = cur->next;
		return head;
	}
	return head;
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
	if( t->kind == HF32 )
		return M_F32;
	if( t->kind == HF64 )
		return M_F64;
	return M_PTR;
}

static ereg new_value( emit_ctx *ctx ) {
	ereg r = int_arr_count(ctx->values);
	int_arr_add(ctx->values, ctx->emit_pos-1);
	return r;
}

static ereg *get_tmp_args( emit_ctx *ctx, int count ) {
	if( count > MAX_TMP_ARGS ) jit_error("Too many arguments");
	return ctx->tmp_args;
}

static unsigned char emit_get_mode( emit_ctx *ctx, ereg v ) {
	if( IS_NULL(v) ) jit_assert();
	if( v < 0 )
		return GET_PHI(v)->mode;
	return ctx->instrs[int_arr_get(ctx->values,v)].mode;
}

static const char *phi_prefix( emit_ctx *ctx ) {
	static char tmp[20];
	int sp = 3 + ctx->phi_depth * 2;
	if( sp > 19 ) sp = 19;
	memset(tmp,0x20,sp);
	tmp[sp] = 0;
	return tmp;
}

static einstr *emit_instr( emit_ctx *ctx, emit_op op ) {
	if( ctx->emit_pos == ctx->max_instrs ) {
		int pos = ctx->emit_pos;
		int next_size = ctx->max_instrs ? (ctx->max_instrs << 1) : 256;
		einstr *instrs = (einstr*)malloc(sizeof(einstr) * next_size);
		if( instrs == NULL ) jit_error("Out of memory");
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
	e->mode = GET_MODE(from);
	e->size_offs = offs;
	e->a = to;
	e->b = from;
}

static void store_args( emit_ctx *ctx, einstr *e, ereg *args, int count ) {
	if( count < 0 ) jit_assert();
	if( count > 64 ) jit_error("Too many arguments");
	e->nargs = (unsigned char)count;
	if( count == 0 ) return;
	if( count == 1 ) {
		e->size_offs = args[0];
		return;
	}
	int *args_data = int_arr_reserve(ctx->args_data, count);
	e->size_offs = (int)(args_data - ctx->args_data.values);
	memcpy(args_data, args, sizeof(int) * count);
}

ereg *hl_emit_get_args( emit_ctx *ctx, einstr *e ) {
	if( e->nargs == 0 )
		return NULL;
	if( e->nargs == 1 )
		return (ereg*)&e->size_offs;
	return (ereg*)(ctx->args_data.values + e->size_offs);
}

static ereg emit_gen_ext( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode, int size_offs ) {
	einstr *e = emit_instr(ctx, op);
	if( (unsigned char)mode != mode ) jit_assert();
	e->mode = (unsigned char)mode;
	e->size_offs = size_offs;
	e->a = a;
	e->b = b;
	return mode == 0 || mode == M_NORET ? UNUSED : new_value(ctx);
}

static ereg emit_gen( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode ) {
	return emit_gen_ext(ctx,op,a,b,mode,0);
}

static ereg emit_gen_size( emit_ctx *ctx, emit_op op, int size_offs ) {
	return emit_gen_ext(ctx,op,UNUSED,UNUSED,op==ALLOC_STACK ? M_PTR : 0,size_offs);
}

static void patch_instr_mode( emit_ctx *ctx, int mode ) {
	ctx->instrs[ctx->emit_pos-1].mode = (unsigned char)mode;
}

static tmp_phi *alloc_phi( emit_ctx *ctx, emit_block *b, vreg *r ) {
	if( ctx->phi_count == ctx->max_phis ) {
		int new_size = ctx->max_phis ? ctx->max_phis << 1 : 64;
		tmp_phi **phis = (tmp_phi**)malloc(sizeof(tmp_phi*) * new_size);
		if( phis == NULL ) jit_error("Out of memory");
		memcpy(phis, ctx->phis, sizeof(tmp_phi*) * ctx->phi_count);
		free(ctx->phis);
		ctx->phis = phis;
		ctx->max_phis = new_size;
	}
	tmp_phi *p = (tmp_phi*)hl_zalloc(&ctx->jit->falloc, sizeof(tmp_phi));
	p->b = b;
	p->r = r;
	if( r ) p->mode = hl_type_mode(r->t);
	p->value = -(++ctx->phi_count);
	phi_add(b->phis,p);
	GET_PHI(p->value) = p;
	return p;
}

static int emit_jump( emit_ctx *ctx, bool cond ) {
	int p = ctx->emit_pos;
	emit_gen(ctx, cond ? JCOND : JUMP, UNUSED, UNUSED, 0);
	return p;
}

static void patch_jump( emit_ctx *ctx, int jpos ) {
	ctx->instrs[jpos].size_offs = ctx->emit_pos - (jpos + 1);
}

static emit_block *alloc_block( emit_ctx *ctx ) {
	emit_block *b = hl_zalloc(&ctx->jit->falloc, sizeof(emit_block));
	b->id = blocks_count(ctx->blocks);
	b->start_pos = ctx->emit_pos;
	blocks_add(ctx->blocks, b);
	if( b->id > 0 ) emit_gen_size(ctx, BLOCK, b->id);
	return b;
}

static void block_add_pred( emit_ctx *ctx, emit_block *b, emit_block *p ) {
	blocks_add(b->preds,p);
	blocks_add(p->nexts,b);
	emit_debug("  PRED #%d\n",p->id);
}

static void store_block_var( emit_ctx *ctx, emit_block *b, vreg *r, ereg v ) {
	if( IS_NULL(v) ) jit_assert();
	vreg_replace(b->written_vars,r->id,v);
	if( v < 0 ) {
		tmp_phi *p = GET_PHI(v);
		p->ref_blocks = link_add_sort_unique(ctx,b->id,b,p->ref_blocks);
	}
}

static void split_block( emit_ctx *ctx ) {
	emit_block *b = alloc_block(ctx);
	b->sealed = true;
	emit_debug("BLOCK #%d@%X[%X]\n",b->id,b->start_pos,ctx->op_pos);
	while( ctx->arrival_points && ctx->arrival_points->id == ctx->op_pos ) {
		block_add_pred(ctx, b, (emit_block*)ctx->arrival_points->ptr);
		ctx->arrival_points = ctx->arrival_points->next;
	}
	bool dead_code = blocks_count(b->preds) == 0; // if we have no reach, force previous block dependency, this is rare dead code emit by compiler
	einstr *eprev = &ctx->instrs[b->start_pos-1];
	if( (eprev->op != JUMP && eprev->op != RET && eprev->mode != M_NORET) || ctx->fun->ops[ctx->op_pos].op == OTrap || dead_code )
		block_add_pred(ctx, b, ctx->current_block);
	ctx->current_block->end_pos = b->start_pos;
	ctx->current_block = b;
}

static void register_jump( emit_ctx *ctx, int jpos, int offs ) {
	int target = offs + ctx->op_pos + 1;
	int_arr_add(ctx->jump_regs, jpos);
	int_arr_add(ctx->jump_regs, target);
	if( offs > 0 ) {
		ctx->arrival_points = link_add_sort_unique(ctx, target, ctx->current_block, ctx->arrival_points);
		if( ctx->arrival_points->id != ctx->op_pos + 1 && ctx->fun->ops[ctx->op_pos].op != OSwitch && ctx->fun->ops[ctx->op_pos+1].op != OLabel )
			split_block(ctx);
	}
}

static ereg emit_load_const( emit_ctx *ctx, uint64 value, hl_type *size_t ) {
	einstr *e = emit_instr(ctx, LOAD_CONST);
	e->mode = hl_type_mode(size_t);
	e->value = value;
	return new_value(ctx);
}

static ereg emit_load_mem( emit_ctx *ctx, ereg v, int offset, hl_type *size_t ) {
	einstr *e = emit_instr(ctx, LOAD_ADDR);
	e->mode = hl_type_mode(size_t);
	e->a = v;
	e->b = UNUSED;
	e->size_offs = offset;
	return new_value(ctx);
}

static void emit_store_reg( emit_ctx *ctx, vreg *to, ereg v ) {
	if( to->t->kind == HVOID ) return;
	if( IS_NULL(v) ) jit_assert();
	store_block_var(ctx,ctx->current_block,to,v);
}

static ereg emit_native_call( emit_ctx *ctx, void *native_ptr, ereg args[], int nargs, hl_type *ret ) {
	einstr *e = emit_instr(ctx, CALL_PTR);
	e->mode = (unsigned char)(ret ? hl_type_mode(ret) : M_NORET);
	e->value = (int_val)native_ptr;
	store_args(ctx, e, args, nargs);
	return ret == NULL || e->mode == M_VOID ? UNUSED : new_value(ctx);
}

static ereg emit_dyn_call( emit_ctx *ctx, ereg f, ereg args[], int nargs, hl_type *ret ) {
	einstr *e = emit_instr(ctx, CALL_REG);
	e->mode = hl_type_mode(ret);
	e->a = f;
	store_args(ctx, e, args, nargs);
	return e->mode == M_VOID ? UNUSED : new_value(ctx);
}

static void emit_test( emit_ctx *ctx, ereg v, hl_op o ) {
	emit_gen_ext(ctx, TEST, v, UNUSED, 0, o);
	patch_instr_mode(ctx, GET_MODE(v));
}

static void emit_cmp( emit_ctx *ctx, ereg a, ereg b, hl_op o ) {
	emit_gen_ext(ctx, CMP, a, b, 0, o);
	patch_instr_mode(ctx, GET_MODE(a));
}

static void phi_remove_val( emit_ctx *ctx, tmp_phi *p, ereg v ) {
	ereg_remove(&p->vals,v);
	emit_debug("%sPHI-REM-DEP %s = %s\n", phi_prefix(ctx), val_str(p->value,p->mode), val_str(v,p->mode));
}

static void phi_add_val( emit_ctx *ctx, tmp_phi *p, ereg v ) {
	if( !p->b ) jit_assert();
	if( IS_NULL(v) ) jit_assert();
	if( p->value == v )
		return;
	if( !ereg_add(p->vals,v) )
		return;
	emit_debug("%sPHI-DEP %s = %s\n", phi_prefix(ctx), val_str(p->value,p->mode), val_str(v,p->mode));
	if( v < 0 ) {
		tmp_phi *p2 = GET_PHI(v);
		phi_add(p2->ref_phis,p);
	}
}

static ereg optimize_phi_rec( emit_ctx *ctx, tmp_phi *p ) {

	if( p->locked ) jit_assert();
	ereg same = UNUSED;
	for_iter(ereg,v,p->vals) {
		if( v == same || v == p->value )
			continue;
		if( !IS_NULL(same) )
			return p->value;
		same = v;
	}
	if( IS_NULL(same) )
		return p->value; // sealed (no dep yet)

	if( !phi_count(p->ref_phis) && !p->ref_blocks )
		return same;

	if( p->locked || p->opt ) jit_assert();

	emit_debug("%sPHI-OPT %s = %s\n", phi_prefix(ctx), val_str(p->value,p->mode), val_str(same,p->mode));
	p->opt = true;
	ctx->phi_depth++;
	linked_inf *l = p->ref_blocks;
	while( l ) {
		emit_block *b = (emit_block*)l->ptr;
		if( vreg_find(b->written_vars,p->r->id) == p->value )
			store_block_var(ctx,b,p->r,same);
		l = l->next;
	}
	for_iter(phi,p2,p->ref_phis) {
		phi_remove_val(ctx,p2,p->value);
		phi_add_val(ctx,p2,same);
	}
	p->ref_blocks = NULL;
	int count = phi_count(p->ref_phis);
	tmp_phi **phis = phi_free(&p->ref_phis);
	for(int i=0;i<count;i++)
		optimize_phi_rec(ctx, phis[i]);
	ctx->phi_depth--;
	emit_debug("%sPHI-OPT-DONE %s = %s\n", phi_prefix(ctx), val_str(p->value,p->mode), val_str(same,p->mode));
	return optimize_phi_rec(ctx,p);
}

static ereg emit_load_reg_block( emit_ctx *ctx, emit_block *b, vreg *r );

static ereg gather_phis( emit_ctx *ctx, tmp_phi *p ) {
	p->locked = true;
	for_iter(blocks,b,p->b->preds) {
		ereg r = p->r ? emit_load_reg_block(ctx, b, p->r) : p->value;
		phi_add_val(ctx, p, r);
	}
	p->locked = false;
	return optimize_phi_rec(ctx, p);
}

static ereg emit_load_reg_block( emit_ctx *ctx, emit_block *b, vreg *r ) {
	ereg v = vreg_find(b->written_vars,r->id);
	if( !IS_NULL(v) )
		return v;
	if( !b->sealed ) {
		tmp_phi *p = alloc_phi(ctx,b,r);
		emit_debug("%sPHI-SEALED %s = R%d\n",phi_prefix(ctx),val_str(p->value,p->mode),r->id);
		v = p->value;
	} else if( blocks_count(b->preds) == 1 )
		v = emit_load_reg_block(ctx, blocks_get(b->preds,0), r);
	else {
		tmp_phi *p = alloc_phi(ctx,b,r);
		store_block_var(ctx,b,r,p->value);
		v = gather_phis(ctx, p);
	}
	store_block_var(ctx,b,r,v);
	return v;
}

static ereg emit_load_reg( emit_ctx *ctx, vreg *r ) {
	if( !IS_NULL(r->ref) )
		return LOAD_MEM(r->ref,0,r->t);
	return emit_load_reg_block(ctx, ctx->current_block, r);
}

static void seal_block( emit_ctx *ctx, emit_block *b ) {
	emit_debug("  SEAL #%d\n",b->id);
	for_iter(phi,p,b->phis)
		gather_phis(ctx, p);
	b->sealed = true;
}

static ereg emit_phi( emit_ctx *ctx, ereg v1, ereg v2 ) {
	unsigned char mode = GET_MODE(v1);
	if( mode != GET_MODE(v2) ) jit_assert();
	tmp_phi *p = alloc_phi(ctx, ctx->current_block, NULL);
	p->mode = mode;
	phi_add_val(ctx, p, v1);
	phi_add_val(ctx, p, v2);
	return p->value;
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
		e->a = findex;
		store_args(ctx, e, args, count);
		STORE(dst, e->mode == M_VOID ? UNUSED : new_value(ctx));
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

static ereg emit_conv( emit_ctx *ctx, ereg v, emit_mode from, emit_mode to, bool _unsigned ) {
	return emit_gen_ext(ctx, _unsigned ? CONV_UNSIGNED : CONV, v, UNUSED, to, from);
}

static bool dyn_need_type( hl_type *t ) {
	return !(IS_FLOAT(t) || t->kind == HI64 || t->kind == HGUID);
}

static ereg emit_dyn_cast( emit_ctx *ctx, ereg v, hl_type *t, hl_type *dt ) {
	if( t->kind == HNULL && t->tparam->kind == dt->kind ) {
		emit_test(ctx, v, OJNotNull);
		int jnot = emit_jump(ctx, true);
		split_block(ctx);
		ereg v1 = LOAD_CONST(0,dt);
		int jend = emit_jump(ctx, false);
		patch_jump(ctx, jnot);
		split_block(ctx);
		ereg v2 = LOAD_MEM(v,0,dt);
		patch_jump(ctx, jend);
		split_block(ctx);
		return emit_phi(ctx, v1, v2);
	}
	bool need_dyn = dyn_need_type(dt);
	ereg st = emit_gen_size(ctx, ALLOC_STACK, HL_WSIZE);
	STORE_MEM(st, 0, v);
	ereg args[3];
	args[0] = st;
	args[1] = LOAD_CONST_PTR(t);
	if( need_dyn ) args[2] = LOAD_CONST_PTR(dt);
	ereg r = emit_native_call(ctx, get_dyncast(dt), args, need_dyn ? 3 : 2, dt);
	return r;
}

static void emit_opcode( emit_ctx *ctx, hl_opcode *o );

static void remap_phi_reg( emit_ctx *ctx, ereg *r ) {
	if( *r >= 0 || IS_NULL(*r) )
		return;
	tmp_phi *p = GET_PHI(*r);
	while( p->final_id < 0 ) {
		if( p->target >= 0 ) {
			*r = p->target;
			return;
		}
		p = GET_PHI(p->target);
	}
	if( p->final_id == 0 )
		return;
	*r = -p->final_id; // new phis
}

static void emit_write_block( emit_ctx *ctx, emit_block *b ) {
	jit_ctx *jit = ctx->jit;
	eblock *bl = jit->blocks + b->id;
	bl->start_pos = b->id == 0 ? 0 : b->start_pos;
	bl->end_pos = b->end_pos;
	bl->pred_count = blocks_count(b->preds);
	bl->next_count = blocks_count(b->nexts);
	bl->preds = (int*)hl_malloc(&jit->falloc,sizeof(int)*bl->pred_count);
	bl->nexts = (int*)hl_malloc(&jit->falloc,sizeof(int)*bl->next_count);
	for(int i=0;i<bl->pred_count;i++)
		bl->preds[i++] = blocks_get(b->preds,i)->id;
	for(int i=0;i<bl->next_count;i++)
		bl->nexts[i++] = blocks_get(b->nexts,i)->id;
	// write phis
	{
		for_iter(phi,p,b->phis)
			if( p->final_id >= 0 )
				bl->phi_count++;
	}
	bl->phis = (ephi*)hl_zalloc(&jit->falloc,sizeof(ephi)*bl->phi_count);
	jit->phi_count += bl->phi_count;
	int i = 0;
	for_iter(phi,p,b->phis) {
		if( p->final_id < 0 )
			continue;
		ephi *p2 = bl->phis + i++;
		if( p->final_id == 0 )
			p2->value = p->value;
		else
			p2->value = -p->final_id;
		p2->mode = p->mode;
		p2->nvalues = ereg_count(p->vals);
		p2->values = (ereg*)hl_malloc(&jit->falloc,sizeof(ereg)*p2->nvalues);
		int k = 0;
		for_iter(ereg,v,p->vals) {
			remap_phi_reg(ctx, &v);
			p2->values[k++] = v;
		}
	}
}

void hl_emit_flush( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	int i = 0;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	while( i < int_arr_count(ctx->jump_regs) ) {
		int pos = int_arr_get(ctx->jump_regs,i++);
		einstr *e = ctx->instrs + pos;
		int target = int_arr_get(ctx->jump_regs,i++);
		e->size_offs = ctx->pos_map[target] - (pos + 1);
	}
	ctx->pos_map[ctx->fun->nops] = -1;
	ctx->current_block->end_pos = ctx->emit_pos;
	jit->instrs = ctx->instrs;
	jit->instr_count = ctx->emit_pos;
	jit->emit_pos_map = ctx->pos_map;
	jit->phi_count = 0;
	jit->block_count = ctx->current_block->id + 1;
	jit->blocks = hl_zalloc(&jit->falloc,sizeof(eblock) * jit->block_count);
	jit->value_count = int_arr_count(ctx->values);
	jit->values_writes = ctx->values.values;
	for_iter(blocks,b,ctx->blocks)
		emit_write_block(ctx,b);
}

void hl_emit_reg_iter( jit_ctx *jit, einstr *e, void *ctx, void (*iter_reg)( void *, ereg * ) ) {
	switch( e->op ) {
	case CALL_REG:
		iter_reg(ctx,&e->a);
	case CALL_FUN:
	case CALL_PTR:
		{
			int i;
			ereg *args = hl_emit_get_args(jit->emit, e);
			for(i=0;i<e->nargs;i++)
				iter_reg(ctx, args + i);
		}
		break;
	case LOAD_CONST:
	case PUSH_CONST:
		// skip
		break;
	default:
		if( !IS_NULL(e->a) ) {
			iter_reg(ctx,&e->a);
			if( !IS_NULL(e->b) )
				iter_reg(ctx,&e->b);
		}
		break;
	}
}

ereg **hl_emit_get_regs( einstr *e, int *count ) {
	static ereg *tmp[2];
	int k = 0;
	switch( e->op ) {
	case CALL_REG:
	case CALL_FUN:
	case CALL_PTR:
		jit_assert();
		break;
	case LOAD_CONST:
	case PUSH_CONST:
		// skip
		break;
	default:
		if( !IS_NULL(e->a) ) {
			tmp[k++] = &e->a;
			if( !IS_NULL(e->b) )
				tmp[k++] = &e->b;
		}
		break;
	}
	*count = k;
	return tmp;
}

static void hl_emit_clean_phis( emit_ctx *ctx ) {
	for(int i=0;i<ctx->phi_count;i++) {
		tmp_phi *p = ctx->phis[i];
		tmp_phi *cur = p;
		ereg r;
		while( true ) {
			cur->opt = false;
			r = optimize_phi_rec(ctx,cur);
			if( r >= 0 || r == cur->value ) break;
			cur = GET_PHI(r);
		}
		p->target = r;
	}
	int new_phis = 0;
	for(int i=0;i<ctx->phi_count;i++) {
		tmp_phi *p = ctx->phis[i];
		if( p->target == p->value )
			p->final_id = ++new_phis;
		else
			p->final_id = -1;
	}
	for(int i=0;i<ctx->emit_pos;i++)
		hl_emit_reg_iter(ctx->jit, ctx->instrs + i, ctx, remap_phi_reg);
}

void hl_emit_function( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	hl_function *f = jit->fun;
	int i;
	ctx->mod = jit->mod;
	ctx->fun = f;
	ctx->emit_pos = 0;
	ctx->trap_count = 0;
	ctx->phi_count = 0;
	ctx->flushed = false;
	int_arr_free(&ctx->args_data);
	int_arr_free(&ctx->jump_regs);
	int_arr_free(&ctx->values);
	blocks_free(&ctx->blocks);
	int_arr_add(ctx->values,-1);
	ctx->current_block = alloc_block(ctx);
	ctx->current_block->sealed = true;
	ctx->arrival_points = NULL;
	emit_debug("---- begin [%X] ----\n",f->findex);
	if( f->nregs > ctx->max_regs ) {
		free(ctx->vregs);
		ctx->vregs = (vreg*)malloc(sizeof(vreg) * (f->nregs + 1));
		if( ctx->vregs == NULL ) jit_assert();
		for(i=0;i<f->nregs;i++)
			R(i)->id = i;
		ctx->max_regs = f->nregs;
	}

	if( f->nops >= ctx->pos_map_size ) {
		free(ctx->pos_map);
		ctx->pos_map = (int*)malloc(sizeof(int) * (f->nops+1));
		if( ctx->pos_map == NULL ) jit_assert();
		ctx->pos_map_size = f->nops;
	}

	for(i=0;i<f->nregs;i++) {
		vreg *r = R(i);
		r->t = f->regs[i];
		r->ref = 0;
	}

	emit_gen(ctx,ENTER,UNUSED,UNUSED,M_NONE);
	for(i=0;i<f->type->fun->nargs;i++) {
		hl_type *t = f->type->fun->args[i];
		if( t->kind == HVOID ) continue;
		STORE(R(i), emit_gen(ctx, LOAD_ARG, UNUSED, UNUSED, hl_type_mode(t)));
	}

	for(int op_pos=0;op_pos<f->nops;op_pos++) {
		ctx->op_pos = op_pos;
		ctx->pos_map[op_pos] = ctx->emit_pos;
		if( op_pos == 0 ) {
			ctx->current_block->start_pos = ctx->emit_pos;
			emit_gen_size(ctx, BLOCK, 0);
		}
		if( ctx->arrival_points && ctx->arrival_points->id == op_pos )
			split_block(ctx);
		emit_opcode(ctx,f->ops + op_pos);
	}

	hl_emit_clean_phis(ctx);
	hl_emit_flush(ctx->jit);
	if( ctx->wait_seal ) jit_assert();
}

void hl_emit_alloc( jit_ctx *jit ) {
	emit_ctx *ctx = (emit_ctx*)malloc(sizeof(emit_ctx));
	if( ctx == NULL ) jit_assert();
	memset(ctx,0,sizeof(emit_ctx));
	ctx->jit = jit;
	jit->emit = ctx;
	if( sizeof(einstr) != 16 ) jit_assert();
}

void hl_emit_free( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	free(ctx->vregs);
	free(ctx->instrs);
	free(ctx->pos_map);
	free(ctx);
	jit->emit = NULL;
}

void hl_emit_final( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	vclosure *l = ctx->closure_list;
	while( l ) {
		vclosure *n = (vclosure*)l->value;
		l->value = NULL;
		l->fun = jit->final_code + (int_val)jit->mod->functions_ptrs[(int_val)l->fun];
		l = n;
	}
	ctx->closure_list = NULL;
}

static bool seal_block_rec( emit_ctx *ctx, emit_block *b, int target ) {
	if( b->start_pos < target )
		return false;
	if( b->start_pos == target ) {
		b->wait_nexts--;
		block_add_pred(ctx, b, ctx->current_block);
		while( b && b->wait_nexts == 0 && ctx->wait_seal == b ) {
			seal_block(ctx,b);
			b = b->wait_seal_next;
			ctx->wait_seal = b;
		}
		return true;
	}
	for_iter(blocks,p,b->preds)
		if( p->start_pos < b->start_pos && seal_block_rec(ctx,p,target) )
			return true;
	return false;
}

static void register_block_jump( emit_ctx *ctx, int offs, bool cond ) {
	int jidx = emit_jump(ctx, cond);
	register_jump(ctx, jidx, offs);
	if( offs < 0 ) {
		int target = ctx->pos_map[ctx->op_pos + 1 + offs];
		emit_block *b = ctx->current_block;
		if( !seal_block_rec(ctx, b, target) ) jit_assert();
	}
}

static void prepare_loop_block( emit_ctx *ctx ) {
	int i, last_jump = -1;
	emit_block *b = ctx->current_block;
	// gather all backward jumps to know when the block will be finished
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
			emit_debug("  WAIT @%X\n",i);
			b->wait_nexts++;
			if( b->sealed ) {
				b->sealed = false;
				b->wait_seal_next = ctx->wait_seal;
				ctx->wait_seal = b;
			}
			last_jump = i;
		}
	}
}

#define HL_DYN_VALUE 8

static void emit_jump_dyn( emit_ctx *ctx, hl_op op, hl_type *at, ereg a, hl_type *bt, ereg b, int offset ) {
	if( at->kind == HDYN || bt->kind == HDYN || at->kind == HFUN || bt->kind == HFUN ) {
		ereg args[2] = { a, b };
		ereg ret = emit_native_call(ctx,hl_dyn_compare,args,2,&hlt_i32);
		if( op == OJSGt || op == OJSGte ) {
			emit_cmp(ctx, ret, LOAD_CONST(hl_invalid_comparison,&hlt_i32), OJEq);
			int jinvalid = emit_jump(ctx, true);
			emit_test(ctx, ret, op);
			register_block_jump(ctx, offset, true);
			patch_jump(ctx, jinvalid);
			return;
		}
		emit_test(ctx, ret, op);
		// continue
	} else switch( at->kind ) {
	case HTYPE:
		{
			ereg args[2] = { a, b };
			ereg ret = emit_native_call(ctx,hl_same_type,args,2,&hlt_bool);
			emit_test(ctx, ret, op);
		}
		break;
	case HNULL:
		{
			if( op == OJEq ) {
				// if( a == b || (a && b && a->v == b->v) ) goto
				emit_cmp(ctx,a,b,OJEq);
				register_block_jump(ctx,offset,true);
				emit_test(ctx,a,OJNull);
				int ja = emit_jump(ctx,true);
				emit_test(ctx,b,OJNull);
				int jb = emit_jump(ctx,true);				
				hl_type *vt = at->tparam;
				emit_cmp(ctx, LOAD_MEM(a,HL_DYN_VALUE,vt), LOAD_MEM(b,HL_DYN_VALUE,vt), OJEq);
				register_block_jump(ctx,offset,true);
				patch_jump(ctx,ja);
				patch_jump(ctx,jb);
			} else if( op == OJNotEq ) {
				// if( a != b && (!a || !b || a->v != b->v) ) goto
				emit_cmp(ctx,a,b,OJEq);
				int jeq = emit_jump(ctx,true);
				emit_test(ctx,a,OJEq);
				register_block_jump(ctx,offset,true);
				emit_test(ctx,b,OJEq);
				register_block_jump(ctx,offset,true);
				hl_type *vt = at->tparam;
				emit_cmp(ctx, LOAD_MEM(a,HL_DYN_VALUE,vt), LOAD_MEM(b,HL_DYN_VALUE,vt), OJNull);
				int jcmp = emit_jump(ctx,true);
				register_block_jump(ctx,offset,true);
				patch_jump(ctx,jcmp);
				patch_jump(ctx,jeq);
			} else
				jit_assert();
			return;
		}
	case HVIRTUAL:
		{
			BREAK();
			/*
			preg p;
			preg *pa = alloc_cpu(ctx,a,true);
			preg *pb = alloc_cpu(ctx,b,true);
			int ja,jb,jav,jbv,jvalue;
			if( b->t->kind == HOBJ ) {
				if( op->op == OJEq ) {
					// if( a ? (b && a->value == b) : (b == NULL) ) goto
					op64(ctx,TEST,pa,pa);
					XJump_small(JZero,ja);
					op64(ctx,TEST,pb,pb);
					XJump_small(JZero,jb);
					op64(ctx,MOV,pa,pmem(&p,pa->id,HL_WSIZE));
					op64(ctx,CMP,pa,pb);
					XJump_small(JAlways,jvalue);
					patch_jump(ctx,ja);
					op64(ctx,TEST,pb,pb);
					patch_jump(ctx,jvalue);
					register_jump(ctx,do_jump(ctx,OJEq,false),targetPos);
					patch_jump(ctx,jb);
				} else if( op->op == OJNotEq ) {
					// if( a ? (b == NULL || a->value != b) : (b != NULL) ) goto
					op64(ctx,TEST,pa,pa);
					XJump_small(JZero,ja);
					op64(ctx,TEST,pb,pb);
					register_jump(ctx,do_jump(ctx,OJEq,false),targetPos);
					op64(ctx,MOV,pa,pmem(&p,pa->id,HL_WSIZE));
					op64(ctx,CMP,pa,pb);
					XJump_small(JAlways,jvalue);
					patch_jump(ctx,ja);
					op64(ctx,TEST,pb,pb);
					patch_jump(ctx,jvalue);
					register_jump(ctx,do_jump(ctx,OJNotEq,false),targetPos);
				} else
					ASSERT(op->op);
				scratch(pa);
				return;
			}
			op64(ctx,CMP,pa,pb);
			if( op->op == OJEq ) {
				// if( a == b || (a && b && a->value && b->value && a->value == b->value) ) goto
				register_jump(ctx,do_jump(ctx,OJEq, false),targetPos);
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,ja);
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jb);
				op64(ctx,MOV,pa,pmem(&p,pa->id,HL_WSIZE));
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,jav);
				op64(ctx,MOV,pb,pmem(&p,pb->id,HL_WSIZE));
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jbv);
				op64(ctx,CMP,pa,pb);
				XJump_small(JNeq,jvalue);
				register_jump(ctx,do_jump(ctx,OJEq, false),targetPos);
				patch_jump(ctx,ja);
				patch_jump(ctx,jb);
				patch_jump(ctx,jav);
				patch_jump(ctx,jbv);
				patch_jump(ctx,jvalue);
			} else if( op->op == OJNotEq ) {
				int jnext;
				// if( a != b && (!a || !b || !a->value || !b->value || a->value != b->value) ) goto
				XJump_small(JEq,jnext);
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,ja);
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jb);
				op64(ctx,MOV,pa,pmem(&p,pa->id,HL_WSIZE));
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,jav);
				op64(ctx,MOV,pb,pmem(&p,pb->id,HL_WSIZE));
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jbv);
				op64(ctx,CMP,pa,pb);
				XJump_small(JEq,jvalue);
				patch_jump(ctx,ja);
				patch_jump(ctx,jb);
				patch_jump(ctx,jav);
				patch_jump(ctx,jbv);
				register_jump(ctx,do_jump(ctx,OJAlways, false),targetPos);
				patch_jump(ctx,jnext);
				patch_jump(ctx,jvalue);
			} else
				ASSERT(op->op);
			scratch(pa);
			scratch(pb);
			return;
			*/
		}
		break;
	case HOBJ:
	case HSTRUCT:
		if( bt->kind == HVIRTUAL ) {
			emit_jump_dyn(ctx,op,bt,b,at,a,offset); // inverse
			return;
		}
		BREAK();
		/*
		if( hl_get_obj_rt(a->t)->compareFun ) {
			preg *pa = alloc_cpu(ctx,a,true);
			preg *pb = alloc_cpu(ctx,b,true);
			preg p;
			int jeq, ja, jb, jcmp;
			int args[] = { a->stack.id, b->stack.id };
			switch( op->op ) {
			case OJEq:
				// if( a == b || (a && b && cmp(a,b) == 0) ) goto
				op64(ctx,CMP,pa,pb);
				XJump_small(JEq,jeq);
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,ja);
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jb);
				op_call_fun(ctx,NULL,(int)(int_val)a->t->obj->rt->compareFun,2,args);
				op32(ctx,TEST,PEAX,PEAX);
				XJump_small(JNotZero,jcmp);
				patch_jump(ctx,jeq);
				register_jump(ctx,do_jump(ctx,OJAlways,false),targetPos);
				patch_jump(ctx,ja);
				patch_jump(ctx,jb);
				patch_jump(ctx,jcmp);
				break;
			case OJNotEq:
				// if( a != b && (!a || !b || cmp(a,b) != 0) ) goto
				op64(ctx,CMP,pa,pb);
				XJump_small(JEq,jeq);
				op64(ctx,TEST,pa,pa);
				register_jump(ctx,do_jump(ctx,OJEq,false),targetPos);
				op64(ctx,TEST,pb,pb);
				register_jump(ctx,do_jump(ctx,OJEq,false),targetPos);

				op_call_fun(ctx,NULL,(int)(int_val)a->t->obj->rt->compareFun,2,args);
				op32(ctx,TEST,PEAX,PEAX);
				XJump_small(JZero,jcmp);

				register_jump(ctx,do_jump(ctx,OJNotEq,false),targetPos);
				patch_jump(ctx,jcmp);
				patch_jump(ctx,jeq);
				break;
			default:
				// if( a && b && cmp(a,b) ?? 0 ) goto
				op64(ctx,TEST,pa,pa);
				XJump_small(JZero,ja);
				op64(ctx,TEST,pb,pb);
				XJump_small(JZero,jb);
				op_call_fun(ctx,NULL,(int)(int_val)a->t->obj->rt->compareFun,2,args);
				op32(ctx,CMP,PEAX,pconst(&p,0));
				register_jump(ctx,do_jump(ctx,op->op,false),targetPos);
				patch_jump(ctx,ja);
				patch_jump(ctx,jb);
				break;
			}
			return;
		}
		*/
		// fallthrough
	default:
		emit_cmp(ctx, a, b, op);
		break;
	}
	register_block_jump(ctx, offset, true);
}

static void emit_opcode( emit_ctx *ctx, hl_opcode *o ) {
	vreg *dst = R(o->p1);
	vreg *ra = R(o->p2);
	vreg *rb = R(o->p3);
	hl_module *m = ctx->mod;
#ifdef HL_DEBUG
	int uid = (ctx->fun->findex << 16) | ctx->op_pos;
	__ignore(&uid);
#endif
	switch( o->op ) {
	case OMov:
	case OUnsafeCast:
		STORE(dst, emit_gen(ctx,MOV,LOAD(ra),UNUSED,hl_type_mode(ra->t)));
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
		STORE(dst, emit_gen_ext(ctx, UNOP, LOAD(ra), UNUSED, hl_type_mode(dst->t), o->op));
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
		emit_jump_dyn(ctx,o->op,dst->t,LOAD(dst),ra->t,LOAD(ra),o->p3);
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
		STORE(dst, emit_conv(ctx,LOAD(ra),hl_type_mode(ra->t),hl_type_mode(dst->t), o->op == OToUFloat));
		break;
	case ORet:
		emit_gen(ctx, RET, dst->t->kind == HVOID ? UNUSED : LOAD(dst), 0, M_NORET);
		patch_instr_mode(ctx, hl_type_mode(dst->t));
		break;
	case OIncr:
	case ODecr:
		{
			if( IS_FLOAT(dst->t) ) {
				jit_assert();
			} else {
				STORE(dst, emit_gen_ext(ctx,UNOP,LOAD(dst),UNUSED,hl_type_mode(dst->t),o->op));
			}
		}
		break;
	case ONew:
		{
			ereg arg = UNUSED;
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
				jit_assert();
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
			ereg st = emit_gen_size(ctx, ALLOC_STACK, o->p3 * HL_WSIZE);
			for(i=0;i<o->p3;i++) {
				vreg *r = R(o->extra[i]);
				if( !hl_is_dynamic(r->t) ) jit_assert();
				STORE_MEM(st,i*HL_WSIZE,LOAD(r));
			}
			ereg args[3];
			args[0] = LOAD(ra);
			args[1] = st;
			args[2] = LOAD_CONST(o->p3,&hlt_i32);
			STORE(dst, emit_dyn_cast(ctx,emit_native_call(ctx,hl_dyn_call,args,3,dst->t),ra->t,dst->t));
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
			if( dst->t->kind != HVOID ) STORE(dst, emit_phi(ctx,v1,v2));
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
							STORE(dst,OFFSET(r, UNUSED, 0, rt->fields_indexes[o->p3]));
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
				jit_assert();
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
				jit_assert();
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
					STORE(dst, OFFSET(obj, UNUSED, 0, field_pos));
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
					ereg eargs = emit_gen_size(ctx, ALLOC_STACK, nargs * HL_WSIZE);
					for(i=0;i<nargs;i++)
						STORE_MEM(eargs,i*HL_WSIZE,LOAD(R(o->extra[i+1])));
					bool need_dyn = !hl_is_ptr(dst->t) && dst->t->kind != HVOID;
					int dyn_size = sizeof(vdynamic);
					ereg edyn = need_dyn ? emit_gen_size(ctx, ALLOC_STACK, dyn_size) : LOAD_CONST_PTR(NULL);

					args = get_tmp_args(ctx, 4);
					args[0] = LOAD_MEM_PTR(obj,HL_WSIZE);
					args[1] = LOAD_CONST(_o->t->virt->fields[o->p2].hashed_name,&hlt_i32);
					args[2] = eargs;
					args[3] = edyn;

					ereg v2 = emit_native_call(ctx, hl_dyn_call_obj, args, 4, dst->t);

					patch_jump(ctx, jend);

					if( dst->t->kind != HVOID ) STORE(dst, emit_phi(ctx, v1, v2));
				}
				break;
			default:
				jit_assert();
				break;
			}
		}
		break;
	case OThrow:
	case ORethrow:
		{
			ereg arg = LOAD(dst);
			emit_native_call(ctx, o->op == OThrow ? hl_throw : hl_rethrow, &arg, 1, NULL);
		}
		break;
	case OLabel:
		if( ctx->current_block->start_pos != ctx->emit_pos-1 )
			split_block(ctx);
		prepare_loop_block(ctx);
		break;
	case OGetI8:
	case OGetI16:
	case OGetMem:
		{
			ereg offs = OFFSET(LOAD(ra),LOAD(rb),1,0);
			ereg val = LOAD_MEM(offs, 0, dst->t);
			if( o->op != OGetMem ) val = emit_conv(ctx, val, M_I32, hl_type_mode(dst->t), false);
			STORE(dst, val);
		}
		break;
	case OSetI8:
	case OSetI16:
	case OSetMem:
		{
			ereg offs = OFFSET(LOAD(dst), LOAD(ra),1,0);
			ereg val = LOAD(rb);
			if( o->op != OSetMem ) val = emit_conv(ctx, val, M_I32, hl_type_mode(dst->t), false);
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
			if( IS_NULL(ra->ref) )
				ra->ref = emit_gen_size(ctx, ALLOC_STACK, hl_type_size(ra->t));
			ereg r = vreg_find(ctx->current_block->written_vars, ra->id);
			if( !IS_NULL(r) ) {
				STORE_MEM(ra->ref, 0, r);
				vreg_remove(&ctx->current_block->written_vars, ra->id);
			}
			STORE(dst,ra->ref);
		}
		break;
	case OUnref:
		STORE(dst, LOAD_MEM(LOAD(ra),0,dst->t));
		break;
	case OSetref:
		STORE_MEM(dst->ref,0,LOAD(ra));
		break;
	case ORefData:
		switch( ra->t->kind ) {
		case HARRAY:
			STORE(dst, OFFSET(LOAD(ra),UNUSED,0,sizeof(varray)));
			break;
		default:
			jit_assert();
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
				if( f == NULL ) jit_assert();
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
			null_field_access = false;
			if( null_field_access ) {
				einstr *e = emit_instr(ctx, PUSH_CONST);
				e->mode = M_PTR;
				e->value = hashed_name;
			}
			emit_native_call(ctx, null_field_access ? hl_jit_null_field_access : hl_null_access, NULL, 0, NULL);
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
			current_addr = OFFSET(thread, UNUSED, 0, (int)(int_val)&tinf->trap_current);
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
			args[0] = st;
#if defined(HL_WIN) && defined(HL_64)
			// On Win64 setjmp actually takes two arguments
			// the jump buffer and the frame pointer (or the stack pointer if there is no FP)
			nargs = 2;
			args[1] = emit_gen(ctx,LEA,MK_STACK_REG(0),UNUSED,M_PTR);
#endif
#ifdef HL_MINGW
			fun = _setjmp;
#endif
			ereg ret = emit_native_call(ctx, fun, args, nargs, &hlt_i32);
			emit_test(ctx, ret, OJNull);
			int jskip = emit_jump(ctx, true);
			STORE(dst, tinf ? LOAD_CONST_PTR(&tinf->exc_value) : LOAD_MEM_PTR(thread,(int)(int_val)&tinf->exc_value));

			int jtrap = emit_jump(ctx, false);
			register_jump(ctx, jtrap, o->p2);
			patch_jump(ctx, jskip);

			if( ctx->trap_count == MAX_TRAPS ) jit_error("Too many try/catch depth");
			ctx->traps[ctx->trap_count++] = st;
		}
		break;
	case OEndTrap:
		{
			if( ctx->trap_count == 0 ) jit_assert();
			ereg st = ctx->traps[ctx->trap_count - 1];

			ereg thread, current_addr;
			static hl_thread_info *tinf = NULL;
			static hl_trap_ctx *trap = NULL;
#			ifndef HL_THREADS
			if( tinf == NULL ) tinf = hl_get_thread();
			current_addr = LOAD_CONST_PTR(&tinf->trap_current);
#			else
			thread = emit_native_call(ctx, hl_get_thread, NULL, 0, &hlt_bytes);
			current_addr = OFFSET(thread, UNUSED, 0, (int)(int_val)&tinf->trap_current);
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
		}
		break;
	case OSwitch:
		{
			ereg v = LOAD(dst);
			int count = o->p2;
			emit_cmp(ctx,v,LOAD_CONST(count,&hlt_i32),OJUGte);
			int jdefault = emit_jump(ctx, true);
			emit_gen_ext(ctx, JUMP_TABLE, v, UNUSED, 0, count);
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
		emit_native_call(ctx, hl_jit_assert, NULL, 0, &hlt_void);
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
						r = OFFSET(r, UNUSED, 0, rt->fields_indexes[o->p2-1]);
					}
					break;
				default:
					jit_assert();
					break;
				}
			}
			emit_gen(ctx, PREFETCH, r, UNUSED, o->p3);
		}
		break;
	case OAsm:
		jit_assert();
		break;
	case OCatch:
		// Only used by OTrap typing
		break;
	default:
		jit_error(hl_op_name(o->op));
		break;
	}
}
