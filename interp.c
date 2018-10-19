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
#include <hl.h>
#include <hlmodule.h>

#ifdef HL_INTERP_ENABLE

#define HL_INTERP_DEBUG

#define OP_BEGIN
#define OP(name,_)

struct interp_ctx {
	hl_alloc alloc;
	hl_module *m;
	int **fregs;
};

static interp_ctx *interp = NULL;

#define REG_KIND(rid)	f->regs[rid]->kind
#define REG(rid,T)		*((T*)(regs + regsPos[rid]))
#define interp_error(err)	{ hl_debug_break(); hl_fatal(err); }

void *hl_interp_run( interp_ctx *ctx, hl_function *f, vdynamic *ret ) {
	hl_opcode *o = f->ops;
	hl_module *m = ctx->m;
	int *regsPos = ctx->fregs[f->findex];
	char *regs = (char*)malloc(regsPos[f->nregs]);
	void *pret = NULL;
#	ifdef HL_INTERP_DEBUG
	memset(regs,0xCD,regsPos[f->nregs]);
#	endif
	while( true ) {
		switch( o->op ) {
		case OCall0:
			{
				int fidx = m->functions_indexes[o->p2];
				if( fidx < m->code->nfunctions ) {
					pret = hl_interp_run(ctx, m->code->functions + fidx, ret);
					if( hl_is_ptr(f->regs[o->p1]) )
						REG(o->p1,void*) = pret;
					else
						memcpy(&REG(o->p1,void*),&ret->v,hl_type_size(f->regs[o->p1]));
				} else {
					switch( REG_KIND(o->p1) ) {
					case HABSTRACT:
						REG(o->p1,void*) = ((void*(*)())m->functions_ptrs[o->p2])();
						break;
					default:
						interp_error("TODO");
						break;
					}
				}
			}
			break;
		case OCall3:
			{
				interp_error("TODO");
			}
			break;
		case OInt:
			REG(o->p1,int) = o->p2; 
			break;
		case OSetGlobal:
			{
				void *addr = m->globals_data + m->globals_indexes[o->p1];
				memcpy(addr,&REG(o->p2,void*),hl_type_size(f->regs[o->p2]));
			}
			break;
		case ORet:
			{
				switch( REG_KIND(o->p1) ) {
				case HVOID:
					return NULL;
				default:
					interp_error("TODO");
					break;
				}
			}
			break;
		case OType:
			REG(o->p1,hl_type*) = m->code->types + o->p2;
			break;
		case OString:
			REG(o->p1,const uchar*) = hl_get_ustring(m->code,o->p2);
			break;
		default:
			interp_error(hl_op_name(o->op));
			break;
		}
		o++;
	}
	free(regs);
	return pret;
}

static void *callback_c2hl( void *fptr, hl_type *t, void **args, vdynamic *ret ) {
	hl_code *code = interp->m->code;
	hl_function *f = (hl_function*)fptr;
	if( f >= code->functions && f < code->functions + code->nfunctions )
		return hl_interp_run(interp, f, ret); 
	interp_error("assert");
	return NULL;
}

interp_ctx *hl_interp_alloc() {
	interp_ctx *i = (interp_ctx*)malloc(sizeof(interp_ctx));
	memset(i,0,sizeof(interp_ctx));
	hl_alloc_init(&i->alloc);
	return i;
}

void hl_interp_init( interp_ctx *ctx, hl_module *m ) {
	int i;
	m->codeptr = ctx;
	m->isinterp = true;
	ctx->m = m;
	interp = ctx;
	ctx->fregs = (int**)hl_zalloc(&ctx->alloc, sizeof(int*)*(m->code->nfunctions + m->code->nnatives) );
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		int *regs = hl_malloc(&ctx->alloc,(f->nregs+1)*sizeof(int));
		int regPos = 0;
		int j;
		for(j=0;j<f->nregs;j++) {
			regs[j] = regPos;
			regPos += hl_type_size(f->regs[j]);
		}
		regs[j] = regPos;
		ctx->fregs[f->findex] = regs;
	}
	hl_setup_callbacks(callback_c2hl, NULL);
}

void *hl_interp_function( interp_ctx *i, hl_module *m, hl_function *f ) {
	return f;
}

#endif

