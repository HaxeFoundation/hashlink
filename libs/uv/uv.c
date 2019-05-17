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

// ------------- TYPES ----------------------------------------------

// Handle types

#define _LOOP _ABSTRACT(uv_loop_t)
#define _HANDLE _ABSTRACT(uv_handle_t)
#define _DIR _ABSTRACT(uv_dir_t)
#define _STREAM _ABSTRACT(uv_stream_t)
#define _TCP _HANDLE
//_ABSTRACT(uv_tcp_t)
#define _UDP _ABSTRACT(uv_udp_t)
#define _PIPE _ABSTRACT(uv_pipe_t)
#define _TTY _ABSTRACT(uv_tty_t)
#define _POLL _ABSTRACT(uv_poll_t)
#define _TIMER _ABSTRACT(uv_timer_t)
#define _PREPARE _ABSTRACT(uv_prepare_t)
#define _CHECK _ABSTRACT(uv_check_t)
#define _IDLE _ABSTRACT(uv_idle_t)
#define _ASYNC _ABSTRACT(uv_async_t)
#define _PROCESS _ABSTRACT(uv_process_t)
#define _FS_EVENT _ABSTRACT(uv_fs_event_t)
#define _FS_POLL _ABSTRACT(uv_fs_poll_t)
#define _SIGNAL _ABSTRACT(uv_signal_t)

// Request types

#define _REQ _ABSTRACT(uv_req_t)
#define _GETADDRINFO _ABSTRACT(uv_getaddrinfo_t)
#define _GETNAMEINFO _ABSTRACT(uv_getnameinfo_t)
#define _SHUTDOWN _ABSTRACT(uv_shutdown_t)
#define _WRITE _ABSTRACT(uv_write_t)
#define _CONNECT _ABSTRACT(uv_connect_t)
#define _UDP_SEND _ABSTRACT(uv_udp_send_t)
#define _FS _ABSTRACT(uv_fs_t)
#define _WORK _ABSTRACT(uv_work_t)

// Other types

#define _CPU_INFO _ABSTRACT(uv_cpu_info_t)
#define _INTERFACE_ADDRESS _ABSTRACT(uv_interface_address_t)
#define _DIRENT _ABSTRACT(uv_dirent_t)
#define _PASSWD _ABSTRACT(uv_passwd_t)
#define _UTSNAME _ABSTRACT(uv_utsname_t)
#define _FILE _ABSTRACT(uv_file)
#define _BUF _ABSTRACT(uv_buf_t)

// Non-UV types

typedef struct sockaddr uv_sockaddr;
#define _SOCKADDR _ABSTRACT(uv_sockaddr)

// Error type
#define _ERROR _OBJ(_OBJ(_BYTES _I32))

// ------------- UTILITY MACROS -------------------------------------

#define UV_HANDLE_DATA(h) (((uv_handle_t *)(h))->data)
#define UV_REQ_DATA(r) (((uv_req_t *)(r))->data)
#define UV_DATA(h) ((events_data *)((h)->data))
#define UV_ALLOC(t) ((t *)malloc(sizeof(t)))

// ------------- MEMORY MANAGEMENT ----------------------------------
/*
static events_data *init_hl_data(void) {
	events_data *d = hl_gc_alloc_raw(sizeof(events_data));
	memset(d,0,sizeof(events_data));
	return d;
}

static void init_hl_data_handle(uv_handle_t *handle) {
	UV_HANDLE_DATA(h) = init_hl_data();
	hl_add_root(&UV_HANDLE_DATA(h));
}

static void init_hl_data_req(uv_req_t *req) {
	UV_REQ_DATA(h) = init_hl_data();
	hl_add_root(&UV_REQ_DATA(h));
}

static void clean_hl_data_handle(uv_handle_t *h) {
	hl_remove_root(UV_HANDLE_DATA(h));
	UV_HANDLE_DATA(h) = NULL;
	/ *
	events_data *ev = UV_DATA(h);
	if( !ev ) return;
	trigger_callb(h, EVT_CLOSE, NULL, 0, false);
	free(ev->write_data);
	hl_remove_root(&h->data);
	h->data = NULL;
	free(h);
	* /
}

static void clean_hl_data_req(uv_req_t *h) {
	hl_remove_root(UV_REQ_DATA(h));
	UV_REQ_DATA(h) = NULL;
}
*/

// ------------- ERROR HANDLING -------------------------------------

static vdynamic * (*construct_error)(vbyte *);

HL_PRIM void HL_NAME(glue_register_error)(vclosure *cb) {
	construct_error = (vdynamic * (*)(vbyte *))cb->fun;
}

DEFINE_PRIM(_VOID, glue_register_error, _FUN(_DYN, _BYTES));

#define UV_ALLOC_CHECK(var, type) \
	type *var = UV_ALLOC(type); \
	if (var == NULL) { \
		hl_throw(construct_error((vbyte *)"malloc " #type " failed")); \
	} else {}
#define UV_ERROR_CHECK(expr) do { \
		int __tmp_result = expr; \
		if (__tmp_result < 0) { \
			vdynamic *err = construct_error((vbyte *)strdup(uv_strerror(__tmp_result))); \
			hl_throw(err); \
		} \
	} while (0)

// ------------- LOOP -----------------------------------------------

HL_PRIM uv_loop_t * HL_NAME(w_loop_init)(void) {
	UV_ALLOC_CHECK(loop, uv_loop_t);
	UV_ERROR_CHECK(uv_loop_init(loop));
	return loop;
}

HL_PRIM bool HL_NAME(w_loop_close)(uv_loop_t *loop) {
	UV_ERROR_CHECK(uv_loop_close(loop));
	free(loop);
	return true;
}

HL_PRIM bool HL_NAME(w_run)(uv_loop_t *loop, uv_run_mode mode) {
	return uv_run(loop, mode) == 0;
}

HL_PRIM bool HL_NAME(w_loop_alive)(uv_loop_t *loop) {
	return uv_loop_alive(loop) != 0;
}

DEFINE_PRIM(_LOOP, w_loop_init, _NO_ARG);
// DEFINE_ PRIM(_I32, loop_configure, _LOOP ...);
DEFINE_PRIM(_BOOL, w_loop_close, _LOOP);
DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);
DEFINE_PRIM(_BOOL, w_run, _LOOP _I32);
DEFINE_PRIM(_BOOL, w_loop_alive, _LOOP);
DEFINE_PRIM(_VOID, stop, _LOOP);

// ------------- HANDLE ---------------------------------------------
// ------------- FILESYSTEM -----------------------------------------

void handle_fs_cb(uv_fs_t *req) {
	int result = req->result;
	vclosure *cb = (vclosure *)UV_REQ_DATA(req);
	uv_fs_req_cleanup(req);
	hl_remove_root(UV_REQ_DATA(req));
	free(req);
	vdynamic *err = NULL;
	if (result < 0) {
		err = construct_error((vbyte *)strdup(uv_strerror(result)));
	}
	vdynamic **args = &err;
	hl_dyn_call(cb, args, 1);
}

#define UV_WRAP(name, sign, call, ffi) \
	HL_PRIM void HL_NAME(w_ ## name)(uv_loop_t *loop, sign, vclosure *cb) { \
		UV_ALLOC_CHECK(req, uv_fs_t); \
		UV_REQ_DATA(req) = (void *)cb; \
		hl_add_root(UV_REQ_DATA(req)); \
		UV_ERROR_CHECK(uv_ ## name(loop, req, call, handle_fs_cb)); \
	} \
	DEFINE_PRIM(_VOID, w_ ## name, _LOOP ffi _FUN(_VOID, _ERROR));

#define COMMA ,
#define UV_WRAP1(name, arg1, sign) \
	UV_WRAP(name, arg1 _arg1, _arg1, sign)
#define UV_WRAP2(name, arg1, arg2, sign) \
	UV_WRAP(name, arg1 _arg1 COMMA arg2 _arg2, _arg1 COMMA _arg2, sign)
#define UV_WRAP3(name, arg1, arg2, arg3, sign) \
	UV_WRAP(name, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3, _arg1 COMMA _arg2 COMMA _arg3, sign)
#define UV_WRAP4(name, arg1, arg2, arg3, arg4, sign) \
	UV_WRAP(name, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3 COMMA arg4 _arg4, _arg1 COMMA _arg2 COMMA _arg3 COMMA _arg4, sign)

UV_WRAP1(fs_close, uv_file, _FILE);
UV_WRAP3(fs_open, const char*, int, int, _BYTES _I32 _I32);
UV_WRAP4(fs_read, uv_file, const uv_buf_t*, unsigned int, int64_t, _FILE _ARR _I32 _I64);
UV_WRAP1(fs_unlink, const char*, _BYTES);
UV_WRAP4(fs_write, uv_file, const uv_buf_t*, unsigned int, int64_t, _FILE _ARR _I32 _I64);
UV_WRAP2(fs_mkdir, const char*, int, _BYTES _I32);
UV_WRAP1(fs_mkdtemp, const char*, _BYTES);
UV_WRAP1(fs_rmdir, const char*, _BYTES);
UV_WRAP2(fs_scandir, const char*, int, _BYTES _I32);
UV_WRAP1(fs_stat, const char*, _BYTES);
UV_WRAP1(fs_fstat, uv_file, _FILE);
UV_WRAP1(fs_lstat, const char*, _BYTES);
UV_WRAP2(fs_rename, const char*, const char*, _BYTES _BYTES);
UV_WRAP1(fs_fsync, uv_file, _FILE);
UV_WRAP1(fs_fdatasync, uv_file, _FILE);
UV_WRAP2(fs_ftruncate, uv_file, int64_t, _FILE _I64);
UV_WRAP4(fs_sendfile, uv_file, uv_file, int64_t, size_t, _FILE _FILE _I64 _I64);
UV_WRAP2(fs_access, const char*, int, _BYTES _I32);
UV_WRAP2(fs_chmod, const char*, int, _BYTES _I32);
UV_WRAP2(fs_fchmod, uv_file, int, _FILE _I32);
UV_WRAP3(fs_utime, const char*, double, double, _BYTES _F64 _F64);
UV_WRAP3(fs_futime, uv_file, double, double, _FILE _F64 _F64);
UV_WRAP2(fs_link, const char*, const char*, _BYTES _BYTES);
UV_WRAP3(fs_symlink, const char*, const char*, int, _BYTES _BYTES _I32);
UV_WRAP1(fs_readlink, const char*, _BYTES);
UV_WRAP1(fs_realpath, const char*, _BYTES);
UV_WRAP3(fs_chown, const char*, uv_uid_t, uv_gid_t, _BYTES _I32 _I32);
UV_WRAP3(fs_fchown, uv_file, uv_uid_t, uv_gid_t, _FILE _I32 _I32);

#undef UV_WRAP1
#undef UV_WRAP2
#undef UV_WRAP3
#undef UV_WRAP
#undef COMMA

/*

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

#define _CALLB	_FUN(_VOID,_NO_ARG)

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
*/
