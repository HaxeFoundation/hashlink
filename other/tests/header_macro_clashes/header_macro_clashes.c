/*
	We want to limit the amount of macros defined by including hl headers.
*/

#define HLC_BOOT /* Enables some undefs in hlc.h */

#include <hl.h>
#include <hlc.h>

#include "check.h"

/* check before and after including hlc_main.c */
#include <hlc_main.c>

#include "check.h"

/* Required by hlc.h */
void* hlc_static_call(void *fun, hl_type *t, void **args, vdynamic *out) {
	return NULL;
}

void* hlc_get_wrapper( hl_type* type ) {
	return NULL;
}

void hl_entry_point() {}
