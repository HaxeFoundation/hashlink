#include <hl.h>

vclosure *hl_alloc_closure_void( hl_type *t, void *fvalue ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = t;
	c->fun = fvalue;
	c->bits = 0;
	return c;
}

vclosure *hl_alloc_closure_ptr( hl_type *t, void *fvalue, void *v ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = t;
	c->fun = fvalue;
	c->hasValue = 1;
	c->value = v;
	return c;
}

HL_PRIM vdynamic *hl_make_var_args( vclosure *c ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic *hl_no_closure( vdynamic *c ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic* hl_get_closure_value( vdynamic *c ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic* hl_call_method( vdynamic *c, varray *args ) {
	hl_fatal("TODO");
	return NULL;
}

