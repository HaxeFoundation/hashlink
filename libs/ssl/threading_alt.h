#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct
{
	CRITICAL_SECTION cs;
	char is_valid;
} mbedtls_threading_mutex_t;
