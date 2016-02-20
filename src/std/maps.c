#include <hl.h>

#define H_SIZE_INIT 7

// successive primes that double every time
static int H_PRIMES[] = {
	7,17,37,79,163,331,673,1361,2729,5471,10949,21911,43853,87613,175229,350459,700919,1401857,2803727,5607457,11214943,22429903,44859823,89719661,179424673,373587883,776531401,1611623773
};

typedef struct _hl_bytes_map hl_bytes_map;

struct _hl_bytes_map {
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

typedef struct _hl_int_map hl_int_map;
typedef struct _hl_int_cell hl_int_cell;

#define ICELL_SIZE	3

struct _hl_int_cell {
	int nvalues;
	int keys[ICELL_SIZE];
	vdynamic *values[ICELL_SIZE];
	hl_int_cell *next;
};

struct _hl_int_map {
	hl_int_cell **cells;
	int ncells;
	int nentries;
};

HL_PRIM hl_int_map *hl_hialloc() {
	hl_int_map *m = (hl_int_map*)hl_gc_alloc(sizeof(hl_int_map));
	m->ncells = H_SIZE_INIT;
	m->nentries = 0;
	m->cells = (hl_int_cell **)hl_gc_alloc(sizeof(hl_int_cell*)*m->ncells);
	memset(m->cells,0,m->ncells * sizeof(void*));
	return m;
}

static vdynamic **hl_hifind( hl_int_map *m, int key ) {
	int ckey = ((unsigned)key) % ((unsigned)m->ncells);
	hl_int_cell *c = m->cells[ckey];
	int i;
	while( c ) {
		for(i=0;i<c->nvalues;i++)
			if( c->keys[i] == key )
				return c->values + i;
		c = c->next;
	}
	return NULL;
}

static bool hl_hiadd( hl_int_map *m, int key, vdynamic *value, hl_int_cell *reuse ) {
	int ckey = ((unsigned)key) % ((unsigned)m->ncells);
	hl_int_cell *c = m->cells[ckey];
	hl_int_cell *pspace = NULL;
	int i;
	while( c ) {
		for(i=0;i<c->nvalues;i++)
			if( c->keys[i] == key ) {
				c->values[i] = value;
				return false;
			}
		if( !pspace && c->nvalues < ICELL_SIZE ) pspace = c;
		c = c->next;
	}
	if( pspace ) {
		pspace->keys[pspace->nvalues] = key;
		pspace->values[pspace->nvalues] = value;
		pspace->nvalues++;
		m->nentries++;
		return false;
	}
	if( reuse )
		c = reuse;
	else
		c = (hl_int_cell*)hl_gc_alloc(sizeof(hl_int_cell));
	memset(c,0,sizeof(hl_int_cell));
	c->keys[0] = key;
	c->values[0] = value;
	c->nvalues = 1;
	c->next = m->cells[ckey];
	m->cells[ckey] = c;
	m->nentries++;
	return true;
}

static void hl_higrow( hl_int_map *m ) {
	int i = 0;
	int oldsize = m->ncells;
	hl_int_cell **old_cells = m->cells;
	hl_int_cell *reuse = NULL;
	while( H_PRIMES[i] <= m->ncells ) i++;
	m->ncells = H_PRIMES[i];
	m->cells = (hl_int_cell **)hl_gc_alloc(sizeof(hl_int_cell*)*m->ncells);
	memset(m->cells,0,m->ncells * sizeof(void*));
	m->nentries = 0;
	for(i=0;i<oldsize;i++) {
		hl_int_cell *c = old_cells[i];
		while( c ) {
			hl_int_cell *next = c->next;
			int j;
			for(j=0;j<c->nvalues;j++) {
				if( j == c->nvalues-1 ) {
					c->next = reuse;
					reuse = c;
				}
				if( hl_hiadd(m,c->keys[j],c->values[j],reuse) )
					reuse = reuse->next;
			}
			c = next;
		}
	}
}

HL_PRIM void hl_hiset( hl_int_map *m, int key, vdynamic *value ) {
	if( hl_hiadd(m,key,value,NULL) && m->nentries > m->ncells * ICELL_SIZE * 2 )
		hl_higrow(m);
}

HL_PRIM bool hl_hiexists( hl_int_map *m, int key ) {
	return hl_hifind(m,key) != NULL;
}

HL_PRIM vdynamic* hl_higet( hl_int_map *m, int key ) {
	vdynamic **v = hl_hifind(m,key);
	if( v == NULL ) return NULL;
	return *v;
}

HL_PRIM bool hl_hiremove( hl_int_map *m, int key ) {
	int ckey = ((unsigned)key) % ((unsigned)m->ncells);
	hl_int_cell *c = m->cells[ckey];
	hl_int_cell *prev = NULL;
	int i;
	while( c ) {
		for(i=0;i<c->nvalues;i++)
			if( c->keys[i] == key ) {
				c->nvalues--;
				if( c->nvalues ) {
					int j;
					for(j=i;j<c->nvalues;j++) {
						c->keys[j] = c->keys[j+1];
						c->values[j] = c->values[j+1];
					}
					c->values[j] = NULL; // GC friendly
				} else if( prev )
					prev->next = c->next;
				else
					m->cells[ckey] = c->next;
				return true;
			}
		prev = c;
		c = c->next;
	}
	return false;
}

HL_PRIM varray* hl_hikeys( hl_int_map *m ) {
	varray *a = (varray*)hl_gc_alloc_noptr(sizeof(varray)+sizeof(int)*m->nentries);
	int *keys = (int*)(a+1);
	int p = 0;
	int i;
	a->t = &hlt_array;
	a->at = &hlt_i32;
	a->size = m->nentries;
	for(i=0;i<m->ncells;i++) {
		int j;
		hl_int_cell *c = m->cells[i];
		while( c ) {
			for(j=0;j<c->nvalues;j++)
				keys[p++] = c->keys[j];
			c = c->next;
		}
	}
	return a;
}

HL_PRIM varray* hl_hivalues( hl_int_map *m ) {
	varray *a = (varray*)hl_gc_alloc(sizeof(varray)+sizeof(void*)*m->nentries);
	vdynamic **values = (vdynamic**)(a+1);
	int p = 0;
	int i;
	a->t = &hlt_array;
	a->at = &hlt_dyn;
	a->size = m->nentries;
	for(i=0;i<m->ncells;i++) {
		int j;
		hl_int_cell *c = m->cells[i];
		while( c ) {
			for(j=0;j<c->nvalues;j++)
				values[p++] = c->values[j];
			c = c->next;
		}
	}
	return a;
}

