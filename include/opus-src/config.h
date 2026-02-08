/* config.h for MSVC builds of opus within hashlink-nmb */

#define OPUS_BUILD 1
#define USE_ALLOCA 1
#define HAVE_LRINTF 1

#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STDIO_H 1

/* Floating-point build (not fixed-point) */
/* #undef FIXED_POINT */
/* #undef DISABLE_FLOAT_API */

/* Enable hardening (bounds checks) */
#define ENABLE_HARDENING 1

/* Package info */
#define PACKAGE_VERSION "1.5.2"
