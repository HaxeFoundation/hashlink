/*
	We want to limit the amount of macros defined by including hl headers.
*/

#define HLC_BOOT /* Enables some undefs in hlc.h */

#include <hl.h>
#include <hlc.h>

#ifdef WITHOUT_HLC_MAIN
int main() {}
#else
#include <hlc_main.c>
#endif

/*
	Variables here are declared as externs, and are defined in `variables.c`.

	If any of the variables clash with defines from the hl headers,
	then there will be syntax errors or the declarations will fail to link against
	the definitions in variables.c, leading to a linker error.
*/
#define WRAP_VARIABLE(VAR) extern int VAR;
#include "variables.h"


void use(int value) {}

void use_variables() {
	/* If any have been renamed, the undefined usage causes a linking error. */
	#undef WRAP_VARIABLE
	#define WRAP_VARIABLE(VAR) use(VAR);

	#include "variables.h"
}

/* Required by hlc.h */
void* hlc_static_call(void *fun, hl_type *t, void **args, vdynamic *out) {
	return NULL;
}

void* hlc_get_wrapper( hl_type* type ) {
	return NULL;
}

void hl_entry_point() {}
