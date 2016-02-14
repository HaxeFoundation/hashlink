#include <hl.h>

/*
HL_PRIM void do_log( vdynamic *v ) {
	printf("%s\n",hl_to_string(v));
}

HL_PRIM int utf8length( unsigned char *s, int pos, int l ) {
	int count = 0;
	s += pos;
	while( l ) {
		unsigned char c = *s;
		count++;
		if( c < 0x7F ) {
			l--;
			s++;
		} else if( c < 0xC0 )
			hl_error("Invalid utf8 string");
		else if( c < 0xE0 ) {
			l-=2;
			s+=2;
		} else if( c < 0xF0 ) {
			l-=3;
			s+=3;
		} else {
			l-=4;
			s+=4;
		}
	}
	return count;
}

*/

HL_PRIM vbytes *hl_itos( int i, int *len ) {
	uchar tmp[24];
	*len = (int)usprintf(tmp,24,USTR("%d"),i);
	return hl_bcopy((vbytes*)tmp,(*len + 1) << 1);
}

HL_PRIM vbytes *hl_ftos( double d, int *len ) {
	uchar tmp[48];
	*len = (int)usprintf(tmp,48,USTR("%.16g"),d);
	return hl_bcopy((vbytes*)tmp,(*len + 1) << 1);
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
			return hl_buffer_content(b,len);
		}
	}
}

HL_PRIM int hl_ucs2length( vbytes *str, int pos ) {
	hl_fatal("TODO");
	return 0;
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
