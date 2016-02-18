#include <hl.h>

HL_PRIM vdynamic *hl_obj_get_field( vdynamic *obj, int hfield ) {
	return hl_dyn_getp(obj,hfield,&hlt_dyn);
}

HL_PRIM void hl_obj_set_field( vdynamic *obj, int hfield, vdynamic *v ) {
	hl_dyn_setp(obj,hfield,v->t,&v);
}

HL_PRIM bool hl_obj_has_field( vdynamic *obj, int hfield ) {
	hl_fatal("TODO");
	return false;
}

HL_PRIM bool hl_obj_delete_field( vdynamic *obj, int hfield ) {
	hl_fatal("TODO");
	return false;
}

HL_PRIM varray *hl_obj_fields( vdynamic *obj ) {
	hl_fatal("TODO");
	return NULL;
}
