#include <hl.h>

HL_PRIM void do_log( vdynamic *v ) {
	printf("%s\n",hl_to_string(v));
}

HL_PRIM void *do_itos( int i, int *len ) {
	char tmp[12];
	*len = sprintf(tmp,"%d",i);
	return hl_copy_bytes(tmp,*len + 1);
}

HL_PRIM void *do_ftos( double d, int *len ) {
	char tmp[24];
	*len = sprintf(tmp,"%.16g",d);
	return hl_copy_bytes(tmp,*len + 1);
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

HL_PRIM void *value_to_string( vdynamic *d, int *len ) {
	if( d == NULL ) {
		*len = 4;
		return "null";
	}
	switch( d->t->kind ) {
	case HI32:
		return do_itos(d->v.i,len);
	case HF64:
		return do_ftos(d->v.d,len);
	default:
		{
			hl_buffer *b = hl_alloc_buffer();
			hl_buffer_val(b, d);
			return hl_buffer_content(b,len);
		}
	}
}

DEFINE_PRIM_WITH_NAME(_VOID,do_log,_DYN,log);
DEFINE_PRIM_WITH_NAME(_BYTES,do_itos,_I32 _REF(_I32),itos);
DEFINE_PRIM_WITH_NAME(_BYTES,do_ftos,_F64 _REF(_I32),ftos);
DEFINE_PRIM(_I32,utf8length,_BYTES _I32 _I32);
DEFINE_PRIM(_BYTES,value_to_string,_DYN _REF(_I32));
