#include <hl.h>

HL_PRIM void hl_sys_print( vbyte *msg ) {
	uprintf(USTR("%s"),(uchar*)msg);
}

HL_PRIM void hl_sys_exit( int code ) {
	exit(code);
}

#ifndef HL_JIT

#if defined(HL_VCC) && defined(_DEBUG)
#	include <crtdbg.h>
#else
#	define _CrtSetDbgFlag(x)
#endif

extern void hl_entry_point();

int main() {
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF /*| _CRTDBG_CHECK_ALWAYS_DF*/ );
	hl_entry_point();
	return 0;
}

#endif
