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

#ifdef HL_WIN
#	include <windows.h>
#	define dlopen(l,p)		(void*)( (l) ? LoadLibraryA(l) : GetModuleHandle(NULL))
#	define dlsym(h,n)		GetProcAddress((HANDLE)h,n)
#else
#	include <dlfcn.h>
#endif

hl_runtime_obj *hl_get_obj_rt( hl_module *m, hl_type *ot ) {
	hl_alloc *alloc = &m->code->alloc;
	hl_type_obj *o = ot->obj;
	hl_runtime_obj *p = NULL, *t;
	int i, size, start;
	if( o->rt ) return o->rt;
	if( o->super ) p = hl_get_obj_rt(m, o->super);
	t = (hl_runtime_obj*)hl_malloc(alloc,sizeof(hl_runtime_obj));
	t->nfields = o->nfields + (p ? p->nfields : 0);
	t->fields_indexes = (int*)hl_malloc(alloc,sizeof(int)*t->nfields);
	t->toString = NULL;
	
	// fields indexes
	start = 0;
	if( p ) {
		start = p->nfields;
		memcpy(t->fields_indexes, p->fields_indexes, sizeof(int)*p->nfields);
	}
	size = p ? p->size : HL_WSIZE; // hl_type*
	for(i=0;i<o->nfields;i++) {
		hl_type *ft = o->fields[i].t;
		size += hl_pad_size(size,ft);
		t->fields_indexes[i+start] = size;
		size += hl_type_size(ft);
	}
	t->size = size;
	t->proto = NULL;
	o->rt = t;
	return t;
}

hl_runtime_obj *hl_get_obj_proto( hl_module *m, hl_type *ot ) {
	hl_alloc *alloc = &m->code->alloc;
	hl_type_obj *o = ot->obj;
	hl_runtime_obj *p = NULL, *t = hl_get_obj_rt(m, ot);
	int i;
	if( t->proto ) return t;
	if( o->super ) p = hl_get_obj_proto(m,o->super);
	if( p )
		t->nproto = p->nproto;
	else
		t->nproto = 0;
	for(i=0;i<o->nproto;i++) {
		hl_obj_proto *p = o->proto + i;
		if( p->pindex >= t->nproto ) t->nproto = p->pindex + 1;
		if( memcmp(p->name,"__string",9) == 0 )
			t->toString = m->functions_ptrs[p->findex];
	}
	t->proto = (vobj_proto*)hl_malloc(alloc, sizeof(vobj_proto) + t->nproto * sizeof(void*));
	t->proto->t = ot;
	if( t->nproto ) {
		void **fptr = (void**)((unsigned char*)t->proto + sizeof(vobj_proto));
		if( p )
			memcpy(fptr, (unsigned char*)p->proto + sizeof(vobj_proto), p->nproto * sizeof(void*));
		for(i=0;i<o->nproto;i++) {
			hl_obj_proto *p = o->proto + i;
			if( p->pindex >= 0 ) fptr[p->pindex] = m->functions_ptrs[p->findex];
		}
	}
	return t;
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
	if( m->functions_ptrs == NULL || m->functions_indexes == NULL ) {
		hl_module_free(m);
		return NULL;
	}
	memset(m->functions_ptrs,0,sizeof(void*)*(c->nfunctions + c->nnatives));
	memset(m->functions_indexes,0xFF,sizeof(int)*(c->nfunctions + c->nnatives));
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
				append_type(p,t->fun->args + i);
			*(*p)++ = '_';
			append_type(p,t->fun->ret);
			break;
		}
	case HARRAY:
		append_type(p,t->t);
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
		if( t->kind == HFUN ) *(fptr*)(m->globals_data + m->globals_indexes[i]) = null_function;
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
#				ifdef HL_64
				strcpy(tmp+strlen(tmp),"64.hdll");
#				else
				strcpy(tmp+strlen(tmp),".hdll");
#				endif
				libHandler = dlopen(memcmp(n->lib,"std",4) == 0 ? NULL : tmp,RTLD_LAZY);
				if( libHandler == NULL )
					hl_error("Failed to load library %s",tmp);
			}
			strcpy(p,"hl_");
			p += 3;
			strcpy(p,n->name);
			p += strlen(n->name);
			*p++ = 0;
			f = dlsym(libHandler,tmp);
			if( f == NULL )
				hl_error("Failed to load function %s@%s",n->lib,n->name);
			m->functions_ptrs[n->findex] = ((void *(*)( const char **p ))f)(&sign);
			p = tmp;
			append_type(&p,n->t);
			*p++ = 0;
			if( memcmp(sign,tmp,strlen(sign)+1) != 0 )
				hl_error("Invalid signature for function %s@%s : %s should be %s",n->lib,n->name,tmp,sign);
		}
	}
	// INIT indexes
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_indexes[f->findex] = i;
	}
	for(i=0;i<m->code->nnatives;i++) {
		hl_native *n = m->code->natives + i;
		m->functions_indexes[n->findex] = i + m->code->nfunctions;
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
	m->jit_code = hl_jit_code(ctx, m, &m->codesize);
	for(i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		m->functions_ptrs[f->findex] = ((unsigned char*)m->jit_code) + ((int_val)m->functions_ptrs[f->findex]);
	}
	hl_callback_init(((unsigned char*)m->jit_code) + entry);
	hl_jit_free(ctx);
	return 1;
}

void hl_module_free( hl_module *m ) {
	hl_free_executable_memory(m->code, m->codesize);
	free(m->functions_indexes);
	free(m->functions_ptrs);
	free(m->globals_indexes);
	free(m->globals_data);
	free(m);
}
