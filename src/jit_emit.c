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

typedef struct {
	hl_type *t;
	int id;
} vreg;

#define MAX_TMP_ARGS	32
#define MAX_TRAPS		32
#define MAX_REFS		512 // TODO : different impl

typedef struct _linked_inf linked_inf;
typedef struct _emit_block emit_block;
typedef struct _tmp_phi tmp_phi;

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
	linked_inf *nexts;
	linked_inf *preds;
	linked_inf *written_vars;
	linked_inf *phis;
	emit_block *wait_seal_next;
};

struct _tmp_phi {
	ereg value;
	vreg *r;
	emit_block *b;
	linked_inf *vals;
};

struct _emit_ctx {
	hl_module *mod;
	hl_function *fun;
	jit_ctx *jit;

	einstr *instrs;
	vreg *vregs;
	int max_instrs;
	int max_regs;
	int emit_pos;
	int op_pos;
	int phi_count;
	bool flushed;

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
	int_alloc values;
	
	emit_block *root_block;
	emit_block *current_block;
	emit_block *wait_seal;
	linked_inf *arrival_points;
	void *closure_list; // TODO : patch with good addresses
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
#define BREAK() emit_gen(ctx, DEBUG_BREAK, ENULL, ENULL, 0)
#define GET_WRITE(r) ctx->instrs[ctx->values.data[(r).index]]

#define HDYN_VALUE 8

#define IS_FLOAT(t)	((t)->kind == HF64 || (t)->kind == HF32)

static hl_type hlt_ui8 = { HUI8, 0 };
static hl_type hlt_ui16 = { HUI16, 0 };
static ereg ENULL = {VAL_NULL};

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

static void *link_sort_remove( linked_inf *head, int id ) {
	linked_inf *prev = NULL;
	linked_inf *cur = head;
	while( cur && cur->id < id ) {
		prev = cur;
		cur = cur->next;
	}
	if( cur && cur->id == id ) {
		if( !prev )
			return head->ptr;
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
	return M_PTR;
}

static ereg new_value( emit_ctx *ctx ) {
	ereg r = {ctx->values.cur};
	int_alloc_store(&ctx->values, ctx->emit_pos-1);
	return r;
}

static ereg *get_tmp_args( emit_ctx *ctx, int count ) {
	if( count > MAX_TMP_ARGS ) jit_error("Too many arguments");
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
	e->mode = GET_WRITE(from).mode;
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
		e->size_offs = args[0].index;
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
	return (ereg*)(ctx->args_data.data + e->size_offs);
}

static ereg emit_gen_ext( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode, int size_offs ) {
	einstr *e = emit_instr(ctx, op);
	if( (unsigned char)mode != mode ) jit_assert();
	e->mode = (unsigned char)mode;
	e->size_offs = size_offs;
	e->a = a;
	e->b = b;
	return mode == 0 || mode == M_NORET ? ENULL : new_value(ctx);
}

static ereg emit_gen( emit_ctx *ctx, emit_op op, ereg a, ereg b, int mode ) {
	return emit_gen_ext(ctx,op,a,b,mode,0);
}

static ereg emit_gen_size( emit_ctx *ctx, emit_op op, int size_offs ) {
	return emit_gen_ext(ctx,op,ENULL,ENULL,op==ALLOC_STACK ? M_PTR : 0,size_offs);
}

static void patch_instr_mode( emit_ctx *ctx, int mode ) {
	ctx->instrs[ctx->emit_pos-1].mode = (unsigned char)mode;
}

static tmp_phi *alloc_phi( emit_ctx *ctx, emit_block *b, vreg *r ) {
	tmp_phi *p = (tmp_phi*)hl_zalloc(&ctx->jit->falloc, sizeof(tmp_phi));
	p->b = b;
	p->r = r;
	p->value.index = -(++ctx->phi_count);
	b->phis = link_add(ctx, r->id, p, b->phis);
	return p;
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
	return hl_zalloc(&ctx->jit->falloc, sizeof(emit_block));
}

static void block_add_pred( emit_ctx *ctx, emit_block *b, emit_block *p ) {
	b->preds = link_add(ctx,0,p,b->preds);
	p->nexts = link_add(ctx,0,b,p->nexts);
	jit_debug("  PRED #%d\n",p->id);
}

static void store_block_var( emit_ctx *ctx, emit_block *b, vreg *r, ereg v ) {
	if( IS_NULL(v) ) jit_assert();
	b->written_vars = link_add_sort_replace(ctx,r->id,(void*)(int_val)(v.index < 0 ? v.index : v.index + 1),b->written_vars); 
}

static ereg lookup_block_var( emit_block *b, vreg *r ) {
	void *p = link_sort_lookup(b->written_vars,r->id);
	if( !p ) return ENULL;
	int e = (int)(int_val)p;
	ereg v;
	v.index = e < 0 ? e : e-1;
	return v;
}

static void remove_block_var( emit_block *b, vreg *r ) {
	b->written_vars = link_sort_remove(b->written_vars, r->id);
}

static void split_block( emit_ctx *ctx ) {
	emit_block *b = alloc_block(ctx);
	b->sealed = true;
	b->id = ctx->current_block->id + 1;
	b->start_pos = ctx->emit_pos;
	jit_debug("BLOCK #%d@%X[%X]\n",b->id,b->start_pos,ctx->op_pos);
	while( ctx->arrival_points && ctx->arrival_points->id == ctx->op_pos ) {
		block_add_pred(ctx, b, (emit_block*)ctx->arrival_points->ptr);
		ctx->arrival_points = ctx->arrival_points->next;
	}
	bool dead_code = b->preds == NULL; // if we have no reach, force previous block dependency, this is rare dead code emit by compiler
	einstr *eprev = &ctx->instrs[ctx->emit_pos-1];
	if( (eprev->op != JUMP && eprev->op != RET && eprev->mode != M_NORET) || ctx->fun->ops[ctx->op_pos].op == OTrap || dead_code )
		block_add_pred(ctx, b, ctx->current_block);
	ctx->current_block->end_pos = ctx->emit_pos - 1;
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
	return new_value(ctx);
}

static ereg emit_load_mem( emit_ctx *ctx, ereg v, int offset, hl_type *size_t ) {
	einstr *e = emit_instr(ctx, LOAD_ADDR);
	e->mode = hl_type_mode(size_t);
	e->a = v;
	e->b.index = offset;
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
	return ret == NULL || e->mode == M_VOID ? ENULL : new_value(ctx);
}

static ereg emit_dyn_call( emit_ctx *ctx, ereg f, ereg args[], int nargs, hl_type *ret ) {
	einstr *e = emit_instr(ctx, CALL_REG);
	e->mode = hl_type_mode(ret);
	e->a = f;
	store_args(ctx, e, args, nargs);
	return e->mode == M_VOID ? ENULL : new_value(ctx);
}

static void emit_test( emit_ctx *ctx, ereg v, hl_op o ) {
	emit_gen_ext(ctx, TEST, v, ENULL, 0, o);
	patch_instr_mode(ctx, GET_WRITE(v).mode);
}

static void phi_add_val( emit_ctx *ctx, tmp_phi *p, ereg v ) {
	if( !p->b ) jit_assert();
	if( IS_NULL(v) ) jit_assert();
	if( link_sort_lookup(p->vals,v.index) )
		return;
	jit_debug("  PHI-DEP %s = %s\n", val_str(p->value), val_str(v));
	p->vals = link_add_sort_unique(ctx,v.index,p,p->vals);
}

static ereg optimize_phi_rec( emit_ctx *ctx, tmp_phi *p ) {
	ereg same = ENULL;
	linked_inf *l = p->vals;
	while( l ) {
		if( l->id == same.index || l->id == p->value.index ) {
			l = l->next;
			continue;
		}
		if( !IS_NULL(same) )
			return p->value;
		same.index = l->id;
		l = l->next;
	}
	if( IS_NULL(same) ) jit_assert(); // unwritten var access ?
	// TODO
	jit_debug("  PHI-OPT %s = %s\n", val_str(p->value), val_str(same));
	return same;
}

static ereg emit_load_reg_block( emit_ctx *ctx, emit_block *b, vreg *r );

static ereg gather_phis( emit_ctx *ctx, tmp_phi *p ) {
	linked_inf *l = p->b->preds;
	while( l ) {
		ereg r = emit_load_reg_block(ctx, (emit_block*)l->ptr, p->r);
		phi_add_val(ctx, p, r);
		l = l->next;
	}
	return optimize_phi_rec(ctx, p);
}

static ereg emit_load_reg_block( emit_ctx *ctx, emit_block *b, vreg *r ) {
	ereg v = lookup_block_var(b,r);
	if( !IS_NULL(v) )
		return v;
	if( !b->sealed ) {
		tmp_phi *p = alloc_phi(ctx,b,r);
		v = p->value;
	} else if( b->preds && !b->preds->next )
		v = emit_load_reg_block(ctx, (emit_block*)b->preds->ptr, r);
	else {
		tmp_phi *p = alloc_phi(ctx,b,r);
		store_block_var(ctx,b,r,p->value);
		v = gather_phis(ctx, p);
	}
	store_block_var(ctx,b,r,v);
	return v;
}

static ereg emit_load_reg( emit_ctx *ctx, vreg *r ) {
	ereg ref = resolve_ref(ctx, r->id);
	if( ref.index >= 0 )
		return LOAD_MEM(ref,0,r->t);
	return emit_load_reg_block(ctx, ctx->current_block, r);
}

static void seal_block( emit_ctx *ctx, emit_block *b ) {
	jit_debug("  SEAL #%d\n",b->id);
	linked_inf *l = b->phis;
	while( l ) {
		tmp_phi *p = (tmp_phi*)l->ptr;
		gather_phis(ctx, p);
		l = l->next;
	}
	b->sealed = true;
}

static ereg emit_phi( emit_ctx *ctx, ereg v1, ereg v2 ) {
	unsigned char mode = GET_WRITE(v1).mode;
	if( mode != GET_WRITE(v2).mode ) jit_assert();
	tmp_phi *p = alloc_phi(ctx, ctx->current_block, NULL);
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
		e->a.index = findex;
		store_args(ctx, e, args, count);
		STORE(dst, e->mode == M_VOID ? ENULL : new_value(ctx));
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

static void emit_write_blocks( jit_ctx *jit, emit_block *b ) {
	eblock *bl = jit->blocks + b->id;
	if( bl->id >= 0 ) return; // already set
	bl->id = b->id;
	bl->start_pos = b->start_pos;
	bl->end_pos = b->end_pos;
	linked_inf *tmp;
	tmp = b->preds;
	while( tmp ) {
		bl->pred_count++;
		tmp = tmp->next;
	}
	tmp = b->nexts;
	while( tmp ) {
		bl->next_count++;
		tmp = tmp->next;
	}
	bl->preds = (int*)hl_malloc(&jit->falloc,sizeof(int)*bl->pred_count);
	bl->nexts = (int*)hl_malloc(&jit->falloc,sizeof(int)*bl->next_count);
	int i;
	i = 0;
	tmp = b->preds;
	while( tmp ) {
		bl->preds[i++] = ((emit_block*)tmp->ptr)->id;
		tmp = tmp->next;
	}
	i = 0;
	tmp = b->nexts;
	while( tmp ) {
		emit_block *n = (emit_block*)tmp->ptr;
		bl->nexts[i++] = n->id;
		if( n->start_pos > b->start_pos ) emit_write_blocks(jit, n);
		tmp = tmp->next;
	}
	// write phis
	tmp = b->phis;
	while( tmp ) {
		bl->phi_count++;
		tmp = tmp->next;
	}
	bl->phis = (ephi*)hl_zalloc(&jit->falloc,sizeof(ephi)*bl->phi_count);
	tmp = b->phis;
	i = 0;
	while( tmp ) {
		tmp_phi *p = (tmp_phi*)tmp->ptr;
		ephi *p2 = bl->phis + i++;
		p2->value = p->value;
		linked_inf *l = p->vals;
		while( l ) {
			p2->nvalues++;
			l = l->next;
		}
		p2->values = (ereg*)hl_malloc(&jit->falloc,sizeof(ereg)*p2->nvalues);
		l = p->vals;
		int k = 0;
		while( l ) {
			p2->values[k++].index = l->id;
			l = l->next;
		}
		tmp = tmp->next;
	}
}

void hl_emit_flush( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	int i = 0;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	while( i < ctx->jump_regs.cur ) {
		int pos = ctx->jump_regs.data[i++];
		einstr *e = ctx->instrs + pos;
		int target = ctx->jump_regs.data[i++];
		e->size_offs = ctx->pos_map[target] - (pos + 1);
	}
	ctx->pos_map[ctx->fun->nops] = -1;
	ctx->current_block->end_pos = ctx->emit_pos - 1;
	jit->instrs = ctx->instrs;
	jit->instr_count = ctx->emit_pos;
	jit->emit_pos_map = ctx->pos_map;
	jit->block_count = ctx->current_block->id + 1;
	jit->blocks = hl_zalloc(&jit->falloc,sizeof(eblock) * jit->block_count);
	for(i=0;i<jit->block_count;i++)
		jit->blocks[i].id = -1;
	jit->value_count = ctx->values.cur;
	jit->values_writes = ctx->values.data;
	emit_write_blocks(jit, ctx->root_block);
}

void hl_emit_function( jit_ctx *jit ) {
	emit_ctx *ctx = jit->emit;
	hl_function *f = jit->fun;
	int i;
	ctx->mod = jit->mod;
	ctx->fun = f;
	ctx->emit_pos = 0;
	ctx->trap_count = 0;
	ctx->ref_count = 0;
	ctx->phi_count = 0;
	ctx->flushed = false;
	int_alloc_reset(&ctx->args_data);
	int_alloc_reset(&ctx->jump_regs);
	int_alloc_reset(&ctx->values);
	ctx->root_block = ctx->current_block = alloc_block(ctx);
	ctx->current_block->sealed = true;
	ctx->arrival_points = NULL;
	jit_debug("---- begin [%X] ----\n",f->findex);
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
	}

	for(i=0;i<f->type->fun->nargs;i++) {
		hl_type *t = f->type->fun->args[i];
		if( t->kind == HVOID ) continue;
		STORE(R(i), emit_gen(ctx, LOAD_ARG, ENULL, ENULL, hl_type_mode(t)));
	}

	for(i=f->nops-1;i>=0;i--) {
		hl_opcode *o = f->ops + i;
		if( o->op == ORef ) {
			ereg ref = resolve_ref(ctx, o->p2);
			if( ref.index >= 0 ) continue;
			if( ctx->ref_count == MAX_REFS ) jit_error("Too many refs");
			ctx->refs[ctx->ref_count].r = emit_gen_size(ctx, ALLOC_STACK, hl_type_size(R(o->p2)->t));
			ctx->refs[ctx->ref_count].reg = o->p2;
			ctx->ref_count++;
		}
	}

	for(int op_pos=0;op_pos<f->nops;op_pos++) {
		ctx->op_pos = op_pos;
		ctx->pos_map[op_pos] = ctx->emit_pos;
		if( ctx->arrival_points && ctx->arrival_points->id == op_pos )
			split_block(ctx);
		emit_opcode(ctx,f->ops + op_pos);
	}

	hl_emit_flush(ctx->jit);
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
	int_alloc_free(&ctx->jump_regs);
	int_alloc_free(&ctx->args_data);
	int_alloc_free(&ctx->values);
	free(ctx);
	jit->emit = NULL;
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
			jit_debug("  WAIT @%X\n",i);
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
		STORE(dst, emit_gen(ctx,MOV,LOAD(ra),ENULL,hl_type_mode(ra->t)));
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
			emit_gen_ext(ctx, CMP, LOAD(dst), LOAD(ra), 0, o->op);
			patch_instr_mode(ctx, hl_type_mode(dst->t));
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
		emit_gen(ctx, RET, dst->t->kind == HVOID ? ENULL : LOAD(dst), ENULL, M_NORET);
		patch_instr_mode(ctx, hl_type_mode(dst->t));
		break;
	case OIncr:
	case ODecr:
		{
			if( IS_FLOAT(dst->t) ) {
				jit_assert();
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
			ereg st = emit_gen_size(ctx, ALLOC_STACK, o->p3);
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
		if( ctx->current_block->start_pos != ctx->emit_pos )
			split_block(ctx);
		prepare_loop_block(ctx);
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
			if( IS_NULL(ref) ) jit_assert();
			ereg r = lookup_block_var(ctx->current_block, ra);
			if( !IS_NULL(r) ) {
				STORE_MEM(ref, 0, LOAD(ra));
				remove_block_var(ctx->current_block, ra);
			}
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
			ereg arg = null_field_access ? LOAD_CONST(hashed_name,&hlt_i32) : ENULL;
			emit_native_call(ctx, null_field_access ? hl_jit_null_field_access : hl_jit_null_access, &arg, null_field_access ? 1 : 0, NULL);
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
			emit_gen_ext(ctx, CMP, v, LOAD_CONST(count,&hlt_i32), 0, OJUGte);
			patch_instr_mode(ctx, M_I32);
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
						r = OFFSET(r, ENULL, 0, rt->fields_indexes[o->p2-1]);
					}
					break;
				default:
					jit_assert();
					break;
				}
			}
			emit_gen(ctx, PREFETCH, r, ENULL, o->p3);
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
