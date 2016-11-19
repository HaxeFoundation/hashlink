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

#ifdef HL_WIN
#	include <windows.h>
#	define dlopen(l,p)		(void*)( (l) ? LoadLibraryA(l) : GetModuleHandle(NULL))
#	define dlsym(h,n)		GetProcAddress((HANDLE)h,n)
#else
#	include <dlfcn.h>
#endif

extern void hl_callback_init( void *e );

static hl_module *cur_module;
static void *stack_top;

static uchar *module_resolve_symbol( void *addr, uchar *out, int *outSize ) {
	int code_pos = ((int)(int_val)((unsigned char*)addr - (unsigned char*)cur_module->jit_code)) >> JIT_CALL_PRECISION;
	int debug_pos = cur_module->jit_debug[code_pos];
	int *debug_addr;
	int file, line;
	int size = *outSize;
	int pos = 0;
	hl_function *fdebug;
	if( !debug_pos )
		return NULL;
	fdebug = cur_module->code->functions + cur_module->functions_indexes[((unsigned)debug_pos)>>16];
	debug_addr = fdebug->debug + ((debug_pos&0xFFFF) * 2);
	file = debug_addr[0];
	line = debug_addr[1];
	if( fdebug->obj )
		pos += usprintf(out,size - pos,USTR("%s.%s("),fdebug->obj->name,fdebug->field);
	else
		pos += usprintf(out,size - pos,USTR("fun$%d("),fdebug->findex);
	pos += hl_from_utf8(out + pos,size - pos,cur_module->code->debugfiles[file]);
	pos += usprintf(out + pos, size - pos, USTR(":%d)"), line);
	*outSize = pos;
	return out;
}

static int module_capture_stack( void **stack, int size ) {
	void **stack_ptr = (void**)&stack;
	void *stack_bottom = stack_ptr;
	int count = 0;
	unsigned char *code = cur_module->jit_code;
	int code_size = cur_module->codesize;
	while( stack_ptr < (void**)stack_top ) {
		void *stack_addr = *stack_ptr++; // EBP
		if( stack_addr > stack_bottom && stack_addr < stack_top ) {
			void *module_addr = *stack_ptr; // EIP
			if( module_addr >= (void*)code && module_addr < (void*)(code + code_size) ) {
				if( count == size ) break;
				stack[count++] = module_addr;
			}
		}
	}
	if( count ) count--;
	return count;
}

static void hl_init_enum( hl_type_enum *e ) {
	int i, j;
	for(i=0;i<e->nconstructs;i++) {
		hl_enum_construct *c = &e->constructs[i];
		c->hasptr = false;
		c->size = sizeof(int); // index
		for(j=0;j<c->nparams;j++) {
			hl_type *t = c->params[j];
			c->size += hl_pad_size(c->size,t);
			c->offsets[j] = c->size;
			if( hl_is_ptr(t) ) c->hasptr = true;
			c->size += hl_type_size(t);
		}
	}
}

hl_module *hl_module_alloc( hl_code *c ) {
	int i;
	int gsize = 0;
	hl_module *m = (hl_module*)malloc(sizeof(hl_module));
	if( m == NULL )
		return NULL;	
	memset(m,0,sizeof(hl_module));
	m->code = c;
	m->globals_indexes = (int*)malloc(sizeof(int)*c->nglobals);
	if( m == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	for(i=0;i<c->nglobals;i++) {
		gsize += hl_pad_size(gsize, c->globals[i]);
		m->globals_indexes[i] = gsize;
		gsize += hl_type_size(c->globals[i]);
	}
	m->globals_data = (unsigned char*)malloc(gsize);
	if( m->globals_data == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->globals_data,0,gsize);
	m->functions_ptrs = (void**)malloc(sizeof(void*)*(c->nfunctions + c->nnatives));
	m->functions_indexes = (int*)malloc(sizeof(int)*(c->nfunctions + c->nnatives));
	m->ctx.functions_types = (hl_type**)malloc(sizeof(void*)*(c->nfunctions + c->nnatives));
	if( m->functions_ptrs == NULL || m->functions_indexes == NULL || m->ctx.functions_types == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->functions_ptrs,0,sizeof(void*)*(c->nfunctions + c->nnatives));
	memset(m->functions_indexes,0xFF,sizeof(int)*(c->nfunctions + c->nnatives));
	memset(m->ctx.functions_types,0,sizeof(void*)*(c->nfunctions + c->nnatives));
	hl_alloc_init(&m->ctx.alloc);
	m->ctx.functions_ptrs = m->functions_ptrs;
	return m;
}

static void null_function() {
	hl_error("Null function ptr");
}

static void append_type( char **p, hl_type *t ) {
	*(*p)++ = TYPE_STR[t->kind];
	switch( t->kind ) {
	case HFUN:
		{
			int i;
			for(i=0;i<t->fun->nargs;i++)
				append_type(p,t->fun->args[i]);
			*(*p)++ = '_';
			append_type(p,t->fun->ret);
			break;
		}
	case HREF:
	case HNULL:
		append_type(p,t->tparam);
		break;
	case HOBJ:
		{
			int i;
			for(i=0;i<t->obj->nfields;i++)
				append_type(p,t->obj->fields[i].t);
			*(*p)++ = '_';
		}
		break;
	case HABSTRACT:
		*p += utostr(*p,100,t->abs_name);
		*(*p)++ = '_';
		break;
	default:
		break;
	}
}

int hl_module_init( hl_module *m ) {
	int i, entry;
	jit_ctx *ctx;
	// RESET globals
	for(i=0;i<m->code->nglobals;i++) {
		hl_type *t = m->code->globals[i];
		if( t->kind == HFUN ) *(void**)(m->globals_data + m->globals_indexes[i]) = null_function;
		if( hl_is_ptr(t) )
			hl_add_root(m->globals_data+m->globals_indexes[i]);
	}
	// INIT natives
	{
		char tmp[256];
		void *libHandler = NULL;
		const char *curlib = NULL, *sign;
		for(i=0;i<m->code->nnatives;i++) {
			hl_native *n = m->code->natives + i;
			char *p = tmp;
			void *f;
			if( curlib != n->lib ) {
				curlib = n->lib;
				strcpy(tmp,n->lib);
				if( strcmp(n->lib,"std") == 0 )
#				ifndef HL_WIN
					libHandler = RTLD_DEFAULT;
				else {
#				else
					strcpy(tmp, "libhl.dll");
				else
#				endif
#					ifdef HL_64
					strcpy(tmp+strlen(tmp),"64.hdll");
#					else
					strcpy(tmp+strlen(tmp),".hdll");
#					endif
					libHandler = dlopen(tmp,RTLD_LAZY);
					if( libHandler == NULL )
						hl_fatal1("Failed to load library %s",tmp);
#				ifndef HL_WIN
				}
#				endif
			}
			strcpy(p,"hlp_");
			p += 4;
			strcpy(p,n->name);
			p += strlen(n->name);
			*p++ = 0;
			f = dlsym(libHandler,tmp);
			if( f == NULL )
				hl_fatal2("Failed to load function %s@%s",n->lib,n->name);
			m->functions_ptrs[n->findex] = ((void *(*)( const char **p ))f)(&sign);
			p = tmp;
			append_type(&p,n->t);
			*p++ = 0;
			if( memcmp(sign,tmp,strlen(sign)+1) != 0 )
				hl_fatal4("Invalid signature for function %s@%s : %s required but %s found in hdll",n->lib,n->name,tmp,sign);
		}
	}
	// INIT indexes
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_indexes[f->findex] = i;
		m->ctx.functions_types[f->findex] = f->type;
	}
	for(i=0;i<m->code->nnatives;i++) {
		hl_native *n = m->code->natives + i;
		m->functions_indexes[n->findex] = i + m->code->nfunctions;
		m->ctx.functions_types[n->findex] = n->t;
	}
	for(i=0;i<m->code->ntypes;i++) {
		hl_type *t = m->code->types + i;
		switch( t->kind ) {
		case HOBJ:
			t->obj->m = &m->ctx;
			t->obj->global_value = ((int)(int_val)t->obj->global_value) ? (void**)(int_val)(m->globals_data + m->globals_indexes[(int)(int_val)t->obj->global_value-1]) : NULL;
			{
				int j;
				for(j=0;j<t->obj->nproto;j++) {
					hl_obj_proto *p = t->obj->proto + j;
					hl_function *f = m->code->functions + m->functions_indexes[p->findex];
					f->obj = t->obj;
					f->field = p->name;
				}
			}
			break;
		case HENUM:
			hl_init_enum(t->tenum);
			t->tenum->global_value = ((int)(int_val)t->tenum->global_value) ? (void**)(int_val)(m->globals_data + m->globals_indexes[(int)(int_val)t->tenum->global_value-1]) : NULL;
			break;
		case HVIRTUAL:
			hl_init_virtual(t,&m->ctx);
			break;
		default:
			break;
		}
	}
	// JIT
	ctx = hl_jit_alloc();
	if( ctx == NULL )
		return 0;
	hl_jit_init(ctx, m);
	entry = hl_jit_init_callback(ctx);
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		int fpos = hl_jit_function(ctx, m, f);
		if( fpos < 0 ) {
			hl_jit_free(ctx);
			return 0;
		}
		m->functions_ptrs[f->findex] = (void*)(int_val)fpos;
	}
	m->jit_code = hl_jit_code(ctx, m, &m->codesize, &m->jit_debug);
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_ptrs[f->findex] = ((unsigned char*)m->jit_code) + ((int_val)m->functions_ptrs[f->findex]);
	}
	hl_callback_init(((unsigned char*)m->jit_code) + entry);
	cur_module = m;
	stack_top = &m;
	hl_setup_exception(module_resolve_symbol, module_capture_stack);
	hl_jit_free(ctx);
	return 1;
}

void hl_module_free( hl_module *m ) {
	hl_free(&m->ctx.alloc);
	hl_free_executable_memory(m->code, m->codesize);
	free(m->functions_indexes);
	free(m->functions_ptrs);
	free(m->ctx.functions_types);
	free(m->globals_indexes);
	free(m->globals_data);
	free(m);
}
