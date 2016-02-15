#include <hl.h>

void hl_sys_print( vbytes *msg ) {
	uprintf(USTR("%s"),(uchar*)msg);
}


#ifndef HL_JIT

extern void hl_entry_point();

int main() {
	hl_entry_point();
	return 0;
}

#endif
