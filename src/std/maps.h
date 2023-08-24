
#undef t_map
#undef t_entry
#undef t_value
#undef t_key
#define t_key _MKEY_TYPE
#define t_map _MNAME(_map)
#define t_entry _MNAME(_entry)
#define t_value _MNAME(_value)
#ifdef _MNO_EXPORTS
#define _MSTATIC
#else
#define _MSTATIC static
#endif

typedef struct {
	t_entry *entries;
	t_value *values;
	int *psl;
	int nbuckets;
	int nentries;
	int maxentries;
} t_map;

#ifndef _MNO_EXPORTS
HL_PRIM 
#endif
t_map *_MNAME(alloc)() {
	t_map *m = (t_map*)hl_gc_alloc_raw(sizeof(t_map));
	memset(m,0,sizeof(t_map));
	return m;
}

_MSTATIC _MVAL_TYPE *_MNAME(find)( t_map *m, t_key key ) {
	int c, dc = 0;
	unsigned int hash;

	if (m->nbuckets == 0) return NULL;
	hash = _MNAME(hash)(key);
	c = hash % ((unsigned)m->nbuckets);
	while( m->psl[c] >= 0 && dc <= m->psl[c] ) {
		if (_MMATCH(c))
			return &m->values[c].value;
		dc++;
		c = (c + 1) % m->nbuckets;
	}
	return NULL;
}

static void _MNAME(resize)( t_map *m );

_MSTATIC void _MNAME(set_impl)( t_map *m, t_key key, _MVAL_TYPE value ) {
	int c = -1, prev = -1, dc = 0;
	unsigned int hash = _MNAME(hash)(key);
	if( m->nbuckets > 0 ) {
		c = hash % ((unsigned)m->nbuckets);
		while (m->psl[c] >= 0 && dc <= m->psl[c]) {
			if (_MMATCH(c)) {
				m->values[c].value = value;
				return;
			}
			dc++;
			prev = c;
			c = (c + 1) % m->nbuckets;
		}
		if (prev > 0) {
			dc--;
			c = prev;
		}
	}
	if (m->nentries >= m->maxentries) {
		_MNAME(resize)(m);
		dc = 0;
		c = hash % ((unsigned)m->nbuckets);
	}
	while (m->psl[c] >= 0) {
		if (dc > m->psl[c]) {
			t_key key_tmp = _MKEY(m, c);
			_MVAL_TYPE value_tmp = m->values[c].value;
			int dc_tmp = m->psl[c];
			_MSET(c);
			m->values[c].value = value;
			m->psl[c] = dc;
			key = key_tmp;
			value = value_tmp;
			dc = dc_tmp;
			hash = _MNAME(hash)(key);
		}
		dc++;
		c = (c + 1) % m->nbuckets;
	}
	_MSET(c);
	m->values[c].value = value;
	m->psl[c] = dc;
	m->nentries++;
}

static void _MNAME(resize)( t_map *m ) {
	// save
	t_map old = *m;

	if( m->nentries != m->maxentries ) hl_error("assert");

	// resize
	int nbuckets = old.nbuckets ? old.nbuckets << 1 : H_SIZE_INIT;
	m->entries = (t_entry *)hl_gc_alloc_noptr(nbuckets * sizeof(t_entry));
	m->values = (t_value *)hl_gc_alloc_raw(nbuckets * sizeof(t_value));
	m->psl = (int *)hl_gc_alloc_noptr(nbuckets * sizeof(int));
	m->nbuckets = nbuckets;
	m->maxentries = nbuckets * 11 >> 4;

	// remap
	m->nentries = 0;
	memset(m->values, 0, nbuckets * sizeof(t_value));
	memset(m->psl, -1, nbuckets * sizeof(int));
	for (int c = 0; c < old.nbuckets; c++) {
		if (old.psl[c] >= 0) {
			_MNAME(set_impl)(m, _MKEY((&old), c), old.values[c].value);
		}
	}
}

#ifndef _MNO_EXPORTS

HL_PRIM void _MNAME(set)( t_map *m, t_key key, _MVAL_TYPE value ) {
	_MNAME(set_impl)(m,_MNAME(filter)(key),value);
}

HL_PRIM bool _MNAME(exists)( t_map *m, t_key key ) {
	return _MNAME(find)(m,_MNAME(filter)(key)) != NULL;
}

HL_PRIM vdynamic* _MNAME(get)( t_map *m, t_key key ) {
	vdynamic **v = _MNAME(find)(m,_MNAME(filter)(key));
	if( v == NULL ) return NULL;
	return *v;
}

HL_PRIM bool _MNAME(remove)( t_map *m, t_key key ) {
	int c, dc = 0;
	unsigned int hash;
	if( m->nentries == 0 ) return false;
	key = _MNAME(filter)(key);
	hash = _MNAME(hash)(key);
	c = hash % ((unsigned)m->nbuckets);
	while (m->psl[c] >= 0 && dc <= m->psl[c]) {
		if (_MMATCH(c)) {
			m->nentries--;
			_MERASE(c);
			m->values[c].value = NULL;
			m->psl[c] = -1;
			// Move all following elements
			int next = (c + 1) % m->nbuckets;
			while (m->psl[next] > 0) {
				key = _MKEY(m, next);
				hash = _MNAME(hash)(key);
				_MSET(c);
				_MERASE(next);
				m->values[c].value = m->values[next].value;
				m->values[next].value = NULL;
				m->psl[c] = m->psl[next] - 1;
				m->psl[next] = -1;
				c = next;
				next = (c + 1) % m->nbuckets;
			}
			return true;
		}
		dc++;
		c = (c + 1) % m->nbuckets;
	}
	return false;
}

HL_PRIM varray* _MNAME(keys)( t_map *m ) {
	varray *a = hl_alloc_array(&hlt_key, m->nentries);
	t_key *keys = hl_aptr(a, t_key);
	int p = 0;
	for (int c = 0; c < m->nbuckets; c++) {
		if (m->psl[c] >= 0) {
			keys[p++] = _MKEY(m, c);
		}
	}
	return a;
}

HL_PRIM varray* _MNAME(values)( t_map *m ) {
	varray *a = hl_alloc_array(&hlt_dyn, m->nentries);
	vdynamic **values = hl_aptr(a, vdynamic*);
	int p = 0;
	for (int c = 0; c < m->nbuckets; c++) {
		if (m->psl[c] >= 0) {
			values[p++] = m->values[c].value;
		}
	}
	return a;
}

HL_PRIM void _MNAME(clear)( t_map *m ) {
	memset(m,0,sizeof(t_map));
}

HL_PRIM int _MNAME(size)( t_map *m ) {
	return m->nentries;
}


#endif

#undef hlt_key
#undef _MKEY_TYPE
#undef _MNAME
#undef _MMATCH
#undef _MKEY
#undef _MSET
#undef _MERASE
#undef _MOLD_KEY
#undef _MINDEX
#undef _MNEXT
#undef _MSTATIC
