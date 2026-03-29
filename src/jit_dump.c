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
	"mov",
	"alloc-stack",
	"free-stack",
	"native-reg",
	"prefetch",
	"debug-break",
};

const char *hl_emit_regstr( ereg v ) {
	static char fmts[4][10];
	static int flip = 0;
	// allow up to four concurrent val_str
	char *fmt = fmts[flip++&3];
	if( IS_NULL(v) )
		sprintf(fmt,"NULL???");
	else if( v.index < 0 )
		sprintf(fmt,"P%d",-v.index);
	else
		sprintf(fmt,"V%d",v.index);
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

static void hl_dump_args( jit_ctx *ctx, einstr *e ) {
	ereg *v = hl_emit_get_args(ctx->emit, e);
	printf("(");
	for(int i=0;i<e->nargs;i++) {
		if( i != 0 ) printf(",");
		printf("%s", val_str(v[i]));
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
		DYN(f);
		DYN(d);
		DYN(i64);
		DYN(i);
		DYN(p);
		N2("null_field",hl_jit_null_field_access);
		N2("null_access",hl_jit_null_access);
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

void hl_emit_dump( jit_ctx *ctx ) {
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
	// check blocks intervals
	int cur = 0;
	for(i=0;i<ctx->block_count;i++) {
		eblock *b = ctx->blocks + i;
		if( b->id != i ) printf("  ??? BLOCK @%d ID is %d\n",i,b->id);
		if( b->start_pos != cur ) printf("  ??? BLOCK %d START AT %X != %X\n", i, b->start_pos, cur);
		if( b->end_pos < b->start_pos ) printf("  ??? BLOCK %d RANGE [%X,%X]\n", i, b->start_pos, b->end_pos);
		cur = b->end_pos + 1;
	}
	if( cur != ctx->instr_count )
		printf("  ??? MISSING BLOCK FOR RANGE %X-%X\n", cur, ctx->instr_count);
	// print instrs
	int vpos = 0;
	cur = 0;
	for(i=0;i<ctx->instr_count;i++) {
		while( ctx->blocks[cur].start_pos == i ) {
			eblock *b = &ctx->blocks[cur];
			printf("--- BLOCK #%d ---\n", cur);
			for(int k=0;k<b->phi_count;k++) {
				ephi *p = b->phis + k;
				printf("\t\t@%X %s = phi(",i,val_str(p->value));
				for(int n=0;n<p->nvalues;n++) {
					if( n > 0 ) printf(",");
					printf("%s",val_str(p->values[n]));
				}
				printf(")");
				if( p->nvalues <= 1 )
					printf(" ???");
				printf("\n");
			}
			cur++;
		}
		while( ctx->emit_pos_map[cur_op] == i ) {
			printf("@%X ", cur_op);
			hl_dump_op(ctx->fun, f->ops + cur_op);
			printf("\n");
			cur_op++;
		}
		einstr *e = ctx->instrs + i;
		printf("\t\t@%X ", i);
		if( vpos < ctx->value_count && ctx->values_writes[vpos] == i )
			printf("V%d = ", vpos++);
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
			printf(" %s", val_str(e->a));
			hl_dump_args(ctx,e);
			break;
		case CALL_PTR:
			printf(" ");
			hl_dump_ptr_name(ctx, (void*)e->value);
			hl_dump_args(ctx,e);
			break;
		case JUMP:
		case JCOND:
			printf(" @%X", i + 1 + e->size_offs);
			break;
		case LOAD_IMM:
			printf(" ");
			dump_value(ctx, e->value, e->mode);
			break;
		case LOAD_ADDR:
			if( (e->b.index>>8) )
				printf(" %s[%Xh]", val_str(e->a), e->b.index);
			else
				printf(" %s[%d]", val_str(e->a), e->b.index);
			break;
		case STORE:
			{
				int offs = e->size_offs;
				if( offs == 0 )
					printf(" [%s]", val_str(e->a));
				else
					printf(" %s[%d]", val_str(e->a), offs);
				printf(" = %s", val_str(e->b));
				//if( e->mode == 0 || e->mode != ctx->instrs[ctx->values_writes[e->b.index]].mode )
				//	printf(" ???");
			}
			break;
		default:
			if( !IS_NULL(e->a) ) {
				printf(" %s", val_str(e->a));
				if( !IS_NULL(e->b) ) printf(", %s", val_str(e->b));
				if( e->a.index >= vpos || e->b.index >= vpos ) printf(" ???");
			}
			if( show_size && e->size_offs != 0 )
				printf(" %d", e->size_offs);
			break;
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
	printf("\n\n");
	fflush(stdout);
}
