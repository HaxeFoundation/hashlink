#define HL_NAME(n) uv_##n
#ifdef _WIN32
#	include <uv.h>
#	include <hl.h>
#else
#	include <hl.h>
#	include <uv.h>
#endif

#if (UV_VERSION_MAJOR <= 0)
#	error "libuv1-dev required, uv version 0.x found"
#endif

typedef struct sockaddr uv_sockaddr;
typedef struct sockaddr_in uv_sockaddr_in;
typedef struct sockaddr_in6 uv_sockaddr_in6;
typedef struct sockaddr_storage uv_sockaddr_storage;

#define EVT_CLOSE	1

#define EVT_STREAM_READ		0
#define EVT_STREAM_LISTEN	2

#define EVT_CONNECT	0	// connect_t

#define EVT_MAX		2 // !!!!!!!!!!!!!!

typedef struct {
	vclosure *events[EVT_MAX + 1];
	void *write_data;
} events_data;

#define UV_DATA(h)		((events_data*)((h)->data))

#define _LOOP	_ABSTRACT(uv_loop)
#define _HANDLE _ABSTRACT(uv_handle)
#define _SOCKADDR _ABSTRACT(uv_sockaddr_storage)
#define _CALLB	_FUN(_VOID,_NO_ARG)
#define UV_ALLOC(t)		((t*)malloc(sizeof(t)))
#define UV_CHECK_NULL(handle,fail_return) \
	if( !handle ) { \
		hl_null_access(); \
		return fail_return; \
	}

#define UV_CHECK_ERROR(action,cleanup,fail_return) \
	int __result__ = action; \
	if(__result__ < 0) { \
		cleanup; \
		hx_error(__result__); \
		return fail_return; \
	}

// Errors

static int errno_uv2hx( int uv_errno ) {
	switch(uv_errno) {
		case 0: return 0; break;
		case UV_E2BIG: return 1; break;
		case UV_EACCES: return 2; break;
		case UV_EADDRINUSE: return 3; break;
		case UV_EADDRNOTAVAIL: return 4; break;
		case UV_EAFNOSUPPORT: return 5; break;
		case UV_EAGAIN: return 6; break;
		case UV_EAI_ADDRFAMILY: return 7; break;
		case UV_EAI_AGAIN: return 8; break;
		case UV_EAI_BADFLAGS: return 9; break;
		case UV_EAI_BADHINTS: return 10; break;
		case UV_EAI_CANCELED: return 11; break;
		case UV_EAI_FAIL: return 11; break;
		case UV_EAI_FAMILY: return 12; break;
		case UV_EAI_MEMORY: return 13; break;
		case UV_EAI_NODATA: return 14; break;
		case UV_EAI_NONAME: return 15; break;
		case UV_EAI_OVERFLOW: return 16; break;
		case UV_EAI_PROTOCOL: return 17; break;
		case UV_EAI_SERVICE: return 18; break;
		case UV_EAI_SOCKTYPE: return 19; break;
		case UV_EALREADY: return 20; break;
		case UV_EBADF: return 21; break;
		case UV_EBUSY: return 22; break;
		case UV_ECANCELED: return 23; break;
		case UV_ECHARSET: return 24; break;
		case UV_ECONNABORTED: return 25; break;
		case UV_ECONNREFUSED: return 26; break;
		case UV_ECONNRESET: return 27; break;
		case UV_EDESTADDRREQ: return 28; break;
		case UV_EEXIST: return 29; break;
		case UV_EFAULT: return 30; break;
		case UV_EFBIG: return 31; break;
		case UV_EHOSTUNREACH: return 32; break;
		case UV_EINTR: return 33; break;
		case UV_EINVAL: return 34; break;
		case UV_EIO: return 35; break;
		case UV_EISCONN: return 36; break;
		case UV_EISDIR: return 37; break;
		case UV_ELOOP: return 38; break;
		case UV_EMFILE: return 39; break;
		case UV_EMSGSIZE: return 40; break;
		case UV_ENAMETOOLONG: return 41; break;
		case UV_ENETDOWN: return 42; break;
		case UV_ENETUNREACH: return 43; break;
		case UV_ENFILE: return 44; break;
		case UV_ENOBUFS: return 45; break;
		case UV_ENODEV: return 46; break;
		case UV_ENOENT: return 47; break;
		case UV_ENOMEM: return 48; break;
		case UV_ENONET: return 49; break;
		case UV_ENOPROTOOPT: return 50; break;
		case UV_ENOSPC: return 51; break;
		case UV_ENOSYS: return 52; break;
		case UV_ENOTCONN: return 53; break;
		case UV_ENOTDIR: return 54; break;
		case UV_ENOTEMPTY: return 55; break;
		case UV_ENOTSOCK: return 56; break;
		case UV_ENOTSUP: return 57; break;
		case UV_EPERM: return 58; break;
		case UV_EPIPE: return 59; break;
		case UV_EPROTO: return 60; break;
		case UV_EPROTONOSUPPORT: return 61; break;
		case UV_EPROTOTYPE: return 62; break;
		case UV_ERANGE: return 63; break;
		case UV_EROFS: return 64; break;
		case UV_ESHUTDOWN: return 65; break;
		case UV_ESPIPE: return 66; break;
		case UV_ESRCH: return 67; break;
		case UV_ETIMEDOUT: return 68; break;
		case UV_ETXTBSY: return 69; break;
		case UV_EXDEV: return 70; break;
		case UV_UNKNOWN: return 71; break;
		case UV_EOF: return 72; break;
		case UV_ENXIO: return 73; break;
		case UV_EMLINK: return 74; break;
		case UV_EHOSTDOWN: return 75; break;
		case UV_EREMOTEIO: return 76; break;
		case UV_ENOTTY: return 77; break;
		default: return 71; //UV_UNKNOWN;
	}
}

static void hx_error(int uv_errno) {
	//TODO: throw hl.uv.UVException
	hl_error("%s", hl_to_utf16(uv_err_name(uv_errno)));
}

// HANDLE

static events_data *init_hl_data( uv_handle_t *h ) {
	events_data *d = hl_gc_alloc_raw(sizeof(events_data));
	memset(d,0,sizeof(events_data));
	hl_add_root(&h->data);
	h->data = d;
	return d;
}

static void register_callb( uv_handle_t *h, vclosure *c, int event_kind ) {
	if( !h || !h->data )
		hl_fatal("Missing handle or handle data");
	UV_DATA(h)->events[event_kind] = c;
}

static void clear_callb( uv_handle_t *h, int event_kind ) {
	register_callb(h,NULL,event_kind);
}

static void trigger_callb( uv_handle_t *h, int event_kind, vdynamic **args, int nargs, bool repeat ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[event_kind] : NULL;
	if( !c ) return;
	if( !repeat ) ev->events[event_kind] = NULL;
	hl_dyn_call(c, args, nargs);
}

static void on_close( uv_handle_t *h ) {
	events_data *ev = UV_DATA(h);
	if( !ev ) return;
	trigger_callb(h, EVT_CLOSE, NULL, 0, false);
	free(ev->write_data);
	hl_remove_root(&h->data);
	h->data = NULL;
	free(h);
}

static void free_handle( void *h ) {
	uv_close((uv_handle_t*)h, on_close);
}

static void free_request( void *r ) {
	clear_callb((uv_handle_t*)r,0);
	free(r);
}

HL_PRIM void HL_NAME(close_wrap)( uv_handle_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	register_callb(h, c, EVT_CLOSE);
	free_handle(h);
}
DEFINE_PRIM(_VOID, close_wrap, _HANDLE _CALLB);

HL_PRIM bool HL_NAME(is_active_wrap)( uv_handle_t *h ) {
	UV_CHECK_NULL(h,false);
	return uv_is_active(h) != 0;
}
DEFINE_PRIM(_BOOL, is_active_wrap, _HANDLE);

HL_PRIM bool HL_NAME(is_closing_wrap)( uv_handle_t *h ) {
	UV_CHECK_NULL(h,false);
	return uv_is_closing(h) != 0;
}
DEFINE_PRIM(_BOOL, is_closing_wrap, _HANDLE);

HL_PRIM bool HL_NAME(has_ref_wrap)( uv_handle_t *h ) {
	UV_CHECK_NULL(h,false);
	return uv_has_ref(h) != 0;
}
DEFINE_PRIM(_BOOL, has_ref_wrap, _HANDLE);

HL_PRIM void HL_NAME(ref_wrap)( uv_handle_t *h ) {
	UV_CHECK_NULL(h,);
	uv_ref(h);
}
DEFINE_PRIM(_VOID, ref_wrap, _HANDLE);

HL_PRIM void HL_NAME(unref_wrap)( uv_handle_t *h ) {
	UV_CHECK_NULL(h,);
	uv_unref(h);
}
DEFINE_PRIM(_VOID, unref_wrap, _HANDLE);

// STREAM

static void on_shutdown( uv_shutdown_t *r, int status ) {
	events_data *ev = UV_DATA(r);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in shutdown request");
	free_request(r);
	hl_call1(void, c, int, errno_uv2hx(status));
}

HL_PRIM void HL_NAME(shutdown_wrap)( uv_stream_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	uv_shutdown_t *req = UV_ALLOC(uv_shutdown_t);
	register_callb((uv_handle_t*)req,c,0);
	UV_CHECK_ERROR(uv_shutdown(req, h, on_shutdown),free_request(req),);
}
DEFINE_PRIM(_VOID, shutdown_wrap, _HANDLE _FUN(_VOID,_I32));

static void on_listen( uv_stream_t *h, int status ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[EVT_STREAM_LISTEN] : NULL;
	if( !c )
		hl_fatal("No listen callback in stream handle");
	hl_call1(void, c, int, errno_uv2hx(status));
}

HL_PRIM void HL_NAME(listen_wrap)( uv_stream_t *h, int backlog, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,EVT_STREAM_LISTEN);
	UV_CHECK_ERROR(uv_listen(h, backlog, on_listen),clear_callb((uv_handle_t*)h,EVT_STREAM_LISTEN),);
}
DEFINE_PRIM(_VOID, listen_wrap, _HANDLE _BYTES _I32 _FUN(_VOID,_BOOL));

HL_PRIM void HL_NAME(accept_wrap)( uv_stream_t *h, uv_stream_t *client ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(client,);
	UV_CHECK_ERROR(uv_accept(h, client),,);
}
DEFINE_PRIM(_VOID, accept_wrap, _HANDLE _HANDLE);

static void on_alloc( uv_handle_t* h, size_t size, uv_buf_t *buf ) {
	*buf = uv_buf_init(malloc(size), (int)size);
}

static void on_read( uv_stream_t *h, ssize_t nread, const uv_buf_t *buf ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[EVT_STREAM_READ] : NULL;
	if( !c )
		hl_fatal("No listen callback in stream handle");
	if( nread < 0 ) {
		hl_call3(void, c, int, errno_uv2hx(nread), vbyte *, NULL, int, 0);
	} else {
		hl_call3(void, c, int, 0, vbyte *, (vbyte *)buf->base, int, nread);
	}
	free(buf->base);
}

HL_PRIM void HL_NAME(read_start_wrap)( uv_stream_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,EVT_STREAM_READ);
	UV_CHECK_ERROR(uv_read_start(h,on_alloc,on_read), clear_callb((uv_handle_t*)h,EVT_STREAM_READ),);
}
DEFINE_PRIM(_VOID, read_start_wrap, _HANDLE _FUN(_VOID,_I32 _BYTES _I32));

HL_PRIM void HL_NAME(read_stop_wrap)( uv_stream_t *h ) {
	UV_CHECK_NULL(h,);
	clear_callb((uv_handle_t*)h,EVT_STREAM_READ);
	uv_read_stop(h);
}
DEFINE_PRIM(_VOID, read_stop_wrap, _HANDLE);

static void on_write( uv_write_t *r, int status ) {
	events_data *ev = UV_DATA(r);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in write request");
	hl_call1(void, c, int, status < 0 ? errno_uv2hx(status) : 0);
	free_request(r);
}

HL_PRIM void HL_NAME(write_wrap)( uv_stream_t *h, vbyte *b, int length, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(b,);
	UV_CHECK_NULL(c,);
	uv_write_t *r = UV_ALLOC(uv_write_t);
	events_data *d = init_hl_data((uv_handle_t*)r);
	// keep a copy of the data
	uv_buf_t buf;
	d->write_data = malloc(length);
	memcpy(d->write_data,b,length);
	buf.base = d->write_data;
	buf.len = length;
	register_callb((uv_handle_t*)r,c,0);
	UV_CHECK_ERROR(uv_write(r,h,&buf,1,on_write),free_request(r),);
}
DEFINE_PRIM(_VOID, write_wrap, _HANDLE _BYTES _I32 _FUN(_VOID,_I32));

HL_PRIM int HL_NAME(try_write_wrap)( uv_stream_t *h, vbyte *b, int length ) {
	UV_CHECK_NULL(h,0);
	UV_CHECK_NULL(b,0);
	uv_buf_t buf = uv_buf_init((char *)b, length);
	int result = uv_try_write(h,&buf,1);
	if( result < 0 ) {
		hx_error(result);
		return 0;
	}
	return result;
}
DEFINE_PRIM(_VOID, try_write_wrap, _HANDLE _BYTES _I32);

HL_PRIM bool HL_NAME(is_readable_wrap)( uv_stream_t *h ) {
	UV_CHECK_NULL(h,false);
	return uv_is_readable(h) != 0;
}
DEFINE_PRIM(_BOOL, is_readable_wrap, _HANDLE);

HL_PRIM bool HL_NAME(is_writable_wrap)( uv_stream_t *h ) {
	UV_CHECK_NULL(h,false);
	return uv_is_writable(h) != 0;
}
DEFINE_PRIM(_BOOL, is_writable_wrap, _HANDLE);

// Timer
HL_PRIM uv_timer_t *HL_NAME(timer_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_timer_t *t = UV_ALLOC(uv_timer_t);
	UV_CHECK_ERROR(uv_timer_init(loop,t),free(t),NULL);
	init_hl_data((uv_handle_t*)t);
	return t;
}
DEFINE_PRIM(_HANDLE, timer_init_wrap, _LOOP);

static void on_timer( uv_timer_t *h ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in timer handle");
	hl_call0(void, c);
}

// TODO: change `timeout` and `repeat` to uint64
HL_PRIM void HL_NAME(timer_start_wrap)(uv_timer_t *t, vclosure *c, int timeout, int repeat) {
	UV_CHECK_NULL(t,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)t,c,0);
	UV_CHECK_ERROR(
		uv_timer_start(t,on_timer, (uint64_t)timeout, (uint64_t)repeat),
		clear_callb((uv_handle_t*)t,0),
	);
}
DEFINE_PRIM(_VOID, timer_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG) _I32 _I32);

HL_PRIM void HL_NAME(timer_stop_wrap)(uv_timer_t *t) {
	UV_CHECK_NULL(t,);
	clear_callb((uv_handle_t*)t,0);
	UV_CHECK_ERROR(uv_timer_stop(t),,);
}
DEFINE_PRIM(_VOID, timer_stop_wrap, _HANDLE);

HL_PRIM void HL_NAME(timer_again_wrap)(uv_timer_t *t) {
	UV_CHECK_NULL(t,);
	UV_CHECK_ERROR(uv_timer_again(t),,);
}
DEFINE_PRIM(_VOID, timer_again_wrap, _HANDLE);

HL_PRIM int HL_NAME(timer_get_due_in_wrap)(uv_timer_t *t) {
	UV_CHECK_NULL(t,0);
	return (int)uv_timer_get_due_in(t); //TODO: change to uint64_t
}
DEFINE_PRIM(_I32, timer_get_due_in_wrap, _HANDLE);

HL_PRIM int HL_NAME(timer_get_repeat_wrap)(uv_timer_t *t) {
	UV_CHECK_NULL(t,0);
	return (int)uv_timer_get_repeat(t); //TODO: change to uint64_t
}
DEFINE_PRIM(_I32, timer_get_repeat_wrap, _HANDLE);

//TODO: change `value` to uint64_t
HL_PRIM int HL_NAME(timer_set_repeat_wrap)(uv_timer_t *t, int value) {
	UV_CHECK_NULL(t,0);
	uv_timer_set_repeat(t, (uint64_t)value);
	return value;
}
DEFINE_PRIM(_I32, timer_set_repeat_wrap, _HANDLE _I32);

// Async

static void on_async( uv_async_t *a ) {
	events_data *ev = UV_DATA(a);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in async handle");
	hl_call1(void, c, uv_async_t *, a);
}

HL_PRIM uv_async_t *HL_NAME(async_init_wrap)( uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,NULL);
	UV_CHECK_NULL(c,NULL);
	uv_async_t *a = UV_ALLOC(uv_async_t);
	UV_CHECK_ERROR(uv_async_init(loop,a,on_async),free(a),NULL);
	init_hl_data((uv_handle_t*)a);
	register_callb((uv_handle_t*)a,c,0);
	return a;
}
DEFINE_PRIM(_HANDLE, async_init_wrap, _LOOP _FUN(_VOID,_HANDLE));

HL_PRIM void HL_NAME(async_send_wrap)( uv_async_t *a ) {
	UV_CHECK_NULL(a,);
	UV_CHECK_ERROR(uv_async_send(a),,);
}
DEFINE_PRIM(_VOID, async_send_wrap, _HANDLE);

// Idle

static void on_idle( uv_idle_t *h ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in idle handle");
	hl_call0(void, c);
}

HL_PRIM uv_idle_t *HL_NAME(idle_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_idle_t *h = UV_ALLOC(uv_idle_t);
	UV_CHECK_ERROR(uv_idle_init(loop,h),free(h),NULL);
	init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, idle_init_wrap, _LOOP);

HL_PRIM void HL_NAME(idle_start_wrap)( uv_idle_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,0);
	uv_idle_start(h, on_idle);
}
DEFINE_PRIM(_VOID, idle_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG));

HL_PRIM void HL_NAME(idle_stop_wrap)( uv_idle_t *h ) {
	UV_CHECK_NULL(h,);
	uv_idle_stop(h);
}
DEFINE_PRIM(_VOID, idle_stop_wrap, _HANDLE);

// Prepare

static void on_prepare( uv_prepare_t *h ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in prepare handle");
	hl_call0(void, c);
}

HL_PRIM uv_prepare_t *HL_NAME(prepare_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_prepare_t *h = UV_ALLOC(uv_prepare_t);
	UV_CHECK_ERROR(uv_prepare_init(loop,h),free(h),NULL);
	init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, prepare_init_wrap, _LOOP);

HL_PRIM void HL_NAME(prepare_start_wrap)( uv_prepare_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,0);
	uv_prepare_start(h, on_prepare);
}
DEFINE_PRIM(_VOID, prepare_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG));

HL_PRIM void HL_NAME(prepare_stop_wrap)( uv_prepare_t *h ) {
	UV_CHECK_NULL(h,);
	uv_prepare_stop(h);
}
DEFINE_PRIM(_VOID, prepare_stop_wrap, _HANDLE);

// Check

static void on_check( uv_check_t *h ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in check handle");
	hl_call0(void, c);
}

HL_PRIM uv_check_t *HL_NAME(check_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_check_t *h = UV_ALLOC(uv_check_t);
	UV_CHECK_ERROR(uv_check_init(loop,h),free(h),NULL);
	init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, check_init_wrap, _LOOP);

HL_PRIM void HL_NAME(check_start_wrap)( uv_check_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,0);
	uv_check_start(h, on_check);
}
DEFINE_PRIM(_VOID, check_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG));

HL_PRIM void HL_NAME(check_stop_wrap)( uv_check_t *h ) {
	UV_CHECK_NULL(h,);
	uv_check_stop(h);
}
DEFINE_PRIM(_VOID, check_stop_wrap, _HANDLE);

// Signal

static int signum_hx2uv( int hx ) {
	switch(hx) {
		case -1: return SIGABRT; break;
		case -2: return SIGFPE; break;
		case -3: return SIGHUP; break;
		case -4: return SIGILL; break;
		case -5: return SIGINT; break;
		case -6: return SIGKILL; break;
		case -7: return SIGSEGV; break;
		case -8: return SIGTERM; break;
		case -9: return SIGWINCH; break;
		default: return hx; break;
	}
}

static int signum_uv2hx( int uv ) {
	switch(uv) {
		case SIGABRT: return -1; break;
		case SIGFPE: return -2; break;
		case SIGHUP: return -3; break;
		case SIGILL: return -4; break;
		case SIGINT: return -5; break;
		case SIGKILL: return -6; break;
		case SIGSEGV: return -7; break;
		case SIGTERM: return -8; break;
		case SIGWINCH: return -9; break;
		default: return uv; break;
	}
}

static void on_signal( uv_signal_t *h, int signum ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in signal handle");
	hl_call1(void, c, int, signum_uv2hx(signum));
}

static void on_signal_oneshot( uv_signal_t *h, int signum ) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( !c )
		hl_fatal("No callback in signal handle");
	clear_callb((uv_handle_t *)h,0);
	hl_call1(void, c, int, signum_uv2hx(signum));
}

HL_PRIM uv_signal_t *HL_NAME(signal_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_signal_t *h = UV_ALLOC(uv_signal_t);
	UV_CHECK_ERROR(uv_signal_init(loop,h),free(h),NULL);
	init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, signal_init_wrap, _LOOP);

HL_PRIM void HL_NAME(signal_start_wrap)( uv_signal_t *h, int signum, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,0);
	UV_CHECK_ERROR(uv_signal_start(h, on_signal, signum_hx2uv(signum)),clear_callb((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, signal_start_wrap, _HANDLE _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(signal_start_oneshot_wrap)( uv_signal_t *h, int signum, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	register_callb((uv_handle_t*)h,c,0);
	UV_CHECK_ERROR(uv_signal_start_oneshot(h, on_signal_oneshot, signum_hx2uv(signum)),clear_callb((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, signal_start_oneshot_wrap, _HANDLE _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(signal_stop_wrap)( uv_signal_t *h ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_signal_stop(h),,);
}
DEFINE_PRIM(_VOID, signal_stop_wrap, _HANDLE);

HL_PRIM int HL_NAME(signal_get_sigNum_wrap)(uv_signal_t *h) {
	UV_CHECK_NULL(h,0);
	return signum_uv2hx(h->signum);
}
DEFINE_PRIM(_I32, signal_get_sigNum_wrap, _HANDLE);

// Sockaddr

HL_PRIM uv_sockaddr_storage *HL_NAME(ip4_addr_wrap)( vstring *ip, int port ) {
	UV_CHECK_NULL(ip,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_ip4_addr(hl_to_utf8(ip->bytes), port, (uv_sockaddr_in *)addr),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR, ip4_addr_wrap, _STRING _I32);

HL_PRIM uv_sockaddr_storage *HL_NAME(ip6_addr_wrap)( vstring *ip, int port ) {
	UV_CHECK_NULL(ip,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_ip6_addr(hl_to_utf8(ip->bytes), port, (uv_sockaddr_in6 *)addr),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR, ip6_addr_wrap, _STRING _I32);

HL_PRIM vdynamic *HL_NAME(get_port)( uv_sockaddr_storage *addr ) {
	UV_CHECK_NULL(addr,NULL);
	int port;
	if( addr->ss_family == AF_INET ) {
		port = ntohs(((uv_sockaddr_in *)addr)->sin_port);
	} else if( addr->ss_family == AF_INET6 ) {
		port = ntohs(((uv_sockaddr_in6 *)addr)->sin6_port);
	} else {
		return NULL;
	}
	return hl_make_dyn(&port, &hlt_i32);
}
DEFINE_PRIM(_NULL(_I32), get_port, _SOCKADDR);

//How to return vstring instead of vbyte?
HL_PRIM vbyte *HL_NAME(ip_name_wrap)( uv_sockaddr_storage *addr ) {
	UV_CHECK_NULL(addr,NULL);
	vbyte *dst = hl_alloc_bytes(128);
	if( addr->ss_family == AF_INET ) {
		UV_CHECK_ERROR(uv_ip4_name((uv_sockaddr_in *)addr, (char *)dst, 128),,NULL); //free bytes?
	} else if( addr->ss_family == AF_INET6 ) {
		UV_CHECK_ERROR(uv_ip6_name((uv_sockaddr_in6 *)addr, (char *)dst, 128),,NULL);
	} else {
		return NULL;
	}
	return dst;
}
DEFINE_PRIM(_BYTES, ip_name_wrap, _SOCKADDR);


// loop

HL_PRIM uv_loop_t *HL_NAME(loop_init_wrap)( ) {
	uv_loop_t *loop = UV_ALLOC(uv_loop_t);
	if( uv_loop_init(loop) < 0) {
		free(loop);
		//TODO: throw error
		return NULL;
	}
	return loop;
}
DEFINE_PRIM(_LOOP, loop_init_wrap, _NO_ARG);

DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);

HL_PRIM void HL_NAME(loop_close_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_ERROR(uv_loop_close(loop),,);
}
DEFINE_PRIM(_VOID, loop_close_wrap, _LOOP);

HL_PRIM bool HL_NAME(run_wrap)( uv_loop_t *loop, int mode ) {
	UV_CHECK_NULL(loop,false);
	return uv_run(loop, mode) != 0;
}
DEFINE_PRIM(_BOOL, run_wrap, _LOOP _I32);

HL_PRIM bool HL_NAME(loop_alive_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,false);
	return uv_loop_alive(loop) != 0;
}
DEFINE_PRIM(_BOOL, loop_alive_wrap, _LOOP);

HL_PRIM void HL_NAME(stop_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,);
	uv_stop(loop);
}
DEFINE_PRIM(_VOID, stop_wrap, _LOOP);

DEFINE_PRIM(_BYTES, strerror, _I32);
