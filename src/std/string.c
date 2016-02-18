#include <hl.h>

HL_PRIM vbytes *hl_itos( int i, int *len ) {
	uchar tmp[24];
	int k = (int)usprintf(tmp,24,USTR("%d"),i);
	*len = k;
	return hl_bcopy((vbytes*)tmp,(k + 1)<<1);
}

HL_PRIM vbytes *hl_ftos( double d, int *len ) {
	uchar tmp[48];
	int k = (int)usprintf(tmp,48,USTR("%.16g"),d);
	*len = k;
	return hl_bcopy((vbytes*)tmp,(k + 1) << 1);
}

HL_PRIM vbytes *hl_value_to_string( vdynamic *d, int *len ) {
	if( d == NULL ) {
		*len = 4;
		return (vbytes*)USTR("null");
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
			return (vbytes*)hl_buffer_content(b,len);
		}
	}
}

HL_PRIM int hl_ucs2length( vbytes *str, int pos ) {
	return (int)ustrlen((uchar*)(str + pos));
}

HL_PRIM vbytes* hl_utf8_to_utf16( vbytes *str, int pos, int *len ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vbytes* hl_ucs2_upper( vbytes *str, int pos, int len ) {
	hl_fatal("TODO");
	return NULL;
}

HL_PRIM vbytes* hl_ucs2_lower( vbytes *str, int pos, int len ) {
	hl_fatal("TODO");
	return NULL;
}

DEFINE_PRIM(_BYTES,hl_itos,_I32 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_ftos,_F64 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_value_to_string,_DYN _REF(_I32));
DEFINE_PRIM(_I32,hl_ucs2length,_BYTES _I32);
DEFINE_PRIM(_BYTES,hl_utf8_to_utf16,_BYTES _I32 _REF(_I32));
DEFINE_PRIM(_BYTES,hl_ucs2_upper,_BYTES _I32 _I32);
DEFINE_PRIM(_BYTES,hl_ucs2_lower,_BYTES _I32 _I32);
