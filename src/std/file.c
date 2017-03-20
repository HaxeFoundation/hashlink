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
#include <stdio.h>
#ifdef HL_WIN
#	include <windows.h>
#	define fopen(name,mode) _wfopen(name,mode)
#	define HL_UFOPEN
#endif

typedef struct _hl_fdesc hl_fdesc;
struct _hl_fdesc {
	void (*finalize)( hl_fdesc * );
	FILE *f;
};

static void fdesc_finalize( hl_fdesc *f ) {
	if( f->f ) fclose(f->f);
}

HL_PRIM hl_fdesc *hl_file_open( vbyte *name, int mode, bool binary ) {
#	ifdef HL_UFOPEN
	static const uchar *MODES[] = { USTR("r"), USTR("w"), USTR("a"), NULL, USTR("rb"), USTR("wb"), USTR("ab") };
	FILE *f = fopen((uchar*)name,MODES[mode|(binary?4:0)]);
#	else
	static const char *MODES[] = { "r", "w", "a", NULL, "rb", "wb", "ab" };
	FILE *f = fopen((char*)name,MODES[mode|(binary?4:0)]);
#	endif
	hl_fdesc *fd;
	if( f == NULL ) return NULL;
	fd = (hl_fdesc*)hl_gc_alloc_finalizer(sizeof(hl_fdesc));
	fd->finalize = fdesc_finalize;
	fd->f = f;
	return fd;
}

HL_PRIM void hl_file_close( hl_fdesc *f ) {
	if( !f ) return;
	if( f->f ) fclose(f->f);
	f->f = NULL;
	f->finalize = NULL;
}

HL_PRIM int hl_file_write( hl_fdesc *f, vbyte *buf, int pos, int len ) {
	if( !f ) return -1;
	return (int)fwrite(buf+pos,1,len,f->f);
}

HL_PRIM int hl_file_read( hl_fdesc *f, vbyte *buf, int pos, int len ) {
	if( !f ) return -1;
	return (int)fread((char*)buf+pos,1,len,f->f);
}

HL_PRIM bool hl_file_write_char( hl_fdesc *f, int c ) {
	unsigned char cc = (unsigned char)c;
	if( !f ) return false;
	return fwrite(&cc,1,1,f->f) == 1;
}

HL_PRIM int hl_file_read_char( hl_fdesc *f ) {
	unsigned char cc;
	if( !f || fread(&cc,1,1,f->f) != 1 )
		return -2;
	return cc;
}

HL_PRIM bool hl_file_seek( hl_fdesc *f, int pos, int kind ) {
	if( !f ) return false;
	return fseek(f->f,pos,kind) == 0;
}

HL_PRIM int hl_file_tell( hl_fdesc *f ) {
	if( !f ) return -1;
	return ftell(f->f);
}

HL_PRIM bool hl_file_eof( hl_fdesc *f ) {
	if( !f ) return true;
	return (bool)feof(f->f);
}

HL_PRIM bool hl_file_flush( hl_fdesc *f ) {
	if( !f ) return false;
	return fflush( f->f ) == 0;
}

#define MAKE_STDIO(k) \
	HL_PRIM hl_fdesc *hl_file_##k() { \
		hl_fdesc *f; \
		f = (hl_fdesc*)hl_gc_alloc_noptr(sizeof(hl_fdesc)); \
		f->f = k; \
		f->finalize = NULL; \
		return f; \
	}

MAKE_STDIO(stdin);
MAKE_STDIO(stdout);
MAKE_STDIO(stderr);

HL_PRIM vbyte *hl_file_contents( vbyte *name, int *size ) {
	int len;
	int p = 0;
	vbyte *content;
#	ifdef HL_UFOPEN
	FILE *f = fopen((uchar*)name,USTR("rb"));
#	else
	FILE *f = fopen((char*)name,"rb");
#	endif
	if( f == NULL )
		return NULL;
	fseek(f,0,SEEK_END);
	len = ftell(f);
	if( size ) *size = len;
	fseek(f,0,SEEK_SET);
	content = (vbyte*)hl_gc_alloc_noptr(size ? len : len+1);
	if( !size ) content[len] = 0; // final 0 for UTF8
	while( len > 0 ) {
		int d = (int)fread((char*)content + p,1,len,f);
		if( d <= 0 ) {
			fclose(f);
			return NULL;
		}
		p += d;
		len -= d;
	}
	fclose(f);
	return content;
}

#define _FILE _ABSTRACT(hl_fdesc)
DEFINE_PRIM(_FILE, file_open, _BYTES _I32 _BOOL);
DEFINE_PRIM(_VOID, file_close, _FILE);
DEFINE_PRIM(_I32, file_write, _FILE _BYTES _I32 _I32);
DEFINE_PRIM(_I32, file_read, _FILE _BYTES _I32 _I32);
DEFINE_PRIM(_BOOL, file_write_char, _FILE _I32);
DEFINE_PRIM(_I32, file_read_char, _FILE);
DEFINE_PRIM(_BOOL, file_seek, _FILE _I32 _I32);
DEFINE_PRIM(_I32, file_tell, _FILE);
DEFINE_PRIM(_BOOL, file_eof, _FILE);
DEFINE_PRIM(_BOOL, file_flush, _FILE);
DEFINE_PRIM(_FILE, file_stdin, _NO_ARG);
DEFINE_PRIM(_FILE, file_stdout, _NO_ARG);
DEFINE_PRIM(_FILE, file_stderr, _NO_ARG);
DEFINE_PRIM(_BYTES, file_contents, _BYTES _REF(_I32));
