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

#define EVT_CLOSE	1

#define EVT_READ	0	// stream
#define EVT_LISTEN	2	// stream

#define EVT_WRITE	0	// write_t
#define EVT_CONNECT	0	// connect_t

#define EVT_TIMER_TICK 0 // timer

#define EVT_MAX		2

typedef struct {
	vclosure *events[EVT_MAX + 1];
	void *write_data;
} events_data;

#define UV_DATA(h)		((events_data*)((h)->data))

#define _LOOP	_ABSTRACT(uv_loop)
#define _HANDLE _ABSTRACT(uv_handle)
#define _CALLB	_FUN(_VOID,_NO_ARG)
#define UV_ALLOC(t)		((t*)malloc(sizeof(t)))

// Errors

// static int errno_uv2hx( int uv_errno ) {
// 	switch(uv_errno) {
// 		case 0: return 0; break;
// 		case UV_E2BIG: return 1; break;
// 		case UV_EACCES: return 2; break;
// 		case UV_EADDRINUSE: return 3; break;
// 		case UV_EADDRNOTAVAIL: return 4; break;
// 		case UV_EAFNOSUPPORT: return 5; break;
// 		case UV_EAGAIN: return 6; break;
// 		case UV_EAI_ADDRFAMILY: return 7; break;
// 		case UV_EAI_AGAIN: return 8; break;
// 		case UV_EAI_BADFLAGS: return 9; break;
// 		case UV_EAI_BADHINTS: return 10; break;
// 		case UV_EAI_CANCELED: return 11; break;
// 		case UV_EAI_FAIL: return 11; break;
// 		case UV_EAI_FAMILY: return 12; break;
// 		case UV_EAI_MEMORY: return 13; break;
// 		case UV_EAI_NODATA: return 14; break;
// 		case UV_EAI_NONAME: return 15; break;
// 		case UV_EAI_OVERFLOW: return 16; break;
// 		case UV_EAI_PROTOCOL: return 17; break;
// 		case UV_EAI_SERVICE: return 18; break;
// 		case UV_EAI_SOCKTYPE: return 19; break;
// 		case UV_EALREADY: return 20; break;
// 		case UV_EBADF: return 21; break;
// 		case UV_EBUSY: return 22; break;
// 		case UV_ECANCELED: return 23; break;
// 		case UV_ECHARSET: return 24; break;
// 		case UV_ECONNABORTED: return 25; break;
// 		case UV_ECONNREFUSED: return 26; break;
// 		case UV_ECONNRESET: return 27; break;
// 		case UV_EDESTADDRREQ: return 28; break;
// 		case UV_EEXIST: return 29; break;
// 		case UV_EFAULT: return 30; break;
// 		case UV_EFBIG: return 31; break;
// 		case UV_EHOSTUNREACH: return 32; break;
// 		case UV_EINTR: return 33; break;
// 		case UV_EINVAL: return 34; break;
// 		case UV_EIO: return 35; break;
// 		case UV_EISCONN: return 36; break;
// 		case UV_EISDIR: return 37; break;
// 		case UV_ELOOP: return 38; break;
// 		case UV_EMFILE: return 39; break;
// 		case UV_EMSGSIZE: return 40; break;
// 		case UV_ENAMETOOLONG: return 41; break;
// 		case UV_ENETDOWN: return 42; break;
// 		case UV_ENETUNREACH: return 43; break;
// 		case UV_ENFILE: return 44; break;
// 		case UV_ENOBUFS: return 45; break;
// 		case UV_ENODEV: return 46; break;
// 		case UV_ENOENT: return 47; break;
// 		case UV_ENOMEM: return 48; break;
// 		case UV_ENONET: return 49; break;
// 		case UV_ENOPROTOOPT: return 50; break;
// 		case UV_ENOSPC: return 51; break;
// 		case UV_ENOSYS: return 52; break;
// 		case UV_ENOTCONN: return 53; break;
// 		case UV_ENOTDIR: return 54; break;
// 		case UV_ENOTEMPTY: return 55; break;
// 		case UV_ENOTSOCK: return 56; break;
// 		case UV_ENOTSUP: return 57; break;
// 		case UV_EPERM: return 58; break;
// 		case UV_EPIPE: return 59; break;
// 		case UV_EPROTO: return 60; break;
// 		case UV_EPROTONOSUPPORT: return 61; break;
// 		case UV_EPROTOTYPE: return 62; break;
// 		case UV_ERANGE: return 63; break;
// 		case UV_EROFS: return 64; break;
// 		case UV_ESHUTDOWN: return 65; break;
// 		case UV_ESPIPE: return 66; break;
// 		case UV_ESRCH: return 67; break;
// 		case UV_ETIMEDOUT: return 68; break;
// 		case UV_ETXTBSY: return 69; break;
// 		case UV_EXDEV: return 70; break;
// 		case UV_UNKNOWN: return 71; break;
// 		case UV_EOF: return 72; break;
// 		case UV_ENXIO: return 73; break;
// 		case UV_EMLINK: return 74; break;
// 		case UV_EHOSTDOWN: return 75; break;
// 		case UV_EREMOTEIO: return 76; break;
// 		case UV_ENOTTY: return 77; break;
// 		default: return UV_UNKNOWN;
// 	}
// }

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
	if( !h || !h->data ) return;
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
	if( h ) uv_close((uv_handle_t*)h, on_close);
}

HL_PRIM void HL_NAME(close_handle)( uv_handle_t *h, vclosure *c ) {
	register_callb(h, c, EVT_CLOSE);
	free_handle(h);
}

DEFINE_PRIM(_VOID, close_handle, _HANDLE _CALLB);

DEFINE_PRIM(_I32, is_active, _HANDLE);
DEFINE_PRIM(_I32, is_closing, _HANDLE);
DEFINE_PRIM(_VOID, ref, _HANDLE);
DEFINE_PRIM(_VOID, unref, _HANDLE);
DEFINE_PRIM(_I32, has_ref, _HANDLE);

// STREAM

static void on_write( uv_write_t *wr, int status ) {
	vdynamic b;
	vdynamic *args = &b;
	b.t = &hlt_bool;
	b.v.b = status == 0;
	trigger_callb((uv_handle_t*)wr,EVT_WRITE,&args,1,false);
	on_close((uv_handle_t*)wr);
}

HL_PRIM bool HL_NAME(stream_write)( uv_stream_t *s, vbyte *b, int size, vclosure *c ) {
	uv_write_t *wr = UV_ALLOC(uv_write_t);
	events_data *d = init_hl_data((uv_handle_t*)wr);
	// keep a copy of the data
	uv_buf_t buf;
	d->write_data = malloc(size);
	memcpy(d->write_data,b,size);
	buf.base = d->write_data;
	buf.len = size;
	register_callb((uv_handle_t*)wr,c,EVT_WRITE);
	if( uv_write(wr,s,&buf,1,on_write) < 0 ) {
		on_close((uv_handle_t*)wr);
		return false;
	}
	return true;
}

static void on_alloc( uv_handle_t* h, size_t size, uv_buf_t *buf ) {
	*buf = uv_buf_init(malloc(size), (int)size);
}

static void on_read( uv_stream_t *s, ssize_t nread, const uv_buf_t *buf ) {
	vdynamic bytes;
	vdynamic len;
	vdynamic *args[2];
	bytes.t = &hlt_bytes;
	bytes.v.ptr = buf->base;
	len.t = &hlt_i32;
	len.v.i = (int)nread;
	args[0] = &bytes;
	args[1] = &len;
	trigger_callb((uv_handle_t*)s,EVT_READ,args,2,true);
	free(buf->base);
}

HL_PRIM bool HL_NAME(stream_read_start)( uv_stream_t *s, vclosure *c ) {
	register_callb((uv_handle_t*)s,c,EVT_READ);
	return uv_read_start(s,on_alloc,on_read) >= 0;
}

HL_PRIM void HL_NAME(stream_read_stop)( uv_stream_t *s ) {
	uv_read_stop(s);
	clear_callb((uv_handle_t*)s,EVT_READ); // clear callback
}

static void on_listen( uv_stream_t *s, int status ) {
	trigger_callb((uv_handle_t*)s, EVT_LISTEN, NULL, 0, true);
}

HL_PRIM bool HL_NAME(stream_listen)( uv_stream_t *s, int count, vclosure *c ) {
	register_callb((uv_handle_t*)s,c,EVT_LISTEN);
	return uv_listen(s,count,on_listen) >= 0;
}

DEFINE_PRIM(_BOOL, stream_write, _HANDLE _BYTES _I32 _FUN(_VOID,_BOOL));
DEFINE_PRIM(_BOOL, stream_read_start, _HANDLE _FUN(_VOID,_BYTES _I32));
DEFINE_PRIM(_VOID, stream_read_stop, _HANDLE);
DEFINE_PRIM(_BOOL, stream_listen, _HANDLE _I32 _CALLB);

// Timer
HL_PRIM uv_timer_t *HL_NAME(timer_init_wrap)( uv_loop_t *loop ) {
	uv_timer_t *t = UV_ALLOC(uv_timer_t);
	int result = uv_timer_init(loop,t);
	if(result < 0) {
		free(t);
		hx_error(result);
		return NULL;
	}
	init_hl_data((uv_handle_t*)t);
	return t;
}
DEFINE_PRIM(_HANDLE, timer_init_wrap, _LOOP);

static void on_timer_tick( uv_timer_t *t ) {
	trigger_callb((uv_handle_t*)t, EVT_TIMER_TICK, NULL, 0, true);
}

// TODO: change `timeout` and `repeat` to uint64
HL_PRIM void HL_NAME(timer_start_wrap)(uv_timer_t *t, vclosure *c, int timeout, int repeat) {
	register_callb((uv_handle_t*)t,c,EVT_TIMER_TICK);
	int result = uv_timer_start(t,on_timer_tick, (uint64_t)timeout, (uint64_t)repeat);
	if(result < 0) {
		clear_callb((uv_handle_t*)t, EVT_TIMER_TICK);
		hx_error(result);
	}
}
DEFINE_PRIM(_VOID, timer_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG) _I32 _I32);

HL_PRIM void HL_NAME(timer_stop_wrap)(uv_timer_t *t) {
	clear_callb((uv_handle_t*)t, EVT_TIMER_TICK);
	int result = uv_timer_stop(t);
	if(result < 0) {
		hx_error(result);
	}
}
DEFINE_PRIM(_VOID, timer_stop_wrap, _HANDLE);

HL_PRIM void HL_NAME(timer_again_wrap)(uv_timer_t *t) {
	int result = uv_timer_again(t);
	if(result < 0) {
		hx_error(result);
	}
}
DEFINE_PRIM(_VOID, timer_again_wrap, _HANDLE);

HL_PRIM int HL_NAME(timer_get_due_in_wrap)(uv_timer_t *t) {
	return (int)uv_timer_get_due_in(t); //TODO: change to uint64_t
}
DEFINE_PRIM(_I32, timer_get_due_in_wrap, _HANDLE);

HL_PRIM int HL_NAME(timer_get_repeat_wrap)(uv_timer_t *t) {
	return (int)uv_timer_get_repeat(t); //TODO: change to uint64_t
}
DEFINE_PRIM(_I32, timer_get_repeat_wrap, _HANDLE);

//TODO: change `value` to uint64_t
HL_PRIM int HL_NAME(timer_set_repeat_wrap)(uv_timer_t *t, int value) {
	uv_timer_set_repeat(t, (uint64_t)value);
	return value;
}
DEFINE_PRIM(_I32, timer_set_repeat_wrap, _HANDLE _I32);

// TCP

#define _TCP _HANDLE

HL_PRIM uv_tcp_t *HL_NAME(tcp_init_wrap)( uv_loop_t *loop ) {
	uv_tcp_t *t = UV_ALLOC(uv_tcp_t);
	if( uv_tcp_init(loop,t) < 0 ) {
		free(t);
		return NULL;
	}
	init_hl_data((uv_handle_t*)t);
	return t;
}

static void on_connect( uv_connect_t *cnx, int status ) {
	vdynamic b;
	vdynamic *args = &b;
	b.t = &hlt_bool;
	b.v.b = status == 0;
	trigger_callb((uv_handle_t*)cnx,EVT_CONNECT,&args,1,false);
	on_close((uv_handle_t*)cnx);
}

HL_PRIM uv_connect_t *HL_NAME(tcp_connect_wrap)( uv_tcp_t *t, int host, int port, vclosure *c ) {
	uv_connect_t *cnx = UV_ALLOC(uv_connect_t);
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	*(int*)&addr.sin_addr.s_addr = host;
	if( !t || uv_tcp_connect(cnx,t,(uv_sockaddr *)&addr,on_connect) < 0 ) {
		free(cnx);
		return NULL;
	}
	memset(&addr,0,sizeof(addr));
	init_hl_data((uv_handle_t*)cnx);
	register_callb((uv_handle_t*)cnx, c, EVT_CONNECT);
	return cnx;
}

HL_PRIM bool HL_NAME(tcp_bind_wrap)( uv_tcp_t *t, int host, int port ) {
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((unsigned short)port);
	*(int*)&addr.sin_addr.s_addr = host;
	return uv_tcp_bind(t,(uv_sockaddr *)&addr,0) >= 0;
}


HL_PRIM uv_tcp_t *HL_NAME(tcp_accept_wrap)( uv_tcp_t *t ) {
	uv_tcp_t *client = UV_ALLOC(uv_tcp_t);
	if( uv_tcp_init(t->loop, client) < 0 ) {
		free(client);
		return NULL;
	}
	if( uv_accept((uv_stream_t*)t,(uv_stream_t*)client) < 0 ) {
		uv_close((uv_handle_t*)client, NULL);
		return NULL;
	}
	init_hl_data((uv_handle_t*)client);
	return client;
}

HL_PRIM void HL_NAME(tcp_nodelay_wrap)( uv_tcp_t *t, bool enable ) {
	uv_tcp_nodelay(t,enable?1:0);
}

DEFINE_PRIM(_TCP, tcp_init_wrap, _LOOP);
DEFINE_PRIM(_HANDLE, tcp_connect_wrap, _TCP _I32 _I32 _FUN(_VOID,_BOOL));
DEFINE_PRIM(_BOOL, tcp_bind_wrap, _TCP _I32 _I32);
DEFINE_PRIM(_HANDLE, tcp_accept_wrap, _HANDLE);
DEFINE_PRIM(_VOID, tcp_nodelay_wrap, _TCP _BOOL);

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
DEFINE_PRIM(_I32, loop_close, _LOOP);
DEFINE_PRIM(_I32, run, _LOOP _I32);
DEFINE_PRIM(_I32, loop_alive, _LOOP);
DEFINE_PRIM(_VOID, stop, _LOOP);

DEFINE_PRIM(_BYTES, strerror, _I32);
