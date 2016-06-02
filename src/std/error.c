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
#include <hlc.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#	pragma warning(disable:4091)
#	include <DbgHelp.h>
#	pragma comment(lib, "Dbghelp.lib")
#endif

HL_PRIM hl_trap_ctx *hl_current_trap = NULL;
HL_PRIM vdynamic *hl_current_exc = NULL;

static vdynamic *stack_last_exc = NULL;
static void *stack_trace[0x1000];
static int stack_count = 0;
#ifdef _WIN32
static HANDLE stack_process_handle = NULL;
#endif

HL_PRIM void *hl_fatal_error( const char *msg, const char *file, int line ) {
	printf("%s(%d) : FATAL ERROR : %s\n",file,line,msg);
	hl_debug_break();
	exit(1);
	return NULL;
}

HL_PRIM void hl_throw( vdynamic *v ) {
	hl_trap_ctx *t = hl_current_trap;
#ifdef _WIN32
	if( v != stack_last_exc ) {
		stack_last_exc = v;
		stack_count = CaptureStackBackTrace(1, 0x1000, stack_trace, NULL) - 8; // 8 startup
	}
#endif
	hl_current_exc = v;
	hl_current_trap = t->prev;
	if( hl_current_trap == NULL ) hl_debug_break();
	longjmp(t->buf,1);
}

static uchar *hl_resolve_symbol( void *addr ) {
#ifdef _WIN32
	DWORD64 index;
	uchar tmp[512];
	IMAGEHLP_LINEW64 line;
	struct {
		SYMBOL_INFOW sym;
		uchar buffer[256];
	} data;
	data.sym.SizeOfStruct = sizeof(data.sym);
	data.sym.MaxNameLen = 255;
	if( SymFromAddrW(stack_process_handle,(DWORD64)addr,&index,&data.sym) ) {
		int size;
		DWORD offset = 0;
		line.SizeOfStruct = sizeof(line);
		line.FileName = USTR("\\?");
		line.LineNumber = 0;
		SymGetLineFromAddrW64(stack_process_handle, (DWORD64)addr, &offset, &line);
		size = usprintf(tmp,512,USTR("%s(%s) line %d"),data.sym.Name,wcsrchr(line.FileName,'\\')+1,line.LineNumber);
		return (uchar*)hl_copy_bytes((vbyte*)tmp,sizeof(uchar)*(size+1));
	}
#endif
	return NULL;
}

HL_PRIM varray *hl_exception_stack() {
	varray *a = hl_alloc_array(&hlt_bytes, stack_count);
	int i;
	uchar tmp[32];
#ifdef _WIN32
	if( !stack_process_handle ) {
		stack_process_handle = GetCurrentProcess();
		SymSetOptions(SYMOPT_LOAD_LINES);
		SymInitialize(stack_process_handle,NULL,TRUE);
	}
#endif
	for(i=0;i<stack_count;i++) {
		void *addr = stack_trace[i];
		uchar *str = hl_resolve_symbol(addr);
		if( str == NULL ) {
			int size = usprintf(tmp,32,USTR("@0x%X"),(int)(int_val)addr);
			str = (uchar*)hl_copy_bytes((vbyte*)tmp,sizeof(uchar)*(size+1));
		}
		hl_aptr(a,vbyte*)[i] = (vbyte*)str;
	}
	return a;
}

HL_PRIM void hl_rethrow( vdynamic *v ) {
	stack_last_exc = v;
	hl_throw(v);
}

HL_PRIM void hl_error_msg( const uchar *fmt, ... ) {
	uchar buf[256];
	vdynamic *d;
	int len;
	va_list args;
	va_start(args, fmt);
	len = uvsprintf(buf,fmt,args);
	va_end(args);
	d = hl_alloc_dynamic(&hlt_bytes);
	d->v.ptr = hl_copy_bytes((vbyte*)buf,(len + 1) << 1);
	hl_throw(d);
}

HL_PRIM void hl_fatal_fmt(const char *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf,fmt, args);
	va_end(args);
	hl_fatal(buf);
}
