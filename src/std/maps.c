/*
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <hl.h>
#ifdef HL_VCC
#	pragma warning(disable:4034) // sizeof(void) == 0
#endif

#define H_SIZE_INIT 8

#define _MVAL_TYPE vdynamic*

// ----- INT MAP ---------------------------------

typedef struct {
	int key;
} hl_hi_entry;

typedef struct {
	vdynamic *value;
} hl_hi_value;

#define hlt_key		hlt_i32
#define hl_hifilter(key) key
#define hl_hihash(h)	((unsigned)(h))
#define _MKEY_TYPE	int
#define _MNAME(n)	hl_hi##n
#define _MMATCH(c)	m->entries[c].key == key
#define _MKEY(m,c)	m->entries[c].key
#define	_MSET(c)	m->entries[c].key = key
#define _MERASE(c)

#include "maps.h"


// ----- INT64 MAP ---------------------------------

typedef struct {
	int64 key;
} hl_hi64_entry;

typedef struct {
	vdynamic *value;
} hl_hi64_value;

#define hlt_key		hlt_i64
#define hl_hi64filter(key) key
#define hl_hi64hash(h)	(((unsigned int)h) ^ ((unsigned int)(h>>32)))
#define _MKEY_TYPE	int64
#define _MNAME(n)	hl_hi64##n
#define _MMATCH(c)	m->entries[c].key == key
#define _MKEY(m,c)	m->entries[c].key
#define	_MSET(c)	m->entries[c].key = key
#define _MERASE(c)

#include "maps.h"

// ----- BYTES MAP ---------------------------------

typedef struct {
	unsigned int hash;
} hl_hb_entry;

typedef struct {
	uchar *key;
	vdynamic *value;
} hl_hb_value;

#define hlt_key		hlt_bytes
#define hl_hbfilter(key) key
#define hl_hbhash(key)	((unsigned)hl_hash_gen(key,false))
#define _MKEY_TYPE	uchar*
#define _MNAME(n)	hl_hb##n
#define _MMATCH(c)	m->entries[c].hash == hash && ucmp(m->values[c].key,key) == 0
#define _MKEY(m,c)	m->values[c].key
#define	_MSET(c)	m->entries[c].hash = hash; m->values[c].key = key
#define _MERASE(c)  m->values[c].key = NULL

#include "maps.h"

// ----- OBJECT MAP ---------------------------------

typedef void hl_ho_entry;

typedef struct {
	vdynamic *key;
	vdynamic *value;
} hl_ho_value;

static vdynamic *hl_hofilter( vdynamic *key ) {
	if( key )
		switch( key->t->kind ) {
		// erase virtual (prevent mismatch once virtualized)
		case HVIRTUAL:
			key = hl_virtual_make_value((vvirtual*)key);
			break;
		// store real pointer instead of dynamic wrapper
		case HBYTES:
		case HTYPE:
		case HABSTRACT:
		case HREF:
			key = (vdynamic*)key->v.ptr;
			break;
		default:
			break;
		}
	return key;
}

#define hlt_key		hlt_dyn
#define hl_hohash(key)	((unsigned int)(int_val)(key))
#define _MKEY_TYPE	vdynamic*
#define _MNAME(n)	hl_ho##n
#define _MMATCH(c)	m->values[c].key == key
#define _MKEY(m,c)	m->values[c].key
#define	_MSET(c)	m->values[c].key = key
#define _MERASE(c)  m->values[c].key = NULL

#include "maps.h"

// ----- LOOKUP MAP ---------------------------------

#undef _MVAL_TYPE
#define _MVAL_TYPE int

typedef struct {
	void *key;
} hl_mlookup__entry;

typedef struct {
	int value;
} hl_mlookup__value;

#define hl_mlookup_hash(h) ((unsigned int)(int_val)(h))
#define _MKEY_TYPE	void*
#define _MNAME(n)	hl_mlookup_##n
#define _MMATCH(c)	m->entries[c].key == key
#define _MKEY(m,c)	m->entries[c].key
#define	_MSET(c)	m->entries[c].key = key
#define _MERASE(c)
#define _MNO_EXPORTS

#include "maps.h"

/// ----------------------------------------------

#define _IMAP _ABSTRACT(hl_int_map)
DEFINE_PRIM( _IMAP, hialloc, _NO_ARG );
DEFINE_PRIM( _VOID, hiset, _IMAP _I32 _DYN );
DEFINE_PRIM( _BOOL, hiexists, _IMAP _I32 );
DEFINE_PRIM( _DYN, higet, _IMAP _I32 );
DEFINE_PRIM( _BOOL, hiremove, _IMAP _I32 );
DEFINE_PRIM( _ARR, hikeys, _IMAP );
DEFINE_PRIM( _ARR, hivalues, _IMAP );
DEFINE_PRIM( _VOID, hiclear, _IMAP );
DEFINE_PRIM( _I32, hisize, _IMAP );

#define _I64MAP _ABSTRACT(hl_int64_map)
DEFINE_PRIM( _I64MAP, hi64alloc, _NO_ARG );
DEFINE_PRIM( _VOID, hi64set, _I64MAP _I64 _DYN );
DEFINE_PRIM( _BOOL, hi64exists, _I64MAP _I64 );
DEFINE_PRIM( _DYN, hi64get, _I64MAP _I64 );
DEFINE_PRIM( _BOOL, hi64remove, _I64MAP _I64 );
DEFINE_PRIM( _ARR, hi64keys, _I64MAP );
DEFINE_PRIM( _ARR, hi64values, _I64MAP );
DEFINE_PRIM( _VOID, hi64clear, _I64MAP );
DEFINE_PRIM( _I32, hi64size, _I64MAP );

#define _BMAP _ABSTRACT(hl_bytes_map)
DEFINE_PRIM( _BMAP, hballoc, _NO_ARG );
DEFINE_PRIM( _VOID, hbset, _BMAP _BYTES _DYN );
DEFINE_PRIM( _BOOL, hbexists, _BMAP _BYTES );
DEFINE_PRIM( _DYN, hbget, _BMAP _BYTES );
DEFINE_PRIM( _BOOL, hbremove, _BMAP _BYTES );
DEFINE_PRIM( _ARR, hbkeys, _BMAP );
DEFINE_PRIM( _ARR, hbvalues, _BMAP );
DEFINE_PRIM( _VOID, hbclear, _BMAP );
DEFINE_PRIM( _I32, hbsize, _BMAP );

#define _OMAP _ABSTRACT(hl_obj_map)
DEFINE_PRIM( _OMAP, hoalloc, _NO_ARG );
DEFINE_PRIM( _VOID, hoset, _OMAP _DYN _DYN );
DEFINE_PRIM( _BOOL, hoexists, _OMAP _DYN );
DEFINE_PRIM( _DYN, hoget, _OMAP _DYN );
DEFINE_PRIM( _BOOL, horemove, _OMAP _DYN );
DEFINE_PRIM( _ARR, hokeys, _OMAP );
DEFINE_PRIM( _ARR, hovalues, _OMAP );
DEFINE_PRIM( _VOID, hoclear, _OMAP );
DEFINE_PRIM( _I32, hosize, _OMAP );
