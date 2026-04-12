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

#define VAL(k)		(ctx->values + (k))

#if defined(HL_WIN_CALL) && defined(HL_64)
#	define IS_WINCALL64 1
#else
#	define IS_WINCALL64 0
#endif

#define VIDX(e)	(((e) < 0) ? ctx->jit->value_count + (-(e)-1) : (e)) 
#define REG_MODE(m)	(((m) == M_F32 || (m) == M_F64) ? 1 :0)

typedef struct {
	int stack_pos;
	emit_mode mode;
	ereg current;
	bool saved;
} value_info;

#define S_TYPE			values
#define S_NAME(name)	values_##name
#define S_VALUE			value_info*
#include "data_struct.c"
#define values_add(set,v)		values_add_impl(DEF_ALLOC,&(set),v)

struct _regs_ctx {
	jit_ctx *jit;
	regs_config cfg;
	value_info *values;
	values scratch;
	int max_instrs;
	int cur_op;
	int emit_pos;
	int persists_uses[2];
	einstr *instrs;
	ereg *out_write;
	int *pos_map;
};

typedef int call_regs[2];

void hl_jit_init_regs( regs_config cfg );

static ereg get_call_reg( regs_ctx *ctx, call_regs regs, emit_mode m ) {
	ereg r;
	int mode = REG_MODE(m);
	reg_config *cfg = &ctx->cfg[mode];
	if( regs[mode] < cfg->nargs )
		r = cfg->arg[regs[mode]++];
	else
		r = VAL_NULL;
	return r;
}

static void regs_write_instr( regs_ctx *ctx, einstr *e, ereg out ) {
	if( ctx->emit_pos == ctx->max_instrs ) {
		int pos = ctx->emit_pos;
		int next_size = ctx->max_instrs ? (ctx->max_instrs << 1) : 256;
		einstr *instrs = (einstr*)malloc(sizeof(einstr) * next_size);
		ereg *out = (ereg*)malloc(sizeof(ereg) * next_size);
		if( instrs == NULL || out == NULL ) jit_error("Out of memory");
		memcpy(instrs, ctx->instrs, pos * sizeof(einstr));
		memcpy(out, ctx->out_write, pos * sizeof(ereg));
		memset(instrs + pos, 0, (next_size - pos) * sizeof(einstr));
		free(ctx->instrs);
		free(ctx->out_write);
		ctx->instrs = instrs;
		ctx->out_write = out;
		ctx->max_instrs = next_size;
	} else if( (ctx->emit_pos & 0xFF) == 0 )
		memset(ctx->instrs + ctx->emit_pos, 0, 256 * sizeof(einstr));
	ctx->out_write[ctx->emit_pos] = out;
	ctx->instrs[ctx->emit_pos++] = *e;
}

static void regs_emit_mov( regs_ctx *ctx, ereg to, ereg from, emit_mode m ) {
	if( to == from ) return;
	einstr e;
	e.header = MOV;
	e.mode = m;
	e.size_offs = 0;
	e.a = from;
	e.b = VAL_NULL;
	regs_write_instr(ctx, &e, to);
}

#define regs_todo()	regs_emit_todo_impl(ctx,__LINE__)
static void regs_emit_todo_impl( regs_ctx *ctx, int line ) {
	einstr e;
	e.header = PUSH_CONST;
	e.mode = M_I32;
	e.value = line;
	regs_write_instr(ctx, &e, VAL_NULL);
	einstr e2;
	e2.header = DEBUG_BREAK;
	e2.size_offs = 0;
	e2.a = VAL_NULL;
	e2.b = VAL_NULL;
	regs_write_instr(ctx, &e2, VAL_NULL);
}


static void regs_assign( regs_ctx *ctx, value_info *v, ereg *forced ) {
	if( forced ) {		
		if( v->current == *forced )
			return;
		// erase previous value
		for_iter(values,v2,ctx->scratch) {
			if( v2->current == *forced ) {
				if( !v2->saved ) jit_assert();
				v2->current = VAL_NULL;
				values_remove(&ctx->scratch,v2);
				break;
			}
		}
		v->current = *forced;
		values_add(ctx->scratch,v);
	} else {
		// lookup available reg
		int mode = REG_MODE(v->mode);
		reg_config *cfg = &ctx->cfg[mode];
		for(int i=0;i<cfg->nscratchs;i++) {
			ereg r = cfg->scratch[i];
			for_iter(values,v2,ctx->scratch) {
				if( v2->current == r ) {
					r = VAL_NULL;
					break;
				}
			}
			if( !IS_NULL(r) ) {
				v->current = r;
				values_add(ctx->scratch,v);
				return;
			}
		}
		if( ctx->persists_uses[mode] < cfg->npersists ) {
			v->current = cfg->persist[ctx->persists_uses[mode]++];
			return;
		}
		// free the oldest scratch reg
		value_info *v2 = values_first(ctx->scratch);
		if( !v2->saved ) {
			regs_emit_mov(ctx, MK_STACK_REG(v2->stack_pos), v2->current, v2->mode);
			v2->saved = true;			
		}
		v->current = v2->current;
		values_remove(&ctx->scratch, v2);
		values_add(ctx->scratch, v);
	}
}

static void regs_assign_stack_regs( regs_ctx *ctx ) {
	// dummy regs assign for testing : set all values on stack
	int stack_pos = 0;
	int nargs = ctx->jit->fun->type->fun->nargs;
	call_regs regs = {0};
	int args_size = 0;
	for(int i=0;i<ctx->jit->value_count;i++) {
		value_info *v = VAL(i);
		einstr *e = ctx->jit->instrs + ctx->jit->values_writes[i];
		int size = hl_emit_mode_sizes[e->mode];
		if( size <= 0 ) hl_jit_assert();
		if( i < nargs ) {
			ereg r = get_call_reg(ctx,regs,e->mode);
			if( !IS_NULL(r) ) {
				v->current = r;
				values_add(ctx->scratch,v);
			}
			if( IS_NULL(r) || IS_WINCALL64 ) {
				// use existing stack storage
				v->stack_pos = args_size + HL_WSIZE*2;
				args_size += size < 4 ? 4 : size;
				continue;
			}
		}
		stack_pos += size;
		stack_pos += jit_pad_size(stack_pos,size);
		v->stack_pos = MK_STACK_REG(-stack_pos);
	}
	for(int i=0;i<ctx->jit->block_count;i++) {
		eblock *b = ctx->jit->blocks + i;
		for(int k=0;k<b->phi_count;k++) {
			ephi *p = b->phis + k;
			int size = hl_emit_mode_sizes[p->mode];
			value_info *v = VAL(VIDX(p->value));
			stack_pos += size;
			stack_pos += jit_pad_size(stack_pos,size);
			v->stack_pos = MK_STACK_REG(-stack_pos);
		}
	}
}

void hl_regs_alloc( jit_ctx *jit ) {
	regs_ctx *ctx = malloc(sizeof(regs_ctx));
	memset(ctx,0,sizeof(regs_ctx));
	ctx->jit = jit;
	jit->regs = ctx;
	hl_jit_init_regs(ctx->cfg);
}

void hl_regs_flush( jit_ctx *jit ) {
	regs_ctx *ctx = jit->regs;
	jit->reg_instr_count = ctx->emit_pos;
	jit->reg_instrs = ctx->instrs;
	jit->reg_writes = ctx->out_write;
	jit->reg_pos_map = ctx->pos_map;
	ctx->pos_map[ctx->cur_op+1] = ctx->emit_pos;
}

void hl_regs_function( jit_ctx *jit ) {
	regs_ctx *ctx = jit->regs;
	int nvalues = jit->value_count + jit->phi_count;
	memset(ctx->persists_uses,0,sizeof(ctx->persists_uses));
	free(ctx->pos_map);
	ctx->pos_map = (int*)malloc((jit->instr_count + 1) * sizeof(int));
	ctx->emit_pos = 0;
	ctx->cur_op = 0;
	ctx->pos_map[0] = 0;
	values_reset(&ctx->scratch);
	ctx->values = malloc(sizeof(value_info) * nvalues);
	memset(ctx->values, 0, sizeof(value_info) * nvalues);
	for(int i=0;i<nvalues;i++) {
		value_info *v = VAL(i);
		v->current = VAL_NULL;
	}
	regs_assign_stack_regs(ctx);
	int write_index = 0;
	for(int i=0;i<jit->instr_count;i++) {
		einstr e = jit->instrs[i];
		ereg *ret_val = NULL;
		int nread;
		ctx->cur_op = i;
		if( i > 0 )
			ctx->pos_map[i] = ctx->emit_pos;
		if( IS_CALL(e.op) ) {
			ereg *args = hl_emit_get_args(ctx->jit->emit,&e);
			for_iter(values,v,ctx->scratch) {
				if( !v->saved ) {
					v->saved = true;
					regs_emit_mov(ctx, MK_STACK_REG(v->stack_pos), v->current, v->mode);
				}
			}
			call_regs regs = {0};
			for(int k=0;k<e.nargs;k++) {
				value_info *v = VAL(VIDX(args[k]));
				ereg r = get_call_reg(ctx,regs,v->mode);
				if( IS_NULL(r) ) {
					regs_todo();
				} else if( IS_NULL(v->current) )
					regs_emit_mov(ctx, r, MK_STACK_REG(v->stack_pos), v->mode);
				else
					regs_emit_mov(ctx, r, v->current, v->mode);
			}
			for_iter(values,v2,ctx->scratch)
				v2->current = VAL_NULL;
			values_reset(&ctx->scratch);
			e.nargs = 0xFF;
			ret_val = &ctx->cfg[REG_MODE(e.mode)].ret;
		} else {
			ereg **regs = hl_emit_get_regs(&e,&nread);
			for(int k=0;k<nread;k++) {
				ereg *r = regs[k];
				value_info *v = VAL(VIDX(*r));
				if( IS_NULL(v->current) ) {
					regs_assign(ctx,v,NULL);
					regs_emit_mov(ctx, v->current, MK_STACK_REG(v->stack_pos), v->mode);
				}
				*r = v->current;
			}
		}
		ereg out;
		if( write_index < jit->value_count && jit->values_writes[write_index] == i ) {
			value_info *v = VAL(write_index++);
			regs_assign(ctx,v,ret_val);
			out = v->current;
		} else
			out = VAL_NULL;
		regs_write_instr(ctx, &e, out);
	}
	free(ctx->values);
	hl_regs_flush(ctx->jit);
}

void hl_regs_free( regs_ctx *ctx ) {
	free(ctx->pos_map);
	free(ctx->instrs);
	free(ctx->out_write);
	free(ctx);
}

