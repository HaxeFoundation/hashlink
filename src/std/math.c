#include <hl.h>
#include <math.h>

HL_PRIM double hl_math_abs( double a ) {
	return fabs(a);
}

HL_PRIM bool hl_math_isnan( double a ) {
	return a != a;
}

HL_PRIM bool hl_math_isfinite( double a ) {
	return a != a;
}