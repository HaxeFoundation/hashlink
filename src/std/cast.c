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
	case TK2(HI32,HI32):
		return *(int*)data;
	default:
		switch( t->kind ) {
		case HNULL:
			{
				vdynamic *v = *(vdynamic**)data;
				if( v == NULL ) return 0;
				return hl_dyn_casti(&v->v,t->t,to);
			}
		}
		break;
	}
	hl_error_msg(USTR("Can't cast %s(%s) to %s"),hl_to_string(hl_make_dyn(data,t)),hl_type_str(t),hl_type_str(to));
	return 0;
}

void *hl_dyn_castp( void *data, hl_type *t, hl_type *to ) {
	if( t == to )
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
	}
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

HL_PRIM vdynamic* hl_safe_cast( vdynamic *v, hl_type *t ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM bool hl_type_check( hl_type *t, vdynamic *value ) {
	hl_fatal("TODO");
	return false;
}
