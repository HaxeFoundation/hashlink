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
	switch( TK2(t->kind,to->kind) ) {
	case TK2(HI8,HI8):
		return *(char*)data;
	case TK2(HI16,HI16):
		return *(short*)data;
	case TK2(HI32,HI32):
		return *(int*)data;
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

void *hl_dyn_castp( void *data, hl_type *t, hl_type *to ) {
	if( t == to || hl_same_type(t,to) )
		return *(void**)data;
	switch( TK2(t->kind,to->kind) ) {
	case TK2(HDYN,HOBJ):
		{
			vdynamic *v = *(vdynamic**)data;
			if( v == NULL ) return NULL;
			return hl_dyn_castp(data,v->t,to);
		}
	case TK2(HOBJ,HOBJ):
		{
			hl_type_obj *t1 = t->obj;
			hl_type_obj *t2 = t->obj;
			while( true ) {
				if( t1 == t2 )
					return *(void**)data;
				if( t1->super == NULL )
					break;
				t1 = t1->super->obj;
			}
		}
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
	default:
		hl_error_msg(USTR("Can't cast %s(%s) to double"),hl_to_string(hl_make_dyn(data,t)),hl_type_str(t));
		break;
	}
	return 0;
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
		{
			float d = a->v.f - b->v.f;
			if( d != d ) return hl_invalid_comparison;
 			return d == 0.f ? 0 : (d > 0.f ? 1 : -1);
		}
	case TK2(HF64,HF64):
		{
			double d = a->v.d - b->v.d;
			if( d != d ) return hl_invalid_comparison;
 			return d == 0. ? 0 : (d > 0. ? 1 : -1);
		}
	case TK2(HBOOL,HBOOL):
		return a->v.b - b->v.b;
	case TK2(HF64, HI32):
		{
			double d = a->v.d - (double)b->v.i;
			if( d != d ) return hl_invalid_comparison;
 			return d == 0. ? 0 : (d > 0. ? 1 : -1);
		}
	case TK2(HI32, HF64):
		{
			double d = (double)a->v.i - b->v.d;
			if( d != d ) return hl_invalid_comparison;
 			return d == 0. ? 0 : (d > 0. ? 1 : -1);
		}
	case TK2(HOBJ,HOBJ):
		if( a->t->obj == b->t->obj && a->t->obj->rt->compareFun )
			return a->t->obj->rt->compareFun(a,b);
		break;
	}
	uprintf(USTR("Don't know how to compare %s(%s) and %s(%s)\n"),hl_to_string(a),hl_type_str(a->t),hl_to_string(b),hl_type_str(b->t));
	return hl_invalid_comparison;
}

HL_PRIM vdynamic* hl_safe_cast( vdynamic *v, hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM bool hl_type_check( hl_type *t, vdynamic *value ) {
	hl_fatal("TODO");
	return false;
}
