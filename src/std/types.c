#include <hl.h>

HL_PRIM vdynamic* hl_safe_cast( vdynamic *fun, hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM bool hl_type_check( hl_type *t, vdynamic *value ) {
	hl_fatal("TODO");
	return false;
}

HL_PRIM vbytes* hl_type_name( hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM varray* hl_type_enum_fields( hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

DEFINE_PRIM(_DYN, hl_safe_cast, _DYN _TYPE);
DEFINE_PRIM(_BOOL, hl_type_check, _TYPE _DYN);
DEFINE_PRIM(_BYTES, hl_type_name, _TYPE);
DEFINE_PRIM(_ARR, hl_type_enum_fields, _TYPE);
