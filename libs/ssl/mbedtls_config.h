#ifdef __ORBIS__
#	undef MBEDTLS_FS_IO
#	undef MBEDTLS_TIMING_C
#endif

#ifdef _WIN32
#	define MBEDTLS_THREADING_ALT
#else
#	define MBEDTLS_THREADING_PTHREAD
#endif

#undef MBEDTLS_NET_C

#define MBEDTLS_THREADING_C
