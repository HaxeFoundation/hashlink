#include "hl.h"

int hl_hash( const char *name ) {
	int h = 0;
	const char *oname = name;
	while( *name ) {
		h = 223 * h + *((unsigned char*)name);
		name++;
	}
	h %= 0x1FFFFF7B;
	return h;
}

static void hl_lookup_insert( hl_field_lookup *l, int size, const char *name, hl_type *t, int index ) {
	int min = 0;
	int max = size;
	int hash = hl_hash(name);
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

/**
	Builds class metadata (fields indexes, etc.)
	Does not require the JIT to be finalized.
**/
hl_runtime_obj *hl_get_obj_rt( hl_module *m, hl_type *ot ) {
	hl_alloc *alloc = &m->code->alloc;
	hl_type_obj *o = ot->obj;
	hl_runtime_obj *p = NULL, *t;
	int i, size, start;
	if( o->rt ) return o->rt;
	if( o->super ) p = hl_get_obj_rt(m, o->super);
	t = (hl_runtime_obj*)hl_malloc(alloc,sizeof(hl_runtime_obj));
	t->m = m;
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
		hl_lookup_insert(t->lookup,i,o->fields[i].name,o->fields[i].t,size);
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
		hl_lookup_insert(t->lookup,i + o->nfields,p->name,m->code->functions[m->functions_indexes[p->findex]].type,-i);
	}
	return t;
}

/**
	Fill class prototype with method pointers.
	Requires JIT to be finilized
**/
hl_runtime_obj *hl_get_obj_proto( hl_module *m, hl_type *ot ) {
	hl_alloc *alloc = &m->code->alloc;
	hl_type_obj *o = ot->obj;
	hl_runtime_obj *p = NULL, *t = hl_get_obj_rt(m, ot);
	hl_field_lookup *strField = hl_lookup_find(t->lookup,t->nlookup,hl_hash("__string"));
	int i;
	if( t->proto ) return t;
	if( o->super ) p = hl_get_obj_proto(m,o->super);
	t->toString = strField ? m->functions_ptrs[o->proto[-(strField->field_index+1)].findex] : (p ? p->toString : NULL);
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

/**
	Allocate a virtual fields mapping to a given value.
**/
vvirtual *hl_to_virtual( hl_type *vt, vdynamic *obj ) {
	vvirtual *v;
	int *indexes;
	if( obj == NULL ) return NULL;
	v = (vvirtual*)malloc(sizeof(vvirtual));
	v->proto = (vvirtual_proto*)malloc(sizeof(vvirtual_proto) + sizeof(int)*vt->virt->nfields);
	indexes = (int*)(v->proto + 1);
	v->proto->t = vt;
	v->original = obj;
	switch( (*obj->t)->kind ) {
	case HOBJ:
		{ 
			int i;
			vobj *o = (vobj*)obj;
			hl_runtime_obj *rt =o->proto->t->obj->rt;
			v->field_data = o;
			for(i=0;i<vt->virt->nfields;i++) {
				hl_runtime_obj *rtt = rt;
				hl_field_lookup *f = NULL;
				while( rtt ) {
					f = hl_lookup_find(rtt->lookup,rtt->nlookup,vt->virt->fields[i].hashed_name);
					if( f != NULL ) break;
					rtt = rtt->parent;
				}
				indexes[i] = f == NULL || f->field_index <= 0 ? 0 : f->field_index;
			}
		}
		break;
	default:
		hl_error("Don't know how to virtual %d",(*obj->t)->kind);
	}
	return v;
}

void *hl_fetch_virtual_method( vvirtual *v, int fid ) {
	hl_obj_field *f = v->proto->t->virt->fields + fid;
	switch( (*v->original->t)->kind ) {
	case HOBJ:
		{
			vobj *o = (vobj*)v->original;
			hl_runtime_obj *rt = o->proto->t->obj->rt;
			while( rt ) {
				hl_field_lookup *found = hl_lookup_find(rt->lookup,rt->nlookup,f->hashed_name);
				if( found ) {
					int fid;
					if( found->field_index > 0 ) return NULL;
					fid = rt->obj->proto[-found->field_index].findex;
#					ifdef HL_64
					return hl_alloc_closure_i64(rt->m, fid, (int_val)o);
#					else
					return hl_alloc_closure_i32(rt->m, fid, (int)o);
#					endif
				}
				rt = rt->parent;
			}
		}
		break;
	}
	return NULL;
}
