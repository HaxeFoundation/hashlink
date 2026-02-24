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
#include <hlc.h>

#if defined(HL_MOBILE) && defined(sdl__Sdl__val)
#   include <SDL_main.h>
#endif

#if defined(HL_WIN_DESKTOP) || defined(HL_XBS)
#	pragma warning(disable:4091)
#	undef _GUID
#ifndef WIN32_LEAN_AND_MEAN
#	define WIN32_LEAN_AND_MEAN
#endif
#	include <windows.h>
#	include <dbghelp.h>
#	pragma comment(lib, "Dbghelp.lib")
#	undef CONST
#	undef IN
#	undef OUT
#	undef OPTIONAL
#	undef DELETE
#	undef ERROR
#	undef NO_ERROR
#	undef STRICT
#	undef TRUE
#	undef FALSE
#	undef CW_USEDEFAULT
#	undef far
#	undef FAR
#	undef near
#	undef NEAR
#	undef GENERIC_READ
#	undef ALTERNATE
#	undef DIFFERENCE
#	undef DOUBLE_CLICK
#	undef WAIT_FAILED
#	undef TRANSPARENT
#	undef CopyFile
#	undef COLOR_HIGHLIGHT
#	undef __valid
#endif

#ifdef HL_CONSOLE
extern void sys_global_init();
extern void sys_global_exit();
#else
#define sys_global_init()
#define sys_global_exit()
#endif

#if defined(HL_LINUX) && (!defined(HL_ANDROID) || __ANDROID_MIN_SDK_VERSION__ >= 33)
#define HL_LINUX_BACKTRACE
#endif

#if defined(HL_LINUX_BACKTRACE) || defined(HL_MAC)
#	include <execinfo.h>
#endif

static uchar *hlc_resolve_symbol( void *addr, uchar *out, int *outSize ) {
#ifdef HL_WIN_DESKTOP
	static HANDLE stack_process_handle = NULL;
	DWORD64 index;
	IMAGEHLP_LINEW64 line;
	struct {
		SYMBOL_INFOW sym;
		uchar buffer[256];
	} data;
	data.sym.SizeOfStruct = sizeof(data.sym);
	data.sym.MaxNameLen = 255;
	if( !stack_process_handle ) {
		stack_process_handle = GetCurrentProcess();
		SymSetOptions(SYMOPT_LOAD_LINES);
		SymInitialize(stack_process_handle,NULL,(BOOL)1);
	}
	if( SymFromAddrW(stack_process_handle,(DWORD64)(int_val)addr,&index,&data.sym) ) {
		DWORD offset = 0;
		line.SizeOfStruct = sizeof(line);
		line.FileName = USTR("\\?");
		line.LineNumber = 0;
		SymGetLineFromAddrW64(stack_process_handle, (DWORD64)(int_val)addr, &offset, &line);
		*outSize = usprintf(out,*outSize,USTR("%s(%s:%d)"),data.sym.Name,wcsrchr(line.FileName,'\\')+1,(int)line.LineNumber);
		return out;
	}
#elif defined(HL_LINUX_BACKTRACE) || defined(HL_MAC)
	void *array[1];
	char **strings;
	array[0] = addr;
	strings = backtrace_symbols(array, 1);
	if (strings != NULL) {
		*outSize = (int)strlen(strings[0]) << 1;
		out = (uchar*)hl_gc_alloc_noptr(*outSize);
		hl_from_utf8(out,*outSize,strings[0]);
		free(strings);
		return out;
	}
#endif
	return NULL;
}

static int hlc_capture_stack( void **stack, int size ) {
	int count = 0;
#	if defined(HL_WIN_DESKTOP) || defined(HL_LINUX_BACKTRACE) || defined(HL_MAC)
	// force return total count when output stack is null
	static void* tmpstack[HL_EXC_MAX_STACK];
	if( stack == NULL ) {
		stack = tmpstack;
		size = HL_EXC_MAX_STACK;
	}
#	endif
#	ifdef HL_WIN_DESKTOP
	count = CaptureStackBackTrace(2, size, stack, NULL);
	if( size == HL_EXC_MAX_STACK ) count -= 8; // 8 startup
#	elif defined(HL_LINUX_BACKTRACE)
	count = backtrace(stack, size);
	if( size == HL_EXC_MAX_STACK ) count -= 8;
#	elif defined(HL_MAC)
	count = backtrace(stack, size);
	if( size == HL_EXC_MAX_STACK ) count -= 6;
#	endif
	if( count < 0 ) count = 0;
	return count;
}

#if defined(HL_WIN_DESKTOP) || defined(HL_XBS)
int wmain(int argc, uchar *argv[]) {
#else
int main(int argc, char *argv[]) {
#endif
	vdynamic *ret;
	bool isExc = false;
	hl_type_fun tf = { 0 };
	hl_type clt = { 0 };
	vclosure cl = { 0 };
	sys_global_init();
	hl_global_init();
	hl_register_thread(&ret);
	hl_setup.resolve_symbol = hlc_resolve_symbol;
	hl_setup.capture_stack = hlc_capture_stack;
	hl_setup.static_call = hlc_static_call;
	hl_setup.get_wrapper = hlc_get_wrapper;
	hl_setup.sys_args = (pchar**)(argv + 1);
	hl_setup.sys_nargs = argc - 1;
	hl_sys_init();
	tf.ret = &hlt_void;
	clt.kind = HFUN;
	clt.fun = &tf;
	cl.t = &clt;
	cl.fun = hl_entry_point;
	ret = hl_dyn_call_safe(&cl, NULL, 0, &isExc);
	if( isExc ) {
		hl_print_uncaught_exception(ret);
	}
	hl_global_free();
	sys_global_exit();
	return (int)isExc;
}

#if (defined(HL_WIN_DESKTOP) && !defined(_CONSOLE)) || defined(HL_XBS)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
	return wmain(__argc, __wargv);
}
#endif
