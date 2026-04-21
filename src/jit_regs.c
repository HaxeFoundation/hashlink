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

//#define REGS_DEBUG

#ifdef REGS_DEBUG
#	define regs_debug	jit_debug
#else
#	define regs_debug(...)
#endif

#define INVALID	0x80000000

#define VIDX(e)	(((e) < 0) ? ctx->jit->value_count + (-(e)-1) : (e))
#define VAL_REG(e) VAL(VIDX(e))
#define REG_MODE(m)	(((m) == M_F32 || (m) == M_F64) ? 1 :0)
#define REG_CFG(m)	(m ? &ctx->jit->cfg.floats : &ctx->jit->cfg.regs)

#if defined(HL_WIN_CALL) && defined(HL_64)
#	define IS_WINCALL64 1
#else
#	define IS_WINCALL64 0
#endif

typedef struct {
	int stack_pos;
	int last_read;
	emit_mode mode;
	ereg pref_reg;
	ereg reg;
} value_info;

#define S_TYPE			values
#define S_NAME(name)	values_##name
#define S_VALUE			value_info*
#include "data_struct.c"
#define values_add(set,v)		values_add_impl(DEF_ALLOC,&(set),v)

struct _regs_ctx {
	jit_ctx *jit;
	value_info *values;
	values scratch;
	int_arr jump_regs;
	int_arr pack_movs;
	int_arr *blocks_phis;
	int max_instrs;
	int cur_op;
	int emit_pos;
	int stack_size;
	int stack_offset;
	einstr *instrs;
	ereg *out_write;
	int *pos_map;
	bool flushed;
	bool has_direct_call;
	int persists_uses[2];
};

typedef int call_regs[2];

static ereg get_call_reg( regs_ctx *ctx, call_regs regs, emit_mode m ) {
	ereg r;
	int mode = REG_MODE(m);
	reg_config *cfg = REG_CFG(mode);
	if( regs[mode] < cfg->nargs )
		r = cfg->arg[regs[mode]++];
	else
		r = UNUSED;
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

static void regs_emit( regs_ctx *ctx, ereg out, emit_op op, ereg a, ereg b, emit_mode m, int size_offs ) {
	einstr e;
	e.header = op;
	e.mode = m;
	e.a = a;
	e.b = b;
	e.size_offs = size_offs;
	regs_write_instr(ctx, &e, out);
}

static void regs_emit_mov( regs_ctx *ctx, ereg to, ereg from, emit_mode m ) {
	if( to == from ) return;
	regs_emit(ctx,to,MOV,from,UNUSED,m,0);
}

#define regs_todo()	regs_emit_todo_impl(ctx,__LINE__)
static void regs_emit_todo_impl( regs_ctx *ctx, int line ) {
	einstr e;
	e.header = PUSH_CONST;
	e.mode = M_PTR;
	e.size_offs = line;
	regs_write_instr(ctx, &e, UNUSED);
	einstr e2;
	e2.header = DEBUG_BREAK;
	e2.size_offs = 0;
	e2.a = UNUSED;
	e2.b = UNUSED;
	regs_write_instr(ctx, &e2, UNUSED);
}

static int regs_alloc_stack( regs_ctx *ctx, int size ) {
	ctx->stack_size += size;
	ctx->stack_size += jit_pad_size(ctx->stack_size,size);
	return -ctx->stack_size;
}

#define value_str(v)	value_to_str(ctx,v)

static const char *value_to_str( regs_ctx *ctx, value_info *v ) {
	static char out[20];
	int idx = (int)(v - ctx->values);
	ereg e = (ereg)(idx >= ctx->jit->value_count ? -(idx-ctx->jit->value_count) - 1 : idx);
	sprintf(out,"%s:%s", val_str(e,v->mode), val_str(v->reg,v->mode));
	return out;
}

static void spill( regs_ctx *ctx, value_info *v ) {
	if( v->reg == UNUSED ) jit_assert();
	if( v->stack_pos == INVALID ) v->stack_pos = regs_alloc_stack(ctx, hl_emit_mode_sizes[v->mode]);
	v->reg = MK_STACK_REG(v->stack_pos);
	values_remove(&ctx->scratch,v);
	regs_debug("REG SPILL %s @%X\n",value_str(v),ctx->cur_op);
	if( v->stack_pos < 0 ) v->reg = UNUSED;
}

static bool regs_alloc_reg( regs_ctx *ctx, value_info *v ) {
	// lookup available reg
	int mode = REG_MODE(v->mode);
	reg_config *cfg = REG_CFG(mode);
	if( !IS_NULL(v->pref_reg) ) {
		bool free = true;
		for_iter(values,v2,ctx->scratch) {
			if( v2->reg == v->pref_reg ) {
				free = false;
				break;
			}
		}
		if( free ) {
			v->reg = v->pref_reg;
			return true;
		}
	}
	for(int i=0;i<cfg->nscratchs;i++) {
		ereg r = cfg->scratch[i];
		for_iter(values,v2,ctx->scratch) {
			if( v2->reg == r ) {
				r = UNUSED;
				break;
			}
		}
		if( !IS_NULL(r) ) {
			v->reg = r;
			return true;
		}
	}
	if( ctx->persists_uses[mode] < cfg->npersists ) {
		v->reg = cfg->persist[ctx->persists_uses[mode]++];
		return false;
	}
	// free the oldest scratch reg
	value_info *v2 = values_first(ctx->scratch);
	if( !v2 ) jit_assert();
	v->reg = v2->reg;
	spill(ctx, v2);
	return true;
}

static void regs_assign( regs_ctx *ctx, value_info *v ) {
	if( v->reg != UNUSED ) jit_assert();
	if( regs_alloc_reg(ctx, v) )
		values_add(ctx->scratch, v);
	regs_debug("REG ASSIGN %s @%X\n",value_str(v),ctx->cur_op);
}

static void regs_write_live( regs_ctx *ctx, ereg *r ) {
	if( IS_NULL(*r) ) jit_assert();
	if( IS_NATREG(*r) ) return;
	value_info *v = VAL_REG(*r);
	v->last_read = ctx->cur_op;
}

static value_info *regs_current( regs_ctx *ctx, ereg r ) {
	for_iter(values,v,ctx->scratch) {
		if( v->reg == r )
			return v;
	}
	return NULL;
}

static eblock *resolve_block_value( regs_ctx *ctx, ereg v ) {
	if( v < 0 ) {
		for(int i=0;i<ctx->jit->block_count;i++) {
			eblock *b = ctx->jit->blocks + i;
			for(int p=0;p<b->phi_count;p++) {
				if( b->phis[p].value == v )
					return b;
			}
		}
	} else {
		if( IS_NATREG(v) ) jit_assert();
		int idx = ctx->jit->values_writes[v];
		int min = 0;
		int max = ctx->jit->block_count;
		while( min < max ) {
			int med = (min + max) >> 1;
			eblock *b = ctx->jit->blocks + med;
			if( idx < b->start_pos ) {
				max = med;
			} else if( idx >= b->end_pos ) {
				min = med;
			} else
				return b;
		}
	}
	jit_assert();
}

static void regs_compute_liveness( regs_ctx *ctx ) {
	int write_index = 1;
	jit_ctx *jit = ctx->jit;
	hl_type *tret = ctx->jit->fun->type->fun->ret;
	emit_mode mret = tret->kind == HF32 || tret->kind == HF64 ? M_F64 : M_PTR;
	ereg ret = REG_CFG(REG_MODE(mret))->ret;
	for(int cur_op=0;cur_op<jit->instr_count;cur_op++) {
		einstr *e = jit->instrs + cur_op;
		value_info *write = NULL;

		if( write_index < jit->value_count && jit->values_writes[write_index] == cur_op )
			write = VAL(write_index++);

		ctx->cur_op = cur_op;
		hl_emit_reg_iter(jit,e,ctx,regs_write_live);
		if( IS_CALL(e->op) ) {
			// anticipate register usage in call so we can previlege this assign
			ereg *r = hl_emit_get_args(jit->emit, e);
			call_regs regs = {0};
			bool needs_push = false;
			for(int k=0;k<e->nargs;k++) {
				value_info *v = VAL_REG(r[k]);
				if( !IS_NULL(v->pref_reg) ) {
					needs_push = true;
					continue;
				}
				v->pref_reg = get_call_reg(ctx,regs,v->mode);
			}
			if( !needs_push && e->mode != M_NORET ) ctx->has_direct_call = true;
			if( write && IS_NULL(write->pref_reg) )
				write->pref_reg = REG_CFG(REG_MODE(e->mode))->ret;
		} else switch( e->op ) {
		case RET:
			if( e->a ) {
				value_info *v = VAL_REG(e->a);
				if( v->pref_reg == UNUSED ) v->pref_reg = ret;
			}
			break;
		case BINOP:
			switch( e->size_offs ) {
			case OSShr:
			case OUShr:
			case OShl:
				if( jit->cfg.req_bit_shifts ) VAL_REG(e->b)->pref_reg = jit->cfg.req_bit_shifts;
				break;
			case OSDiv:
			case OUDiv:
			case OSMod:
			case OUMod:
				if( jit->cfg.req_div_a ) VAL_REG(e->a)->pref_reg = jit->cfg.req_div_a;
				if( jit->cfg.req_div_b ) VAL_REG(e->b)->pref_reg = jit->cfg.req_div_b;
				break;
			}
			break;
		default:
			break;
		}
	}
	// compute reverse phis
	for(int b=0;b<jit->block_count;b++) {
		eblock *bl = jit->blocks + b;
		for(int p=0;p<bl->phi_count;p++) {
			ephi *ph = bl->phis + p;
			VAL_REG(ph->value)->mode = ph->mode;
			for(int k=0;k<ph->nvalues;k++) {
				ereg v = ph->values[k];
				eblock *b2 = resolve_block_value(ctx, v);
				int_arr *arr = &ctx->blocks_phis[b2 - jit->blocks];
				regs_debug("ADD PHI %s:=%s to #%d\n",val_str(ph->value,ph->mode),val_str(v,ph->mode),(int)(b2 - jit->blocks));
				int_arr_add(*arr,v);
				int_arr_add(*arr,ph->value);
				value_info *val = VAL_REG(v);
				if( val->last_read < b2->end_pos ) val->last_read = b2->end_pos;
			}
		}
	}
}

static void regs_assign_regs( regs_ctx *ctx ) {
	jit_ctx *jit = ctx->jit;
	// assign args
	call_regs regs = {0};
	int args_size = 0;
	for(int i=1;i<=ctx->jit->fun->type->fun->nargs;i++) {
		value_info *v = VAL(i);
		einstr *e = ctx->jit->instrs + ctx->jit->values_writes[i];
		int size = hl_emit_mode_sizes[e->mode];
		if( size <= 0 ) hl_jit_assert();
		ereg r = get_call_reg(ctx,regs,e->mode);
		if( !IS_NULL(r) ) {
			v->reg = r;
			values_add(ctx->scratch,v);
		}
		if( IS_NULL(r) || IS_WINCALL64 ) {
			// use existing stack storage
			v->stack_pos = args_size + HL_WSIZE*2;
			args_size += size < 4 ? 4 : size;
			if( IS_NULL(r) ) v->reg = MK_STACK_REG(-v->stack_pos);
		}
	}
	// assign registers
	int write_index = 1;
	for(int cur_op=0;cur_op<jit->instr_count;cur_op++) {
		einstr e = jit->instrs[cur_op];
		ctx->cur_op = cur_op;

		for_iter_back(values,v,ctx->scratch) {
			if( v->last_read <= cur_op )
				values_remove(&ctx->scratch,v);
		}

		if( IS_CALL(e.op) ) {
			ereg *args = hl_emit_get_args(ctx->jit->emit,&e);
			call_regs regs = {0};
			bool will_scratch = e.mode != M_NORET;
			if( will_scratch ) {
				for_iter_back(values,v2,ctx->scratch) {
					if( v2->last_read > cur_op )
						spill(ctx,v2);
				}
			}
			for(int k=0;k<e.nargs;k++) {
				value_info *v = VAL_REG(args[k]);
				ereg r = get_call_reg(ctx,regs,v->mode);
				if( !IS_NULL(r) ) {
					value_info *cur = regs_current(ctx,r);
					if( cur && cur != v )
						spill(ctx,cur);
				}
			}
			if( will_scratch ) values_reset(&ctx->scratch);
		}
		if( e.op == BLOCK ) {
			for_iter_back(values,v,ctx->scratch) {
				if( v->last_read == cur_op )
					values_remove(&ctx->scratch,v);
			}
			eblock *bl = jit->blocks + e.size_offs;
			for(int k=0;k<bl->phi_count;k++) {
				ephi *p = bl->phis + k;
				value_info *v = VAL_REG(p->value);
				for(int n=0;n<p->nvalues;n++) {
					value_info *vn = VAL_REG(p->values[n]);
					// ignore previously set pref_reg (minimize moves)
					if( IS_PURE(vn->reg) && !regs_current(ctx,vn->reg) ) {
						v->pref_reg = vn->reg;
						break;
					}
				}
				regs_assign(ctx, v);
			}
		}
		if( write_index < jit->value_count && jit->values_writes[write_index] == cur_op ) {
			value_info *w = VAL(write_index++);
			switch( e.op ) {
			case ALLOC_STACK:
				w->reg = MK_STACK_OFFS(regs_alloc_stack(ctx, e.size_offs));
				break;
			case LOAD_ARG:
				if( w->reg == UNUSED )
					regs_assign(ctx, w); // assign for stack reg
				break;
			case UNOP:
			case BINOP:
				if( w->pref_reg == UNUSED ) {
					value_info *v = VAL_REG(e.a);
					if( IS_PURE(v->reg) ) {
						values_remove(&ctx->scratch,v);
						w->pref_reg = v->reg;
					}
				}
				// fallback
			default:
				regs_assign(ctx, w);
				break;
			}
		}
	}
	// assign stack regs
	int nvalues = jit->value_count + jit->phi_count;
	ctx->stack_offset = (ctx->persists_uses[0] + ctx->persists_uses[1]) * 8;
	for(int i=0;i<nvalues;i++) {
		value_info *v = ctx->values + i;
		if( v->reg == UNUSED ) v->reg = MK_STACK_REG(v->stack_pos);// + ctx->stack_offset);
	}
}

static void flush_movs( regs_ctx *ctx ) {
	int_arr movs = ctx->pack_movs;
	while( true ) {
		int size = int_arr_count(movs);
		if( !size ) break;
		bool cycle = true;
		for(int k=0;k<size;k+=3) {
			ereg to = int_arr_get(movs,k);
			bool read = false;
			for(int k2=1;k2<size;k2+=3) {
				ereg from = int_arr_get(movs,k2);
				if( from == to ) {
					read = true;
					break;
				}
			}
			if( !read ) {
				ereg from = int_arr_get(movs,k+1);
				int mode = int_arr_get(movs,k+2);
				regs_emit_mov(ctx,to,from,mode);
				int_arr_remove_range(&movs,k,3);
				cycle = false;
				break;
			}
		}
		if( cycle ) {
			ereg to = int_arr_get(movs,0);
			ereg from = int_arr_get(movs,1);
			int mode = int_arr_get(movs,2);
			regs_emit(ctx,UNUSED,XCHG,to,from,mode,0);
			int_arr_remove_range(&movs,0,3);
			size -= 3;
			for(int k=0;k<size;k+=3) {
				if( int_arr_get(movs,k) == to )
					movs.values[k] = from;
				else if( int_arr_get(movs,k) == from )
					movs.values[k] = to;
			}
		}
	}
	ctx->pack_movs = movs;
	int_arr_reset(&ctx->pack_movs);
}

static void flush_phis( regs_ctx *ctx, eblock *b ) {
	if( !b ) return;
	jit_ctx *jit = ctx->jit;
	int bid = (int)(b - jit->blocks);
	int_arr arr = ctx->blocks_phis[bid];
	int idx = 0;
	int_arr movs = ctx->pack_movs;
	while( idx < int_arr_count(arr) ) {
		ereg a = int_arr_get(arr,idx++);
		ereg b = int_arr_get(arr,idx++);
		value_info *from = VAL_REG(a);
		value_info *to = VAL_REG(b);
		if( from->reg == to->reg ) continue;
		int size = int_arr_count(movs);
		bool dup = false;
		for(int k=0;k<size;k+=3) {
			if( int_arr_get(movs,k) == to->reg && int_arr_get(movs,k+1) == from->reg ) {
				dup = true;
				break;
			}
		}
		if( !dup ) {
			int_arr_add(movs, to->reg);
			int_arr_add(movs, from->reg);
			int_arr_add(movs, from->mode);
		}
	}
	ctx->pack_movs = movs;
	int_arr_free(&ctx->blocks_phis[bid]);
	flush_movs(ctx);
}

static void regs_emit_instrs( regs_ctx *ctx ) {
	jit_ctx *jit = ctx->jit;
	eblock *cur_block = NULL;
	call_regs regs = {0};
	int write_index = 1;
	ctx->pos_map[0] = 0;

	int stack_offset = ctx->stack_size;
	int push_size = HL_WSIZE * 2 + ctx->stack_offset; // RIP + RBP save
	if( IS_WINCALL64 && ctx->has_direct_call ) stack_offset += 0x20; // reserve
	if( jit->cfg.stack_align ) {
		int align = (stack_offset + push_size) % jit->cfg.stack_align;
		if( align ) stack_offset += jit->cfg.stack_align - align;
	}

	for(int cur_op=0;cur_op<jit->instr_count;cur_op++) {
		einstr e = jit->instrs[cur_op];
		ereg *ret_val = NULL;
		int nread;
		ctx->cur_op = cur_op;
		if( IS_CALL(e.op) ) {
			ereg *args = hl_emit_get_args(ctx->jit->emit,&e);
			call_regs regs = {0};
			for(int k=0;k<e.nargs;k++) {
				value_info *v = VAL_REG(args[k]);
				ereg r = get_call_reg(ctx,regs,v->mode);
				if( IS_NULL(r) ) {
					regs_todo();
				} else if( r != v->reg ) {
					int_arr_add(ctx->pack_movs,r);
					int_arr_add(ctx->pack_movs,v->reg);
					int_arr_add(ctx->pack_movs,v->mode);
				}
			}
			flush_movs(ctx);
			e.nargs = 0xFF;
			ret_val = &REG_CFG(REG_MODE(e.mode))->ret;
			if( e.op == CALL_REG )
				e.a = VAL(VIDX(e.a))->reg;
		} else {
			ereg **regs = hl_emit_get_regs(&e,&nread);
			for(int k=0;k<nread;k++) {
				ereg *r = regs[k];
				if( IS_NATREG(*r) ) continue;
				value_info *v = VAL(VIDX(*r));
				*r = v->reg;
			}
		}
		ereg out = UNUSED;
		if( write_index < jit->value_count && jit->values_writes[write_index] == cur_op )
			out = VAL(write_index++)->reg;
		switch( e.op ) {
		case ALLOC_STACK:
			break;
		case BLOCK:
			flush_phis(ctx,cur_block);
			cur_block = jit->blocks + e.size_offs;
			break;
		case LOAD_ARG:
			{
				ereg def = get_call_reg(ctx,regs,e.mode);
				if( def && out != def )
					regs_emit_mov(ctx,out,def,e.mode);
				else
					regs_write_instr(ctx, &e, out);
			}
			break;
		case ENTER:
			{
				regs_emit(ctx,UNUSED,PUSH,jit->cfg.stack_pos,UNUSED,M_PTR,0);
				regs_emit_mov(ctx,jit->cfg.stack_pos,jit->cfg.stack_reg,M_PTR);
				for(int i=0;i<ctx->persists_uses[0];i++)
					regs_emit(ctx,UNUSED,PUSH,ctx->jit->cfg.regs.persist[i],UNUSED,M_PTR,0);
				for(int i=0;i<ctx->persists_uses[1];i++)
					regs_emit(ctx,UNUSED,PUSH,ctx->jit->cfg.floats.persist[i],UNUSED,M_F64,0);
				regs_emit(ctx,UNUSED,STACK_OFFS,UNUSED,UNUSED,M_PTR,-stack_offset);
			}
			break;
		case JUMP:
		case JCOND:
		case JUMP_TABLE:
			flush_phis(ctx,cur_block);
			if( e.op == JUMP_TABLE ) {
				// copy args (remap later)
				hl_emit_store_args(jit->emit,&e,hl_emit_get_args(jit->emit,&e),e.nargs);
			}
			regs_write_instr(ctx, &e, out);
			int_arr_add(ctx->jump_regs, ctx->emit_pos - 1);
			int_arr_add(ctx->jump_regs, cur_op + 1 + (e.op == JUMP_TABLE ? 0 : e.size_offs));
			break;
		case RET:
			if( e.a ) {
				ereg ret = REG_CFG(REG_MODE(e.mode))->ret;
				if( e.a != ret )
					regs_emit_mov(ctx, ret, e.a, e.mode);
			}
			regs_emit(ctx,UNUSED,STACK_OFFS,UNUSED,UNUSED,M_PTR,stack_offset);
			for(int i=ctx->persists_uses[1]-1;i>=0;i--)
				regs_emit(ctx,UNUSED,POP,ctx->jit->cfg.floats.persist[i],UNUSED,M_F64,0);
			for(int i=ctx->persists_uses[0]-1;i>=0;i--)
				regs_emit(ctx,UNUSED,POP,ctx->jit->cfg.regs.persist[i],UNUSED,M_PTR,0);
			regs_emit(ctx,UNUSED,POP,jit->cfg.stack_pos,UNUSED,M_PTR,0);
			regs_emit(ctx,UNUSED,RET,UNUSED,UNUSED,M_NONE,0);
			break;
		default:
			if( ret_val && out ) {
				regs_write_instr(ctx, &e, *ret_val);
				regs_emit_mov(ctx, out, *ret_val, e.mode);
			} else
				regs_write_instr(ctx, &e, out);
			break;
		}
		ctx->pos_map[cur_op+1] = ctx->emit_pos;
	}
}

void hl_regs_flush( jit_ctx *jit ) {
	regs_ctx *ctx = jit->regs;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	jit->reg_instr_count = ctx->emit_pos;
	jit->reg_instrs = ctx->instrs;
	jit->reg_writes = ctx->out_write;
	jit->reg_pos_map = ctx->pos_map;
	if( ctx->pos_map ) ctx->pos_map[ctx->cur_op+1] = ctx->emit_pos;
	hl_emit_remap_jumps(jit->emit, &ctx->jump_regs, ctx->instrs, ctx->pos_map);
}

void hl_regs_function( jit_ctx *jit ) {
	regs_ctx *ctx = jit->regs;
	int nvalues = jit->value_count + jit->phi_count;
	memset(ctx->persists_uses,0,sizeof(ctx->persists_uses));
	free(ctx->pos_map);
	ctx->flushed = false;
	ctx->has_direct_call = false;
	ctx->pos_map = (int*)malloc((jit->instr_count + 1) * sizeof(int));
	ctx->emit_pos = 0;
	ctx->cur_op = 0;
	ctx->stack_size = 0;
	jit->reg_instrs = NULL;
	values_free(&ctx->scratch);
	int_arr_free(&ctx->jump_regs);
	int_arr_free(&ctx->pack_movs);
	ctx->blocks_phis = (int_arr*)hl_zalloc(&jit->falloc,sizeof(int_arr) * jit->block_count);
	ctx->values = (value_info*)hl_zalloc(&jit->falloc,sizeof(value_info) * nvalues);
	for(int i=1;i<nvalues;i++) {
		value_info *v = VAL(i);
		v->reg = UNUSED;
		v->pref_reg = UNUSED;
		v->stack_pos = INVALID;
		v->last_read = -1;
		if( i < jit->value_count )
			v->mode = jit->instrs[jit->values_writes[i]].mode;
		else
			v->mode = M_NONE; // TODO : phi mode
	}
	regs_compute_liveness(ctx);
	regs_assign_regs(ctx);
	regs_emit_instrs(ctx);
	hl_regs_flush(ctx->jit);
}


void hl_regs_alloc( jit_ctx *jit ) {
	regs_ctx *ctx = malloc(sizeof(regs_ctx));
	memset(ctx,0,sizeof(regs_ctx));
	ctx->jit = jit;
	jit->regs = ctx;
}

void hl_regs_free( jit_ctx *jit ) {
	regs_ctx *ctx = jit->regs;
	free(ctx->pos_map);
	free(ctx->instrs);
	free(ctx->out_write);
	free(ctx);
}

