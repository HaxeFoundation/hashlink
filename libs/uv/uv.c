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

DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);
DEFINE_PRIM(_I32, loop_close, _LOOP);
DEFINE_PRIM(_I32, run, _LOOP _I32);
DEFINE_PRIM(_I32, loop_alive, _LOOP);
DEFINE_PRIM(_VOID, stop, _LOOP);

DEFINE_PRIM(_BYTES, strerror, _I32);
