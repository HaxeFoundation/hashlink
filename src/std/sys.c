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

#ifndef HL_WIN
#	include <sys/time.h>
#	include <sys/times.h>
#	define HL_UTF8PATH
#endif

HL_PRIM bool hl_sys_utf8_path() {
#ifdef HL_UTF8_PATH
	return true;
#else
	return false;
#endif
}

HL_PRIM vbyte *hl_sys_string() {
#if defined(HL_WIN)
	return (vbyte*)USTR("Windows");
#elif defined(NEKO_GNUKBSD)
	return (vbyte*)USTR("GNU/kFreeBSD");
#elif defined(NEKO_LINUX)
	return (vbyte*)USTR("Linux");
#elif defined(NEKO_BSD)
	return (vbyte*)USTR("BSD");
#elif defined(NEKO_MAC)
	return (vbyte*)USTR("Mac");
#else
#error Unknow system string
#endif
}

HL_PRIM void hl_sys_print( vbyte *msg ) {
	uprintf(USTR("%s"),(uchar*)msg);
}

HL_PRIM void hl_sys_exit( int code ) {
	exit(code);
}

HL_PRIM double hl_sys_time() {
#ifdef HL_WIN
#define EPOCH_DIFF	(134774*24*60*60.0)
	SYSTEMTIME t;
	FILETIME ft;
    ULARGE_INTEGER ui;
	GetSystemTime(&t);
	if( !SystemTimeToFileTime(&t,&ft) )
		return 0.;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
	return ((double)ui.QuadPart) / 10000000.0 - EPOCH_DIFF;
#else
	struct timeval tv;
	if( gettimeofday(&tv,NULL) != 0 )
		return 0.;
	return tv.tv_sec + ((double)tv.tv_usec) / 1000000.0;
#endif
}

HL_PRIM int hl_random( int max ) {
	if( max <= 0 ) return 0;
	return rand() % max;
}

#ifndef HL_JIT

#include <hlc.h>
#if defined(HL_VCC) && defined(_DEBUG)
#	include <crtdbg.h>
#else
#	define _CrtSetDbgFlag(x)
#endif

extern void hl_entry_point();

int main() {
	hl_trap_ctx ctx;
	vdynamic *exc;
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF /*| _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF*/ );
	hlc_trap(ctx,exc,on_exception);
	hl_entry_point();
	return 0;
on_exception:
	uprintf(USTR("Uncaught exception: %s\n"),hl_to_string(exc));
	return 1;
}

#endif
