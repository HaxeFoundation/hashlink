#include <hl.h>

hl_type hlt_array = { HARRAY };
hl_type hlt_bytes = { HBYTES };
hl_type hlt_dynobj = { HDYNOBJ };

HL_PRIM vdynamic* hl_safe_cast( vdynamic *fun, hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM bool hl_type_check( hl_type *t, vdynamic *value ) {
	hl_fatal("TODO");
	return false;
}

HL_PRIM vbytes* hl_type_name( hl_type *t ) {
	switch( t->kind ) {
	case HOBJ:
		return (vbytes*)t->obj->name;
	case HENUM:
		return (vbytes*)t->tenum->name;
	default:
		break;
	}
	return NULL;
}

HL_PRIM varray* hl_type_enum_fields( hl_type *t ) {
	varray *a = (varray*)hl_gc_alloc_noptr(sizeof(varray) + t->tenum->nconstructs * sizeof(void*));
	int i;
	a->t = &hlt_array;
	a->at = &hlt_bytes;
	a->size = t->tenum->nconstructs;
	for( i=0; i<t->tenum->nconstructs;i++)
		((void**)(a+1))[i] = (vbytes*)t->tenum->constructs[i].name;
	return a;
}

HL_PRIM int hl_type_args_count( hl_type *t ) {
	if( t->kind == HFUN )
		return t->fun->nargs;
	return 0;
}

HL_PRIM varray *hl_type_instance_fields( hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic *hl_type_get_global( hl_type *t ) {
	switch( t->kind ) {
	case HOBJ:
		return (vdynamic*)t->obj->global_value;
	case HENUM:
		hl_fatal("TODO");
		break;
	default:
		break;
	}
	return NULL;
}
DEFINE_PRIM(_DYN, hl_safe_cast, _DYN _TYPE);
DEFINE_PRIM(_BOOL, hl_type_check, _TYPE _DYN);
DEFINE_PRIM(_BYTES, hl_type_name, _TYPE);
DEFINE_PRIM(_ARR, hl_type_enum_fields, _TYPE);
