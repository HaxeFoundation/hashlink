#include <hl.h>
#include <limits.h>
#include <inttypes.h>

// I64

HL_PRIM int64 hl_num_i64_of_int( int i ) {
	return i;
}
DEFINE_PRIM(_I64, num_i64_of_int, _I32);

HL_PRIM int64 hl_num_i64_max() {
	return LLONG_MAX;
}
DEFINE_PRIM(_I64, num_i64_max, _NO_ARG);

HL_PRIM int64 hl_num_i64_min() {
	return LLONG_MIN;
}
DEFINE_PRIM(_I64, num_i64_min, _NO_ARG);

HL_PRIM int64 hl_num_i64_of_string( vstring *s ) {
	if( !s )
		hl_null_access();
	return strtoll(hl_to_utf8(s->bytes), NULL, 10);
}
DEFINE_PRIM(_I64, num_i64_of_string, _STRING);

HL_PRIM vbyte *hl_num_i64_to_bytes( int64 i ) {
	char buf[20];
	int length = sprintf(buf, "%lld", i);
	return hl_copy_bytes((vbyte *)buf, length);
}
DEFINE_PRIM(_BYTES, num_i64_to_bytes, _I64);

HL_PRIM int64 hl_num_i64_add( int64 a, int64 b ) {
	return a + b;
}
DEFINE_PRIM(_I64, num_i64_add, _I64 _I64);

HL_PRIM int64 hl_num_i64_sub( int64 a, int64 b ) {
	return a - b;
}
DEFINE_PRIM(_I64, num_i64_sub, _I64 _I64);

HL_PRIM int64 hl_num_i64_mul( int64 a, int64 b ) {
	return a * b;
}
DEFINE_PRIM(_I64, num_i64_mul, _I64 _I64);

HL_PRIM int64 hl_num_i64_mod( int64 a, int64 b ) {
	return a % b;
}
DEFINE_PRIM(_I64, num_i64_mod, _I64 _I64);

HL_PRIM int64 hl_num_i64_div( int64 a, int64 b ) {
	return a / b;
}
DEFINE_PRIM(_I64, num_i64_div, _I64 _I64);

HL_PRIM int64 hl_num_i64_logand( int64 a, int64 b ) {
	return a & b;
}
DEFINE_PRIM(_I64, num_i64_logand, _I64 _I64);

HL_PRIM int64 hl_num_i64_logor( int64 a, int64 b ) {
	return a | b;
}
DEFINE_PRIM(_I64, num_i64_logor, _I64 _I64);

HL_PRIM int64 hl_num_i64_logxor( int64 a, int64 b ) {
	return a ^ b;
}
DEFINE_PRIM(_I64, num_i64_logxor, _I64 _I64);

HL_PRIM int64 hl_num_i64_shift_left( int64 a, int b ) {
	return a << b;
}
DEFINE_PRIM(_I64, num_i64_shift_left, _I64 _I32);

HL_PRIM int64 hl_num_i64_shift_right( int64 a, int b ) {
	return a >> b;
}
DEFINE_PRIM(_I64, num_i64_shift_right, _I64 _I32);

HL_PRIM int64 hl_num_i64_lognot( int64 a ) {
	return ~a;
}
DEFINE_PRIM(_I64, num_i64_lognot, _I64);

HL_PRIM bool hl_num_i64_eq( int64 a, int64 b ) {
	return a == b;
}
DEFINE_PRIM(_BOOL, num_i64_eq, _I64 _I64);

HL_PRIM bool hl_num_i64_lt( int64 a, int64 b ) {
	return a < b;
}
DEFINE_PRIM(_BOOL, num_i64_lt, _I64 _I64);

HL_PRIM bool hl_num_i64_gt( int64 a, int64 b ) {
	return a > b;
}
DEFINE_PRIM(_BOOL, num_i64_gt, _I64 _I64);

HL_PRIM bool hl_num_i64_lte( int64 a, int64 b ) {
	return a <= b;
}
DEFINE_PRIM(_BOOL, num_i64_lte, _I64 _I64);

HL_PRIM bool hl_num_i64_gte( int64 a, int64 b ) {
	return a >= b;
}
DEFINE_PRIM(_BOOL, num_i64_gte, _I64 _I64);