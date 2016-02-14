#include <hl.h>

typedef struct _hl_bytes_map hl_bytes_map;

struct _hl_bytes_map {
	int _;
};

HL_PRIM hl_bytes_map *hl_hballoc() {
	hl_bytes_map *b = (hl_bytes_map*)malloc(sizeof(hl_bytes_map));
	return b;
}

HL_PRIM void hl_hbset( hl_bytes_map *m, vbytes *key, vdynamic *val ) {
	hl_fatal("TODO");
}
