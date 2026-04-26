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
#include <jit.h>

static const char *op_names[] = {
	"load-addr",
	"load-const",
	"load-arg",
	"load-fun",
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
	"mov",
	"cmov",
	"xchg",
	"cxhg",
	"push-const",
	"push",
	"pop",
	"alloc-stack",
	"prefetch",
	"debug-break",
	"block",
	"enter",
	"stack",
	"catch",
	"address",
};

bool hl_jit_dump_bin = false;

const char *hl_natreg_str( int reg, emit_mode m );

const char *hl_emit_regstr( ereg v, emit_mode m ) {
	static char fmts[4][10];
	static int flip = 0;
	// allow up to four concurrent val_str
	char *fmt = fmts[flip++&3];
	if( IS_NULL(v) )
		sprintf(fmt,"NULL???");
	else if( v < 0 )
		sprintf(fmt,"P%d",-v);
	else if( (v&FL_STACKREG) == FL_STACKREG ) {
		int index = GET_STACK_OFFS(v);
		if( index < 0 )
			sprintf(fmt,"[ST-%Xh]", -index);
		else
			sprintf(fmt,"[ST+%Xh]", index);
	} else if( (v&FL_STACKOFFS) == FL_STACKOFFS ) {
		int index = GET_STACK_OFFS(v);
		if( index < 0 )
			sprintf(fmt,"ST-%Xh", -index);
		else
			sprintf(fmt,"ST+%Xh", index);
	} else if( IS_NATREG(v) )
		sprintf(fmt,"%s", hl_natreg_str(v,m));
	else
		sprintf(fmt,"V%d",v);
	return fmt;
}

static void hl_dump_arg( hl_function *fun, int fmt, int val, char sep, int pos ) {
	if( fmt == 0 ) return;
	printf("%c", sep);
	switch( fmt ) {
	case 1:
	case 2:
		printf("R%d", val);
		if( val < 0 || val >= fun->nregs ) printf("?");
		break;
	case 3:
		printf("%d", val);
		break;
	case 4:
		printf("[%d]", val);
		break;
	case 5:
	case 6:
		printf("@%X", val + pos + 1);
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

static void hl_dump_op( hl_function *fun, hl_opcode *op ) {
	printf("%s", hl_op_name(op->op) + 1);
	int fmt = hl_op_fmt[op->op];
	int pos = (int)(op - fun->ops);
	hl_dump_arg(fun, fmt & 0xFF, op->p1, ' ', pos);
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
				printf("@%X", (op->extra[i] + pos + 1));
			}
			printf(",def=@%X", op->p3 + pos + 1);
		} else {
			if( count == 0xFF )
				count = op->p3;
			else {
				printf("%d,%d,",op->p2,op->p3);
				count -= 3;
			}
			for(int i=0;i<count;i++) {
				if( i != 0 ) printf(",");
				printf("%d", op->extra[i]);
			}
		}
		printf("]");
	} else {
		hl_dump_arg(fun, (fmt >> 8) & 0xFF, op->p2,',', pos);
		hl_dump_arg(fun, fmt >> 16, op->p3,',', pos);
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

static void dump_value( jit_ctx *ctx, uint64 value, emit_mode mode ) {
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
		else if( mode == M_PTR && value == (uint64)mod->globals_data )
			printf("<globals>");
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

static void hl_dump_args( jit_ctx *ctx, einstr *e ) {
	if( e->nargs == 0xFF )
		return;
	ereg *v = hl_emit_get_args(ctx->emit, e);
	printf("(");
	for(int i=0;i<e->nargs;i++) {
		if( i != 0 ) printf(",");
		printf("%s", val_str(v[i],M_NONE));
	}
	printf(")");
}

typedef struct { const char *name; void *ptr; } named_ptr;
static void hl_dump_ptr_name( jit_ctx *ctx, void *ptr ) {
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
		N(hl_dyn_compare);
		N(hl_same_type);
		DYN(f);
		DYN(d);
		DYN(i64);
		DYN(i);
		DYN(p);
		N2("null_field",hl_jit_null_field_access);
		N2("null_access",hl_null_access);
		N(hl_get_thread);
		N(setjmp);
		N(_setjmp);
		N2("assert",hl_jit_assert);
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

void hl_emit_flush( jit_ctx *ctx );
void hl_regs_flush( jit_ctx *ctx );
void hl_codegen_flush( jit_ctx *ctx );

#define reg_str(r) val_str(r,e->mode)

static void dump_instr( jit_ctx *ctx, einstr *e, int cur_pos ) {
	printf("%s", op_names[e->op]);
	bool show_size = true;
	switch( e->op ) {
	case TEST:
	case CMP:
		printf("-%s", hl_op_name(e->size_offs)+2);
		show_size = false;
		break;
	case BINOP:
	case UNOP:
		printf("-%s", hl_op_name(e->size_offs)+1);
		show_size = false;
		break;
	default:
		break;
	}
	if( e->mode )
		printf("%s", emit_mode_str(e->mode));
	switch( e->op ) {
	case CALL_FUN:
		printf(" ");
		{
			int fid = ctx->mod->functions_indexes[e->a];
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
		printf(" %s", val_str(e->a,M_PTR));
		hl_dump_args(ctx,e);
		break;
	case CALL_PTR:
		printf(" ");
		hl_dump_ptr_name(ctx, (void*)e->value);
		hl_dump_args(ctx,e);
		break;
	case JUMP:
	case JCOND:
		printf(" @%X", cur_pos + 1 + e->size_offs);
		break;
	case JUMP_TABLE:
		{
			int *offsets = hl_emit_get_args(ctx->emit, e);
			printf(" %s (", reg_str(e->a));
			for(int k=0;k<e->nargs;k++) {
				if( k > 0 ) printf(",");
				printf("@%X", cur_pos + 1 + offsets[k]);
			}
			printf(")");
		}	
		break;
	case BLOCK:
		printf(" #%d", e->size_offs);
		if( e->size_offs && ctx->blocks[e->size_offs].pred_count == 0 )
			printf(" ???DEAD");
		break;
	case STACK_OFFS:
		if( e->size_offs >= 0 )
			printf(" +%Xh", e->size_offs);
		else
			printf(" -%Xh", -e->size_offs);
		break;
	case LOAD_CONST:
	case PUSH_CONST:
		printf(" ");
		dump_value(ctx, e->value, e->mode);
		break;
	case LOAD_ADDR:
		printf(" %s[%Xh]", val_str(e->a,M_PTR), e->size_offs);
		break;
	case STORE:
		{
			int offs = e->size_offs;
			if( offs == 0 )
				printf(" [%s]", val_str(e->a,M_PTR));
			else
				printf(" %s[%Xh]", val_str(e->a,M_PTR), offs);
			printf(" = %s", reg_str(e->b));
		}
		break;
	case CONV:
	case CONV_UNSIGNED:
		printf("%s %s", emit_mode_str(e->size_offs), val_str(e->a,(emit_mode)e->size_offs));
		break;
	case LEA:
		printf(" [%s", reg_str(e->a));
		if( !IS_NULL(e->b) ) printf("+%s", reg_str(e->b));
		if( (e->size_offs&0xFF) > 1 ) printf("*%d",e->size_offs&0xFF);
		if( e->size_offs >> 8 ) printf("+%Xh", e->size_offs>>8);
		printf("]");
		break;	
	default:
		if( !IS_NULL(e->a) ) {
			printf(" %s", reg_str(e->a));
			if( !IS_NULL(e->b) ) printf(", %s", reg_str(e->b));
		}
		if( show_size && e->size_offs != 0 )
			printf(" %d", e->size_offs);
		break;
	}
}

void hl_emit_dump( jit_ctx *ctx ) {
	hl_function *f = ctx->fun;
	int nargs = f->type->fun->nargs;
	// if it not was not before (in case of dump during process)
	hl_emit_flush(ctx);
	hl_regs_flush(ctx);
	hl_codegen_flush(ctx);
	printf("function ");
	hl_dump_fun_name(f);
	printf("(");
	for(int i=0;i<nargs;i++) {
		if( i > 0 ) printf(",");
		uprintf(USTR("R%d"), i);
	}
	printf(")\n");
	for(int i=0;i<f->nregs;i++)
		uprintf(USTR("\tR%d : %s\n"),i, hl_type_str(f->regs[i]));
	// check blocks intervals
	int cur = 0;
	for(int i=0;i<ctx->block_count;i++) {
		eblock *b = ctx->blocks + i;
		if( b->start_pos != cur ) printf("  ??? BLOCK %d START AT %X != %X\n", i, b->start_pos, cur);
		if( b->end_pos < b->start_pos ) printf("  ??? BLOCK %d RANGE [%X,%X]\n", i, b->start_pos, b->end_pos);
		cur = b->end_pos;
	}
	if( cur != ctx->instr_count )
		printf("  ??? MISSING BLOCK FOR RANGE %X-%X\n", cur, ctx->instr_count);
	// print instrs
	int vpos = 1;
	int rpos = 0;
	int cpos = 0;
	int cur_op = 0;
	bool new_op = false;
	eblock *cur_block = NULL;
	for(int icount=0;icount<ctx->instr_count;icount++) {
		while( ctx->emit_pos_map[cur_op] == icount ) {
			printf("@%X ", cur_op);
			hl_dump_op(ctx->fun, f->ops + cur_op);
			printf("\n");
			new_op = true;
			cur_op++;
		}
		einstr *e = ctx->instrs + icount;
		printf("\t\t@%X ", icount);
		if( vpos < ctx->value_count && ctx->values_writes[vpos] == icount )
			printf("V%d = ", vpos++);
		dump_instr(ctx, e, icount);
		if( e->op == JCOND || e->op == JUMP ) {
			int target = icount + 1 + e->size_offs;
			bool bad = false;
			if( icount + 1 >= ctx->instr_count || target < 0 || target >= ctx->instr_count )
				bad = true;
			else if( ctx->instrs[target].op != BLOCK || (e->op == JCOND && ctx->instrs[icount+1].op != BLOCK) )
				bad = true;
			else {
				bool found = false;
				for(int k=0;k<cur_block->next_count;k++) {
					if( cur_block->nexts[k] == ctx->instrs[target].size_offs )
						found = true;
					if( (e->op == JUMP || e->op == JUMP_TABLE) && ctx->instrs[icount+1].op == BLOCK && ctx->instrs[icount+1].size_offs == cur_block->nexts[k] )
						printf(" ???LEAK");
				}
				if( !found ) printf(" ???NEXT");
			}
			if( bad )
				printf(" ???");
		}
		if( e->op == BLOCK ) {
			eblock *b = &ctx->blocks[e->size_offs];
			for(int k=0;k<b->pred_count;k++) {
				eblock *p = &ctx->blocks[b->preds[k]];
				einstr *pe = &ctx->instrs[p->end_pos-1];				
				if( p->end_pos == icount )
					continue;
				bool bad = false;
				if( (pe->op == JUMP || pe->op == JCOND) && pe->size_offs == icount - p->end_pos )
					bad = false;
				else if( pe->op != JUMP_TABLE )
					bad = true;
				if( bad )
					printf(" ???PREV#%d",b->preds[k]);
			}
			for(int k=0;k<b->phi_count;k++) {
				ephi *p = b->phis + k;
				printf("\n\t\t@%X %s = phi%s(",icount,val_str(p->value,p->mode),emit_mode_str(p->mode));
				for(int n=0;n<p->nvalues;n++) {
					if( n > 0 ) printf(",");
					printf("%s:%d",val_str(p->values[n],p->mode),p->blocks[n]);
				}
				printf(")");
				if( p->nvalues <= 1 )
					printf(" ???");
			}
			cur_block = b;
		}
		while( rpos < ctx->reg_instr_count && rpos < ctx->reg_pos_map[icount+1] ) {
			ereg out = ctx->reg_writes[rpos];
			e = ctx->reg_instrs + rpos;
			printf("\n\t\t\t\t@%X ",rpos);
			if( !IS_NULL(out) ) printf("%s = ",reg_str(out));
			dump_instr(ctx,e,rpos);
			bool first = true;
			while( cpos < ctx->code_size && cpos < ctx->code_pos_map[rpos+1] ) {
				if( first ) {
					if( hl_jit_dump_bin )
						printf("\t\t\t");
					else
						printf("\033[80G");
					first = false;
					if( new_op ) {
						new_op = false;
						cpos += ctx->cfg.debug_prefix_size;
					}
				}
				printf("%.2X",ctx->code_instrs[cpos++]);
			}
			rpos++;
		}
		printf("\n");
	}
	// invalid ?
	while( vpos < ctx->value_count )
		printf("  ??? UNWRITTEN VALUE V%d @%X\n", vpos, ctx->values_writes[vpos++]);
	// interrupted
	if( cur_op < f->nops ) {
		printf("@%X ", cur_op);
		hl_dump_op(ctx->fun, f->ops + cur_op);
		printf("\n\t\t...\n");
	}
	if( cpos == ctx->code_size && cpos > 0 ) {
		int n = 1;
		for(int i=0;i<cpos;i++) {
			while( ctx->code_pos_map[n] == i ) {
				if( (n & 15) == 0 ) printf("\n"); else printf(" ");
				n++;
			}
			printf("%.2X", ctx->code_instrs[i]);
		}
	}
	printf("\n\n");
	fflush(stdout);
}
