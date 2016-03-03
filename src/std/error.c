#define HLC_NO_STATIC
#include <hlc.h>
#include <stdarg.h>
#include <string.h>

hl_trap_ctx *current_trap = NULL;
vdynamic *current_exc = NULL;

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
	hl_trap_ctx *t = current_trap;
	current_exc = v;
	current_trap = t->prev;
	longjmp(t->buf,1);
}

void hl_rethrow( vdynamic *v ) {
	hl_throw(v);
}

void hl_error_msg( const uchar *fmt, ... ) {
	uchar buf[256];
	vdynamic *d;
	int len;
	va_list args;
	va_start(args, fmt);
	len = uvsprintf(buf,fmt,args);
	va_end(args);
	d = hl_alloc_dynamic(&hlt_bytes);
	d->v.ptr = hl_gc_alloc_noptr((len + 1) << 1);
	memcpy(d->v.ptr,buf,(len + 1) << 1);
	hl_throw(d);
}

void hl_fatal_fmt(const char *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf,fmt, args);
	va_end(args);
	hl_fatal(buf);
}
