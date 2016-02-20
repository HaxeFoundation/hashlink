#include <hl.h>

typedef struct _hl_bytes_map hl_bytes_map;
typedef struct _hl_int_map hl_int_map;

struct _hl_bytes_map {
	int _;
};

struct _hl_int_map {
	int _;
};

HL_PRIM hl_bytes_map *hl_hballoc() {
	hl_bytes_map *m = (hl_bytes_map*)hl_gc_alloc(sizeof(hl_bytes_map));
	return m;
}

HL_PRIM void hl_hbset( hl_bytes_map *m, vbytes *key, vdynamic *val ) {
}


HL_PRIM bool hl_hbexists( hl_bytes_map *m, vbytes *key ) {
	return false;
}

HL_PRIM vdynamic *hl_hbget( hl_bytes_map *m, vbytes *key ) {
	return NULL;
}

HL_PRIM bool hl_hbremove( hl_bytes_map *m, vbytes *key ) {
	return false;
}

HL_PRIM varray *hl_hbkeys( hl_bytes_map *m ) {
	return NULL;
}

HL_PRIM varray *hl_hbvalues( hl_bytes_map *m ) {
	return NULL;
}

// ----- INT MAP ---------------------------------

HL_PRIM hl_int_map *hl_hialloc() {
	hl_int_map *m = (hl_int_map*)hl_gc_alloc(sizeof(hl_int_map));
	return m;
}

HL_PRIM void hl_hiset( hl_int_map *m, int key, vdynamic *value ) {
}

HL_PRIM bool hl_hiexists( hl_int_map *m, int key ) {
	return false;
}

HL_PRIM vdynamic* hl_higet( hl_int_map *m, int key ) {
	return NULL;
}

HL_PRIM bool hl_hiremove( hl_int_map *m, int key ) {
	return false;
}

HL_PRIM varray* hl_hikeys( hl_int_map *m ) {
	return NULL;
}

HL_PRIM varray* hl_hivalues( hl_int_map *m ) {
	return NULL;
}

