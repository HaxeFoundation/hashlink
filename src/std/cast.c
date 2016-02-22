#include <hl.h>

#define TK2(a,b)		((a) | ((b)<<5))

vdynamic *hl_make_dyn( void *data, hl_type *t ) {
	vdynamic *v;
	switch( t->kind ) {
	case HI8:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.c = *(char*)data;
		return v;
	case HI16:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.s = *(short*)data;
		return v;
	case HI32:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.i = *(int*)data;
		return v;
	case HF32:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.f = *(float*)data;
		return v;
	case HF64:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.d = *(double*)data;
		return v;
	case HBOOL:
		v = (vdynamic*)hl_gc_alloc_noptr(sizeof(vdynamic));
		v->t = t;
		v->v.b = *(bool*)data;
		return v;
	case HBYTES:
	case HTYPE:
	case HREF:
	case HABSTRACT:
	case HENUM:
		v = (vdynamic*)hl_gc_alloc(sizeof(vdynamic));
		v->t = t;
		v->v.ptr = *(void**)data;
		return v;
	default:
		return *(vdynamic**)data;
	}
}


int hl_dyn_casti( void *data, hl_type *t, hl_type *to ) {
	if( t->kind == HDYN ) {
		vdynamic *v = *((vdynamic**)data);
		if( v == NULL ) return 0;
		t = v->t;
		if( !hl_is_dynamic(t) ) data = &v->v;
	}
	switch( TK2(t->kind,to->kind) ) {
	case TK2(HI8,HI8):
		return *(char*)data;
	case TK2(HI16,HI16):
		return *(short*)data;
	case TK2(HI32,HI32):
		return *(int*)data;
	case TK2(HF32,HI32):
		return (int)*(float*)data;
	case TK2(HF64,HI32):
		return (int)*(double*)data;
	case TK2(HBOOL,HBOOL):
		return *(bool*)data;
	default:
		switch( t->kind ) {
		case HNULL:
			{
				vdynamic *v = *(vdynamic**)data;
				if( v == NULL ) return 0;
				return hl_dyn_casti(&v->v,t->tparam,to);
			}
		}
		break;
	}
	hl_error_msg(USTR("Can't cast %s(%s) to %s"),hl_to_string(hl_make_dyn(data,t)),hl_type_str(t),hl_type_str(to));
	return 0;
}


#define HL_MAX_ARGS 0

void *hlc_dyn_call( void *fun, hl_type *t, vdynamic **args );

void *hl_wrap0( vclosure *c ) {
	return c->hasValue ? hlc_dyn_call(c->fun,c->t->fun->parent,(vdynamic**)&c->value) : hlc_dyn_call(c->fun,c->t,NULL);
}

vclosure *hl_make_fun_wrapper( vclosure *c, hl_type *to ) {
	static void *fptr[HL_MAX_ARGS+1] = { hl_wrap0 };
	hl_type_fun *ct = c->t->fun;
	int i;
	if( ct->nargs != to->fun->nargs )
		return NULL;
	if( !hl_is_ptr(to->fun->ret) )
		return NULL;
	for(i=0;i<to->fun->nargs;i++)
		if( !hl_is_ptr(to->fun->args[i]) )
			return NULL;
	if( to->fun->nargs > HL_MAX_ARGS )
		return NULL;
	return hl_alloc_closure_wrapper(to,fptr[to->fun->nargs],c);
}

void *hl_dyn_castp( void *data, hl_type *t, hl_type *to ) {
	if( t->kind == HDYN ) {
		vdynamic *v = *(vdynamic**)data;
		if( v == NULL ) return NULL;
		t = v->t;
		if( !hl_is_dynamic(t) ) data = &v->v;
	}
	if( t == to || hl_safe_cast(t,to) )
		return *(void**)data;
	switch( TK2(t->kind,to->kind) ) {
	case TK2(HOBJ,HOBJ):
		{
			hl_type_obj *t1 = t->obj;
			hl_type_obj *t2 = to->obj;
			while( true ) {
				if( t1 == t2 )
					return *(void**)data;
				if( t1->super == NULL )
					break;
				t1 = t1->super->obj;
			}
			if( t->obj->rt->castFun ) {
				vdynamic *v = t->obj->rt->castFun(*(vdynamic**)data,to);
				if( v ) return v;
			}
			break;
		}
	case TK2(HFUN,HFUN):
		{
			vclosure *c = *(vclosure**)data;
			if( c == NULL ) return NULL;
			c = hl_make_fun_wrapper(c,to);
			if( c ) return c;
		}
		break;
	case TK2(HOBJ,HDYN):
	case TK2(HDYNOBJ,HDYN):
	case TK2(HFUN,HDYN):
	case TK2(HNULL,HDYN):
	case TK2(HARRAY,HDYN):
		return *(void**)data;
	}
	if( to->kind == HDYN )
		return hl_make_dyn(data,t);
	hl_error_msg(USTR("Can't cast %s(%s) to %s"),hl_to_string(hl_make_dyn(data,t)),hl_type_str(t),hl_type_str(to));
	return 0;
}

double hl_dyn_castd( void *data, hl_type *t ) {
	switch( t->kind ) {
	case HF32:
		return *(float*)data;
	case HI8:
		return *(char*)data;
	case HI16:
		return *(short*)data;
	case HI32:
		return *(int*)data;
	case HBOOL:
		return *(bool*)data;
	default:
		break;
	}
	return 0.;
}

static int fcompare( float d ) {
	if( d != d ) return hl_invalid_comparison;
 	return d == 0.f ? 0 : (d > 0.f ? 1 : -1);
}

static int dcompare( double d ) {
	if( d != d ) return hl_invalid_comparison;
 	return d == 0. ? 0 : (d > 0. ? 1 : -1);
}

int hl_dyn_compare( vdynamic *a, vdynamic *b ) {
	if( a == b )
		return 0;
	if( a == NULL )
		return -1;
	if( b == NULL )
		return 1;
	switch( TK2(a->t->kind,b->t->kind) ) {
	case TK2(HI8,HI8):
		return a->v.c - b->v.c;
	case TK2(HI16,HI16):
		return a->v.s - b->v.s;
	case TK2(HI32,HI32):
		return a->v.i - b->v.i;
	case TK2(HF32,HF32):
		return fcompare(a->v.f - b->v.f);
	case TK2(HF64,HF64):
		return dcompare(a->v.d - b->v.d);
	case TK2(HBOOL,HBOOL):
		return a->v.b - b->v.b;
	case TK2(HF64, HI32):
		return dcompare(a->v.d - (double)b->v.i);
	case TK2(HI32, HF64):
		return dcompare((double)a->v.i - b->v.d);
	case TK2(HOBJ,HOBJ):
		if( a->t->obj == b->t->obj && a->t->obj->rt->compareFun )
			return a->t->obj->rt->compareFun(a,b);
		break;
	case TK2(HENUM,HENUM):
	case TK2(HTYPE,HTYPE):
	case TK2(HBYTES,HBYTES):
		return a->v.ptr != b->v.ptr;
	}
#ifdef _DEBUG
	uprintf(USTR("Don't know how to compare %s(%s) and %s(%s)\n"),hl_to_string(a),hl_type_str(a->t),hl_to_string(b),hl_type_str(b->t));
#endif
	return hl_invalid_comparison;
}

HL_PRIM vdynamic* hl_value_cast( vdynamic *v, hl_type *t ) {
	if( t->kind == HDYN )
		return v;
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM bool hl_type_check( hl_type *t, vdynamic *value ) {
	if( value == NULL )
		return false;
	if( t == value->t )
		return true;
	switch( TK2(t->kind,value->t->kind) ) {
	}
	return hl_safe_cast(value->t, t);
}
