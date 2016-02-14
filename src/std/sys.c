#include <hl.h>

void hl_sys_print( vbytes *msg ) {
	printf("%s",msg);
}


#ifndef HL_JIT

extern void hl_entry_point();

int main() {
	hl_entry_point();
	return 0;
}

#endif
