// detect automatically CPU type
#include <hl.h>
#ifdef __ANDROID__
#	include "jsimd_none.c"
#elif defined(HL_64)
#	include "x64/jsimd_x86_64.c"
#else
#	include "x86/jsimd_i386.c"
#endif