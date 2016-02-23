#include <hl.h>
#include <math.h>

double hl_nan;

HL_PRIM double hl_math_abs( double a ) {
	return fabs(a);
}

HL_PRIM bool hl_math_isnan( double a ) {
	return a != a;
}

typedef union {
	double d;
	struct {
		unsigned int l;
		unsigned int h;
	} i;
} qw;

HL_PRIM bool hl_math_isfinite( double a ) {
	qw q;
	unsigned int h, l;
	if( a != a )
		return false;
	q.d = a;
	h = q.i.h;
	l = q.i.l;
	l = l | (h & 0xFFFFF);
	h = h & 0x7FF00000;
	return h != 0x7FF00000 || l;
}

HL_PRIM double hl_math_fceil( double d ) {
	return ceil(d);
}

HL_PRIM double hl_math_fround( double d ) {
	return floor(d + 0.5);
}

HL_PRIM double hl_math_ffloor( double d ) {
	return floor(d);
}

HL_PRIM int hl_math_round( double d ) {
	return (int)hl_math_fround(d);
}

HL_PRIM int hl_math_ceil( double d ) {
	return (int)hl_math_fceil(d);
}

HL_PRIM int hl_math_floor( double d ) {
	return (int)hl_math_ffloor(d);
}

HL_PRIM double hl_math_cos( double a ) {
	return cos(a);
}

HL_PRIM double hl_math_sin( double a ) {
	return sin(a);
}

HL_PRIM double hl_math_tan( double a ) {
	return tan(a);
}

HL_PRIM double hl_math_acos( double a ) {
	return acos(a);
}

HL_PRIM double hl_math_asin( double a ) {
	return asin(a);
}

HL_PRIM double hl_math_atan( double a ) {
	return atan(a);
}

HL_PRIM double hl_math_atan2( double a, double b ) {
	return atan2(a,b);
}

HL_PRIM double hl_math_pow( double a, double b ) {
	return pow(a,b);
}

