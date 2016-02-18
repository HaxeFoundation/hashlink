#include "hl.h"
#include <stdarg.h>
#include <string.h>

void *hl_fatal_error( const char *msg, const char *file, int line ) {
	printf("%s(%d) : FATAL ERROR : %s\n",file,line,msg);
#ifdef _DEBUG
	*(int*)NULL = 0;
#else
	exit(0);
#endif
	return NULL;
}

void hl_global_init() {
}

void hl_cache_free();

void hl_global_free() {
	hl_cache_free();
}

int hl_type_size( hl_type *t ) {
	static int SIZES[] = {
		0, // VOID
		1, // I8
		2, // I16
		4, // I32
		4, // F32
		8, // F64
		1, // BOOL
		HL_WSIZE, // BYTES
		HL_WSIZE, // DYN
		HL_WSIZE, // FUN
		HL_WSIZE, // OBJ
		HL_WSIZE, // ARRAY
		HL_WSIZE, // TYPE
		HL_WSIZE, // REF
		HL_WSIZE, // VIRTUAL
		HL_WSIZE, // DYNOBJ
		HL_WSIZE, // ABSTRACT
		HL_WSIZE, // ENUM
		HL_WSIZE, // NULL
	};
	return SIZES[t->kind];
}

int hl_pad_size( int pos, hl_type *t ) {
	int sz = hl_type_size(t);
	int align;
	align = pos & (sz - 1);
	if( align && t->kind != HVOID )
		return sz - align;
	return 0;
}

void hl_throw( vdynamic *v ) {
	*(char*)NULL = 0;
}

void hl_error_msg( const uchar *fmt, ... ) {
	uchar buf[256];
	va_list args;
	va_start(args, fmt);
	uvsprintf(buf,fmt,args);
	va_end(args);
	// TODO : throw
	uprintf(USTR("THROW:%s\n"),buf);
	exit(66);
}

void hl_fatal_fmt(const char *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf,fmt, args);
	va_end(args);
	hl_fatal(buf);
}

typedef struct _stringitem {
	uchar *str;
	int size;
	int len;
	struct _stringitem *next;
} * stringitem;

struct hl_buffer {
	int totlen;
	int blen;
	stringitem data;
};

hl_buffer *hl_alloc_buffer() {
	hl_buffer *b = (hl_buffer*)hl_gc_alloc(sizeof(hl_buffer));
	b->totlen = 0;
	b->blen = 16;
	b->data = NULL;
	return b;
}

static void buffer_append_new( hl_buffer *b, const uchar *s, int len ) {
	int size;
	stringitem it;
	while( b->totlen >= (b->blen << 2) )
		b->blen <<= 1;
	size = (len < b->blen)?b->blen:len;
	it = (stringitem)hl_gc_alloc(sizeof(struct _stringitem));
	it->str = (uchar*)hl_gc_alloc_noptr(size<<1);
	memcpy(it->str,s,len<<1);
	it->size = size;
	it->len = len;
	it->next = b->data;
	b->data = it;
}

void hl_buffer_str_sub( hl_buffer *b, const uchar *s, int len ) {
	stringitem it;
	if( s == NULL || len <= 0 )
		return;
	b->totlen += len;
	it = b->data;
	if( it ) {
		int free = it->size - it->len;
		if( free >= len ) {
			memcpy(it->str + it->len,s,len<<1);
			it->len += len;
			return;
		} else {
			memcpy(it->str + it->len,s,free<<1);
			it->len += free;
			s += free;
			len -= free;
		}
	}
	buffer_append_new(b,s,len);
}

void hl_buffer_str( hl_buffer *b, const uchar *s ) {
	if( s ) hl_buffer_str_sub(b,s,(int)ustrlen(s));
}

void hl_buffer_char( hl_buffer *b, uchar c ) {
	stringitem it;
	b->totlen++;
	it = b->data;
	if( it && it->len != it->size ) {
		it->str[it->len++] = c;
		return;
	}
	buffer_append_new(b,(uchar*)&c,1);
}

uchar *hl_buffer_content( hl_buffer *b, int *len ) {
	uchar *buf = (uchar*)hl_gc_alloc_noptr((b->totlen+1)<<1);
	stringitem it = b->data;
	uchar *s = ((uchar*)buf) + b->totlen;
	*s = 0;
	while( it != NULL ) {
		stringitem tmp;
		s -= it->len;
		memcpy(s,it->str,it->len<<1);
		tmp = it->next;
		it = tmp;
	}
	if( len ) *len = b->totlen;
	return buf;
}

int hl_buffer_length( hl_buffer *b ) {
	return b->totlen;
}

typedef struct vlist {
	vdynamic *v;
	struct vlist *next;
} vlist;

static void hl_buffer_rec( hl_buffer *b, vdynamic *v, vlist *stack );

static void hl_buffer_addr( hl_buffer *b, void *data, hl_type *t, vlist *stack ) {
	uchar buf[32];
	switch( t->kind ) {
	case HI32:
		hl_buffer_str_sub(b,buf,usprintf(buf,32,USTR("%d"),*(int*)data));
		break;
	case HF64:
		hl_buffer_str_sub(b,buf,usprintf(buf,32,USTR("%d"),*(double*)data));
		break;
	case HBYTES:
		hl_buffer_str(b,*(uchar**)data);
		break;
	case HBOOL:
		if( *(unsigned char*)data )
			hl_buffer_str_sub(b,USTR("true"),4);
		else
			hl_buffer_str_sub(b,USTR("false"),5);
		break;
	default:
		hl_buffer_rec(b, *(vdynamic**)data, stack);
		break;
	}
}

static void hl_buffer_rec( hl_buffer *b, vdynamic *v, vlist *stack ) {
	uchar buf[32];
	if( v == NULL ) {
		hl_buffer_str_sub(b,USTR("null"),4);
		return;
	}
	switch( v->t->kind ) {
	case HI32:
		hl_buffer_str_sub(b,buf,usprintf(buf,32,USTR("%d"),v->v.i));
		break;
	case HBYTES:
		hl_buffer_str(b,(uchar*)v->v.bytes);
		break;
	case HF64:
		hl_buffer_str_sub(b,buf,usprintf(buf,32,USTR("%.19g"),v->v.d));
		break;
	case HVOID:
		hl_buffer_str_sub(b,USTR("void"),4);
		break;
	case HBOOL:
		if( v->v.b )
			hl_buffer_str_sub(b,USTR("true"),4);
		else
			hl_buffer_str_sub(b,USTR("false"),5);
		break;
	case HOBJ:
		{
			hl_type_obj *o = ((vobj*)v)->proto->t.obj;
			if( o->rt == NULL || o->rt->toString == NULL ) {
				hl_buffer_char(b,'#');
				hl_buffer_str(b,o->name);
			} else
				hl_buffer_str(b,o->rt->toString(v));
		}
		break;
	case HVIRTUAL:
		hl_buffer_rec(b, ((vvirtual*)v)->value, stack);
		break;
	case HARRAY:
		{
			int i;
			varray *a = (varray*)v;
			hl_type *at = a->at;
			int stride = hl_type_size(at);
			vlist l;
			vlist *vtmp = stack;
			while( vtmp != NULL ) {
				if( vtmp->v == v ) {
					hl_buffer_str_sub(b,USTR("..."),3);
					return;
				}
				vtmp = vtmp->next;
			}
			l.v = v;
			l.next = stack;
			hl_buffer_char(b,'[');
			for(i=0;i<a->size;i++) {
				if( i )
					hl_buffer_str_sub(b,USTR(", "),2);
				hl_buffer_addr(b,(char*)(a + 1) + i * stride,at,&l);
			}
			hl_buffer_char(b,']');
		}
		break;
	case HDYNOBJ:
		{
			vdynobj *o = (vdynobj*)v;
			int i;
			vlist l;
			vlist *vtmp = stack;
			while( vtmp != NULL ) {
				if( vtmp->v == v ) {
					hl_buffer_str_sub(b,USTR("..."),3);
					return;
				}
				vtmp = vtmp->next;
			}
			l.v = v;
			l.next = stack;
			hl_buffer_char(b, '{');
			for(i=0;i<o->nfields;i++) {
				hl_field_lookup *f = &o->dproto->fields + i;
				if( i ) hl_buffer_str_sub(b,USTR(", "),2);
				hl_buffer_str(b,hl_field_name(f->hashed_name));
				hl_buffer_str_sub(b,USTR(" : "),3);
				hl_buffer_addr(b, o->fields_data + f->field_index, f->t, stack);
			}
			hl_buffer_char(b, '}');
		}
		break;
	default:
		hl_buffer_str_sub(b, buf, usprintf(buf, 32, _PTR_FMT USTR("H\n"),(int_val)v));
		break;
	}
}

void hl_buffer_val( hl_buffer *b, vdynamic *v ) {
	hl_buffer_rec(b,v,NULL);
}

uchar *hl_to_string( vdynamic *v ) {
	hl_buffer *b = hl_alloc_buffer();
	hl_buffer_val(b,v);
	hl_buffer_char(b,0);
	return hl_buffer_content(b,NULL);
}
