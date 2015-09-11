/*
 * Copyright (C)2015 Haxe Foundation
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
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef enum {
	HVOID	= 0,
	HUI8	= 1,
	HI32	= 2,
	HF32	= 3,
	HF64	= 4,
	HBOOL	= 5,
	HANY	= 6,
	HFUN	= 7,
	// ---------
	HLAST	= 8
} hl_type_kind;

typedef struct hl_type hl_type;

struct hl_type {
	hl_type_kind kind;
	int nargs;
	hl_type **args;
	hl_type *ret;
};

typedef struct {
	const char *name;
	int global;
} hl_native;

typedef struct {
	int version;
	int nstrings;
	int nfloats;
	int ntypes;
	int nglobals;
	int nnatives;
	int nfunctions;
	int entrypoint;
	char *strings_data;
	int  *strings_lens;
	char **strings;
	double *floats;
	hl_type *types;
	hl_type **globals;
	hl_native *natives;
} hl_code;

typedef struct {
	const unsigned char *b;
	int size;
	int pos;
	const char *error;
	hl_code *code;
} hl_reader;

#define HL_VERSION	010

void hl_global_init() {
}

void hl_global_free() {
}

#define READ() hl_read_b(r)
#define INDEX() hl_read_index(r)
#define UINDEX() hl_read_uindex(r)
#define ERROR(msg) r->error = msg;

static unsigned char hl_read_b( hl_reader *r ) {
	if( r->pos >= r->size ) {
		ERROR("No more data");
		return 0;
	}
	return r->b[r->pos++];
}

static void hl_read_bytes( hl_reader *r, void *data, int size ) {
	if( size < 0 ) {
		ERROR("Invalid size");
		return;
	}
	if( r->pos + size > r->size ) {
		ERROR("No more data");
		return;
	}
	memcpy(data,r->b + r->pos, size);
	r->pos += size;
}

static double hl_read_double( hl_reader *r ) {
	union {
		double d;
		char b[8];
	} s;
	hl_read_bytes(r, s.b, 8);
	return s.d;
}

static int hl_read_i32( hl_reader *r ) {
	unsigned char a, b, c, d;
	if( r->pos + 4 > r->size ) {
		ERROR("No more data");
		return 0;
	}
	a = r->b[r->pos++];
	b = r->b[r->pos++];
	c = r->b[r->pos++];
	d = r->b[r->pos++];
	return a | (b<<8) | (c<<16) | (d<<24);
}

static int hl_read_index( hl_reader *r ) {
	unsigned char b = READ();
	if( (b & 0x80) == 0 )
		return b & 0x7F;
	if( (b & 0x40) == 0 ) {
		int v = READ() | ((b & 31) << 8);
		return (b & 0x20) == 0 ? v : -v;
	}
	{
		int c = READ();
		int d = READ();
		int e = READ();
		int v = ((b & 31) << 24) | (c << 16) | (d << 8) | e;
		return (b & 0x20) == 0 ? v : -v;
	}
}

static int hl_read_uindex( hl_reader *r ) {
	int i = hl_read_index(r);
	if( i < 0 ) {
		ERROR("Negative index");
		return 0;
	}
	return i;
}

void hl_code_free( hl_code *c ) {
	int i;
	if( c == NULL ) return;
	if( c->strings_data ) free(c->strings_data);
	if( c->strings_lens ) free(c->strings_lens);
	if( c->strings ) free(c->strings);
	if( c->floats ) free(c->floats);
	if( c->types ) {
		for(i=0;i<c->ntypes;i++) {
			hl_type *t = c->types + i;
			if( t->args ) free(t->args);
		}
		free(c->types);
	}
	if( c->globals ) free(c->globals);
	if( c->natives ) free(c->natives);
	free(c);
}

hl_type *hl_get_type( hl_reader *r ) {
	int i = INDEX();
	if( i < 0 || i >= r->code->ntypes ) {
		ERROR("Invalid type index");
		i = 0;
	}
	return r->code->types + i;
}

const char *hl_get_string( hl_reader *r ) {
	int i = INDEX();
	if( i < 0 || i >= r->code->nstrings ) {
		ERROR("Invalid string index");
		i = 0;
	}
	return r->code->strings[i];
}

int hl_get_global( hl_reader *r ) {
	int g = INDEX();
	if( g < 0 || g >= r->code->nglobals ) {
		ERROR("Invalid global index");
		g = 0;
	}
	return g;
}

void hl_read_type( hl_reader *r, hl_type *t ) {
	int i;
	t->kind = READ();
	if( t->kind >= HLAST ) {
		ERROR("Invalid type");
		return;
	}
	switch( t->kind ) {
	case HFUN:
		t->nargs = READ();
		t->args = (hl_type**)malloc(sizeof(hl_type*)*t->nargs);
		if( t->args == NULL ) {
			ERROR("Out of memory");
			return;
		}
		for(i=0;i<t->nargs;i++)
			t->args[i] = hl_get_type(r);
		t->ret = hl_get_type(r);
		break;
	default:
		break;
	}
}

#define CHK_ERROR() if( r->error ) { hl_code_free(c); printf("%s\n", r->error); return NULL; }
#define EXIT(msg) { ERROR(msg); CHK_ERROR(); }
#define ALLOC(v,ptr,count) { v = (ptr *)malloc(count*sizeof(ptr)); if( v == NULL ) EXIT("Out of memory") else memset(v, 0, count*sizeof(ptr)); }

hl_code *hl_code_read( const unsigned char *data, int size ) {
	hl_reader _r = { data, size, 0, 0, NULL };	
	hl_reader *r = &_r;
	hl_code *c;
	int i;
	ALLOC(c, hl_code, 1);
	if( READ() != 'H' || READ() != 'L' || READ() != 'B' )
		EXIT("Invalid header");
	r->code = c;
	c->version = READ();
	if( c->version <= 0 || c->version > 1 ) {
		printf("VER=%d\n",c->version);
		EXIT("Unsupported version");
	}
	c->nstrings = UINDEX();
	c->nfloats = UINDEX();
	c->ntypes = UINDEX();
	c->nglobals = UINDEX();
	c->nnatives = UINDEX();
	c->nfunctions = UINDEX();
	c->entrypoint = UINDEX();	
	{
		int size = hl_read_i32(r);
		char *sdata;
		CHK_ERROR();
		sdata = (char*)malloc(sizeof(char) * size);
		if( sdata == NULL )
			EXIT("Out of memory");
		hl_read_bytes(r, sdata, size);
		c->strings_data = sdata;
		ALLOC(c->strings, char*, c->nstrings);
		ALLOC(c->strings_lens, int, c->nstrings);
		for(i=0;i<c->nstrings;i++) {
			int sz = UINDEX();
			c->strings[i] = sdata;
			c->strings_lens[i] = sz;
			sdata += sz;
			if( sdata >= c->strings_data + size || *sdata )
				EXIT("Invalid string");
			sdata++;
		}
	}
	CHK_ERROR();
	ALLOC(c->floats, double, sizeof(double)*c->nfloats);
	for(i=0;i<c->nfloats;i++)
		c->floats[i] = hl_read_double(r);
	CHK_ERROR();
	ALLOC(c->types, hl_type, sizeof(hl_type)*c->ntypes);
	for(i=0;i<c->ntypes;i++) {
		hl_read_type(r, c->types + i);
		CHK_ERROR();
	}
	ALLOC(c->globals, hl_type*, sizeof(hl_type*)*c->nglobals);
	for(i=0;i<c->nglobals;i++)
		c->globals[i] = hl_get_type(r);
	CHK_ERROR();
	ALLOC(c->natives, hl_native, sizeof(hl_native)*c->nnatives);
	for(i=0;i<c->nnatives;i++) {
		c->natives[i].name = hl_get_string(r);
		c->natives[i].global = hl_get_global(r);
		printf("%s,%d\n", c->natives[i].name, c->natives[i].global);
	}
	CHK_ERROR();
	EXIT("TODO");
	return NULL;
}


int main( int argc, char *argv[] ) {
	if( argc == 1 ) {
		printf("HLVM %d.%d.%d (c)2015 Haxe Foundation\n  Usage : hl <file>\n",HL_VERSION/100,(HL_VERSION/10)%10,HL_VERSION%10);
		return 1;
	}
	hl_global_init();
	{
		hl_code *code;
		const char *file = argv[1];
		FILE *f = fopen(file,"rb");
		int pos, size;
		char *fdata;
		if( f == NULL ) {
			printf("File not found '%s'\n",file);
			return -1;
		}
		fseek(f, 0, SEEK_END);
		size = (int)ftell(f);
		fseek(f, 0, SEEK_SET);
		fdata = (char*)malloc(size);
		pos = 0;
		while( pos < size ) {
			int r = (int)fread(fdata + pos, 1, size-pos, f);
			if( r <= 0 ) {
				printf("Failed to read '%s'\n",file);
				return -1;
			}
			pos += r;
		}
		fclose(f);
		code = hl_code_read((unsigned char*)fdata, size);
		if( code == NULL )
			return -1;
	}
	hl_global_free();
	return 0;
}
