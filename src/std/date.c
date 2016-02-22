#include <hl.h>
#include <time.h>

#ifdef HL_WIN

static struct tm *localtime_r( time_t *t, struct tm *r ) {
	struct tm *r2 = localtime(t);
	if( r2 == NULL ) return NULL;
	*r = *r2;
	return r;
}

static struct tm *gmtime_r( time_t *t, struct tm *r ) {
	struct tm *r2 = gmtime(t);
	if( r2 == NULL ) return NULL;
	*r = *r2;
	return r;
}

#endif

HL_PRIM vbyte *hl_date_to_string( int date, int *len ) {
	char buf[127];
	struct tm t;
	time_t d = (time_t)date;
	int size;
	uchar *out;
	if( !localtime_r(&d,&t) )
		hl_error("invalid date");
	size = (int)strftime(buf,127,"%Y-%m-%d %H:%M:%S",&t);
	out = (uchar*)hl_gc_alloc_noptr((size + 1) << 1);
	strtou(out,size,buf);
	out[size] = 0;
	*len = size;
	return (vbyte*)out;
}

HL_PRIM int hl_date_new( int y, int mo, int d, int h, int m, int s ) {
	struct tm t;
	memset(&t,0,sizeof(struct tm));
	t.tm_year = y - 1900;
	t.tm_mon = mo;
	t.tm_mday = d;
	t.tm_hour = h;
	t.tm_min = m;
	t.tm_sec = s;
	t.tm_isdst = -1;
	return (int)mktime(&t);
}

HL_PRIM void hl_date_get_inf( int date, int *y, int *mo, int *day, int *h, int *m, int *s, int *wday ) {
	struct tm t;
	time_t d = (time_t)date;
	if( !localtime_r(&d,&t) )
		hl_error("invalid date");
	if( y ) *y = t.tm_year + 1900;
	if( mo ) *mo = t.tm_mon;
	if( day ) *day = t.tm_mday;
	if( h ) *h = t.tm_hour;
	if( m ) *m = t.tm_min;
	if( s ) *s = t.tm_sec;
	if( wday ) *wday = t.tm_wday;
}
