#include <hl.h>

HL_PRIM void hl_sys_print( vbyte *msg ) {
	uprintf(USTR("%s"),(uchar*)msg);
}

HL_PRIM void hl_sys_exit( int code ) {
	exit(code);
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
