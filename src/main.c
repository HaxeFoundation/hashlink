/*
 * Copyright (C)2015-2016 Haxe Foundation
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
#include <hlmodule.h>

#ifdef HL_VCC
#	include <crtdbg.h>
#else
#	define _CrtSetDbgFlag(x)
#	define _CrtCheckMemory()
#endif

#ifdef HL_WIN
typedef uchar pchar;
#define pprintf(str,file)	uprintf(USTR(str),file)
#define pfopen(file,ext) _wfopen(file,USTR(ext))
#define pcompare wcscmp
#define ptoi(s)	wcstol(s,NULL,10) 
#define PSTR(x) USTR(x)
#else
typedef char pchar;
#define pprintf printf
#define pfopen fopen
#define pcompare strcmp
#define ptoi atoi
#define PSTR(x) x
#endif

extern void *hl_callback( void *f, hl_type *t, void **args, vdynamic *ret );

static hl_code *load_code( const pchar *file ) {
	hl_code *code;
	FILE *f = pfopen(file,"rb");
	int pos, size;
	char *fdata;
	if( f == NULL ) {
		pprintf("File not found '%s'\n",file);
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = (int)ftell(f);
	fseek(f, 0, SEEK_SET);
	fdata = (char*)malloc(size);
	pos = 0;
	while( pos < size ) {
		int r = (int)fread(fdata + pos, 1, size-pos, f);
		if( r <= 0 ) {
			pprintf("Failed to read '%s'\n",file);
			return NULL;
		}
		pos += r;
	}
	fclose(f);
	code = hl_code_read((unsigned char*)fdata, size);
	free(fdata);
	return code;
}

#ifdef HL_VCC
static int throw_handler( int code ) {
	switch( code ) {
	case EXCEPTION_ACCESS_VIOLATION: hl_error("Access violation");
	case EXCEPTION_STACK_OVERFLOW: hl_error("Stack overflow");
	default: hl_error("Unknown runtime error");
	}
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#ifdef HL_WIN
int wmain(int argc, pchar *argv[]) {
#else
int main(int argc, pchar *argv[]) {
#endif
	pchar *file = NULL;
	int debug_port = -1;
	bool debug_wait = false;
	struct {
		hl_code *code;
		hl_module *m;
		vdynamic *exc;
	} ctx;
	hl_trap_ctx trap;
	argv++;
	argc--;
	while( argc-- ) {
		pchar *arg = *argv++;
		if( pcompare(arg,PSTR("--debug")) == 0 ) {
			if( argc-- == 0 ) break;
			debug_port = ptoi(*argv++);
			continue;
		}
		if( pcompare(arg,PSTR("--debug-wait")) == 0 ) {
			debug_wait = true;
			continue;
		}
		file = arg;
		break;
	}
	if( file == NULL ) {
		printf("HL/JIT %d.%d.%d (c)2015-2016 Haxe Foundation\n  Usage : hl [--debug <port>] <file>\n",HL_VERSION>>8,(HL_VERSION>>4)&15,HL_VERSION&15);
		return 1;
	}
#	ifdef HL_64
	fprintf(stderr,"HL/JIT is currently not supported when compiled to 64 bits, use make ARCH=32 to compile to 32 bits\n");
	exit(1);
#	endif
	hl_global_init(&ctx);
	hl_sys_init((void**)argv,argc);
	ctx.code = load_code(file);
	if( ctx.code == NULL )
		return 1;
	ctx.m = hl_module_alloc(ctx.code);
	if( ctx.m == NULL )
		return 2;
	if( !hl_module_init(ctx.m) )
		return 3;
	hl_code_free(ctx.code);
	if( debug_port > 0 && !hl_module_debug(ctx.m,debug_port,debug_wait) ) {
		fprintf(stderr,"Could not start debugger on port %d",debug_port);
		return 4;
	}
	hl_trap(trap, ctx.exc, on_exception);
#	ifdef HL_VCC
	__try {
#	endif
		hl_callback(ctx.m->functions_ptrs[ctx.m->code->entrypoint],ctx.code->functions[ctx.m->functions_indexes[ctx.m->code->entrypoint]].type,NULL,NULL);
#	ifdef HL_VCC
	} __except( throw_handler(GetExceptionCode()) ) {}
#	endif
	hl_module_free(ctx.m);
	hl_free(&ctx.code->alloc);
	hl_global_free();
	return 0;
on_exception:
	{
		varray *a = hl_exception_stack();
		int i;
		uprintf(USTR("Uncaught exception: %s\n"), hl_to_string(ctx.exc));
		for(i=0;i<a->size;i++)
			uprintf(USTR("Called from %s\n"), hl_aptr(a,uchar*)[i]);
		hl_debug_break();
	}
	hl_global_free();
	return 1;
}

