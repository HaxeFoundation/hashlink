
// include by os_types.h as a fallback

#if defined(__ORBIS__) || defined(__NX__) || defined(__ANDROID__)

#  include <sys/types.h>
typedef short ogg_int16_t;
typedef unsigned short ogg_uint16_t;
typedef int ogg_int32_t;
typedef unsigned int ogg_uint32_t;
typedef long long ogg_int64_t;

#else

#error "Could not tell which OS config we compile to"

#endif