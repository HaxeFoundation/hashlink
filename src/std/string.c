#include <hl.h>

HL_PRIM vbyte *hl_itos( int i, int *len ) {
	uchar tmp[24];
	int k = (int)usprintf(tmp,24,USTR("%d"),i);
	*len = k;
	return hl_bcopy((vbyte*)tmp,(k + 1)<<1);
}

HL_PRIM vbyte *hl_ftos( double d, int *len ) {
	uchar tmp[48];
	int k = (int)usprintf(tmp,48,USTR("%.16g"),d);
	*len = k;
	return hl_bcopy((vbyte*)tmp,(k + 1) << 1);
}

HL_PRIM vbyte *hl_value_to_string( vdynamic *d, int *len ) {
	if( d == NULL ) {
		*len = 4;
		return (vbyte*)USTR("null");
	}
	switch( d->t->kind ) {
	case HI32:
		return hl_itos(d->v.i,len);
	case HF64:
		return hl_ftos(d->v.d,len);
	default:
		{
			hl_buffer *b = hl_alloc_buffer();
			hl_buffer_val(b, d);
			return (vbyte*)hl_buffer_content(b,len);
		}
	}
}

HL_PRIM int hl_ucs2length( vbyte *str, int pos ) {
	return (int)ustrlen((uchar*)(str + pos));
}

HL_PRIM vbyte* hl_utf8_to_utf16( vbyte *str, int pos, int *len ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vbyte* hl_ucs2_upper( vbyte *str, int pos, int len ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vbyte* hl_ucs2_lower( vbyte *str, int pos, int len ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vbyte *hl_utf16_to_utf8( vbyte *str, int pos, int *len ) {
	vbyte *out;
	uchar *c = (uchar*)(str + pos);
	int utf8bytes = 0;
	int p = 0;
	while( true ) {
		unsigned int v = (unsigned int)*c;
		if( v == 0 ) break;
		if( v < 0x80 )
			utf8bytes++;
		else if( v < 0x800 )
			utf8bytes += 2;
		else
			utf8bytes += 3;
		c++;
	}
	out = hl_gc_alloc_noptr(utf8bytes + 1);
	c = (uchar*)(str + pos);
	while( true ) {
		unsigned int v = (unsigned int)*c;
		if( v < 0x80 ) {
			out[p++] = (vbyte)v;
			if( v == 0 ) break;
		} else if( v < 0x800 ) {
			out[p++] = (vbyte)(0xC0|(v>>6));
			out[p++] = (vbyte)(0x80|(v&63));
		} else {
			out[p++] = (vbyte)(0xE0|(v>>12));
			out[p++] = (vbyte)(0x80|((v>>6)&63));
			out[p++] = (vbyte)(0x80|(v&63));
		}
		c++;
	}
	*len = utf8bytes;
	return out;
}


DEFINE_PRIM(_BYTES,hl_itos,_I32 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_ftos,_F64 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_value_to_string,_DYN _REF(_I32));
DEFINE_PRIM(_I32,hl_ucs2length,_BYTES _I32);
DEFINE_PRIM(_BYTES,hl_utf8_to_utf16,_BYTES _I32 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_ucs2_upper,_BYTES _I32 _I32);
DEFINE_PRIM(_BYTES,hl_ucs2_lower,_BYTES _I32 _I32);
