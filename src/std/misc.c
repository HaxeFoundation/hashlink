#include <hl.h>

static void do_log( vdynamic *v ) {
	if( v == NULL ) {
		printf("null\n");
		return;
	}
	switch( (*v->t)->kind ) {
	case HI32:
		printf("%di\n",v->v.i);
		break;
	case HF64:
		printf("%.19gf\n",v->v.d);
		break;
	case HVOID:
		printf("void\n");
		break;
	case HBYTES:
		printf("[%s]\n",(char*)v->v.bytes);
		break;
	case HBOOL:
		printf("%s\n",v->v.b ? "true" : "false");
		break;
	case HOBJ:
		{
			hl_type_obj *o = ((vobj*)v)->proto->t->obj;
			if( o->rt == NULL || o->rt->toString == NULL )
				printf("#%s\n",o->name);
			else
				printf("[%s]\n",(char*)hl_callback(o->rt->toString,1,&v));
		}
		break;
	case HVIRTUAL:
		printf("virtual:");
		do_log(((vvirtual*)v)->original);
		break;
	default:
		printf(_PTR_FMT "H\n",(int_val)v->v.ptr);
		break;
	}
}

static void *do_itos( int i, int *len ) {
	char tmp[12];
	*len = sprintf(tmp,"%d",i);
	return hl_copy_bytes(tmp,*len + 1);
}

static void *do_ftos( double d, int *len ) {
	char tmp[24];
	*len = sprintf(tmp,"%.16g",d);
	return hl_copy_bytes(tmp,*len + 1);
}

DEFINE_PRIM_WITH_NAME(_VOID,do_log,_DYN,log);
DEFINE_PRIM_WITH_NAME(_BYTES,do_itos,_I32 _REF(_I32),itos);
DEFINE_PRIM_WITH_NAME(_BYTES,do_ftos,_F64 _REF(_I32),ftos);
