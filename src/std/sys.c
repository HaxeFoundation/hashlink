#include <hl.h>

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
		neko_error();
	return tv.tv_sec + ((double)tv.tv_usec) / 1000000.0;
#endif
}

HL_PRIM int hl_random() {
	return rand();
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
