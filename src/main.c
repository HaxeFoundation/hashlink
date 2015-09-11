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

typedef struct {
	int version;
	int nglobals;
	int nfloats;
	int nfunctions;
	int nnatives;
	int entrypoint;
} hl_code;

typedef struct {
	const unsigned char *b;
	int size;
	int pos;
	int error;
} hl_reader;

#define HL_VERSION	010

void hl_global_init() {
}

void hl_global_free() {
}

#define READ() hl_read_b(r)
#define INDEX() hl_read_index(r)
#define UINDEX() hl_read_uindex(r)
#define ERROR(msg) { printf("%s\n",msg); hl_code_free(c); return NULL; }

static unsigned char hl_read_b( hl_reader *r ) {
	if( r->pos >= r->size ) {
		r->error = 1;
		return 0;
	}
	return r->b[r->pos++];
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
	if( i < 0 ) r->error = 1;
	return i;
}

void hl_code_free( hl_code *c ) {
	free(c);
}

hl_code *hl_code_read( const unsigned char *data, int size ) {
	hl_reader _r = { data, size, 0, 0 };	
	hl_reader *r = &_r;
	hl_code *c = (hl_code*)malloc(sizeof(hl_code));
	memset(c,0,sizeof(hl_code));
	if( READ() != 'H' || READ() != 'L' || READ() != 'B' )
		ERROR("Invalid HL header");
	c->version = READ();
	c->nglobals = UINDEX();
	c->nfloats = UINDEX();
	c->nnatives = UINDEX();
	c->nfunctions = UINDEX();
	c->entrypoint = UINDEX();
	if( r->error )
		ERROR("Invalid HL counts");
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
