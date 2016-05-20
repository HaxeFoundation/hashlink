// detect automatically CPU type
#include <hl.h>
#ifdef HL_64
#	include "x64/jsimd_x86_64.c"
#else
#	include "x86/jsimd_i386.c"
#endif