#include <hl.h>

vclosure *hl_alloc_closure_void( hl_type *t, void *fvalue ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = t;
	c->fun = fvalue;
	c->hasValue = false;
	return c;
}

static hl_type *hl_get_closure_type( hl_type *t ) {
	hl_type_fun *ft = t->fun;
	if( ft->closure_type.kind != HFUN ) {
		if( ft->nargs == 0 ) hl_fatal("assert");
		ft->closure_type.kind = HFUN;
		ft->closure_type.p = &ft->closure;
		ft->closure.nargs = ft->nargs - 1;
		ft->closure.args = ft->closure.nargs ? ft->args + 1 : NULL;
		ft->closure.ret = ft->ret;
		ft->closure.parent = t;
	}
	return (hl_type*)&ft->closure_type;
}

vclosure *hl_alloc_closure_ptr( hl_type *fullt, void *fvalue, void *v ) {
	vclosure *c = (vclosure*)hl_gc_alloc(sizeof(vclosure));
	c->t = hl_get_closure_type(fullt);
	c->fun = fvalue;
	c->hasValue = true;
	c->value = v;
	return c;
}

HL_PRIM vdynamic *hl_make_var_args( vclosure *c ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vdynamic *hl_no_closure( vdynamic *c ) {
	vclosure *cl = (vclosure*)c;
	if( !cl->hasValue ) return c;
	return (vdynamic*)hl_alloc_closure_void(cl->t->fun->parent,cl->fun);
}

HL_PRIM vdynamic* hl_get_closure_value( vdynamic *c ) {
	return ((vclosure*)c)->value;
}

HL_PRIM vdynamic* hl_call_method( vdynamic *c, varray *args ) {
	void *fun = ((vclosure*)c)->fun;
	((void(*)( vdynamic * ))fun)( *(vdynamic**)(args+1) );
	return NULL;
}

