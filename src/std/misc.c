#include <hl.h>

static void do_log( vdynamic *v ) {
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
	default:
		printf(_PTR_FMT "H\n",(int_val)v->v.ptr);
		break;
	}
}

DEFINE_PRIM_WITH_NAME(_VOID,do_log,_DYN,log);
