/*
 * Copyright (C)2015-2026 Haxe Foundation
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
#include <jit.h>

#ifdef HL_WIN
#	undef _GUID
#	include <windows.h>
#endif

HL_API int hl_mark_size( int data_size );
HL_API void hl_gc_enable( bool b );
HL_API void hl_gc_world( bool stop );
HL_API void hl_gc_iter_live( void (*callback)( void *block, int size, int kind, void *ctx ), void *ctx );
HL_API int hl_gc_other_stacks( void **ranges, int max );
HL_API void ***hl_gc_get_roots( int *count );
HL_API int hl_gc_stacks_find( void **blocks, int *sizes, int count, void *skip_start, void *skip_end );
HL_API void hl_gc_remap_reserve( int count );
HL_API void hl_gc_remap_add( void *old_block, void *new_block );
HL_API void hl_gc_remap_apply( void );

void hl_module_init_natives( hl_module *m );
void hl_module_init_indexes( hl_module *m );
void hl_module_init_constant( hl_module *m, hl_constant *c );
void hl_module_add( hl_module *m );
void hl_module_null_function();
bool hl_module_same_type( hl_type *t1, hl_type *t2 );
int hl_module_capture_stack( void **stack, int size );
bool hl_module_is_call_site( void *ret );


#define HOT_RELOAD_EXTRA_GLOBALS	4096

static int hot_init_bits( hl_type *t, int word );

static bool hot_switch( const char *name ) {
	const char *v = getenv(name);
	return v != NULL && *v != 0 && strcmp(v,"0") != 0;
}

HL_API double hl_sys_time();
static double hot_time_last = 0.;
static void hot_time( const char *phase ) {
	static int enabled = -1;
	if( enabled < 0 ) enabled = hot_switch("HL_HOT_RELOAD_TIMES");
	if( !enabled ) return;
	double t = hl_sys_time();
	if( phase ) printf("[HotReload] %s %.1f ms\n", phase, (t - hot_time_last) * 1000.);
	hot_time_last = t;
}
static void hot_record_uses( hl_module *m );


typedef struct {
	int size;
	int count;
	int *keys;
	int *values;
} hot_map;

typedef struct {
	int sign;
	int hash;
	hl_function *f;
	void **slot;
} hot_function;

typedef struct {
	hl_type *t;
	int hash;
} hot_type;

struct hl_hot_reload {
	hl_module *root;
	int nfunctions;
	int max_functions;
	hot_function *functions;
	hot_map functions_map;
	int nglobals;
	int max_globals;
	int globals_size;
	int max_globals_size;
	int *globals_signs;
	hl_type **globals_types;
	hot_map globals_map;
	int ntypes;
	int max_types;
	hot_type *types;
	hot_map types_map;
	int reloads;
};

static unsigned int hot_map_hash( int key ) {
	unsigned int h = (unsigned int)key;
	h ^= h >> 16;
	h *= 0x45d9f3b;
	h ^= h >> 16;
	return h;
}

static void hot_map_add( hot_map *m, int key, int value );

static void hot_map_grow( hot_map *m ) {
	hot_map old = *m;
	m->size = old.size ? old.size * 2 : 64;
	m->count = 0;
	m->keys = (int*)malloc(sizeof(int) * m->size);
	m->values = (int*)malloc(sizeof(int) * m->size);
	memset(m->values,0xFF,sizeof(int) * m->size);
	for(int i=0;i<old.size;i++)
		if( old.values[i] >= 0 )
			hot_map_add(m, old.keys[i], old.values[i]);
	free(old.keys);
	free(old.values);
}

static void hot_map_add( hot_map *m, int key, int value ) {
	if( (m->count + 1) * 2 > m->size ) hot_map_grow(m);
	unsigned int i = hot_map_hash(key) & (m->size - 1);
	while( m->values[i] >= 0 ) i = (i + 1) & (m->size - 1);
	m->keys[i] = key;
	m->values[i] = value;
	m->count++;
}

static int hot_map_next( hot_map *m, int key, int *cursor ) {
	if( !m->size ) return -1;
	unsigned int i = *cursor < 0 ? hot_map_hash(key) & (m->size - 1) : (*cursor + 1) & (m->size - 1);
	while( m->values[i] >= 0 ) {
		if( m->keys[i] == key ) {
			*cursor = (int)i;
			return m->values[i];
		}
		i = (i + 1) & (m->size - 1);
	}
	return -1;
}

static void hot_map_free( hot_map *m ) {
	free(m->keys);
	free(m->values);
}

static int hot_name_hash( const uchar *name ) {
	unsigned int h = 2166136261u;
	while( *name ) {
		h ^= *name++;
		h *= 16777619u;
	}
	return (int)h;
}

static const uchar *hot_type_name( hl_type *t ) {
	switch( t->kind ) {
	case HOBJ:
	case HSTRUCT:
		return t->obj->name;
	case HENUM:
		return t->tenum->global_value ? t->tenum->name : NULL;
	default:
		return NULL;
	}
}

static void hot_add_function( hl_hot_reload *hr, int sign, int hash, hl_function *f, void **slot ) {
	if( hr->nfunctions == hr->max_functions ) {
		hr->max_functions = hr->max_functions ? hr->max_functions * 2 : 256;
		hr->functions = (hot_function*)realloc(hr->functions, sizeof(hot_function) * hr->max_functions);
	}
	hot_function *hf = hr->functions + hr->nfunctions;
	hf->sign = sign;
	hf->hash = hash;
	hf->f = f;
	hf->slot = slot;
	hot_map_add(&hr->functions_map, sign, hr->nfunctions);
	hr->nfunctions++;
}

static void hot_add_type( hl_hot_reload *hr, hl_type *t ) {
	const uchar *name = hot_type_name(t);
	if( name == NULL ) return;
	int hash = hot_name_hash(name);
	int cursor = -1, idx;
	while( (idx = hot_map_next(&hr->types_map, hash, &cursor)) >= 0 ) {
		hl_type *t2 = hr->types[idx].t;
		if( t2->kind == t->kind && ucmp(hot_type_name(t2), name) == 0 )
			return;
	}
	if( hr->ntypes == hr->max_types ) {
		hr->max_types = hr->max_types ? hr->max_types * 2 : 256;
		hr->types = (hot_type*)realloc(hr->types, sizeof(hot_type) * hr->max_types);
	}
	hr->types[hr->ntypes].t = t;
	hr->types[hr->ntypes].hash = hash;
	hot_map_add(&hr->types_map, hash, hr->ntypes);
	hr->ntypes++;
}

static hl_type *hot_find_type( hl_hot_reload *hr, hl_type *t ) {
	const uchar *name = hot_type_name(t);
	if( name == NULL ) return NULL;
	int cursor = -1, idx;
	while( (idx = hot_map_next(&hr->types_map, hot_name_hash(name), &cursor)) >= 0 ) {
		hl_type *t2 = hr->types[idx].t;
		if( t2->kind == t->kind && ucmp(hot_type_name(t2), name) == 0 )
			return t2;
	}
	return NULL;
}

static int hot_reload_extra_globals() {
	static int extra = 0;
	if( extra == 0 ) {
		const char *v = getenv("HL_HOT_RELOAD_GLOBALS");
		extra = v ? atoi(v) : 0;
		if( extra <= 0 ) extra = HOT_RELOAD_EXTRA_GLOBALS;
	}
	return extra;
}

static void hot_reload_init( hl_module *m, int max_globals, int max_globals_size ) {
	hl_hot_reload *hr = (hl_hot_reload*)malloc(sizeof(hl_hot_reload));
	int i;
	memset(hr,0,sizeof(hl_hot_reload));
	hr->root = m;
	hl_code *c = m->code;
	for(i=0;i<c->nfunctions;i++) {
		hl_function *f = c->functions + i;
		if( fun_obj(f) == NULL ) continue;
		hot_add_function(hr, m->hash->functions_signs[i], m->hash->functions_hashes[i], f, m->functions_ptrs + f->findex);
	}
	hr->nglobals = c->nglobals;
	hr->max_globals = max_globals;
	hr->globals_size = m->globals_size;
	hr->max_globals_size = max_globals_size;
	hr->globals_signs = (int*)malloc(sizeof(int) * max_globals);
	hr->globals_types = (hl_type**)malloc(sizeof(hl_type*) * max_globals);
	for(i=0;i<c->nglobals;i++) {
		hr->globals_signs[i] = m->hash->globals_signs[i];
		hr->globals_types[i] = c->globals[i];
		hot_map_add(&hr->globals_map, hr->globals_signs[i], i);
	}
	for(i=0;i<c->ntypes;i++)
		hot_add_type(hr, c->types + i);
	m->hot = hr;
}

void hl_hot_reload_free( hl_hot_reload *hr ) {
	hot_map_free(&hr->functions_map);
	hot_map_free(&hr->globals_map);
	hot_map_free(&hr->types_map);
	free(hr->functions);
	free(hr->globals_signs);
	free(hr->globals_types);
	free(hr->types);
	free(hr);
}


typedef struct {
	char *buf;
	int size;
	int pos;
	int count;
	int running;
} hot_report;

static void hot_print( hot_report *r, const char *fmt, ... ) {
	va_list args;
	if( r->pos >= r->size - 1 ) return;
	va_start(args, fmt);
	int n = vsnprintf(r->buf + r->pos, r->size - r->pos, fmt, args);
	va_end(args);
	if( n > 0 ) r->pos += n;
	if( r->pos > r->size - 1 ) r->pos = r->size - 1;
}

static void hot_print_name( hot_report *r, const uchar *name ) {
	char tmp[256];
	int i = 0;
	while( name && *name && i < 255 ) {
		uchar c = *name++;
		tmp[i++] = c < 128 ? (char)c : '?';
	}
	tmp[i] = 0;
	hot_print(r, "%s", tmp);
}

static void hot_print_fields( hot_report *r, const char *what, hl_type_obj *o1, hl_type_obj *o2 ) {
	bool first = true;
	for(int i=0;i<o1->nfields;i++) {
		hl_obj_field *f1 = o1->fields + i;
		bool found = false;
		for(int k=0;k<o2->nfields;k++) {
			hl_obj_field *f2 = o2->fields + k;
			if( f1->hashed_name == f2->hashed_name && hl_module_same_type(f1->t, f2->t) ) {
				found = true;
				break;
			}
		}
		if( found ) continue;
		hot_print(r, first ? ", %s " : " ", what);
		hot_print_name(r, *f1->name ? f1->name : USTR("(interface)"));
		first = false;
	}
}

static bool hot_is_class_object( hl_type *t ) {
	if( t->kind != HOBJ ) return false;
	const uchar *name = t->obj->name, *last = name;
	while( *name ) {
		if( *name == '.' ) last = name + 1;
		name++;
	}
	return *last == '$';
}

static const uchar *hot_super_name( hl_type_obj *o ) {
	return o->super ? o->super->obj->name : USTR("");
}

static int hot_enum_param( hl_enum_construct *oc, int p, hl_enum_construct *nc ) {
	if( oc->pnames && nc->pnames && oc->pnames[p] ) {
		for(int k=0;k<nc->nparams;k++)
			if( nc->pnames[k] && ucmp(oc->pnames[p], nc->pnames[k]) == 0 ) return k;
		return -1;
	}
	return p < nc->nparams ? p : -1;
}

static bool hot_same_virtuals( hl_type_obj *o1, hl_type_obj *o2 ) {
	for(int pass=0;pass<2;pass++) {
		hl_type_obj *a = pass ? o2 : o1;
		hl_type_obj *b = pass ? o1 : o2;
		for(int i=0;i<a->nproto;i++) {
			hl_obj_proto *p = a->proto + i;
			if( p->pindex < 0 ) continue;
			bool found = false;
			for(int k=0;k<b->nproto;k++)
				if( b->proto[k].hashed_name == p->hashed_name ) {
					found = b->proto[k].pindex == p->pindex;
					break;
				}
			if( !found ) return false;
		}
	}
	return true;
}

#define HOT_LAYOUT_SAME		0
#define HOT_LAYOUT_CHANGED	1
#define HOT_LAYOUT_INSTANCES	2

static void hot_print_type_name( hot_report *r, hl_type_obj *o ) {
	const uchar *name = o->name, *last = name;
	while( *name ) {
		if( *name == '.' ) last = name + 1;
		name++;
	}
	if( *last != '$' ) {
		hot_print_name(r, o->name);
		return;
	}
	char tmp[256];
	int i = 0;
	for(name=o->name;*name && i < 255;name++)
		if( name != last ) tmp[i++] = *name < 128 ? (char)*name : '?';
	tmp[i] = 0;
	hot_print(r, "%s", tmp);
}

static int hot_check_layout( hl_type *told, hl_type *tnew, hot_report *r ) {
	switch( told->kind ) {
	case HOBJ:
	case HSTRUCT:
		{
			hl_type_obj *o1 = told->obj, *o2 = tnew->obj;
			bool same = true;
			if( ucmp(hot_super_name(o1), hot_super_name(o2)) != 0 )
				same = false;
			else if( o1->nfields != o2->nfields )
				same = false;
			else {
				for(int i=0;i<o1->nfields;i++)
					if( o1->fields[i].hashed_name != o2->fields[i].hashed_name || !hl_module_same_type(o1->fields[i].t,o2->fields[i].t) ) {
						same = false;
						break;
					}
			}
			if( same && hot_same_virtuals(o1, o2) )
				return HOT_LAYOUT_SAME;
			if( told->kind == HOBJ )
				return HOT_LAYOUT_INSTANCES;
			hot_print(r, "\n  ");
			hot_print_type_name(r, o1);
			hot_print(r, same ? ": methods changed" : ": fields changed");
			if( ucmp(hot_super_name(o1), hot_super_name(o2)) != 0 ) {
				hot_print(r, ", superclass ");
				hot_print_name(r, hot_super_name(o1));
				hot_print(r, " -> ");
				hot_print_name(r, hot_super_name(o2));
			}
			hot_print_fields(r, "added", o2, o1);
			hot_print_fields(r, "removed", o1, o2);
			r->count++;
			return HOT_LAYOUT_CHANGED;
		}
	case HENUM:
		{
			hl_type_enum *e1 = told->tenum, *e2 = tnew->tenum;
			bool same = e1->nconstructs == e2->nconstructs;
			for(int i=0;same && i<e1->nconstructs;i++) {
				hl_enum_construct *c1 = e1->constructs + i, *c2 = e2->constructs + i;
				if( ucmp(c1->name,c2->name) != 0 || c1->nparams != c2->nparams ) {
					same = false;
					break;
				}
				for(int k=0;k<c1->nparams;k++)
					if( !hl_module_same_type(c1->params[k],c2->params[k]) || hot_enum_param(c1,k,c2) != k ) {
						same = false;
						break;
					}
			}
			return same ? HOT_LAYOUT_SAME : HOT_LAYOUT_INSTANCES;
		}
	default:
		return HOT_LAYOUT_SAME;
	}
}


static int hot_field_size( hl_type *t ) {
	return t->kind == HPACKED ? hl_get_obj_rt(t->tparam)->size : hl_type_size(t);
}

static bool hot_is_num( hl_type *t ) {
	switch( t->kind ) {
	case HUI8: case HUI16: case HI32: case HI64: case HF32: case HF64:
		return true;
	default:
		return false;
	}
}

static bool hot_is_dynamic( hl_type *t ) {
	switch( t->kind ) {
	case HOBJ: case HDYNOBJ: case HVIRTUAL: case HENUM: case HFUN: case HARRAY: case HNULL: case HDYN:
		return true;
	default:
		return false;
	}
}

static bool hot_convertible( hl_type *ot, hl_type *nt ) {
	if( hot_is_num(ot) )
		return hot_is_num(nt) || nt->kind == HDYN || (nt->kind == HNULL && hot_is_num(nt->tparam));
	if( ot->kind == HNULL && (hot_is_num(ot->tparam) || ot->tparam->kind == HBOOL) && (hot_is_num(nt) || nt->kind == HBOOL) )
		return true;
	if( ot->kind == HBOOL && nt->kind == HNULL && nt->tparam->kind == HBOOL )
		return true;
	if( !hot_is_dynamic(ot) ) return false;
	switch( nt->kind ) {
	case HDYN:
		return true;
	case HNULL:
		return (ot->kind == HDYN || ot->kind == HNULL) && hot_is_num(nt->tparam);
	case HOBJ:
	case HVIRTUAL:
		return ot->kind == HOBJ || ot->kind == HDYN || ot->kind == HVIRTUAL || ot->kind == HDYNOBJ;
	default:
		return hot_is_num(nt) && (ot->kind == HDYN || ot->kind == HNULL);
	}
}

static int hot_find_field( hl_type_obj *o, hl_obj_field *f ) {
	for(int i=0;i<o->nfields;i++)
		if( o->fields[i].hashed_name == f->hashed_name && hl_module_same_type(o->fields[i].t, f->t) )
			return i;
	for(int i=0;i<o->nfields;i++)
		if( f->hashed_name && o->fields[i].hashed_name == f->hashed_name && hot_convertible(o->fields[i].t, f->t) )
			return i;
	return -1;
}

static void hot_read_num( hl_type *t, void *p, double *d, int64 *i ) {
	switch( t->kind ) {
	case HUI8: *i = *(unsigned char*)p; break;
	case HUI16: *i = *(unsigned short*)p; break;
	case HI32: *i = *(int*)p; break;
	case HI64: *i = *(int64*)p; break;
	case HF32: *d = *(float*)p; *i = (int64)*d; return;
	case HF64: *d = *(double*)p; *i = (int64)*d; return;
	case HBOOL: *i = *(bool*)p; break;
	default: *i = 0; break;
	}
	*d = (double)*i;
}

static void hot_write_num( hl_type *t, void *p, double d, int64 i ) {
	switch( t->kind ) {
	case HUI8: *(unsigned char*)p = (unsigned char)i; break;
	case HUI16: *(unsigned short*)p = (unsigned short)i; break;
	case HI32: *(int*)p = (int)i; break;
	case HI64: *(int64*)p = i; break;
	case HF32: *(float*)p = (float)d; break;
	case HF64: *(double*)p = d; break;
	case HBOOL: *(bool*)p = i != 0; break;
	default: break;
	}
}

static bool hot_is_subclass( hl_type *t, hl_type *of ) {
	for(;t && t->kind == HOBJ;t=t->obj->super)
		if( ucmp(t->obj->name, of->obj->name) == 0 ) return true;
	return false;
}

static void hot_record_uses( hl_module *m ) {
	hl_code *c = m->code;
	hl_type_obj *tmp[64];
	m->hot_uses = (hl_type_obj***)hl_malloc(&m->ctx.alloc, sizeof(void*) * c->nfunctions);
	m->hot_code = (void**)hl_malloc(&m->ctx.alloc, sizeof(void*) * c->nfunctions);
	for(int k=0;k<c->nfunctions;k++) {
		void *ptr = m->functions_ptrs[c->functions[k].findex];
		unsigned char *code = (unsigned char*)m->jit_code;
		m->hot_code[k] = (unsigned char*)ptr >= code && (unsigned char*)ptr < code + m->codesize ? ptr : NULL;
	}
	for(int k=0;k<c->nfunctions;k++) {
		hl_function *f = c->functions + k;
		int count = 0;
		for(int i=0;i<f->nregs && count < 63;i++) {
			hl_type *t = f->regs[i];
			hl_type_obj *layout;
			if( t->kind == HOBJ || t->kind == HSTRUCT )
				layout = t->obj;
			else if( t->kind == HENUM )
				layout = (hl_type_obj*)t->tenum;
			else
				continue;
			bool found = false;
			for(int j=0;j<count;j++)
				if( tmp[j] == layout ) found = true;
			if( !found ) tmp[count++] = layout;
		}
		hl_type_obj **uses = (hl_type_obj**)hl_malloc(&m->ctx.alloc, sizeof(void*) * (count + 1));
		memcpy(uses, tmp, sizeof(void*) * count);
		uses[count] = NULL;
		m->hot_uses[k] = uses;
	}
}

static hl_function *hot_function_at( void *addr, hl_module **mod ) {
	int modules_count;
	hl_module **cur_modules = hl_get_modules(&modules_count);
	for(int i=0;i<modules_count;i++) {
		hl_module *m = cur_modules[i];
		unsigned char *code = (unsigned char*)m->jit_code;
		if( (unsigned char*)addr < code || (unsigned char*)addr >= code + m->codesize ) continue;
		hl_function *best = NULL;
		unsigned char *best_ptr = NULL;
		if( m->hot_code == NULL ) return NULL;
		for(int k=0;k<m->code->nfunctions;k++) {
			hl_function *f = m->code->functions + k;
			unsigned char *ptr = (unsigned char*)m->hot_code[k];
			if( ptr == NULL || ptr > (unsigned char*)addr ) continue;
			if( ptr > best_ptr ) {
				best = f;
				best_ptr = ptr;
			}
		}
		*mod = m;
		return best;
	}
	return NULL;
}



typedef struct {
	int size;
	int count;
	void **keys;
	int *values;
} hot_pmap;

static unsigned int hot_pmap_hash( void *p ) {
	uint64 h = (uint64)(int_val)p;
	h ^= h >> 33;
	h *= 0xff51afd7ed558ccdULL;
	h ^= h >> 33;
	return (unsigned int)h;
}

static void hot_pmap_set( hot_pmap *m, void *key, int value ) {
	if( (m->count + 1) * 2 > m->size ) {
		hot_pmap old = *m;
		m->size = old.size ? old.size * 2 : 64;
		m->count = 0;
		m->keys = (void**)calloc(m->size, sizeof(void*));
		m->values = (int*)malloc(sizeof(int) * m->size);
		for(int i=0;i<old.size;i++)
			if( old.keys[i] ) hot_pmap_set(m, old.keys[i], old.values[i]);
		free(old.keys);
		free(old.values);
	}
	unsigned int i = hot_pmap_hash(key) & (m->size - 1);
	while( m->keys[i] && m->keys[i] != key ) i = (i + 1) & (m->size - 1);
	if( !m->keys[i] ) m->count++;
	m->keys[i] = key;
	m->values[i] = value;
}

static int hot_pmap_get( hot_pmap *m, void *key ) {
	if( !m->size || !key ) return -1;
	unsigned int i = hot_pmap_hash(key) & (m->size - 1);
	while( m->keys[i] ) {
		if( m->keys[i] == key ) return m->values[i];
		i = (i + 1) & (m->size - 1);
	}
	return -1;
}

static void hot_pmap_free( hot_pmap *m ) {
	free(m->keys);
	free(m->values);
}

typedef struct {
	int noff;
	int ooff;
	hl_type *ot;
	hl_type *nt;
} hot_conv;

#define HOT_INIT_NUM	0
#define HOT_INIT_GLOBAL	1
#define HOT_INIT_DYN	2

typedef struct {
	int off;
	int kind;
	hl_type *t;
	hl_type *vt;
	int64 i;
	double d;
	void **global;
} hot_finit;

typedef struct {
	hl_type *old;
	hl_type *t;
	int index;
	hl_type_obj *old_obj;
	hl_runtime_obj *ort;
	hl_runtime_obj *nrt;
	int *copies;
	int ncopies;
	hot_conv *convs;
	int nconvs;
	unsigned char *filled;
	hot_finit *inits;
	int ninits;
	unsigned int *added;
	int *bindings;
	int nbindings;
	int ninstances;
	int nrelocated;
	int max_block;
	bool fields_same;
	bool slots_same;
	bool is_enum;
	void *old_layout;
	hl_type_enum *old_enum;
	int *cmap;
	int **ccopies;
	int *nccopies;
	int removed_live;
	int removed_ctor;
} hot_itype;

typedef struct {
	hot_itype *types;
	int ntypes;
	hot_pmap aliases;
	hot_pmap virtuals;
	void **objs;
	int *objs_info;
	int nobjs;
	int max_objs;
	void **virts;
	int nvirts;
	int max_virts;
	varray *hooks;
	void **removed;
	int *removed_info;
	int nremoved;
	int max_removed;
	int nrelocated;
	hot_pmap methods;
	int *mnames;
	hl_type **mtypes;
	int nmethods;
	int max_methods;
	void **closures;
	int *closures_info;
	int nclosures;
	int max_closures;
	void ***vconv;
	hl_type **vconv_t;
	int nvconv;
	int max_vconv;
	hl_code *code;
	hl_hot_reload *hr;
	hot_pmap hooktypes;
	void **hooked;
	int *hooked_f;
	int nhooked;
	int max_hooked;
} hot_instances;

static void hot_convert( hot_instances *h, hl_type *ot, void *src, hl_type *nt, void *dst ) {
	double d;
	int64 i;
	char tmp[8];
	if( hot_is_num(ot) || ot->kind == HBOOL ) {
		hot_read_num(ot, src, &d, &i);
		if( hot_is_num(nt) || nt->kind == HBOOL )
			hot_write_num(nt, dst, d, i);
		else if( nt->kind == HNULL ) {
			hot_write_num(nt->tparam, tmp, d, i);
			*(vdynamic**)dst = hl_make_dyn(tmp, nt->tparam);
		} else if( nt->kind == HDYN )
			*(vdynamic**)dst = hl_make_dyn(src, ot);
		return;
	}
	void *v = *(void**)src;
	if( v == NULL ) return;
	if( hot_is_num(nt) || nt->kind == HBOOL || nt->kind == HNULL ) {
		vdynamic *dv = (vdynamic*)v;
		if( !hot_is_num(dv->t) && dv->t->kind != HBOOL ) return;
		hot_read_num(dv->t, &dv->v, &d, &i);
		if( nt->kind != HNULL ) {
			hot_write_num(nt, dst, d, i);
			return;
		}
		hot_write_num(nt->tparam, tmp, d, i);
		*(vdynamic**)dst = hl_make_dyn(tmp, nt->tparam);
		return;
	}
	if( nt->kind == HDYN ) {
		*(void**)dst = v;
		return;
	}
	hl_type *vt = *(hl_type**)v;
	if( vt->kind == HVIRTUAL ) {
		v = ((vvirtual*)v)->value;
		if( v == NULL ) return;
		vt = *(hl_type**)v;
	}
	if( nt->kind == HOBJ ) {
		if( vt->kind == HOBJ && hot_is_subclass(vt, nt) ) *(void**)dst = v;
		return;
	}
	if( nt->kind == HVIRTUAL && (vt->kind == HOBJ || vt->kind == HDYNOBJ) ) {
		*(void**)dst = v;
		if( h->nvconv == h->max_vconv ) {
			h->max_vconv = h->max_vconv ? h->max_vconv * 2 : 16;
			h->vconv = (void***)realloc(h->vconv, sizeof(void*) * h->max_vconv);
			h->vconv_t = (hl_type**)realloc(h->vconv_t, sizeof(hl_type*) * h->max_vconv);
		}
		h->vconv[h->nvconv] = (void**)dst;
		h->vconv_t[h->nvconv] = nt;
		h->nvconv++;
	}
}

static void hot_print_layout_name( hot_report *r, hot_itype *it );

static hl_function **hot_blockers = NULL;
static int hot_nblockers = 0, hot_max_blockers = 0;

static void hot_add_blocker( hl_function *f ) {
	for(int i=0;i<hot_nblockers;i++)
		if( hot_blockers[i] == f ) return;
	if( hot_nblockers == hot_max_blockers ) {
		hot_max_blockers = hot_max_blockers ? hot_max_blockers * 2 : 8;
		hot_blockers = (hl_function**)realloc(hot_blockers, sizeof(hl_function*) * hot_max_blockers);
	}
	hot_blockers[hot_nblockers++] = f;
}

static bool hot_is_blocker( hl_function *f ) {
	for(int i=0;i<hot_nblockers;i++)
		if( hot_blockers[i] == f ) return true;
	return false;
}

static void hot_check_function( hl_module *m, hl_function *f, hot_itype *types, int count, const char *where, hot_report *r ) {
	hl_type_obj **fuses = m->hot_uses[f - m->code->functions];
	for(int k=0;k<count;k++) {
		if( types[k].fields_same && types[k].slots_same ) continue;
		bool uses = false;
		for(int j=0;fuses[j] && !uses;j++)
			uses = (void*)fuses[j] == types[k].old_layout;
		if( !uses ) continue;
		hot_print(r, "\n  ");
		hot_print_layout_name(r, types + k);
		hot_print(r, types[k].fields_same ? ": its methods move to other slots and it is used by " : ": its layout changes and it is used by ");
		if( fun_obj(f) ) {
			hot_print_type_name(r, fun_obj(f));
			hot_print(r, ".");
			hot_print_name(r, fun_field_name(f));
		} else
			hot_print(r, "fun$%d", f->findex);
		hot_print(r, ", which is running%s : move the code that uses it out of it", where);
		r->count++;
		r->running++;
		hot_add_blocker(f);
	}
}

static void hot_check_stack( hot_itype *types, int count, hot_report *r ) {
	void *stack[512];
	if( count == 0 ) return;
	int n = hl_module_capture_stack(stack, 512);
	for(int i=0;i<n;i++) {
		hl_module *m = NULL;
		hl_function *f = hot_function_at(stack[i], &m);
		if( f == NULL || m->hot_uses == NULL ) continue;
		if( f->findex == m->code->entrypoint ) continue;
		hot_check_function(m, f, types, count, "", r);
	}
	void *ranges[256];
	int nstacks = hl_gc_other_stacks(ranges, 128);
	for(int i=0;i<nstacks;i++) {
		for(void **p=(void**)ranges[i*2];p<(void**)ranges[i*2+1];p++) {
			hl_module *m = NULL;
			hl_function *f = hot_function_at(*p, &m);
			if( f == NULL || m->hot_uses == NULL || !hl_module_is_call_site(*p) ) continue;
			if( f->findex == m->code->entrypoint ) continue;
			hot_check_function(m, f, types, count, " in another thread", r);
		}
	}
}

static bool hot_still_blocked( void ) {
	if( hot_nblockers == 0 ) return false;
	bool found = false;
	void *stack[512];
	hl_gc_world(true);
	int n = hl_module_capture_stack(stack, 512);
	for(int i=0;i<n && !found;i++) {
		hl_module *m = NULL;
		hl_function *f = hot_function_at(stack[i], &m);
		found = f != NULL && hot_is_blocker(f);
	}
	void *ranges[256];
	int nstacks = found ? 0 : hl_gc_other_stacks(ranges, 128);
	for(int i=0;i<nstacks && !found;i++)
		for(void **p=(void**)ranges[i*2];p<(void**)ranges[i*2+1] && !found;p++) {
			hl_module *m = NULL;
			hl_function *f = hot_function_at(*p, &m);
			found = f != NULL && hl_module_is_call_site(*p) && hot_is_blocker(f);
		}
	hl_gc_world(false);
	return found;
}

static void hot_add_hooked( hot_instances *h, void *block, void *t ) {
	int fidx = h->hooktypes.count ? hot_pmap_get(&h->hooktypes, t) : -1;
	if( fidx < 0 ) return;
	if( h->nhooked == h->max_hooked ) {
		h->max_hooked = h->max_hooked ? h->max_hooked * 2 : 64;
		h->hooked = (void**)realloc(h->hooked, sizeof(void*) * h->max_hooked);
		h->hooked_f = (int*)realloc(h->hooked_f, sizeof(int) * h->max_hooked);
	}
	h->hooked[h->nhooked] = block;
	h->hooked_f[h->nhooked] = fidx;
	h->nhooked++;
}

static void hot_scan_block( void *block, int size, int kind, void *ctx ) {
	hot_instances *h = (hot_instances*)ctx;
	if( size < (int)sizeof(void*) ) return;
	void *t = *(void**)block;
	if( kind != MEM_KIND_DYNAMIC && kind != MEM_KIND_NOPTR ) return;
	int idx = hot_pmap_get(&h->aliases, t);
	if( idx >= 0 ) {
		hot_itype *it = h->types + idx;
		it->ninstances++;
		int nsize;
		bool hasptr;
		if( it->is_enum ) {
			int oi = ((venum*)block)->index;
			int ni = it->cmap[oi];
			if( ni < 0 ) {
				if( it->old_enum->constructs[oi].nparams ) {
					it->removed_live++;
					it->removed_ctor = oi;
					return;
				}
				if( h->nremoved == h->max_removed ) {
					h->max_removed = h->max_removed ? h->max_removed * 2 : 16;
					h->removed = (void**)realloc(h->removed, sizeof(void*) * h->max_removed);
					h->removed_info = (int*)realloc(h->removed_info, sizeof(int) * 2 * h->max_removed);
				}
				h->removed[h->nremoved] = block;
				h->removed_info[h->nremoved * 2] = idx;
				h->removed_info[h->nremoved * 2 + 1] = oi;
				h->nremoved++;
				return;
			}
			hl_enum_construct *c = it->t->tenum->constructs + ni;
			nsize = c->size;
			hasptr = c->hasptr;
		} else {
			if( it->fields_same ) {
				hot_add_hooked(h, block, t);
				return;
			}
			nsize = it->nrt->size;
			hasptr = it->nrt->hasPtr;
		}
		bool relocate = nsize > size || (kind != MEM_KIND_DYNAMIC && hasptr);
		if( relocate ) {
			it->nrelocated++;
			h->nrelocated++;
		} else if( size > it->max_block )
			it->max_block = size;
		if( h->nobjs == h->max_objs ) {
			h->max_objs = h->max_objs ? h->max_objs * 2 : 256;
			h->objs = (void**)realloc(h->objs, sizeof(void*) * h->max_objs);
			h->objs_info = (int*)realloc(h->objs_info, sizeof(int) * 3 * h->max_objs);
		}
		h->objs[h->nobjs] = block;
		h->objs_info[h->nobjs * 3] = idx;
		h->objs_info[h->nobjs * 3 + 1] = size;
		h->objs_info[h->nobjs * 3 + 2] = relocate;
		h->nobjs++;
		return;
	}
	hot_add_hooked(h, block, t);
	if( kind == MEM_KIND_DYNAMIC && h->nmethods && t && ((hl_type*)t)->kind == HFUN && size >= (int)sizeof(vclosure) ) {
		vclosure *c = (vclosure*)block;
		int mk = c->hasValue && c->value ? hot_pmap_get(&h->methods, c->fun) : -1;
		int vi = mk >= 0 ? hot_pmap_get(&h->aliases, *(void**)c->value) : -1;
		if( vi >= 0 && !h->types[vi].is_enum ) {
			if( h->nclosures == h->max_closures ) {
				h->max_closures = h->max_closures ? h->max_closures * 2 : 64;
				h->closures = (void**)realloc(h->closures, sizeof(void*) * h->max_closures);
				h->closures_info = (int*)realloc(h->closures_info, sizeof(int) * 2 * h->max_closures);
			}
			h->closures[h->nclosures] = block;
			h->closures_info[h->nclosures * 2] = vi;
			h->closures_info[h->nclosures * 2 + 1] = mk;
			h->nclosures++;
		}
		return;
	}
	if( kind == MEM_KIND_DYNAMIC && hot_pmap_get(&h->virtuals, t) >= 0 ) {
		if( h->nvirts == h->max_virts ) {
			h->max_virts = h->max_virts ? h->max_virts * 2 : 256;
			h->virts = (void**)realloc(h->virts, sizeof(void*) * h->max_virts);
		}
		h->virts[h->nvirts++] = block;
	}
}

static void hot_prepare_hooks( hl_module *m, hot_instances *h ) {
	int hash = hl_hash_utf8("onHotReload");
	int count = 0;
	vclosure **found = (vclosure**)malloc(sizeof(void*) * (h->nobjs + h->nhooked + 1));
	for(int k=0;k<h->nhooked;k++) {
		int fidx = h->hooked_f[k];
		hl_function *f = m->code->functions + m->functions_indexes[fidx];
		if( f->type->fun->nargs != 1 ) continue;
		found[count++] = hl_alloc_closure_ptr(f->type, m->functions_ptrs[fidx], h->hooked[k]);
	}
	for(int k=0;k<h->nobjs;k++) {
		char *o = (char*)h->objs[k];
		hot_itype *it = h->types + h->objs_info[k * 3];
		if( it->is_enum ) continue;
		hl_runtime_obj *rt = it->nrt;
		hl_field_lookup *f = NULL;
		for(hl_runtime_obj *r=rt;r && !f;r=r->parent)
			f = hl_lookup_find(r->lookup, r->nlookup, hash);
		if( f == NULL || f->t->kind != HFUN ) continue;
		if( f->field_index < 0 ) {
			if( f->t->fun->nargs != 1 ) continue;
			found[count++] = hl_alloc_closure_ptr(f->t, rt->methods[-f->field_index-1], o);
		} else if( hot_is_class_object(it->t) ) {
			vclosure *c = *(vclosure**)(o + f->field_index);
			if( c && c->t->fun->nargs == 0 ) found[count++] = c;
		}
	}
	if( count ) {
		h->hooks = hl_alloc_array(&hlt_dyn, count);
		memcpy(hl_aptr(h->hooks, void*), found, sizeof(void*) * count);
		hl_add_root(&h->hooks);
	}
	free(found);
}

static void hot_call_hooks( hot_instances *h ) {
	if( h->hooks == NULL ) return;
	int errors = 0;
	for(int i=0;i<h->hooks->size;i++) {
		bool isExc = false;
		vdynamic *ret = hl_dyn_call_safe(hl_aptr(h->hooks, vclosure*)[i], NULL, 0, &isExc);
		if( isExc && errors++ < 10 ) {
			uprintf(USTR("[HotReload] Exception in onHotReload: %s\n"), hl_to_string(ret));
			fflush(stdout);
		}
	}
	hl_remove_root(&h->hooks);
	h->hooks = NULL;
}

static void hot_instances_free( hot_instances *h ) {
	if( h->hooks ) hl_remove_root(&h->hooks);
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		free(it->copies);
		free(it->convs);
		free(it->filled);
		free(it->inits);
		free(it->added);
		free(it->bindings);
		if( it->ccopies ) {
			for(int c=0;c<it->old_enum->nconstructs;c++) free(it->ccopies[c]);
			free(it->ccopies);
		}
		free(it->nccopies);
		free(it->cmap);
	}
	free(h->types);
	hot_pmap_free(&h->aliases);
	hot_pmap_free(&h->virtuals);
	free(h->objs);
	free(h->objs_info);
	free(h->removed);
	free(h->removed_info);
	free(h->virts);
	hot_pmap_free(&h->methods);
	free(h->mnames);
	free(h->mtypes);
	free(h->closures);
	free(h->closures_info);
	free(h->vconv);
	free(h->vconv_t);
	hot_pmap_free(&h->hooktypes);
	free(h->hooked);
	free(h->hooked_f);
}

static void hot_print_layout_name( hot_report *r, hot_itype *it ) {
	if( it->is_enum )
		hot_print_name(r, it->old_enum->name);
	else
		hot_print_type_name(r, it->old_obj);
}

static void hot_enum_plan( hot_itype *it ) {
	hl_type_enum *o = it->old->tenum, *n = it->t->tenum;
	it->is_enum = true;
	it->old_enum = o;
	it->old_layout = o;
	it->cmap = (int*)malloc(sizeof(int) * (o->nconstructs + 1));
	it->ccopies = (int**)calloc(o->nconstructs + 1, sizeof(int*));
	it->nccopies = (int*)calloc(o->nconstructs + 1, sizeof(int));
	for(int i=0;i<o->nconstructs;i++) {
		hl_enum_construct *oc = o->constructs + i;
		it->cmap[i] = -1;
		for(int k=0;k<n->nconstructs;k++)
			if( ucmp(oc->name, n->constructs[k].name) == 0 ) {
				it->cmap[i] = k;
				break;
			}
		if( it->cmap[i] < 0 ) continue;
		hl_enum_construct *nc = n->constructs + it->cmap[i];
		it->ccopies[i] = (int*)malloc(sizeof(int) * 3 * (oc->nparams + 1));
		for(int p=0;p<oc->nparams;p++) {
			int np = hot_enum_param(oc, p, nc);
			if( np < 0 || !hl_module_same_type(oc->params[p], nc->params[np]) ) continue;
			int *c = it->ccopies[i] + it->nccopies[i]++ * 3;
			c[0] = nc->offsets[np];
			c[1] = oc->offsets[p];
			c[2] = hot_field_size(nc->params[np]);
		}
	}
}

static vobj *hot_enum_object( hl_type_enum *e, varray ***values ) {
	*values = NULL;
	if( !e->global_value || !*e->global_value ) return NULL;
	vobj *eobj = (vobj*)*e->global_value;
	hl_runtime_obj *rt = hl_get_obj_rt(eobj->t);
	hl_field_lookup *f = NULL;
	int hash = hl_hash_utf8("__evalues__");
	for(hl_runtime_obj *r=rt;r && !f;r=r->parent)
		f = hl_lookup_find(r->lookup, r->nlookup, hash);
	if( f && f->field_index >= 0 ) *values = (varray**)((char*)eobj + f->field_index);
	return eobj;
}

static struct {
	hot_pmap *removed;
	hot_pmap *excluded;
	int found;
} hot_refs;

static void hot_scan_refs( void *block, int size, int kind, void *ctx ) {
	if( hot_refs.found >= 0 || kind == MEM_KIND_NOPTR ) return;
	if( hot_pmap_get(hot_refs.excluded, block) >= 0 || hot_pmap_get(hot_refs.removed, block) >= 0 ) return;
	void **w = (void**)block;
	for(int i=0;i<size / HL_WSIZE;i++) {
		int k = hot_pmap_get(hot_refs.removed, w[i]);
		if( k >= 0 ) {
			hot_refs.found = k;
			return;
		}
	}
}

static void hot_check_removed( hot_instances *h, hot_report *r ) {
	hot_pmap removed, excluded;
	memset(&removed, 0, sizeof(removed));
	memset(&excluded, 0, sizeof(excluded));
	for(int k=0;k<h->nremoved;k++)
		hot_pmap_set(&removed, h->removed[k], k);
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( !it->is_enum ) continue;
		varray **values;
		vobj *eobj = hot_enum_object(it->old_enum, &values);
		if( eobj ) hot_pmap_set(&excluded, eobj, 0);
		if( values && *values ) hot_pmap_set(&excluded, *values, 0);
	}
	hot_refs.removed = &removed;
	hot_refs.excluded = &excluded;
	hot_refs.found = -1;
	hl_gc_iter_live(hot_scan_refs, NULL);
	int found = hot_refs.found;
	if( found < 0 ) {
		int *sizes = (int*)malloc(sizeof(int) * h->nremoved);
		for(int k=0;k<h->nremoved;k++) sizes[k] = (int)sizeof(venum);
		found = hl_gc_stacks_find(h->removed, sizes, h->nremoved, NULL, NULL);
		free(sizes);
	}
	if( found >= 0 ) {
		hot_itype *it = h->types + h->removed_info[found * 2];
		hot_print(r, "\n  ");
		hot_print_name(r, it->old_enum->name);
		hot_print(r, ".");
		hot_print_name(r, it->old_enum->constructs[h->removed_info[found * 2 + 1]].name);
		hot_print(r, ": the constructor is removed and a live value uses it");
		r->count++;
	}
	hot_pmap_free(&removed);
	hot_pmap_free(&excluded);
}

static void hot_detach_removed( hot_instances *h ) {
	int nroots;
	void ***roots = hl_gc_get_roots(&nroots);
	hot_pmap removed;
	memset(&removed, 0, sizeof(removed));
	for(int k=0;k<h->nremoved;k++)
		hot_pmap_set(&removed, h->removed[k], k);
	for(int i=0;i<nroots;i++)
		if( hot_pmap_get(&removed, *roots[i]) >= 0 ) *roots[i] = NULL;
	for(int k=0;k<h->nremoved;k++)
		*(void**)h->removed[k] = NULL;
	hot_pmap_free(&removed);
}

static bool hot_is_jit_code( void *addr ) {
	int count;
	hl_module **mods = hl_get_modules(&count);
	for(int i=0;i<count;i++) {
		unsigned char *code = (unsigned char*)mods[i]->jit_code;
		if( (unsigned char*)addr >= code && (unsigned char*)addr < code + mods[i]->codesize ) return true;
	}
	return false;
}

static void hot_entry_frame( void **start, void **end ) {
	void **p = (void**)hl_setup.hot_stack_limit;
	void **top = (void**)hl_get_thread()->stack_top;
	if( p == NULL ) return;
	for(;p<top;p++) {
		hl_module *m = NULL;
		hl_function *f = hot_function_at(*p, &m);
		if( f == NULL || f->findex != m->code->entrypoint || *p == m->hot_code[f - m->code->functions] ) continue;
		void **fp = (void**)p[-1];
		if( fp <= p || fp >= top ) continue;
		void **q = p + 1;
		while( q < top && !hot_is_jit_code(*q) ) q++;
		*start = p + 1;
		*end = q;
		return;
	}
}

static void hot_add_methods( hot_instances *h, hot_itype *it ) {
	hl_runtime_obj *rt = it->ort;
	if( rt->methods == NULL || hot_is_class_object(it->old) ) return;
	for(hl_runtime_obj *r=rt;r;r=r->parent)
		for(int i=0;i<r->nlookup;i++) {
			hl_field_lookup *l = r->lookup + i;
			if( l->field_index >= 0 ) continue;
			void *fun = rt->methods[-l->field_index-1];
			if( fun == NULL || hot_pmap_get(&h->methods, fun) >= 0 ) continue;
			if( h->nmethods == h->max_methods ) {
				h->max_methods = h->max_methods ? h->max_methods * 2 : 256;
				h->mnames = (int*)realloc(h->mnames, sizeof(int) * h->max_methods);
				h->mtypes = (hl_type**)realloc(h->mtypes, sizeof(hl_type*) * h->max_methods);
			}
			h->mnames[h->nmethods] = l->hashed_name;
			h->mtypes[h->nmethods] = l->t;
			hot_pmap_set(&h->methods, fun, h->nmethods++);
		}
}

static bool hot_same_method( hl_type *t1, hl_type *t2 ) {
	if( t1->kind != HFUN || t2->kind != HFUN || t1->fun->nargs != t2->fun->nargs ) return false;
	for(int i=1;i<t1->fun->nargs;i++)
		if( !hl_module_same_type(t1->fun->args[i], t2->fun->args[i]) ) return false;
	return hl_module_same_type(t1->fun->ret, t2->fun->ret);
}

static void hot_rebind_closures( hot_instances *h ) {
	for(int k=0;k<h->nclosures;k++) {
		vclosure *c = (vclosure*)h->closures[k];
		hot_itype *it = h->types + h->closures_info[k * 2];
		int mk = h->closures_info[k * 2 + 1];
		hl_field_lookup *f = NULL;
		for(hl_runtime_obj *r=it->nrt;r && !f;r=r->parent)
			f = hl_lookup_find(r->lookup, r->nlookup, h->mnames[mk]);
		if( f && f->field_index < 0 && hot_same_method(f->t, h->mtypes[mk]) )
			c->fun = it->nrt->methods[-f->field_index-1];
	}
}

typedef struct {
	int kind;
	int64 i;
	double d;
	int global;
	hl_type *vt;
} hot_rconst;

static void hot_ctor_inits( hl_module *m, hl_function *f, hot_rconst *fields, int nfields ) {
	hl_code *c = m->code;
	hot_rconst *regs = (hot_rconst*)malloc(sizeof(hot_rconst) * (f->nregs + 1));
	bool *isconst = (bool*)calloc(c->nglobals + 1, sizeof(bool));
	for(int i=0;i<c->nconstants;i++) isconst[c->constants[i].global] = true;
	for(int i=0;i<f->nregs;i++) regs[i].kind = -1;
	int cond_end = 0;
	for(int k=0;k<f->nops;k++) {
		hl_opcode *o = f->ops + k;
		hot_rconst *dst = regs + o->p1;
		int target = -1;
		switch( o->op ) {
		case OJTrue: case OJFalse: case OJNull: case OJNotNull:
			target = k + 1 + o->p2;
			break;
		case OJSLt: case OJSGte: case OJSGt: case OJSLte: case OJULt: case OJUGte: case OJNotLt: case OJNotGte: case OJEq: case OJNotEq:
			target = k + 1 + o->p3;
			break;
		case OJAlways:
			target = k + 1 + o->p1;
			break;
		default:
			break;
		}
		if( target >= 0 ) {
			if( target <= k ) break;
			if( target > cond_end ) cond_end = target;
			continue;
		}
		if( k < cond_end ) {
			switch( o->op ) {
			case ORet: case OThrow: case ORethrow:
			case OSetField: case OSetThis: case OSetGlobal: case OSetI8: case OSetI16: case OSetMem: case OSetArray: case OSetref:
			case OSetEnumField: case ONullCheck: case OAssert: case ONop: case OPrefetch: case OAsm: case ODynSet:
				break;
			case OLabel: case OSwitch: case OTrap: case OEndTrap:
				k = f->nops;
				break;
			default:
				dst->kind = -1;
				break;
			}
			continue;
		}
		switch( o->op ) {
		case OInt:
			dst->kind = HOT_INIT_NUM;
			dst->i = c->ints[o->p2];
			dst->d = (double)dst->i;
			break;
		case OFloat:
			dst->kind = HOT_INIT_NUM;
			dst->d = c->floats[o->p2];
			dst->i = (int64)dst->d;
			break;
		case OBool:
			dst->kind = HOT_INIT_NUM;
			dst->i = o->p2;
			dst->d = o->p2;
			break;
		case ONull:
			dst->kind = 3;
			break;
		case OGetGlobal:
			dst->kind = isconst[o->p2] ? HOT_INIT_GLOBAL : -1;
			dst->global = o->p2;
			break;
		case OMov:
			*dst = regs[o->p2];
			break;
		case OToDyn:
			*dst = regs[o->p2];
			dst->kind = regs[o->p2].kind == HOT_INIT_NUM ? HOT_INIT_DYN : -1;
			dst->vt = f->regs[o->p2];
			break;
		case OSetField:
			if( o->p1 == 0 && o->p2 < nfields ) fields[o->p2] = regs[o->p3];
			break;
		case OSetThis:
			if( o->p1 < nfields ) fields[o->p1] = regs[o->p2];
			break;
		case OLabel: case ORet: case OThrow: case ORethrow: case OSwitch: case OTrap: case OEndTrap:
			k = f->nops;
			break;
		case OSetGlobal: case OSetI8: case OSetI16: case OSetMem: case OSetArray: case OSetref: case OSetEnumField:
		case ONullCheck: case OAssert: case ONop: case OPrefetch: case OAsm: case ODynSet:
			break;
		default:
			dst->kind = -1;
			break;
		}
	}
	free(isconst);
	free(regs);
}

static void hot_field_inits( hl_module *m, hot_itype *it, hot_pmap *ctors ) {
	hl_code *c = m->code;
	int nfields = it->nrt->nfields;
	bool missing = false;
	for(int fi=0;fi<nfields;fi++)
		if( !it->filled[fi] ) missing = true;
	if( !missing ) return;
	hot_rconst *fields = (hot_rconst*)malloc(sizeof(hot_rconst) * nfields);
	for(int fi=0;fi<nfields;fi++) fields[fi].kind = -1;
	hl_type *chain[256];
	int depth = 0;
	for(hl_type *t=it->t;t && depth < 256;t=t->obj->super)
		chain[depth++] = t;
	while( depth > 0 ) {
		int fidx = hot_pmap_get(ctors, chain[--depth]);
		if( fidx >= 0 ) hot_ctor_inits(m, c->functions + fidx, fields, nfields);
	}
	it->inits = (hot_finit*)malloc(sizeof(hot_finit) * nfields);
	for(int fi=0;fi<nfields;fi++) {
		hot_rconst *v = fields + fi;
		if( it->filled[fi] || v->kind < 0 || v->kind == 3 ) continue;
		hl_type *ft = hl_obj_field_fetch(it->t, fi)->t;
		hot_finit *f = it->inits + it->ninits;
		f->off = it->nrt->fields_indexes[fi];
		f->kind = v->kind;
		f->t = ft;
		f->vt = v->vt;
		f->i = v->i;
		f->d = v->d;
		if( v->kind == HOT_INIT_NUM && !hot_is_num(ft) && ft->kind != HBOOL ) continue;
		if( v->kind == HOT_INIT_DYN && ft->kind != HDYN && ft->kind != HNULL ) continue;
		if( v->kind == HOT_INIT_GLOBAL ) {
			if( !hl_is_ptr(ft) ) continue;
			f->global = (void**)(m->globals_data + m->globals_indexes[v->global]);
		}
		it->ninits++;
	}
	free(fields);
}

static bool hot_same_fields( hot_itype *it ) {
	hl_runtime_obj *o = it->ort, *n = it->nrt;
	if( o->nfields != n->nfields || o->size != n->size || o->hasPtr != n->hasPtr ) return false;
	for(int fi=0;fi<n->nfields;fi++) {
		hl_obj_field *of = hl_obj_field_fetch(it->old, fi), *nf = hl_obj_field_fetch(it->t, fi);
		if( of->hashed_name != nf->hashed_name || !hl_module_same_type(of->t, nf->t) || o->fields_indexes[fi] != n->fields_indexes[fi] )
			return false;
	}
	return true;
}

static void hot_slot_names( hl_type_obj *o, int *names, int count ) {
	memset(names, 0, sizeof(int) * count);
	hl_type_obj *chain[256];
	int depth = 0;
	for(hl_type_obj *c=o;c && depth < 256;c=c->super ? c->super->obj : NULL)
		chain[depth++] = c;
	while( depth > 0 ) {
		hl_type_obj *c = chain[--depth];
		for(int i=0;i<c->nproto;i++) {
			int s = c->proto[i].pindex;
			if( s >= 0 && s < count ) names[s] = c->proto[i].hashed_name;
		}
	}
}

static bool hot_same_slots( hot_itype *it ) {
	int no = it->ort->nproto, nn = it->nrt->nproto;
	if( nn < no ) return false;
	if( no == 0 ) return true;
	int *o = (int*)malloc(sizeof(int) * no), *n = (int*)malloc(sizeof(int) * nn);
	hot_slot_names(it->old->obj, o, no);
	hot_slot_names(it->t->obj, n, nn);
	bool same = memcmp(o, n, sizeof(int) * no) == 0;
	free(o);
	free(n);
	return same;
}

static int hot_hook_findex( hl_type *t ) {
	for(;t && t->kind == HOBJ;t=t->obj->super)
		for(int i=0;i<t->obj->nproto;i++) {
			hl_obj_proto *p = t->obj->proto + i;
			if( ucmp(p->name, USTR("onHotReload")) == 0 ) return p->findex;
		}
	return -1;
}

static void hot_hook_types( hot_instances *h ) {
	int modules_count;
	hl_module **cur_modules = hl_get_modules(&modules_count);
	hl_code *c = h->code;
	for(int i=0;c && i<c->ntypes;i++) {
		hl_type *t = c->types + i;
		if( t->kind != HOBJ || hot_is_class_object(t) ) continue;
		int fidx = hot_hook_findex(t);
		hl_type *p = fidx >= 0 ? hot_find_type(h->hr, t) : NULL;
		if( p == NULL ) continue;
		for(int m=0;m<modules_count;m++) {
			hl_code *code = cur_modules[m]->code;
			for(int k=0;k<code->ntypes;k++) {
				hl_type *a = code->types + k;
				if( a->kind == HOBJ && a->obj == p->obj ) hot_pmap_set(&h->hooktypes, a, fidx);
			}
		}
		hot_pmap_set(&h->hooktypes, p, fidx);
	}
}

static void hot_instances_plan( hot_instances *h ) {
	int modules_count;
	hl_module **cur_modules = hl_get_modules(&modules_count);
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->t->kind == HENUM ) {
			hot_enum_plan(it);
			for(int m=0;m<modules_count;m++) {
				hl_code *code = cur_modules[m]->code;
				for(int k=0;k<code->ntypes;k++) {
					hl_type *a = code->types + k;
					if( a->kind == HENUM && a->tenum == it->old_enum ) hot_pmap_set(&h->aliases, a, i);
				}
			}
			hot_pmap_set(&h->aliases, it->old, i);
			continue;
		}
		it->old_obj = it->old->obj;
		it->old_layout = it->old_obj;
		it->ort = hl_get_obj_rt(it->old);
		it->nrt = hl_get_obj_rt(it->t);
		int nown = it->t->obj->nfields;
		it->added = (unsigned int*)calloc(((nown + 31) >> 5) + 1, sizeof(int));
		for(int fi=0;fi<nown;fi++)
			if( hot_find_field(it->old->obj, it->t->obj->fields + fi) < 0 )
				it->added[fi >> 5] |= 1u << (fi & 31);
		it->copies = (int*)malloc(sizeof(int) * 3 * (it->nrt->nfields + 1));
		it->convs = (hot_conv*)malloc(sizeof(hot_conv) * (it->nrt->nfields + 1));
		it->filled = (unsigned char*)calloc(it->nrt->nfields + 1, 1);
		for(int fi=0;fi<it->nrt->nfields;fi++) {
			hl_obj_field *nf = hl_obj_field_fetch(it->t, fi);
			int same = -1, conv = -1;
			for(int j=0;j<it->ort->nfields;j++) {
				hl_obj_field *of = hl_obj_field_fetch(it->old, j);
				if( of->hashed_name != nf->hashed_name ) continue;
				if( hl_module_same_type(of->t, nf->t) ) {
					same = j;
					break;
				}
				if( conv < 0 && nf->hashed_name && hot_convertible(of->t, nf->t) ) conv = j;
			}
			if( same >= 0 ) {
				int *c = it->copies + it->ncopies++ * 3;
				c[0] = it->nrt->fields_indexes[fi];
				c[1] = it->ort->fields_indexes[same];
				c[2] = hot_field_size(nf->t);
				it->filled[fi] = 1;
			} else if( conv >= 0 ) {
				hot_conv *c = it->convs + it->nconvs++;
				c->noff = it->nrt->fields_indexes[fi];
				c->ooff = it->ort->fields_indexes[conv];
				c->ot = hl_obj_field_fetch(it->old, conv)->t;
				c->nt = nf->t;
				it->filled[fi] = 1;
			}
		}
		it->fields_same = hot_same_fields(it);
		it->slots_same = hot_same_slots(it);
		hot_add_methods(h, it);
		for(int m=0;m<modules_count;m++) {
			hl_code *code = cur_modules[m]->code;
			for(int k=0;k<code->ntypes;k++) {
				hl_type *a = code->types + k;
				if( a->kind == HOBJ && a->obj == it->old_obj ) hot_pmap_set(&h->aliases, a, i);
			}
		}
		hot_pmap_set(&h->aliases, it->old, i);
	}
	for(int m=0;m<modules_count;m++) {
		hl_code *code = cur_modules[m]->code;
		for(int k=0;k<code->ntypes;k++)
			if( code->types[k].kind == HVIRTUAL ) hot_pmap_set(&h->virtuals, code->types + k, 0);
	}
}

static void hot_instances_scan( hot_instances *h, hot_report *r ) {
	hl_gc_iter_live(hot_scan_block, h);
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( !it->removed_live ) continue;
		hot_print(r, "\n  ");
		hot_print_name(r, it->old_enum->name);
		hot_print(r, ".");
		hot_print_name(r, it->old_enum->constructs[it->removed_ctor].name);
		hot_print(r, ": the constructor is removed and %d live values use it", it->removed_live);
		r->count++;
	}
	if( r->count ) return;
	if( h->nremoved ) hot_check_removed(h, r);
	if( h->nrelocated == 0 || r->count ) return;
	void **olds = (void**)malloc(sizeof(void*) * h->nrelocated);
	int *sizes = (int*)malloc(sizeof(int) * h->nrelocated);
	int *types = (int*)malloc(sizeof(int) * h->nrelocated);
	int n = 0;
	for(int k=0;k<h->nobjs;k++) {
		if( !h->objs_info[k * 3 + 2] ) continue;
		olds[n] = h->objs[k];
		sizes[n] = h->objs_info[k * 3 + 1];
		types[n] = h->objs_info[k * 3];
		n++;
	}
	void *skip_start = NULL, *skip_end = NULL;
	hot_entry_frame(&skip_start, &skip_end);
	int found = hl_gc_stacks_find(olds, sizes, n, skip_start, skip_end);
	if( found >= 0 ) {
		hot_itype *it = h->types + types[found];
		hot_print(r, "\n  ");
		hot_print_layout_name(r, it);
		hot_print(r, ": an instance that must be moved (no room for the new layout in its block) is referenced by a running function");
		r->count++;
		r->running++;
	}
	free(olds);
	free(sizes);
	free(types);
}

static void hot_set_layout( hl_type *a, hl_type *t ) {
	if( t->kind == HENUM ) {
		a->tenum = t->tenum;
		a->mark_bits = t->mark_bits;
		return;
	}
	a->obj = t->obj;
	a->vobj_proto = t->vobj_proto;
	a->mark_bits = t->mark_bits;
}

static void hot_copy_value( hot_instances *h, hot_itype *it, char *n, char *src ) {
	if( it->is_enum ) {
		int oi = ((venum*)src)->index;
		((venum*)n)->index = it->cmap[oi];
		for(int *c=it->ccopies[oi], *e=c + it->nccopies[oi] * 3;c<e;c+=3)
			memcpy(n + c[0], src + c[1], c[2]);
		return;
	}
	for(int *c=it->copies, *e=c + it->ncopies * 3;c<e;c+=3)
		memcpy(n + c[0], src + c[1], c[2]);
	for(int k=0;k<it->nconvs;k++) {
		hot_conv *c = it->convs + k;
		hot_convert(h, c->ot, src + c->ooff, c->nt, n + c->noff);
	}
	for(int k=0;k<it->ninits;k++) {
		hot_finit *f = it->inits + k;
		switch( f->kind ) {
		case HOT_INIT_NUM:
			hot_write_num(f->t, n + f->off, f->d, f->i);
			break;
		case HOT_INIT_GLOBAL:
			*(void**)(n + f->off) = *f->global;
			break;
		case HOT_INIT_DYN:
			{
				char tmp[8];
				hot_write_num(f->vt, tmp, f->d, f->i);
				*(vdynamic**)(n + f->off) = hl_make_dyn(tmp, f->vt);
			}
			break;
		}
	}
}

static void hot_enum_values( hot_itype *it ) {
	hl_type_enum *e = it->t->tenum;
	varray **field;
	hot_enum_object(e, &field);
	if( field == NULL ) return;
	varray *prev = *field;
	varray *values = hl_alloc_array(&hlt_dyn, e->nconstructs);
	for(int i=0;prev && i<prev->size && i<it->old_enum->nconstructs;i++) {
		venum *v = hl_aptr(prev, venum*)[i];
		int ni = it->cmap[i];
		if( v && ni >= 0 && e->constructs[ni].nparams == 0 ) hl_aptr(values, venum*)[ni] = v;
	}
	for(int i=0;i<e->nconstructs;i++)
		if( e->constructs[i].nparams == 0 && hl_aptr(values, venum*)[i] == NULL )
			hl_aptr(values, venum*)[i] = hl_alloc_enum(it->old, i);
	*field = values;
}

static void hot_instances_apply( hl_module *m, hot_instances *h ) {
	hot_pmap ctors;
	memset(&ctors, 0, sizeof(ctors));
	for(int i=0;i<m->code->nfunctions;i++) {
		hl_function *f = m->code->functions + i;
		if( f->obj && f->field.name && f->type->fun->nargs > 0 && ucmp(f->field.name, USTR("__constructor__")) == 0 )
			hot_pmap_set(&ctors, f->type->fun->args[0], i);
	}
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->is_enum ) continue;
		it->nrt = hl_get_obj_proto(it->t);
		if( !hot_is_class_object(it->t) ) hot_field_inits(m, it, &ctors);
		if( it->t->mark_bits && hl_mark_size(it->nrt->size) < hl_mark_size(it->max_block) ) {
			unsigned int *mark = (unsigned int*)hl_zalloc(&m->ctx.alloc, hl_mark_size(it->max_block));
			memcpy(mark, it->t->mark_bits, hl_mark_size(it->nrt->size));
			it->t->mark_bits = mark;
		}
	}
	hot_pmap_free(&ctors);
	hl_gc_enable(false);
	if( h->nremoved ) hot_detach_removed(h);
	void **news = (void**)malloc(sizeof(void*) * (h->nobjs + 1));
	for(int k=0;k<h->nobjs;k++) {
		char *o = (char*)h->objs[k];
		hot_itype *it = h->types + h->objs_info[k * 3];
		news[k] = o;
		if( !h->objs_info[k * 3 + 2] ) continue;
		int size;
		bool hasptr;
		if( it->is_enum ) {
			hl_enum_construct *c = it->t->tenum->constructs + it->cmap[((venum*)o)->index];
			size = c->size;
			hasptr = c->hasptr;
		} else {
			size = it->nrt->size;
			hasptr = it->nrt->hasPtr;
		}
		char *n = (char*)hl_gc_alloc_gen(*(hl_type**)o, size, (hasptr ? MEM_KIND_DYNAMIC : MEM_KIND_NOPTR) | MEM_ZERO);
		*(void**)n = *(void**)o;
		hot_copy_value(h, it, n, o);
		news[k] = n;
	}
	hot_time("  move: allocate and copy");
	int max_size = 0;
	for(int k=0;k<h->nobjs;k++)
		if( h->objs_info[k * 3 + 1] > max_size ) max_size = h->objs_info[k * 3 + 1];
	char *tmp = (char*)malloc(max_size + 1);
	for(int k=0;k<h->nobjs;k++) {
		if( h->objs_info[k * 3 + 2] ) continue;
		char *o = (char*)h->objs[k];
		hot_itype *it = h->types + h->objs_info[k * 3];
		int block = h->objs_info[k * 3 + 1];
		memcpy(tmp, o, block);
		memset(o + HL_WSIZE, 0, block - HL_WSIZE);
		hot_copy_value(h, it, o, tmp);
	}
	free(tmp);
	for(int k=0;k<h->aliases.size;k++) {
		hl_type *a = (hl_type*)h->aliases.keys[k];
		if( a == NULL ) continue;
		hot_set_layout(a, h->types[h->aliases.values[k]].t);
	}
	if( h->nrelocated ) {
		hl_gc_remap_reserve(h->nrelocated);
		for(int k=0;k<h->nobjs;k++) {
			if( !h->objs_info[k * 3 + 2] ) continue;
			hl_gc_remap_add(h->objs[k], news[k]);
			*(void**)h->objs[k] = NULL;
		}
		hot_time("  in place, types and remaps");
		hl_gc_remap_apply();
		hot_time("  move: remap collection");
	}
	for(int k=0;k<h->nvirts;k++) {
		vvirtual *v = (vvirtual*)h->virts[k];
		char *o = (char*)v->value;
		if( o == NULL ) continue;
		int idx = hot_pmap_get(&h->aliases, *(void**)o);
		if( idx < 0 || h->types[idx].is_enum ) continue;
		hot_itype *it = h->types + idx;
		for(int i=0;i<v->t->virt->nfields;i++) {
			hl_obj_field *vf = v->t->virt->fields + i;
			hl_field_lookup *f = NULL;
			for(hl_runtime_obj *r=it->nrt;r && !f;r=r->parent)
				f = hl_lookup_find(r->lookup, r->nlookup, vf->hashed_name);
			if( f && f->field_index < 0 ) {
				hl_type tmp;
				hl_type_fun tf;
				tmp.kind = HMETHOD;
				tmp.fun = &tf;
				tf.args = f->t->fun->args + 1;
				tf.nargs = f->t->fun->nargs - 1;
				tf.ret = f->t->fun->ret;
				hl_vfields(v)[i] = hl_safe_cast(&tmp, vf->t) ? it->nrt->methods[-f->field_index-1] : NULL;
			} else
				hl_vfields(v)[i] = f == NULL || !hl_same_type(f->t, vf->t) ? NULL : o + f->field_index;
		}
	}
	for(int k=0;k<h->nvconv;k++) {
		void *v = *h->vconv[k];
		*h->vconv[k] = v ? hl_to_virtual(h->vconv_t[k], (vdynamic*)v) : NULL;
	}
	hot_rebind_closures(h);
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->is_enum ) {
			hot_enum_values(it);
			continue;
		}
		int nstart = it->nrt->parent ? it->nrt->parent->nfields : 0;
		it->bindings = (int*)malloc(sizeof(int) * (it->nrt->nbindings + 1));
		for(int b=0;b<it->nrt->nbindings;b++) {
			int own = it->nrt->bindings[b].fid - nstart;
			if( own >= 0 && (it->added[own >> 5] & (1u << (own & 31))) ) it->bindings[it->nbindings++] = b;
		}
	}
	for(int k=0;k<h->nobjs;k++) {
		hot_itype *it = h->types + h->objs_info[k * 3];
		char *o = (char*)news[k];
		if( it->is_enum ) continue;
		for(int i=0;i<it->nbindings;i++) {
			hl_runtime_binding *b = it->nrt->bindings + it->bindings[i];
			*(void**)(o + it->nrt->fields_indexes[b->fid]) = b->closure ? hl_alloc_closure_ptr(b->closure,b->ptr,o) : b->ptr;
		}
	}
	for(int k=0;k<h->nobjs;k++)
		h->objs[k] = news[k];
	hot_prepare_hooks(m, h);
	free(news);
	hl_gc_enable(true);
	if( h->nobjs ) {
		printf("[HotReload] %d objects upgraded", h->nobjs);
		if( h->nrelocated ) printf(", %d moved", h->nrelocated);
		printf("\n");
	}
	hot_time("  virtuals, bindings, hooks");
}


static void hot_discard( hl_module *m ) {
	if( m->hash ) hl_code_hash_free(m->hash);
	if( m->jit_debug ) {
		for(int i=0;i<m->code->nfunctions;i++) {
			free(m->jit_debug[i].offsets);
			free(m->jit_debug[i].vars);
		}
		free(m->jit_debug);
	}
#	ifdef WIN64_UNWIND_TABLES
	free(m->unwind_table);
#	endif
	hl_free(&m->ctx.alloc);
	free(m->functions_extern);
	free(m->functions_indexes);
	free(m->functions_ptrs);
	free(m->ctx.functions_types);
	free(m);
}

static bool hot_retry = false;
static char *hot_last_delay = NULL;

static h_bool hot_refuse_gen( hl_module *m2, const char *msg, const char *details, bool retry ) {
	if( !details ) details = "";
	hot_retry = retry;
	if( retry ) {
		if( hot_last_delay == NULL || strcmp(hot_last_delay, details) != 0 ) {
			printf("[HotReload] Reload delayed: %s%s\n[HotReload] Tried again until the running code no longer uses them\n", msg, details);
			free(hot_last_delay);
			hot_last_delay = strdup(details);
		}
	} else {
		printf("[HotReload] Reload refused: %s%s\n[HotReload] Restart required, the program keeps running the previous code\n", msg, details);
		free(hot_last_delay);
		hot_last_delay = NULL;
	}
	fflush(stdout);
	hot_discard(m2);
	return false;
}

static h_bool hot_refuse( hl_module *m2, const char *msg, const char *details ) {
	return hot_refuse_gen(m2, msg, details, false);
}

static hl_code *hot_init_code = NULL;
static bool *hot_init_new_types = NULL;
static hot_instances *hot_init_instances = NULL;

static int hot_init_bits( hl_type *t, int word ) {
	hl_code *c = hot_init_code;
	if( c == NULL || t < c->types || t >= c->types + c->ntypes )
		return 0;
	if( hot_init_new_types[t - c->types] )
		return word < 0 ? 1 : -1;
	hot_instances *h = hot_init_instances;
	for(int i=0;h && i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->t != t || it->is_enum ) continue;
		if( word < 0 || word > (it->t->obj->nfields >> 5) ) return 0;
		return (int)it->added[word];
	}
	return 0;
}

#define HOT_SKIP		-1
#define HOT_SAME		-2
#define HOT_ADDED		-3

static bool hot_world_stopped = false;

static h_bool hot_patch( hl_module *m1, hl_code *c );

static void hot_check_types( void ) {
	static int enabled = -1;
	if( enabled < 0 ) enabled = hot_switch("HL_HOT_RELOAD_CHECK");
	if( !enabled ) return;
	int count, errors = 0, checked = 0;
	hl_module **mods = hl_get_modules(&count);
	for(int mi=0;mi<count;mi++) {
		hl_code *code = mods[mi]->code;
		for(int i=0;i<code->ntypes;i++) {
			hl_type *t = code->types + i;
			if( t->kind != HOBJ || t->obj == NULL || t->obj->rt == NULL ) continue;
			hl_runtime_obj *rt = t->obj->rt;
			int depth = 0;
			checked++;
			while( rt && depth < 64 ) {
				hl_type_obj *o = rt->t ? rt->t->obj : NULL;
				const char *why = NULL;
				if( o == NULL )
					why = "its runtime object names a type with no object data";
				else if( o->rt != rt )
					why = "its runtime object is not the one of the type it names (stale runtime data)";
				else if( rt->t->vobj_proto ) {
					for(int k=0;k<rt->ninterfaces;k++)
						if( rt->interfaces[k] < 0 || rt->interfaces[k] >= o->nfields ) {
							why = "an interface index is out of the fields of the type";
							break;
						}
				}
				if( why && ++errors <= 30 ) {
					char tname[256], oname[256];
					snprintf(tname, sizeof(tname), "%s", hl_to_utf8(t->obj->name));
					snprintf(oname, sizeof(oname), "%s", o ? hl_to_utf8(o->name) : "?");
					int tm = -1, ti = -1, om = -1, oi = -1;
					for(int k=0;k<count;k++) {
						hl_code *ck = mods[k]->code;
						if( rt->t >= ck->types && rt->t < ck->types + ck->ntypes ) { tm = k; ti = (int)(rt->t - ck->types); }
						for(int j=0;j<ck->ntypes && om < 0;j++)
							if( (ck->types[j].kind == HOBJ || ck->types[j].kind == HSTRUCT) && ck->types[j].obj && ck->types[j].obj->rt == rt ) { om = k; oi = j; }
					}
					printf("[HotReload] CHECK %s (module %d) : superclass level %d, %s : %s (named type: module %d #%d, runtime object of: module %d #%d)\n", tname, mi, depth, oname, why, tm, ti, om, oi);
				}
				rt = rt->parent;
				depth++;
			}
		}
	}
	printf("[HotReload] CHECK %d types, %d errors\n", checked, errors);
	fflush(stdout);
}

h_bool hl_module_patch( hl_module *m1, hl_code *c ) {
	hl_gc_world(true);
	hot_world_stopped = true;
	h_bool r = hot_patch(m1, c);
	if( hot_world_stopped ) {
		hot_world_stopped = false;
		hl_gc_world(false);
	}
	return r;
}

static h_bool hot_patch( hl_module *m1, hl_code *c ) {
	int i;
	hot_time(NULL);
	hot_nblockers = 0;
	hl_hot_reload *hr = m1->hot;
	if( hr == NULL ) return false;

	hl_module *m2 = hl_module_alloc(c);
	if( m2 == NULL ) return false;
	m2->hash = hl_code_hash_alloc(c);

	{
		int count = c->nglobals;
		int *remap = (int*)malloc(sizeof(int) * count);
		bool *used = (bool*)malloc(sizeof(bool) * (hr->nglobals + 1));
		memset(used,0,sizeof(bool) * (hr->nglobals + 1));
		for(i=0;i<count;i++) {
			int sign = m2->hash->globals_signs[i];
			int cursor = -1, g, first = -1;
			remap[i] = -1;
			while( (g = hot_map_next(&hr->globals_map, sign, &cursor)) >= 0 ) {
				if( first < 0 || g < first ) first = g;
				if( !used[g] && (remap[i] < 0 || g < remap[i]) ) remap[i] = g;
			}
			if( remap[i] < 0 ) remap[i] = first;
			if( remap[i] >= 0 ) used[remap[i]] = true;
		}
		free(used);
		hl_code_hash_remap_globals(m2->hash, remap, hr->nglobals, hr->globals_types, hr->globals_signs);
		free(remap);
	}
	if( c->nglobals > hr->max_globals ) {
		char tmp[128];
		sprintf(tmp, " (%d needed, %d available, see HL_HOT_RELOAD_GLOBALS)", c->nglobals, hr->max_globals);
		return hot_refuse(m2, "no room left for the new globals", tmp);
	}

	free(m2->globals_data);
	free(m2->globals_indexes);
	m2->globals_data = m1->globals_data;
	m2->globals_indexes = m1->globals_indexes;
	int gsize = hr->globals_size;
	for(i=hr->nglobals;i<c->nglobals;i++) {
		hl_type *t = c->globals[i];
		gsize += hl_pad_size(gsize, t);
		m2->globals_indexes[i] = gsize;
		gsize += hl_type_size(t);
	}
	m2->globals_size = gsize;
	if( gsize > hr->max_globals_size ) {
		char tmp[128];
		sprintf(tmp, " (%d bytes needed, %d available, see HL_HOT_RELOAD_GLOBALS)", gsize, hr->max_globals_size);
		return hot_refuse(m2, "no room left for the new globals", tmp);
	}

	hot_instances inst;
	memset(&inst, 0, sizeof(inst));
	inst.code = c;
	inst.hr = hr;
	inst.types = (hot_itype*)malloc(sizeof(hot_itype) * (c->ntypes + 1));
	int *itype_of = (int*)malloc(sizeof(int) * (c->ntypes + 1));
	for(i=0;i<c->ntypes;i++) itype_of[i] = -1;
	{
		char buf[4096];
		hot_report r = { buf, sizeof(buf), 0, 0 };
		buf[0] = 0;
		for(i=0;i<c->ntypes;i++) {
			hl_type *t = c->types + i;
			hl_type *p = hot_find_type(hr, t);
			int k = p ? hot_check_layout(p, t, &r) : HOT_LAYOUT_SAME;
			if( k == HOT_LAYOUT_INSTANCES ) {
				hot_itype *it = inst.types + inst.ntypes;
				memset(it, 0, sizeof(hot_itype));
				it->old = p;
				it->t = t;
				it->index = i;
				itype_of[i] = inst.ntypes++;
			}
		}
		for(i=0;i<c->ntypes;i++) {
			hl_type *t = c->types + i;
			if( t->kind != HOBJ || itype_of[i] >= 0 ) continue;
			hl_type *p = hot_find_type(hr, t);
			if( p == NULL ) continue;
			hl_type *sup = t->obj->super;
			while( sup && !(sup >= c->types && sup < c->types + c->ntypes && itype_of[sup - c->types] >= 0) )
				sup = sup->obj->super;
			if( sup == NULL ) continue;
			hot_itype *it = inst.types + inst.ntypes;
			memset(it, 0, sizeof(hot_itype));
			it->old = p;
			it->t = t;
			it->index = i;
			itype_of[i] = inst.ntypes++;
		}
		if( r.count ) {
			free(itype_of);
			hot_instances_free(&inst);
			return hot_refuse(m2, "the layout of a struct changed:", buf);
		}
	}

	hl_module_init_natives(m2);
	hl_module_init_indexes(m2);
	hl_code_hash_finalize(m2->hash);

	if( inst.ntypes ) hot_hook_types(&inst);

	bool *new_types = (bool*)malloc(sizeof(bool) * (c->ntypes + 1));
	bool *upgraded = (bool*)malloc(sizeof(bool) * (c->ntypes + 1));
	memset(upgraded, 0, sizeof(bool) * (c->ntypes + 1));
	for(i=0;i<inst.ntypes;i++)
		upgraded[inst.types[i].index] = true;
	for(i=0;i<c->ntypes;i++) {
		hl_type *t = c->types + i;
		hl_type *p = hot_find_type(hr, t);
		new_types[i] = p == NULL && hot_type_name(t) != NULL;
		if( p == NULL || upgraded[i] ) continue;
		switch( t->kind ) {
		case HOBJ:
		case HSTRUCT:
			if( hl_get_obj_rt(p)->compareFun ) hl_get_obj_proto(p);
			t->obj = p->obj;
			t->vobj_proto = p->vobj_proto;
			break;
		case HENUM:
			t->tenum = p->tenum;
			break;
		default:
			break;
		}
	}
	for(i=0;i<c->ntypes;i++) {
		hl_type *t = c->types + i;
		if( (t->kind != HOBJ && t->kind != HSTRUCT) || t->obj == NULL || t->obj->rt == NULL || t->obj->super == NULL ) continue;
		if( !new_types[i] && !upgraded[i] ) continue;
		t->obj->rt->parent = hl_get_obj_rt(t->obj->super);
	}
	free(upgraded);

	{
		char buf[4096];
		hot_report r = { buf, sizeof(buf), 0, 0 };
		buf[0] = 0;
		hot_time("compare");
		if( inst.ntypes ) hot_instances_plan(&inst);
		hot_check_stack(inst.types, inst.ntypes, &r);
		if( r.count == 0 && inst.ntypes ) {
			hot_instances_scan(&inst, &r);
			hot_time("instances check");
		}
		if( r.count ) {
			free(itype_of);
			free(new_types);
			hot_instances_free(&inst);
			return hot_refuse_gen(m2, "live values cannot be upgraded:", buf, r.count == r.running);
		}
	}

	int nfuns = c->nfunctions;
	int *match = (int*)malloc(sizeof(int) * nfuns);
	bool *taken = (bool*)malloc(sizeof(bool) * (hr->nfunctions + 1));
	memset(taken,0,sizeof(bool) * (hr->nfunctions + 1));
	m2->functions_extern = (void**)malloc(sizeof(void*) * (nfuns + c->nnatives));
	memset(m2->functions_extern,0,sizeof(void*) * (nfuns + c->nnatives));
	int changes_count = 0;
	for(i=0;i<nfuns;i++) {
		hl_function *f2 = c->functions + i;
		match[i] = HOT_SKIP;
		if( fun_obj(f2) == NULL ) continue;
		int sign = m2->hash->functions_signs[i];
		int cursor = -1, k;
		match[i] = HOT_ADDED;
		while( (k = hot_map_next(&hr->functions_map, sign, &cursor)) >= 0 ) {
			hot_function *hf = hr->functions + k;
			hl_function *f1 = hf->f;
			if( taken[k] ) continue;
			if( (f1->obj != NULL) != (f2->obj != NULL) || ucmp(fun_obj(f1)->name,fun_obj(f2)->name) != 0 || ucmp(fun_field_name(f1),fun_field_name(f2)) != 0 ) {
				printf("[HotReload] Signature conflict\n");
				continue;
			}
			taken[k] = true;
			if( hf->hash == m2->hash->functions_hashes[i] ) {
				match[i] = HOT_SAME;
				m2->functions_extern[f2->findex] = *hf->slot;
			} else
				match[i] = k;
			break;
		}
		if( match[i] != HOT_SAME ) {
#			ifdef HL_DEBUG
			uprintf(USTR("%s."), fun_obj(f2)->name);
			uprintf(USTR("%s"), fun_field_name(f2));
			if( !f2->obj && hl_function_lname(c, f2) ) uprintf(USTR("~%s"), hl_function_lname(c, f2)); else if( !f2->obj ) printf("~%d", f2->ref);
			printf(match[i] == HOT_ADDED ? " has been added\n" : " has been modified [%d]\n", f2->nops);
#			endif
			changes_count++;
		}
	}
	free(taken);
	if( changes_count == 0 ) {
		free(match);
		free(new_types);
		hot_instances_free(&inst);
		free(itype_of);
		printf("[HotReload] No changes found\n");
		fflush(stdout);
		hot_discard(m2);
		return false;
	}


	jit_ctx *ctx = hl_jit_alloc();
	if( ctx == NULL ) {
		free(match);
		free(new_types);
		hot_instances_free(&inst);
		free(itype_of);
		return hot_refuse(m2, "out of memory", NULL);
	}
#	ifdef WIN64_UNWIND_TABLES
	m2->unwind_table_size = nfuns + 10;
	m2->unwind_table = malloc(sizeof(RUNTIME_FUNCTION) * m2->unwind_table_size);
	memset(m2->unwind_table, 0, sizeof(RUNTIME_FUNCTION) * m2->unwind_table_size);
#	endif
	m2->debug = m1->debug;
	hl_jit_init(ctx, m2);
	if( m2->jit_debug ) {
		for(i=0;i<nfuns;i++)
			m2->jit_debug[i].start = -1;
	}
	for(i=0;i<nfuns;i++) {
		if( match[i] == HOT_SAME ) continue;
		hl_function *f2 = c->functions + i;
		int fpos = hl_jit_function(ctx, m2, f2);
		if( fpos < 0 ) {
			hl_jit_free(ctx, false);
			free(match);
			free(new_types);
			hot_instances_free(&inst);
			free(itype_of);
			return hot_refuse(m2, "could not JIT the new code", NULL);
		}
		m2->functions_ptrs[f2->findex] = (void*)(int_val)fpos;
	}
	m2->jit_code = hl_jit_code(ctx, m2, &m2->codesize, &m2->jit_debug, m1);
	hot_time("jit");
	hl_jit_free(ctx, false);
	if( m2->jit_code == NULL ) {
		free(match);
		free(new_types);
		hot_instances_free(&inst);
		free(itype_of);
		return hot_refuse(m2, "could not JIT the new code", NULL);
	}

	if( m2->jit_debug ) {
		int start = -1;
		for(i=0;i<nfuns;i++)
			if( m2->jit_debug[i].start >= 0 ) {
				start = m2->jit_debug[i].start;
				break;
			}
		for(i=0;i<nfuns;i++) {
			if( m2->jit_debug[i].start < 0 ) {
				m2->jit_debug[i].start = start;
				m2->jit_debug[i].offsets = NULL;
			} else
				start = m2->jit_debug[i].start;
		}
	}


#	ifdef WIN64_UNWIND_TABLES
	RtlAddFunctionTable(m2->unwind_table, m2->unwind_table_size, (DWORD64)m2->jit_code);
#	endif

	memset(m2->globals_data + hr->globals_size, 0, gsize - hr->globals_size);
	for(i=hr->nglobals;i<c->nglobals;i++) {
		hl_type *t = c->globals[i];
		void *addr = m2->globals_data + m2->globals_indexes[i];
		if( t->kind == HFUN ) *(void**)addr = hl_module_null_function;
		if( hl_is_ptr(t) ) hl_add_root(addr);
		hr->globals_signs[i] = m2->hash->globals_signs[i];
		hr->globals_types[i] = t;
		hot_map_add(&hr->globals_map, hr->globals_signs[i], i);
	}
	int old_nglobals = hr->nglobals;
	hr->nglobals = c->nglobals;
	hr->globals_size = gsize;
	for(i=0;i<c->nconstants;i++) {
		hl_constant *k = c->constants + i;
		if( k->global >= old_nglobals )
			hl_module_init_constant(m2, k);
	}

	for(i=0;i<nfuns;i++) {
		hl_function *f2 = c->functions + i;
		if( match[i] == HOT_SAME ) {
			m2->functions_ptrs[f2->findex] = m2->functions_extern[f2->findex];
			continue;
		}
		void *ptr = ((unsigned char*)m2->jit_code) + ((int_val)m2->functions_ptrs[f2->findex]);
		m2->functions_ptrs[f2->findex] = ptr;
		if( match[i] == HOT_SKIP ) continue;
		int hash = m2->hash->functions_hashes[i];
		int sign = m2->hash->functions_signs[i];
		if( match[i] == HOT_ADDED ) {
			hot_add_function(hr, sign, hash, f2, m2->functions_ptrs + f2->findex);
			continue;
		}
		hot_function *hf = hr->functions + match[i];
		hl_jit_patch_method(*hf->slot, hf->slot);
		*hf->slot = ptr;
		hf->hash = hash;
		hf->f = f2;
	}
	free(match);

	hot_record_uses(m2);

	if( inst.ntypes ) {
		hot_instances_apply(m2, &inst);
		hot_time("instances upgrade");
	}

	for(i=0;i<c->ntypes;i++)
		hot_add_type(hr, c->types + i);
	for(i=0;i<hr->ntypes;i++) {
		hl_type *t = hr->types[i].t;
		if( t->kind == HOBJ || t->kind == HSTRUCT ) hl_flush_proto(t);
	}
	hr->reloads++;

	free(hot_last_delay);
	hot_last_delay = NULL;
	hot_retry = false;
	printf("[HotReload] %d changes\n", changes_count);
	fflush(stdout);
	hl_module_add(m2);
	hot_check_types();

	{
		vclosure cl;
		bool isExc = false;
		cl.t = c->functions[m2->functions_indexes[c->entrypoint]].type;
		cl.fun = m2->functions_ptrs[c->entrypoint];
		cl.hasValue = 0;
		hot_init_code = c;
		hot_init_new_types = new_types;
		hot_init_instances = &inst;
		vdynamic *exc = hl_dyn_call_safe(&cl,NULL,0,&isExc);
		hot_init_code = NULL;
		hot_init_new_types = NULL;
		hot_init_instances = NULL;
		if( isExc ) {
			uprintf(USTR("[HotReload] Exception while initializing the new statics: %s\n"), hl_to_string(exc));
			fflush(stdout);
		}
	}
	free(new_types);
	hot_call_hooks(&inst);
	hot_instances_free(&inst);
	free(itype_of);
	hot_time("entry point");
	return true;
}


void hl_hot_reload_prepare( hl_module *m ) {
	int extra_globals = hot_reload_extra_globals();
	int nsize = m->globals_size + extra_globals * sizeof(void*);
	int *nindexes = malloc(sizeof(int) * (m->code->nglobals + extra_globals));
	memcpy(nindexes,m->globals_indexes,sizeof(int)*m->code->nglobals);
	memset(nindexes + m->code->nglobals,0xFF,extra_globals * sizeof(int));
	free(m->globals_indexes);
	free(m->globals_data);
	m->globals_indexes = nindexes;
	m->globals_data = malloc(nsize);
	memset(m->globals_data,0,m->globals_size);
	memset(m->globals_data + m->globals_size,0xFF,extra_globals * sizeof(void*));
}

void hl_hot_reload_init( hl_module *m ) {
	int extra_globals = hot_reload_extra_globals();
	hot_record_uses(m);
	hl_setup.hot_init_bits = hot_init_bits;
	hl_code_hash_finalize(m->hash);
	hot_reload_init(m, m->code->nglobals + extra_globals, m->globals_size + extra_globals * (int)sizeof(void*));
}



#ifdef HL_WIN
typedef uchar pchar;
#	define pfopen(file,ext) _wfopen(file,USTR(ext))
#else
#	include <sys/stat.h>
typedef char pchar;
#	define pfopen(file,ext) fopen(file,ext)
#endif

static struct {
	int64 time;
	int64 size;
	int64 new_time;
	int64 new_size;
	bool has_build_id;
	unsigned char build_id[HL_BUILD_ID_SIZE];
	double last_try;
} hot_file = {0};

static int64 hot_file_time( const pchar *file, int64 *size ) {
#ifdef HL_WIN
	WIN32_FILE_ATTRIBUTE_DATA data;
	if( !GetFileAttributesExW(file,GetFileExInfoStandard,&data) ) {
		*size = -1;
		return -1;
	}
	*size = ((int64)data.nFileSizeHigh << 32) | data.nFileSizeLow;
	return ((int64)data.ftLastWriteTime.dwHighDateTime << 32) | data.ftLastWriteTime.dwLowDateTime;
#else
	struct stat st;
	if( stat(file,&st) != 0 ) {
		*size = -1;
		return -1;
	}
	*size = (int64)st.st_size;
#	if defined(__APPLE__)
	return (int64)st.st_mtimespec.tv_sec * 1000000000 + st.st_mtimespec.tv_nsec;
#	else
	return (int64)st.st_mtim.tv_sec * 1000000000 + st.st_mtim.tv_nsec;
#	endif
#endif
}

static double hot_retry_delay() {
	static double delay = -1;
	if( delay < 0 ) {
		const char *v = getenv("HL_HOT_RELOAD_RETRY");
		delay = v ? atof(v) : 0.5;
		if( delay < 0 ) delay = 0;
	}
	return delay;
}

static void hot_set_build_id( hl_code *code ) {
	hot_file.has_build_id = code->version >= 7;
	if( hot_file.has_build_id ) memcpy(hot_file.build_id, code->build_id, HL_BUILD_ID_SIZE);
}

static bool hot_check_header( const pchar *file, int64 size, bool *same ) {
	unsigned char head[HL_HEADER_SIZE], id[HL_BUILD_ID_SIZE];
	int file_size;
	*same = false;
	FILE *f = pfopen(file,"rb");
	if( f == NULL ) return false;
	int n = (int)fread(head, 1, HL_HEADER_SIZE, f);
	fclose(f);
	int version = hl_code_read_header(head, n, &file_size, id);
	if( version < 0 ) return false;
	if( version < 7 ) return true;
	if( file_size != size ) return false;
	*same = hot_file.has_build_id && memcmp(id, hot_file.build_id, HL_BUILD_ID_SIZE) == 0;
	return true;
}

void hl_hot_reload_file_init( const void *file, hl_code *code ) {
	hot_file.time = hot_file_time((const pchar*)file, &hot_file.size);
	hot_set_build_id(code);
}

bool hl_hot_reload_file_changed( const void *file ) {
	int64 size;
	int64 time = hot_file_time((const pchar*)file, &size);
	if( time == hot_file.time && size == hot_file.size ) {
		if( !hot_retry || hl_sys_time() - hot_file.last_try < hot_retry_delay() )
			return false;
		if( hot_still_blocked() ) {
			hot_file.last_try = hl_sys_time();
			return false;
		}
	} else {
		bool same;
		if( !hot_check_header((const pchar*)file, size, &same) )
			return false;
		if( same ) {
			hot_file.time = time;
			hot_file.size = size;
			return false;
		}
	}
	hot_file.last_try = hl_sys_time();
	hot_retry = false;
	hot_file.new_time = time;
	hot_file.new_size = size;
	return true;
}

void hl_hot_reload_file_loaded( hl_code *code ) {
	hot_file.time = hot_file.new_time;
	hot_file.size = hot_file.new_size;
	hot_set_build_id(code);
}
