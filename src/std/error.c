#include <hl.h>
#include <stdarg.h>
#include <string.h>

void *hl_fatal_error( const char *msg, const char *file, int line ) {
	printf("%s(%d) : FATAL ERROR : %s\n",file,line,msg);
#ifdef _DEBUG
	*(int*)NULL = 0;
#else
	exit(0);
#endif
	return NULL;
}

void hl_throw( vdynamic *v ) {
	*(char*)NULL = 0;
}

void hl_rethrow( vdynamic *v ) {
	*(char*)NULL = 0;
}

void hl_error_msg( const uchar *fmt, ... ) {
	uchar buf[256];
	va_list args;
	va_start(args, fmt);
	uvsprintf(buf,fmt,args);
	va_end(args);
	// TODO : throw
	uprintf(USTR("THROW:%s\n"),buf);
	exit(66);
}

void hl_fatal_fmt(const char *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf,fmt, args);
	va_end(args);
	hl_fatal(buf);
}
