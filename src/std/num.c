#include <hl.h>

HL_PRIM int64 hl_num_i64_of_int( int i ) {
	return i;
}
DEFINE_PRIM(_I64, num_i64_of_int, _I32);