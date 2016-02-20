#include <hl.h>

typedef struct _ereg {
	int _;
} ereg;

HL_PRIM ereg *regexp_regexp_new_options( vbytes *str, vbytes *opts ) {
	return NULL;
}

HL_PRIM int regexp_regexp_matched_pos( ereg *e, int pos, int *len ) {
	return 0;
}

HL_PRIM vbytes *regexp_regexp_matched( ereg *e, int pos, int *len ) {
	return NULL;
}

HL_PRIM bool regexp_regexp_match( ereg *e, vbytes *s, int pos, int len ) {
	return false;
}

