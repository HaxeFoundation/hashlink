#include "hl.h"
#include <string.h>

static void hl_lookup_insert( hl_field_lookup *l, int size, int hash, hl_type *t, int index ) {
	int min = 0;
	int max = size;
	int pos;
	while( min < max ) {
		int mid = (min + max) >> 1;
		int h = l[mid].hashed_name;
		if( h < hash ) min = mid + 1; else max = mid;
	}
	pos = (min + max) >> 1;
	memcpy(l + pos + 1, l + pos, (size - pos) * sizeof(hl_field_lookup));
	l[pos].field_index = index;
	l[pos].hashed_name = hash;
	l[pos].t = t;
}

static hl_field_lookup *hl_lookup_find( hl_field_lookup *l, int size, int hash ) {
	int min = 0;
	int max = size;
	while( min < max ) {
		int mid = (min + max) >> 1;
		int h = l[mid].hashed_name;
		if( h < hash ) min = mid + 1; else if( h > hash ) max = mid; else return l + mid;
	}
	return NULL;
}

static int hl_lookup_find_index( hl_field_lookup *l, int size, int hash ) {
	int min = 0;
	int max = size;
	while( min < max ) {
		int mid = (min + max) >> 1;
		int h = l[mid].hashed_name;
		if( h < hash ) min = mid + 1; else if( h > hash ) max = mid; else return mid;
	}
	return (min + max) >> 1;
}

static int hl_cache_count = 0;
static int hl_cache_size = 0;
static hl_field_lookup *hl_cache = NULL;

int hl_hash( vbytes *b ) {
	return hl_hash_gen((uchar*)b,true);
}

int hl_hash_gen( const uchar *name, bool cache_name ) {
	int h = 0;
	const uchar *oname = name;
	while( *name ) {
		h = 223 * h + (unsigned)*name;
		name++;
	}
	h %= 0x1FFFFF7B;
	if( cache_name ) {
		hl_field_lookup *l = hl_lookup_find(hl_cache, hl_cache_count, h);
		if( l == NULL ) {
			if( hl_cache_size == hl_cache_count ) {
				// resize
				int newsize = hl_cache_size ? (hl_cache_size * 3) >> 1 : 16;
				hl_field_lookup *cache = (hl_field_lookup*)hl_gc_alloc(sizeof(hl_field_lookup) * newsize);
				memcpy(cache,hl_cache,sizeof(hl_field_lookup) * hl_cache_count);
				free(hl_cache);
				hl_cache = cache;
				hl_cache_size = newsize;
			}
			hl_lookup_insert(hl_cache,hl_cache_count++,h,(hl_type*)ustrdup(oname),0);
		}
	}
	return h;
}

const uchar *hl_field_name( int hash ) {
	hl_field_lookup *l = hl_lookup_find(hl_cache, hl_cache_count, hash);
	return l ? (uchar*)l->t : USTR("???");
}

void hl_cache_free() {
	int i;
	for(i=0;i<hl_cache_count;i++)
		free(hl_cache[i].t);
	free(hl_cache);
	hl_cache = NULL;
	hl_cache_count = hl_cache_size = 0;
}

/**
	Builds class metadata (fields indexes, etc.)
	Does not require the JIT to be finalized.
**/
hl_runtime_obj *hl_get_obj_rt( hl_type *ot ) {
	hl_type_obj *o = ot->obj;
	hl_module_context *m = o->m;
	hl_alloc *alloc = &m->alloc;
	hl_runtime_obj *p = NULL, *t;
	int i, size, start;
	if( o->rt ) return o->rt;
	if( o->super ) p = hl_get_obj_rt(o->super);
	t = (hl_runtime_obj*)hl_malloc(alloc,sizeof(hl_runtime_obj));
	t->obj = o;
	t->nfields = o->nfields + (p ? p->nfields : 0);
	t->nproto = p ? p->nproto : 0;
	t->nlookup = o->nfields + o->nproto;
	t->lookup = (hl_field_lookup*)hl_malloc(alloc,sizeof(hl_field_lookup) * t->nlookup);
	t->fields_indexes = (int*)hl_malloc(alloc,sizeof(int)*t->nfields);
	t->toString = NULL;
	t->parent = p;

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
		hl_lookup_insert(t->lookup,i,o->fields[i].hashed_name,o->fields[i].t,size);
		size += hl_type_size(ft);
	}
	t->size = size;
	t->proto = NULL;
	o->rt = t;

	// fields lookup
	size = 0;
	for(i=0;i<o->nproto;i++) {
		hl_obj_proto *p = o->proto + i;
		if( p->pindex >= t->nproto ) t->nproto = p->pindex + 1;
		hl_lookup_insert(t->lookup,i + o->nfields,p->hashed_name,m->functions_types[p->findex],-(i+1));
	}
	return t;
}

/**
	Fill class prototype with method pointers.
	Requires JIT to be finilized
**/
hl_runtime_obj *hl_get_obj_proto( hl_type *ot ) {
	hl_type_obj *o = ot->obj;
	hl_module_context *m = o->m;
	hl_alloc *alloc = &m->alloc;
	hl_runtime_obj *p = NULL, *t = hl_get_obj_rt(ot);
	hl_field_lookup *strField = hl_lookup_find(t->lookup,t->nlookup,hl_hash_gen(USTR("__string"),false));
	int i;
	if( t->proto ) return t;
	if( o->super ) p = hl_get_obj_proto(o->super);
	t->toString = strField ? m->functions_ptrs[o->proto[-(strField->field_index+1)].findex] : (p ? p->toString : NULL);
	t->proto = (vobj_proto*)hl_malloc(alloc, sizeof(vobj_proto) + t->nproto * sizeof(void*));
	t->proto->t = *ot; // COPY
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

/**
	Allocate a virtual fields mapping to a given value.
**/
vvirtual *hl_to_virtual( hl_type *vt, vdynamic *obj ) {
	vvirtual *v;
	if( obj == NULL ) return NULL;
	switch( obj->t->kind ) {
	case HOBJ:
		{ 
			int i;
			vobj *o = (vobj*)obj;
			hl_runtime_obj *rt = o->proto->t.obj->rt;
			v = (vvirtual*)hl_gc_alloc(sizeof(vvirtual));
			v->t = vt;
			v->fields_data = (char*)obj;
			v->indexes = (int*)hl_gc_alloc(sizeof(int)*vt->virt->nfields);
			v->value = obj;
			v->next = NULL;
			for(i=0;i<vt->virt->nfields;i++) {
				hl_runtime_obj *rtt = rt;
				hl_field_lookup *f = NULL;
				while( rtt ) {
					f = hl_lookup_find(rtt->lookup,rtt->nlookup,vt->virt->fields[i].hashed_name);
					if( f != NULL ) break;
					rtt = rtt->parent;
				}
				v->indexes[i] = f == NULL || f->field_index <= 0 || (f->t != vt->virt->fields[i].t) ? 0 : f->field_index;
			}
		}
		break;
	case HDYNOBJ:
		{
			int i;
			vdynobj *o = (vdynobj*)obj;
			v = o->virtuals;
			while( v ) {
				if( v->t->virt == vt->virt )
					return v;
				v = v->next;
			}
			// allocate a new virtual mapping
			v = (vvirtual*)hl_gc_alloc(sizeof(vvirtual));
			v->t = vt;
			v->fields_data = o->fields_data - sizeof(void*);
			v->indexes = hl_gc_alloc(sizeof(int)*vt->virt->nfields);
			v->value = obj;
			for(i=0;i<vt->virt->nfields;i++) {
				hl_field_lookup *f = hl_lookup_find(&o->dproto->fields,o->nfields,vt->virt->fields[i].hashed_name);
				if( f == NULL ) hl_fatal_fmt("Cast failure: field %s is missing",vt->virt->fields[i].name);
				v->indexes[i] = f == NULL || f->t != vt->virt->fields[i].t ? 0 : f->field_index + sizeof(void*);
			}
			// add it to the list
			v->next = o->virtuals;
			o->virtuals = v;
		}
		break;
	default:
		hl_fatal_fmt("Don't know how to virtual %d",obj->t->kind);
	}
	return v;
}

void *hl_fetch_virtual_method( vvirtual *v, int fid ) {
	hl_obj_field *f = v->t->virt->fields + fid;
	switch( v->value->t->kind ) {
	case HOBJ:
		{
			vobj *o = (vobj*)v->value;
			hl_module_context *m = o->proto->t.obj->m;
			hl_runtime_obj *rt = o->proto->t.obj->rt;
			while( rt ) {
				hl_field_lookup *found = hl_lookup_find(rt->lookup,rt->nlookup,f->hashed_name);
				if( found ) {
					int fid;
					if( found->field_index > 0 ) return NULL;
					fid = rt->obj->proto[-found->field_index].findex;
					return hl_alloc_closure_ptr(m->functions_types[fid], m->functions_ptrs[fid], o);
				}
				rt = rt->parent;
			}
		}
		break;
	}
	return NULL;
}

static vdynamic *hl_to_dyn( vdynamic *v, hl_type *t ) {
	switch( t->kind ) {
	case HDYNOBJ:
	case HOBJ:
	case HARRAY:
		return v;
#ifndef HL_64
	case HF64:
		hl_error("assert : loss of data");
#endif
	default:
		{
			vdynamic *d = hl_alloc_dynamic(t);
			d->v.ptr = v;
			return d;
		}
	}
}

#define B2(t1,t2) ((t1) + ((t2) * HLAST))

static int fetch_data32( hl_type *src, hl_type *dst, void *data ) {	
	if( src != dst ) {
#		ifndef HL_64
		if( dst->kind == HDYN ) {
			if( src->kind == HF64 ) {
				vdynamic *d = hl_alloc_dynamic(src);
				d->v.d = *(double*)data;
				return (int)d;
			}
			return (int)hl_to_dyn(*(vdynamic**)data,src);
		}
#		endif
		hl_error("Invalid dynget cast");
	}
	return *(int*)data;
}

static int64 fetch_data64( hl_type *src, hl_type *dst, void *data ) {	
	if( src != dst ) {
		switch( B2(src->kind,dst->kind) ) {
		case B2(HI32,HF64):
		{
			union {
				double d;
				int64 i;
			} v;
			v.d = *(int*)data;
			return v.i;
		}
		default:
#			ifdef HL_64
			if( dst->kind == HDYN ) {
				if( src->kind == HF64 ) {
					vdynamic *d = hl_alloc_dynamic(src);
					d->v.d = *(double*)data;
					return (int64)d;
				}
				return (int64)hl_to_dyn(*(vdynamic**)data,src);
			}
#			endif
			hl_error("Invalid dynget cast");
		}
	}
	return *(int64*)data;
}

static hl_field_lookup *hl_dyn_alloc_field( vdynobj *o, int hfield, hl_type *t ) {
	int pad = hl_pad_size(o->dataSize, t);
	int size = hl_type_size(t);
	int index;
	char *newData = (char*)hl_gc_alloc(o->dataSize + pad + size);
	vdynobj_proto *proto = (vdynobj_proto*)hl_gc_alloc(sizeof(vdynobj_proto) + sizeof(hl_field_lookup) * (o->nfields + 1 - 1));
	int field_pos = hl_lookup_find_index(&o->dproto->fields, o->nfields, hfield);
	hl_field_lookup *f;
	// update data
	memcpy(newData,o->fields_data,o->dataSize);
	o->fields_data = newData;
	o->dataSize += pad;
	index = o->dataSize;
	o->dataSize += size;
	// update field table
	proto->t = o->dproto->t;
	memcpy(&proto->fields,&o->dproto->fields,field_pos * sizeof(hl_field_lookup));
	f = (&proto->fields) + field_pos;
	f->t = t;
	f->hashed_name = hfield;
	f->field_index = index;
	memcpy(&proto->fields + (field_pos + 1),&o->dproto->fields + field_pos, (o->nfields - field_pos) * sizeof(hl_field_lookup));
	o->nfields++;
	o->dproto = proto;
	return f;
}

int hl_dyn_get32( vdynamic *d, int hfield, hl_type *t ) {
	if( d == NULL ) hl_error("Invalid field access");
	switch( d->t->kind ) {
	case HDYNOBJ:
		{
			vdynobj *o = (vdynobj*)d;
			hl_field_lookup *f = hl_lookup_find(&o->dproto->fields,o->nfields,hfield);
			if( f == NULL ) return 0;
			return fetch_data32(f->t, t, o->fields_data + f->field_index);
		}
		break;
	case HOBJ:
		{
			vobj *o = (vobj*)d;
			hl_runtime_obj *rt = o->proto->t.obj->rt;
			hl_field_lookup *f = NULL;
			do {
				f = hl_lookup_find(rt->lookup,rt->nlookup,hfield);
				if( f != NULL ) break;
				rt = rt->parent;
			} while( rt );
			if( f == NULL )
				hl_fatal_fmt("#%s has no field %s",o->proto->t.obj->name,hl_field_name(hfield));
			return fetch_data32(f->t,t,(char*)o + f->field_index);
		}
		break;
	case HVIRTUAL:
		return hl_dyn_get32(((vvirtual*)d)->value, hfield, t);
	default:
		hl_error("Invalid field access");
		break;
	}
	return 0;
}

int64 hl_dyn_get64( vdynamic *d, int hfield, hl_type *t ) {
	if( d == NULL ) hl_error("Invalid field access");
	switch( d->t->kind ) {
	case HDYNOBJ:
		{
			vdynobj *o = (vdynobj*)d;
			hl_field_lookup *f = hl_lookup_find(&o->dproto->fields,o->nfields,hfield);
			if( f == NULL ) return 0;
			return fetch_data64(f->t,t,o->fields_data + f->field_index);
		}
		break;
	case HOBJ:
		{
			vobj *o = (vobj*)d;
			hl_runtime_obj *rt = o->proto->t.obj->rt;
			hl_field_lookup *f = NULL;
			do {
				f = hl_lookup_find(rt->lookup,rt->nlookup,hfield);
				if( f != NULL ) break;
				rt = rt->parent;
			} while( rt );
			if( f == NULL )
				hl_fatal_fmt("#%s has no field %s",o->proto->t.obj->name,hl_field_name(hfield));
			return fetch_data64(f->t,t,(char*)o + f->field_index);
		}
		break;
	case HVIRTUAL:
		return hl_dyn_get64(((vvirtual*)d)->value, hfield, t);
	default:
		hl_error("Invalid field access");
	}
	return 0;
}

void hl_dyn_set32( vdynamic *d, int hfield, hl_type *t, int value ) {
	if( d == NULL ) hl_error("Invalid field access");
	switch( d->t->kind ) {
	case HDYNOBJ:
		{
			vdynobj *o = (vdynobj*)d;
			hl_field_lookup *f = hl_lookup_find(&o->dproto->fields,o->nfields,hfield);
			if( f == NULL )
				f = hl_dyn_alloc_field(o,hfield,t);
			else if( f->t != t )
				hl_error("Invalid dynset cast");
			*(int*)(o->fields_data + f->field_index) = value;
		}
		break;
	case HOBJ:
		hl_error("TODO");
		break;
	case HVIRTUAL:
		hl_dyn_set32(((vvirtual*)d)->value, hfield, t, value);
		break;
	default:
		hl_error("Invalid field access");
	}
}

void hl_dyn_set64( vdynamic *d, int hfield, hl_type *t, int64 value ) {
	if( d == NULL ) hl_error("Invalid field access");
	switch( d->t->kind ) {
	case HDYNOBJ:
		{
			vdynobj *o = (vdynobj*)d;
			hl_field_lookup *f = hl_lookup_find(&o->dproto->fields,o->nfields,hfield);
			if( f == NULL )
				f = hl_dyn_alloc_field(o,hfield,t);
			else if( f->t != t )
				hl_error("Invalid dynset cast");
			*(int64*)(o->fields_data + f->field_index) = value;
		}
		break;
	case HVIRTUAL:
		hl_dyn_set64(((vvirtual*)d)->value, hfield, t, value);
		break;
	case HOBJ:
		hl_error("TODO");
		break;
	default:
		hl_error("Invalid field access");
	}
}
