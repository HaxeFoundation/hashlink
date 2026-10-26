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

/*
	Hot reload : the code of a new version of the program is compared with the running one, and
	the functions that changed are compiled and patched, the live values (class objects, instances)
	are upgraded when the layout of their type changed. See docs/hot-reload.md.
*/

#define HOT_RELOAD_EXTRA_GLOBALS	4096

static int hot_init_bits( hl_type *t, int word );

// HL_HOT_RELOAD_TIMES prints the time of the phases of a reload
HL_API double hl_sys_time();
static double hot_time_last = 0.;
static void hot_time( const char *phase ) {
	static int enabled = -1;
	if( enabled < 0 ) enabled = getenv("HL_HOT_RELOAD_TIMES") != NULL;
	if( !enabled ) return;
	double t = hl_sys_time();
	if( phase ) printf("[HotReload] %s %.1f ms\n", phase, (t - hot_time_last) * 1000.);
	hot_time_last = t;
}
static void hot_record_uses( hl_module *m );

/*
	The reference state is created once, when the first module is loaded, and updated after
	each reload that succeeds : the new code is always compared with the latest version of every
	function, global and type, wherever it was loaded from.
*/

typedef struct {
	int size;	// power of two
	int count;
	int *keys;
	int *values;	// -1 : empty
} hot_map;

typedef struct {
	int sign;		// identifies the function across reloads
	int hash;		// hash of its current body
	hl_function *f;	// its current version
	void **slot;	// where its current address is (functions_ptrs of the module that added it)
} hot_function;

typedef struct {
	hl_type *t;		// the type that live values have
	int hash;		// hash of its name
} hot_type;

struct hl_hot_reload {
	hl_module *root;
	int nfunctions;
	int max_functions;
	hot_function *functions;
	hot_map functions_map;	// sign -> index in functions
	int nglobals;
	int max_globals;
	int globals_size;
	int max_globals_size;
	int *globals_signs;
	hl_type **globals_types;
	hot_map globals_map;	// sign -> global index
	int ntypes;
	int max_types;
	hot_type *types;
	hot_map types_map;	// name hash -> index in types
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

// several values can have the same key
static void hot_map_add( hot_map *m, int key, int value ) {
	if( (m->count + 1) * 2 > m->size ) hot_map_grow(m);
	unsigned int i = hot_map_hash(key) & (m->size - 1);
	while( m->values[i] >= 0 ) i = (i + 1) & (m->size - 1);
	m->keys[i] = key;
	m->values[i] = value;
	m->count++;
}

// iterates over the values of a key, *cursor starting at -1 ; returns -1 at the end
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
			return; // keep the first one
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
		if( f->field.name == NULL ) continue;
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

/* ---- layout checks ---- */

typedef struct {
	char *buf;
	int size;
	int pos;
	int count;
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
	// fields of o1 that are not in o2 with the same type
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

// the type of a class object (holding the statics of a class) : named $Name
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

/*
	The parameter of the constructor nc that keeps the parameter p of oc : the one of the same name when
	the bytecode has the names (from version 7), else the one at the same position. -1 if none.
*/
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

/*
	Checks that a type of the new code has the same layout as the live one.
	Until live values can be upgraded, any change refuses the reload.
*/
#define HOT_LAYOUT_SAME		0
#define HOT_LAYOUT_CHANGED	1	// refused
#define HOT_LAYOUT_INSTANCES	2	// object type whose fields changed (statics for a class object) : its live values are upgraded

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
	// class object $Name : print the class name
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
			bool class_object = hot_is_class_object(told);
			if( !same && told->kind == HOBJ && ucmp(hot_super_name(o1), hot_super_name(o2)) == 0 && hot_same_virtuals(o1, o2) )
				return HOT_LAYOUT_INSTANCES;
			if( !same ) {
				hot_print(r, "\n  ");
				hot_print_type_name(r, o1);
				hot_print(r, class_object ? ": statics changed" : ": fields changed");
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
			if( !hot_same_virtuals(o1, o2) ) {
				hot_print(r, "\n  ");
				hot_print_type_name(r, o1);
				hot_print(r, ": overridden methods changed");
				r->count++;
				return HOT_LAYOUT_CHANGED;
			}
			return HOT_LAYOUT_SAME;
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
			// live values are remapped by the name of their constructor (U7)
			return same ? HOT_LAYOUT_SAME : HOT_LAYOUT_INSTANCES;
		}
	default:
		return HOT_LAYOUT_SAME;
	}
}

/* ---- live values ---- */

static int hot_field_size( hl_type *t ) {
	return t->kind == HPACKED ? hl_get_obj_rt(t->tparam)->size : hl_type_size(t);
}

static int hot_find_field( hl_type_obj *o, hl_obj_field *f ) {
	for(int i=0;i<o->nfields;i++)
		if( o->fields[i].hashed_name == f->hashed_name && hl_module_same_type(o->fields[i].t, f->t) )
			return i;
	return -1;
}

/*
	Records the object types each function uses, to check the functions running on the stack
	when a reload changes the layout of a type (the registers of the functions are freed after JIT).
*/
static void hot_record_uses( hl_module *m ) {
	hl_code *c = m->code;
	hl_type_obj *tmp[64];
	m->hot_uses = (hl_type_obj***)hl_malloc(&m->ctx.alloc, sizeof(void*) * c->nfunctions);
	// functions_ptrs follows the reloads : keep where the code of this module is
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
			// the layout of the type : obj, or tenum for an enum
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

// the function whose code holds addr, if it is JIT code
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
			// only the functions this module compiled
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


/* ---- instances upgrade (U4), class objects included (U1) ---- */

// pointer -> int, open addressing
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
	hl_type *old;			// the live type : it keeps its pointer and gets the new layout
	hl_type *t;				// the type of the new code
	int index;				// in the types of the new code
	hl_type_obj *old_obj;	// the previous layout
	hl_runtime_obj *ort;
	hl_runtime_obj *nrt;
	int *copies;			// the fields kept : new offset, previous offset, size
	int ncopies;
	unsigned int *added;	// bits of the own fields of t that are new (statics to initialize for a class object)
	int *bindings;			// the bindings (methods) of the fields added
	int nbindings;
	int ninstances;
	int nrelocated;
	int max_block;
	// enums (U7)
	bool is_enum;
	void *old_layout;		// the previous obj or tenum : identifies the previous layout
	hl_type_enum *old_enum;
	int *cmap;				// for each previous constructor, the new index or -1
	int **ccopies;			// for each previous constructor, the params kept : new offset, previous offset, size
	int *nccopies;
	int removed_live;		// live values of removed constructors (with params)
	int removed_ctor;		// one of these constructors
} hot_itype;

typedef struct {
	hot_itype *types;
	int ntypes;
	hot_pmap aliases;		// every hl_type* of a previous layout -> index in types
	hot_pmap virtuals;		// every virtual type -> 0
	void **objs;
	int *objs_info;			// type index, block size, relocated
	int nobjs;
	int max_objs;
	void **virts;
	int nvirts;
	int max_virts;
	varray *hooks;			// the closures of onHotReload to call after the entry point (rooted)
	void **removed;			// enum constants (no params) of removed constructors
	int *removed_info;		// their type index, previous constructor
	int nremoved;
	int max_removed;
	int nrelocated;
} hot_instances;

// reports a running function f which uses one of the layouts
static void hot_print_layout_name( hot_report *r, hot_itype *it );

static void hot_check_function( hl_module *m, hl_function *f, hot_itype *types, int count, const char *where, hot_report *r ) {
	hl_type_obj **fuses = m->hot_uses[f - m->code->functions];
	for(int k=0;k<count;k++) {
		bool uses = false;
		for(int j=0;fuses[j] && !uses;j++)
			uses = (void*)fuses[j] == types[k].old_layout;
		if( !uses ) continue;
		hot_print(r, "\n  ");
		hot_print_layout_name(r, types + k);
		hot_print(r, ": its layout changes and it is used by ");
		if( fun_obj(f) ) {
			hot_print_type_name(r, fun_obj(f));
			hot_print(r, ".");
			hot_print_name(r, fun_field_name(f));
		} else
			hot_print(r, "fun$%d", f->findex);
		hot_print(r, ", which is running%s : move the code that uses it out of it", where);
		r->count++;
	}
}

/*
	The functions running on the stacks keep running their code after the reload : refuse it if one of
	them uses a type whose layout changes (it would use the previous offsets). The stack of the current
	thread is walked; the other threads are stopped, their stacks are scanned for return addresses
	into JIT functions.
*/
static void hot_check_stack( hot_itype *types, int count, hot_report *r ) {
	void *stack[512];
	if( count == 0 ) return;
	int n = hl_module_capture_stack(stack, 512);
	for(int i=0;i<n;i++) {
		hl_module *m = NULL;
		hl_function *f = hot_function_at(stack[i], &m);
		if( f == NULL || m->hot_uses == NULL ) continue;
		// the entry point initializes the class objects before it calls main, and only returns after
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
				// a constant : the enum object and a global reference it, the reload checks the others
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
			nsize = it->nrt->size;
			hasptr = it->nrt->hasPtr;
		}
		// moved when its block has no room for the new layout
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
	if( kind == MEM_KIND_DYNAMIC && hot_pmap_get(&h->virtuals, t) >= 0 ) {
		if( h->nvirts == h->max_virts ) {
			h->max_virts = h->max_virts ? h->max_virts * 2 : 256;
			h->virts = (void**)realloc(h->virts, sizeof(void*) * h->max_virts);
		}
		h->virts[h->nvirts++] = block;
	}
}

/*
	U6 : after an upgrade, the program can initialize the new fields with real values. The method
	onHotReload() of an upgraded object is called once, and the static method onHotReload() of a
	class whose statics changed. The closures are built while the objects are known to be alive,
	and called after the entry point.
*/
static void hot_prepare_hooks( hot_instances *h ) {
	int hash = hl_hash_utf8("onHotReload");
	int count = 0;
	vclosure **found = (vclosure**)malloc(sizeof(void*) * (h->nobjs + 1));
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
			// a method of the instance, without argument
			if( f->t->fun->nargs != 1 ) continue;
			found[count++] = hl_alloc_closure_ptr(f->t, rt->methods[-f->field_index-1], o);
		} else if( hot_is_class_object(it->t) ) {
			// a static method : a field of the class object
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
}

static void hot_print_layout_name( hot_report *r, hot_itype *it ) {
	if( it->is_enum )
		hot_print_name(r, it->old_enum->name);
	else
		hot_print_type_name(r, it->old_obj);
}

/*
	Enums : the live values are remapped by the name of their constructor; the params of a
	constructor are kept by position when their type is the same (they have no name).
*/
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

// the enum object of an enum (holding __evalues__), and the array of its constants
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

/*
	The constant of a removed constructor (without params) is referenced by the enum object and a
	global : the reload is refused only when something else references it (the heap, a stack).
*/
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

// the constants of removed constructors are detached : no global keeps them, nothing scans them
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

/*
	The frame of the entry point is on the stack while the program runs (it calls main, or the
	event loop) : it holds the class objects it initialized, but it only returns after, so these
	values are dead. Finds it in the stack of the current thread, above the call to checkReload :
	from the return address into the entry point (pushed by the call it made, with the frame pointer
	of the entry point just under it) to the next return address into JIT code (its caller).
*/
static void hot_entry_frame( void **start, void **end ) {
	void **p = (void**)hl_setup.hot_stack_limit;
	void **top = (void**)hl_get_thread()->stack_top;
	if( p == NULL ) return;
	for(;p<top;p++) {
		hl_module *m = NULL;
		hl_function *f = hot_function_at(*p, &m);
		if( f == NULL || f->findex != m->code->entrypoint || *p == m->hot_code[f - m->code->functions] ) continue;
		// the saved frame pointer of the entry point : inside the stack, above
		void **fp = (void**)p[-1];
		if( fp <= p || fp >= top ) continue;
		void **q = p + 1;
		while( q < top && !hot_is_jit_code(*q) ) q++;
		*start = p + 1;
		*end = q;
		return;
	}
}

/*
	Finds the live instances of the types whose layout changes and checks that each fits in its block.
	Called before anything is changed : a failure refuses the reload.
*/
static void hot_instances_check( hot_instances *h, hot_report *r ) {
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
		// the fields kept : same name and type
		int nown = it->t->obj->nfields;
		it->added = (unsigned int*)calloc(((nown + 31) >> 5) + 1, sizeof(int));
		for(int fi=0;fi<nown;fi++)
			if( hot_find_field(it->old->obj, it->t->obj->fields + fi) < 0 )
				it->added[fi >> 5] |= 1u << (fi & 31);
		it->copies = (int*)malloc(sizeof(int) * 3 * (it->nrt->nfields + 1));
		for(int fi=0;fi<it->nrt->nfields;fi++) {
			hl_obj_field *nf = hl_obj_field_fetch(it->t, fi);
			for(int j=0;j<it->ort->nfields;j++) {
				hl_obj_field *of = hl_obj_field_fetch(it->old, j);
				if( of->hashed_name == nf->hashed_name && hl_module_same_type(of->t, nf->t) ) {
					int *c = it->copies + it->ncopies++ * 3;
					c[0] = it->nrt->fields_indexes[fi];
					c[1] = it->ort->fields_indexes[j];
					c[2] = hot_field_size(nf->t);
					break;
				}
			}
		}
		// every type pointer that has the previous layout
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
	// the instances to move must not be referenced by a stack : it cannot be rewritten
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
	}
	free(olds);
	free(sizes);
	free(types);
}

/*
	The type a, which live instances have, gets the layout of t. Kept in one place : the representation
	of the types will change.
*/
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

/*
	Rewrites the live instances to the new layout, in place : the fields kept are copied to their
	new offsets, the others are cleared. Then every type pointer of a previous layout gets the new one,
	so that the instances keep their type, and the virtuals of the instances point to the new offsets.
*/
// copies the kept fields (or params) of the value o (whose content is src) to n, in the new layout
static void hot_copy_value( hot_itype *it, char *n, char *src ) {
	if( it->is_enum ) {
		int oi = ((venum*)src)->index;
		((venum*)n)->index = it->cmap[oi];
		for(int *c=it->ccopies[oi], *e=c + it->nccopies[oi] * 3;c<e;c+=3)
			memcpy(n + c[0], src + c[1], c[2]);
		return;
	}
	for(int *c=it->copies, *e=c + it->ncopies * 3;c<e;c+=3)
		memcpy(n + c[0], src + c[1], c[2]);
}

/*
	The constant values of an enum (its constructors without params) are in the array __evalues__ of
	its object, which the entry point reads to set them again : the array is built again in the order
	of the new constructors, with the live constants (same identity).
*/
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
	for(int i=0;i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->is_enum ) continue;
		it->nrt = hl_get_obj_proto(it->t);
		// the GC scans an instance up to the end of its block
		if( it->t->mark_bits && hl_mark_size(it->nrt->size) < hl_mark_size(it->max_block) ) {
			unsigned int *mark = (unsigned int*)hl_zalloc(&m->ctx.alloc, hl_mark_size(it->max_block));
			memcpy(mark, it->t->mark_bits, hl_mark_size(it->nrt->size));
			it->t->mark_bits = mark;
		}
	}
	// no collection while the instances are rewritten and moved
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
		hot_copy_value(it, n, o);
		news[k] = n;
	}
	hot_time("  move: allocate and copy");
	// the instances that have room are rewritten in place : tmp holds their pointers
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
		hot_copy_value(it, o, tmp);
	}
	free(tmp);
	// the type pointers of the previous layouts get the new one
	for(int k=0;k<h->aliases.size;k++) {
		hl_type *a = (hl_type*)h->aliases.keys[k];
		if( a == NULL ) continue;
		hot_set_layout(a, h->types[h->aliases.values[k]].t);
	}
	// the references to the moved instances are replaced by a collection (with the new layouts :
	// the GC scans them precisely). Their previous blocks are garbage : their type is cleared so
	// that nothing scans them.
	hot_pmap moved;
	memset(&moved, 0, sizeof(moved));
	if( h->nrelocated ) {
		hl_gc_remap_reserve(h->nrelocated);
		for(int k=0;k<h->nobjs;k++) {
			if( !h->objs_info[k * 3 + 2] ) continue;
			hl_gc_remap_add(h->objs[k], news[k]);
			if( h->nvirts ) hot_pmap_set(&moved, news[k], k); // to find where the fields of the virtuals were
			*(void**)h->objs[k] = NULL;
		}
		hot_time("  in place, types and remaps");
		hl_gc_remap_apply();
		hot_time("  move: remap collection");
	}
	// virtuals : their fields point into the value
	for(int k=0;k<h->nvirts;k++) {
		vvirtual *v = (vvirtual*)h->virts[k];
		char *o = (char*)v->value;
		if( o == NULL ) continue;
		int idx = hot_pmap_get(&h->aliases, *(void**)o);
		if( idx < 0 || h->types[idx].is_enum ) continue;
		hot_itype *it = h->types + idx;
		int mk = hot_pmap_get(&moved, o);
		char *prev = mk >= 0 ? (char*)h->objs[mk] : o; // where the fields were
		for(int i=0;i<v->t->virt->nfields;i++) {
			char *p = (char*)hl_vfields(v)[i];
			// fields of the previous layout : computed again
			if( p < prev || p >= prev + it->ort->size ) continue; // a method
			hl_obj_field *vf = v->t->virt->fields + i;
			hl_vfields(v)[i] = NULL;
			for(int fi=0;fi<it->nrt->nfields;fi++) {
				hl_obj_field *nf = hl_obj_field_fetch(it->t, fi);
				if( nf->hashed_name == vf->hashed_name && hl_same_type(nf->t, vf->t) ) {
					hl_vfields(v)[i] = o + it->nrt->fields_indexes[fi];
					break;
				}
			}
		}
	}
	hot_pmap_free(&moved);
	// the methods bound to the fields added (static methods of a class object)
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
	hot_prepare_hooks(h);
	free(news);
	hl_gc_enable(true);
	if( h->nobjs ) {
		printf("[HotReload] %d objects upgraded", h->nobjs);
		if( h->nrelocated ) printf(", %d moved", h->nrelocated);
		printf("\n");
	}
	hot_time("  virtuals, bindings, hooks");
}

/* ---- patch ---- */

// frees a module that was not added (the globals are shared with the root module)
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
	free(m->hot_slots);
	free(m->functions_indexes);
	free(m->functions_ptrs);
	free(m->ctx.functions_types);
	free(m);
}

static h_bool hot_refuse( hl_module *m2, const char *msg, const char *details ) {
	printf("[HotReload] Reload refused: %s%s\n[HotReload] Restart required, the program keeps running the previous code\n", msg, details ? details : "");
	fflush(stdout);
	hot_discard(m2);
	return false;
}

// the reload whose entry point is running : the types of its code that are new
static hl_code *hot_init_code = NULL;
static bool *hot_init_new_types = NULL;
static hot_instances *hot_init_instances = NULL;

static int hot_init_bits( hl_type *t, int word ) {
	hl_code *c = hot_init_code;
	if( c == NULL || t < c->types || t >= c->types + c->ntypes )
		return 0;
	if( hot_init_new_types[t - c->types] )
		return word < 0 ? 1 : -1;
	// statics added to an existing class
	hot_instances *h = hot_init_instances;
	for(int i=0;h && i<h->ntypes;i++) {
		hot_itype *it = h->types + i;
		if( it->t != t || it->is_enum ) continue;
		if( word < 0 || word > (it->t->obj->nfields >> 5) ) return 0;
		return (int)it->added[word];
	}
	return 0;
}

#define HOT_SKIP		-1	// not identified : compiled, but not callable by previous code
#define HOT_SAME		-2	// unchanged : uses the current version
#define HOT_ADDED		-3	// new function

static bool hot_world_stopped = false;

static h_bool hot_patch( hl_module *m1, hl_code *c );

/*
	Applies the code of a new version of the program. The other threads are stopped while the code
	is compared and patched and the live values upgraded, then the entry point runs.
*/
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
	hl_hot_reload *hr = m1->hot;
	if( hr == NULL ) return false;

	hl_module *m2 = hl_module_alloc(c);
	if( m2 == NULL ) return false;
	m2->hash = hl_code_hash_alloc(c);

	// match globals with the reference ones
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

	// share global data
	free(m2->globals_data);
	free(m2->globals_indexes);
	m2->globals_data = m1->globals_data;
	m2->globals_indexes = m1->globals_indexes;
	int gsize = hr->globals_size;
	for(i=hr->nglobals;i<c->nglobals;i++) {
		hl_type *t = c->globals[i];
		gsize += hl_pad_size(gsize, t);
		m2->globals_indexes[i] = gsize; // after the reference ones : not read until the patch is applied
		gsize += hl_type_size(t);
	}
	m2->globals_size = gsize;
	if( gsize > hr->max_globals_size ) {
		char tmp[128];
		sprintf(tmp, " (%d bytes needed, %d available, see HL_HOT_RELOAD_GLOBALS)", gsize, hr->max_globals_size);
		return hot_refuse(m2, "no room left for the new globals", tmp);
	}

	// check the layout of the live types before anything is changed
	hot_instances inst;
	memset(&inst, 0, sizeof(inst));
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
		// the subclasses of a type whose layout changes have their fields at other offsets
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
			return hot_refuse(m2, "the layout of live types changed (not supported yet):", buf);
		}
	}

	hl_module_init_natives(m2);
	hl_module_init_indexes(m2);
	hl_code_hash_finalize(m2->hash);

	// the live values whose type layout changes must be found, with room for their new layout
	{
		char buf[4096];
		hot_report r = { buf, sizeof(buf), 0, 0 };
		buf[0] = 0;
		hot_time("compare");
		if( inst.ntypes ) {
			hot_instances_check(&inst, &r);
			hot_time("instances check");
		}
		if( r.count == 0 ) {
			hot_check_stack(inst.types, inst.ntypes, &r);
		}
		if( r.count ) {
			free(itype_of);
			hot_instances_free(&inst);
			return hot_refuse(m2, "live values cannot be upgraded:", buf);
		}
	}

	// match functions with the reference ones
	int nfuns = c->nfunctions;
	int *match = (int*)malloc(sizeof(int) * nfuns);
	bool *taken = (bool*)malloc(sizeof(bool) * (hr->nfunctions + 1));
	memset(taken,0,sizeof(bool) * (hr->nfunctions + 1));
	m2->hot_slots = (void***)malloc(sizeof(void**) * (nfuns + c->nnatives));
	memset(m2->hot_slots,0,sizeof(void**) * (nfuns + c->nnatives));
	int changes_count = 0;
	for(i=0;i<nfuns;i++) {
		hl_function *f2 = c->functions + i;
		match[i] = HOT_SKIP;
		if( f2->field.name == NULL ) continue;
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
				m2->hot_slots[f2->findex] = hf->slot;
			} else
				match[i] = k;
			break;
		}
		if( match[i] != HOT_SAME ) {
#			ifdef HL_DEBUG
			uprintf(USTR("%s."), fun_obj(f2)->name);
			uprintf(USTR("%s"), fun_field_name(f2));
			if( !f2->obj && f2->lname ) uprintf(USTR("~%s"), f2->lname); else if( !f2->obj ) printf("~%d", f2->ref);
			printf(match[i] == HOT_ADDED ? " has been added\n" : " has been modified [%d]\n", f2->nops);
#			endif
			changes_count++;
		}
	}
	free(taken);
	if( changes_count == 0 ) {
		free(match);
		hot_instances_free(&inst);
		free(itype_of);
		printf("[HotReload] No changes found\n");
		fflush(stdout);
		hot_discard(m2);
		return false;
	}

	// alias the types which have the same layout : they are different pointers but share the runtime data
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
	free(upgraded);

	// JIT the new and changed functions
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

	// keep the debug infos ordered for the lookup of a code position
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

	/* ---- nothing can fail past this point : apply the patch and update the reference state ---- */

#	ifdef WIN64_UNWIND_TABLES
	RtlAddFunctionTable(m2->unwind_table, m2->unwind_table_size, (DWORD64)m2->jit_code);
#	endif

	// new globals
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

	// functions
	for(i=0;i<nfuns;i++) {
		hl_function *f2 = c->functions + i;
		if( match[i] == HOT_SAME ) {
			m2->functions_ptrs[f2->findex] = *m2->hot_slots[f2->findex];
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
		// changed : the previous version jumps to the new one through the slot
		hot_function *hf = hr->functions + match[i];
		hl_jit_patch_method(*hf->slot, hf->slot);
		*hf->slot = ptr;
		hf->hash = hash;
		hf->f = f2;
	}
	free(match);

	hot_record_uses(m2);

	// upgrade the live values whose type layout changed, now that the new functions have their address
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

	printf("[HotReload] %d changes\n", changes_count);
	fflush(stdout);
	hl_module_add(m2);

	// call the new entry point : it initializes the new types, and the statics and __init__ that
	// hot_init_bits tells (the ones of the new classes), the other statics keep their values
	{
		vclosure cl;
		bool isExc = false;
		cl.t = c->functions[m2->functions_indexes[c->entrypoint]].type;
		cl.fun = m2->functions_ptrs[c->entrypoint];
		cl.hasValue = 0;
		// the other threads run again : the entry point is code of the program
		hot_world_stopped = false;
		hl_gc_world(false);
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


/*
	Called before the first module is initialized in hot reload mode : the globals get room for the
	ones the reloads add, and the class objects will be allocated with room for more statics.
*/
void hl_hot_reload_prepare( hl_module *m ) {
	int extra_globals = hot_reload_extra_globals();
	hl_setup.hot_reload = true;
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

// called after the first module is initialized : the reference state of the reloads
void hl_hot_reload_init( hl_module *m ) {
	int extra_globals = hot_reload_extra_globals();
	hot_record_uses(m);
	hl_setup.hot_init_bits = hot_init_bits;
	hl_code_hash_finalize(m->hash);
	hot_reload_init(m, m->code->nglobals + extra_globals, m->globals_size + extra_globals * (int)sizeof(void*));
}
