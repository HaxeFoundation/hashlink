#define HL_NAME(n) uv_##n
#ifdef _WIN32
#	include <uv.h>
#	include <hl.h>
#else
#	include <hl.h>
#	include <uv.h>
# include <stdlib.h>
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
#define _TCP _ABSTRACT(uv_tcp_t)
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
// #define _STAT _ABSTRACT(uv_stat_t)
#define _BUF _ABSTRACT(uv_buf_t)

// Haxe types

#define _ERROR _DYN
#define _STAT _OBJ(_I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32)

// Callback types

#define _CB _FUN(_VOID, _ERROR)
#define _CB_STR _FUN(_VOID, _ERROR _BYTES)
#define _CB_BYTES _FUN(_VOID, _ERROR _BYTES _I32)
#define _CB_INT _FUN(_VOID, _ERROR _I32)
#define _CB_FILE _FUN(_VOID, _ERROR _FILE)
#define _CB_STAT _FUN(_VOID, _ERROR _STAT)
#define _CB_SCANDIR _FUN(_VOID, _ERROR _ARR)
#define _CB_FS_EVENT _FUN(_VOID, _ERROR _BYTES _I32)
#define _CB_GAI _FUN(_VOID, _ERROR _ARR)
#define _CB_GNI _FUN(_VOID, _ERROR _BYTES _BYTES)
#define _CB_UDP_RECV _FUN(_VOID, _ERROR _BYTES _I32 _DYN)

// ------------- UTILITY MACROS -------------------------------------

// access the data of a handle or request
#define UV_HANDLE_DATA(h) (((uv_handle_t *)(h))->data)
#define UV_HANDLE_DATA_SUB(h, t) ((t *)((uv_handle_t *)(h))->data)
#define UV_REQ_DATA(r) (((uv_req_t *)(r))->data)

#define UV_ALLOC(t) ((t *)malloc(sizeof(t)))
#define UV_GC_ALLOC(t) ((t *)hl_gc_alloc_noptr(sizeof(t)))

// ------------- HAXE CONSTRUCTORS ----------------------------------

static vdynamic * (*construct_error)(vbyte *, int);
static vdynamic * (*construct_fs_stat)(int, int, int, int, int, int, int, int, int, int, int, int);
static vdynamic * (*construct_fs_dirent)(const char *name, int type);
static vdynamic * (*construct_addrinfo_ipv4)(int ip);
static vdynamic * (*construct_addrinfo_ipv6)(vbyte *ip);
static vdynamic * (*construct_addrport)(vdynamic *addr, int port);

HL_PRIM void HL_NAME(glue_register)(
	vclosure *c_error,
	vclosure *c_fs_stat,
	vclosure *c_fs_dirent,
	vclosure *c_addrinfo_ipv4,
	vclosure *c_addrinfo_ipv6,
	vclosure *c_addrport
) {
	construct_error = (vdynamic * (*)(vbyte *, int))c_error->fun;
	construct_fs_stat = (vdynamic * (*)(int, int, int, int, int, int, int, int, int, int, int, int))c_fs_stat->fun;
	construct_fs_dirent = (vdynamic * (*)(const char *, int))c_fs_dirent->fun;
	construct_addrinfo_ipv4 = (vdynamic * (*)(int))c_addrinfo_ipv4->fun;
	construct_addrinfo_ipv6 = (vdynamic * (*)(vbyte *))c_addrinfo_ipv6->fun;
	construct_addrport = (vdynamic * (*)(vdynamic *, int))c_addrport->fun;
}

DEFINE_PRIM(_VOID, glue_register, _FUN(_DYN, _BYTES _I32) _FUN(_DYN, _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32) _FUN(_DYN, _BYTES _I32) _FUN(_DYN, _I32) _FUN(_DYN, _BYTES) _FUN(_DYN, _DYN _I32));

// ------------- ERROR HANDLING -------------------------------------

#define UV_ALLOC_CHECK(var, type) \
	type *var = UV_ALLOC(type); \
	if (var == NULL) { \
		hl_throw(construct_error((vbyte *)"malloc " #type " failed", 0)); \
	} else {}
#define UV_ALLOC_CHECK_C(var, type, cleanup) \
	type *var = UV_ALLOC(type); \
	if (var == NULL) { \
		cleanup; \
		hl_throw(construct_error((vbyte *)"malloc " #type " failed", 0)); \
	} else {}
#define UV_ERROR_CHECK(expr) do { \
		int __tmp_result = expr; \
		if (__tmp_result < 0) { \
			vdynamic *err = construct_error((vbyte *)strdup(uv_strerror(__tmp_result)), __tmp_result); \
			hl_throw(err); \
		} \
	} while (0)
#define UV_ERROR_CHECK_C(expr, cleanup) do { \
		int __tmp_result = expr; \
		if (__tmp_result < 0) { \
			cleanup; \
			vdynamic *err = construct_error((vbyte *)strdup(uv_strerror(__tmp_result)), __tmp_result); \
			hl_throw(err); \
		} \
	} while (0)

// ------------- LOOP -----------------------------------------------

HL_PRIM uv_loop_t * HL_NAME(w_loop_init)(void) {
	UV_ALLOC_CHECK(loop, uv_loop_t);
	UV_ERROR_CHECK_C(uv_loop_init(loop), free(loop));
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

// ------------- MISC -----------------------------------------------

HL_PRIM uv_buf_t *HL_NAME(w_buf_init)(vbyte *bytes, unsigned int length) {
	uv_buf_t *ptr = UV_GC_ALLOC(uv_buf_t);
	*ptr = uv_buf_init((char *)bytes, length);
	return ptr;
}

DEFINE_PRIM(_BUF, w_buf_init, _BYTES _I32);

// ------------- HANDLE ---------------------------------------------

typedef struct {
	vclosure *cb_close;
} uv_w_handle_t;

static void handle_handle_cb_close(uv_handle_t *handle) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, uv_w_handle_t)->cb_close;
	hl_call1(void, cb, vdynamic *, NULL);
	free(UV_HANDLE_DATA(handle));
	free(handle);
}

static void w_close(uv_handle_t *handle, vclosure *cb) {
	UV_HANDLE_DATA_SUB(handle, uv_w_handle_t)->cb_close = cb;
	uv_close(handle, handle_handle_cb_close);
}

//DEFINE_PRIM(_VOID, ref, _HANDLE);
//DEFINE_PRIM(_VOID, unref, _HANDLE);
//DEFINE_PRIM(_VOID, w_close, _HANDLE _CB);

// ------------- FILESYSTEM -----------------------------------------

static void handle_fs_cb(uv_fs_t *req) {
	vclosure *cb = UV_REQ_DATA(req);
	if (req->result < 0)
		hl_call1(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(req->result)), req->result));
	else
		hl_call1(void, cb, vdynamic *, NULL);
	uv_fs_req_cleanup(req);
	hl_remove_root(UV_REQ_DATA(req));
	free(req);
}
static void handle_fs_cb_sync(uv_fs_t *req) {
	/* TODO: should we call uv_fs_req_cleanup on error here? */
	UV_ERROR_CHECK_C(req->result, free(req));
	uv_fs_req_cleanup(req);
	free(req);
}

#define UV_FS_HANDLER(name, type2, setup) \
	static void name(uv_fs_t *req) { \
		vclosure *cb = UV_REQ_DATA(req); \
		if (req->result < 0) \
			hl_call2(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(req->result)), req->result), type2, (type2)0); \
		else { \
			type2 value2; \
			setup; \
			hl_call2(void, cb, vdynamic *, NULL, type2, value2); \
		} \
		uv_fs_req_cleanup(req); \
		hl_remove_root(UV_REQ_DATA(req)); \
		free(req); \
	} \
	static type2 name ## _sync(uv_fs_t *req) { \
		/* TODO: should we call uv_fs_req_cleanup on error here? */ \
		UV_ERROR_CHECK_C(req->result, free(req)); \
		type2 value2; \
		setup; \
		uv_fs_req_cleanup(req); \
		free(req); \
		return value2; \
	}

UV_FS_HANDLER(handle_fs_cb_bytes, vbyte *, value2 = (vbyte *)hl_to_utf16((const char *)req->ptr));
UV_FS_HANDLER(handle_fs_cb_path, vbyte *, value2 = (vbyte *)hl_to_utf16((const char *)req->path));
UV_FS_HANDLER(handle_fs_cb_int, int, value2 = req->result);
UV_FS_HANDLER(handle_fs_cb_file, uv_file, value2 = req->result);
UV_FS_HANDLER(handle_fs_cb_stat, vdynamic *, value2 = construct_fs_stat(
		req->statbuf.st_dev,
		req->statbuf.st_mode,
		req->statbuf.st_nlink,
		req->statbuf.st_uid,
		req->statbuf.st_gid,
		req->statbuf.st_rdev,
		req->statbuf.st_ino,
		req->statbuf.st_size,
		req->statbuf.st_blksize,
		req->statbuf.st_blocks,
		req->statbuf.st_flags,
		req->statbuf.st_gen
	));
UV_FS_HANDLER(handle_fs_cb_scandir, varray *, {
		uv_dirent_t ent;
		vlist *last = NULL;
		int count = 0;
		while (uv_fs_scandir_next(req, &ent) != UV_EOF) {
			count++;
			vlist *node = (vlist *)malloc(sizeof(vlist));
			node->v = construct_fs_dirent(ent.name, ent.type);
			node->next = last;
			last = node;
		}
		value2 = hl_alloc_array(&hlt_dyn, count);
		for (int i = 0; i < count; i++) {
			hl_aptr(value2, vdynamic *)[i] = last->v;
			vlist *next = last->next;
			free(last);
			last = next;
		}
	});

#define UV_REQ_WRAP(name, reqtype, sign, call, ffi, handler) \
	HL_PRIM void HL_NAME(w_ ## name)(sign, vclosure *cb) { \
		UV_ALLOC_CHECK(req, reqtype); \
		UV_REQ_DATA(req) = (void *)cb; \
		UV_ERROR_CHECK_C(uv_ ## name(req, call, handler), free(req)); \
		hl_add_root(UV_REQ_DATA(req)); \
	} \
	DEFINE_PRIM(_VOID, w_ ## name, ffi);
#define UV_REQ_WRAP_LOOP(name, reqtype, sign, call, ffi, handler) \
	HL_PRIM void HL_NAME(w_ ## name)(uv_loop_t *loop, sign, vclosure *cb) { \
		UV_ALLOC_CHECK(req, reqtype); \
		UV_REQ_DATA(req) = (void *)cb; \
		UV_ERROR_CHECK_C(uv_ ## name(loop, req, call, handler), free(req)); \
		hl_add_root(UV_REQ_DATA(req)); \
	} \
	DEFINE_PRIM(_VOID, w_ ## name, _LOOP ffi);
#define UV_REQ_WRAP_LOOP_SYNC(name, ret, reqtype, sign, call, ffiret, ffi, handler, doret) \
	HL_PRIM ret HL_NAME(w_ ## name ## _sync)(uv_loop_t *loop, sign) { \
		UV_ALLOC_CHECK(req, reqtype); \
		UV_ERROR_CHECK_C(uv_ ## name(loop, req, call, NULL), free(req)); \
		doret handler ## _sync(req); \
	} \
	DEFINE_PRIM(ffiret, w_ ## name ## _sync, _LOOP ffi);

#define COMMA ,
#define FS_WRAP1_LOOP(name, ret, arg1, ffiret, ffi, ffihandler, handler, doret) \
	UV_REQ_WRAP_LOOP(name, uv_fs_t, arg1 _arg1, _arg1, ffi ffihandler, handler); \
	UV_REQ_WRAP_LOOP_SYNC(name, ret, uv_fs_t, arg1 _arg1, _arg1, ffiret, ffi, handler, doret)
#define FS_WRAP2_LOOP(name, ret, arg1, arg2, ffiret, ffi, ffihandler, handler, doret) \
	UV_REQ_WRAP_LOOP(name, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2, _arg1 COMMA _arg2, ffi ffihandler, handler); \
	UV_REQ_WRAP_LOOP_SYNC(name, ret, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2, _arg1 COMMA _arg2, ffiret, ffi, handler, doret)
#define FS_WRAP3_LOOP(name, ret, arg1, arg2, arg3, ffiret, ffi, ffihandler, handler, doret) \
	UV_REQ_WRAP_LOOP(name, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3, _arg1 COMMA _arg2 COMMA _arg3, ffi ffihandler, handler); \
	UV_REQ_WRAP_LOOP_SYNC(name, ret, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3, _arg1 COMMA _arg2 COMMA _arg3, ffiret, ffi, handler, doret)
#define FS_WRAP4_LOOP(name, ret, arg1, arg2, arg3, arg4, ffiret, ffi, ffihandler, handler, doret) \
	UV_REQ_WRAP_LOOP(name, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3 COMMA arg4 _arg4, _arg1 COMMA _arg2 COMMA _arg3 COMMA _arg4, ffi ffihandler, handler); \
	UV_REQ_WRAP_LOOP_SYNC(name, ret, uv_fs_t, arg1 _arg1 COMMA arg2 _arg2 COMMA arg3 _arg3 COMMA arg4 _arg4, _arg1 COMMA _arg2 COMMA _arg3 COMMA _arg4, ffiret, ffi, handler, doret)

/**
	FIXME:
		w_fs_read, w_fs_write, w_fs_read_sync, and w_fs_write_sync
		have a signature different from libuv due to no struct passing support in
		hashlink; currently only a single uv_buf_t can be passed at a time.
**/

HL_PRIM void HL_NAME(w_fs_read)(uv_loop_t *loop, uv_file file, const uv_buf_t *buf, int32_t offset, vclosure *cb) {
	UV_ALLOC_CHECK(req, uv_fs_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_fs_read(loop, req, file, buf, 1, offset, handle_fs_cb_int), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM int HL_NAME(w_fs_read_sync)(uv_loop_t *loop, uv_file file, const uv_buf_t *buf, int32_t offset) {
	UV_ALLOC_CHECK(req, uv_fs_t);
	UV_ERROR_CHECK_C(uv_fs_read(loop, req, file, buf, 1, offset, NULL), free(req));
	return handle_fs_cb_int_sync(req);
}

HL_PRIM void HL_NAME(w_fs_write)(uv_loop_t *loop, uv_file file, const uv_buf_t *buf, int32_t offset, vclosure *cb) {
	UV_ALLOC_CHECK(req, uv_fs_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_fs_write(loop, req, file, buf, 1, offset, handle_fs_cb_int), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM int HL_NAME(w_fs_write_sync)(uv_loop_t *loop, uv_file file, const uv_buf_t *buf, int32_t offset) {
	UV_ALLOC_CHECK(req, uv_fs_t);
	UV_ERROR_CHECK_C(uv_fs_write(loop, req, file, buf, 1, offset, NULL), free(req));
	return handle_fs_cb_int_sync(req);
}

DEFINE_PRIM(_VOID, w_fs_read, _LOOP _FILE _BUF _I32 _CB_INT);
DEFINE_PRIM(_I32, w_fs_read_sync, _LOOP _FILE _BUF _I32);
DEFINE_PRIM(_VOID, w_fs_write, _LOOP _FILE _BUF _I32 _CB_INT);
DEFINE_PRIM(_I32, w_fs_write_sync, _LOOP _FILE _BUF _I32);

FS_WRAP1_LOOP(fs_close, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP3_LOOP(fs_open, uv_file, const char*, int, int, _FILE, _BYTES _I32 _I32, _CB_FILE, handle_fs_cb_file, return);
FS_WRAP1_LOOP(fs_unlink, void, const char*, _VOID, _BYTES, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_mkdir, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP1_LOOP(fs_mkdtemp, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_path, return);
FS_WRAP1_LOOP(fs_rmdir, void, const char*, _VOID, _BYTES, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_scandir, varray *, const char *, int, _ARR, _BYTES _I32, _CB_SCANDIR, handle_fs_cb_scandir, return);
FS_WRAP1_LOOP(fs_stat, vdynamic *, const char*, _STAT, _BYTES, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP1_LOOP(fs_fstat, vdynamic *, uv_file, _STAT, _FILE, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP1_LOOP(fs_lstat, vdynamic *, const char*, _STAT, _BYTES, _CB_STAT, handle_fs_cb_stat, return);
FS_WRAP2_LOOP(fs_rename, void, const char*, const char*, _VOID, _BYTES _BYTES, _CB, handle_fs_cb, );
FS_WRAP1_LOOP(fs_fsync, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP1_LOOP(fs_fdatasync, void, uv_file, _VOID, _FILE, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_ftruncate, void, uv_file, int64_t, _VOID, _FILE _I32, _CB, handle_fs_cb, );
FS_WRAP4_LOOP(fs_sendfile, void, uv_file, uv_file, int64_t, size_t, _VOID, _FILE _FILE _I32 _I32, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_access, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_chmod, void, const char*, int, _VOID, _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_fchmod, void, uv_file, int, _VOID, _FILE _I32, _CB, handle_fs_cb, );
FS_WRAP3_LOOP(fs_utime, void, const char*, double, double, _VOID, _BYTES _F64 _F64, _CB, handle_fs_cb, );
FS_WRAP3_LOOP(fs_futime, void, uv_file, double, double, _VOID, _FILE _F64 _F64, _CB, handle_fs_cb, );
FS_WRAP2_LOOP(fs_link, void, const char*, const char*, _VOID, _BYTES _BYTES, _CB, handle_fs_cb, );
FS_WRAP3_LOOP(fs_symlink, void, const char*, const char*, int, _VOID, _BYTES _BYTES _I32, _CB, handle_fs_cb, );
FS_WRAP1_LOOP(fs_readlink, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_bytes, return);
FS_WRAP1_LOOP(fs_realpath, vbyte *, const char*, _BYTES, _BYTES, _CB_STR, handle_fs_cb_bytes, return);
FS_WRAP3_LOOP(fs_chown, void, const char*, uv_uid_t, uv_gid_t, _VOID, _BYTES _I32 _I32, _CB, handle_fs_cb, );
FS_WRAP3_LOOP(fs_fchown, void, uv_file, uv_uid_t, uv_gid_t, _VOID, _FILE _I32 _I32, _CB, handle_fs_cb, );

// ------------- FILESYSTEM EVENTS ----------------------------------

static void handle_fs_event_cb(uv_fs_event_t *handle, const char *filename, int events, int status) {
	vclosure *cb = UV_HANDLE_DATA(handle);
	if (status < 0)
		hl_call3(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(status)), status), vbyte *, NULL, int, 0);
	else
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)hl_to_utf16((const char *)filename), int, events);
}

HL_PRIM uv_fs_event_t *HL_NAME(w_fs_event_init)(uv_loop_t *loop) {
	UV_ALLOC_CHECK(handle, uv_fs_event_t);
	UV_ERROR_CHECK_C(uv_fs_event_init(loop, handle), free(handle));
	return handle;
}

HL_PRIM void HL_NAME(w_fs_event_start)(uv_fs_event_t *handle, const char *path, int flags, vclosure *cb) {
	UV_HANDLE_DATA(handle) = (void *)cb;
	UV_ERROR_CHECK_C(uv_fs_event_start(handle, handle_fs_event_cb, path, flags), free(handle));
	hl_add_root(UV_HANDLE_DATA(handle));
}

HL_PRIM void HL_NAME(w_fs_event_stop)(uv_fs_event_t *handle) {
	UV_ERROR_CHECK_C(uv_fs_event_stop(handle), free(handle));
	hl_remove_root(UV_HANDLE_DATA(handle));
	free(handle);
}

DEFINE_PRIM(_FS_EVENT, w_fs_event_init, _LOOP);
DEFINE_PRIM(_VOID, w_fs_event_start, _FS_EVENT _BYTES _I32 _CB_FS_EVENT);
DEFINE_PRIM(_VOID, w_fs_event_stop, _FS_EVENT);

// ------------- STREAM ---------------------------------------------

typedef struct {
	uv_w_handle_t w_handle;
	vclosure *cb_read;
	vclosure *cb_connection;
} uv_w_stream_t;

static void handle_stream_cb(uv_req_t *req, int status) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call1(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(status)), status));
	else
		hl_call1(void, cb, vdynamic *, NULL);
	hl_remove_root(UV_REQ_DATA(req));
	free(req);
}

static void handle_stream_cb_connection(uv_stream_t *stream, int status) {
	vclosure *cb = UV_HANDLE_DATA_SUB(stream, uv_w_stream_t)->cb_connection;
	if (status < 0)
		hl_call1(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(status)), status));
	else
		hl_call1(void, cb, vdynamic *, NULL);
}

static void handle_stream_cb_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
	buf->base = hl_gc_alloc_noptr(suggested_size);
	buf->len = suggested_size;
}

static void handle_stream_cb_read(uv_stream_t *stream, long int nread, const uv_buf_t *buf) {
	vclosure *cb = UV_HANDLE_DATA_SUB(stream, uv_w_stream_t)->cb_read;
	if (nread < 0)
		hl_call3(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(nread)), nread), vbyte *, NULL, int, 0);
	else
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)buf->base, int, buf->len);
}

static void w_listen(uv_stream_t *stream, int backlog, vclosure *cb) {
	UV_HANDLE_DATA_SUB(stream, uv_w_stream_t)->cb_connection = cb;
	UV_ERROR_CHECK(uv_listen(stream, backlog, handle_stream_cb_connection));
}

static void w_write(uv_stream_t *stream, const uv_buf_t *buf, vclosure *cb) {
	// note: signature different due to no struct passing support in HL
	// currently only a single uv_buf_t can be passed at a time
	UV_ALLOC_CHECK(req, uv_write_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_write(req, stream, buf, 1, (void (*)(uv_write_t *, int))handle_stream_cb), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

static void w_read_start(uv_stream_t *stream, vclosure *cb) {
	UV_HANDLE_DATA_SUB(stream, uv_w_stream_t)->cb_read = cb;
	UV_ERROR_CHECK(uv_read_start(stream, handle_stream_cb_alloc, handle_stream_cb_read));
}

//DEFINE_PRIM(_VOID, w_listen, _STREAM _I32 _CB);
//DEFINE_PRIM(_VOID, w_write, _STREAM _BUF _CB);
//DEFINE_PRIM(_VOID, w_read_start, _STREAM _CB_READ);
//DEFINE_PRIM(_VOID, read_stop, _STREAM);
UV_REQ_WRAP(shutdown, uv_shutdown_t, uv_stream_t *stream, stream, _STREAM _CB, (void (*)(uv_shutdown_t *, int))handle_stream_cb);

// ------------- NETWORK MACROS -------------------------------------

#define UV_SOCKADDR_IPV4(var, host, port) \
	struct sockaddr_in var; \
	var.sin_family = AF_INET; \
	var.sin_port = htons((unsigned short)port); \
	var.sin_addr.s_addr = htonl(host);
#define UV_SOCKADDR_IPV6(var, host, port) \
	struct sockaddr_in6 var; \
	memset(&var, 0, sizeof(var)); \
	var.sin6_family = AF_INET6; \
	var.sin6_port = htons((unsigned short)port); \
	memcpy(var.sin6_addr.s6_addr, host, 16);

// ------------- TCP ------------------------------------------------

HL_PRIM uv_tcp_t *HL_NAME(w_tcp_init)(uv_loop_t *loop) {
	UV_ALLOC_CHECK(handle, uv_tcp_t);
	UV_ERROR_CHECK_C(uv_tcp_init(loop, handle), free(handle));
	UV_ALLOC_CHECK_C(data, uv_w_stream_t, free(handle));
	memset(data, 0, sizeof(uv_w_stream_t));
	UV_HANDLE_DATA(handle) = data;
	return handle;
}

HL_PRIM void HL_NAME(w_tcp_nodelay)(uv_tcp_t *handle, bool enable) {
	UV_ERROR_CHECK(uv_tcp_nodelay(handle, enable ? 1 : 0));
}

HL_PRIM void HL_NAME(w_tcp_keepalive)(uv_tcp_t *handle, bool enable, unsigned int delay) {
	UV_ERROR_CHECK(uv_tcp_keepalive(handle, enable ? 1 : 0, delay));
}

HL_PRIM uv_tcp_t *HL_NAME(w_tcp_accept)(uv_loop_t *loop, uv_tcp_t *server) {
	uv_tcp_t *client = HL_NAME(w_tcp_init)(loop);
	UV_ERROR_CHECK_C(uv_accept((uv_stream_t *)server, (uv_stream_t *)client), free(client));
	return client;
}

HL_PRIM void HL_NAME(w_tcp_bind_ipv4)(uv_tcp_t *handle, int host, int port) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ERROR_CHECK(uv_tcp_bind(handle, (const struct sockaddr *)&addr, 0));
}

HL_PRIM void HL_NAME(w_tcp_bind_ipv6)(uv_tcp_t *handle, vbyte *host, int port) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ERROR_CHECK(uv_tcp_bind(handle, (const struct sockaddr *)&addr, 0));
}

HL_PRIM void HL_NAME(w_tcp_connect_ipv4)(uv_tcp_t *handle, int host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ALLOC_CHECK(req, uv_connect_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_tcp_connect(req, handle, (const struct sockaddr *)&addr, (void (*)(uv_connect_t *, int))handle_stream_cb), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM void HL_NAME(w_tcp_connect_ipv6)(uv_tcp_t *handle, vbyte *host, int port, vclosure *cb) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ALLOC_CHECK(req, uv_connect_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_tcp_connect(req, handle, (const struct sockaddr *)&addr, (void (*)(uv_connect_t *, int))handle_stream_cb), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM void HL_NAME(w_tcp_read_stop)(uv_tcp_t *stream) {
	uv_read_stop((uv_stream_t *)stream);
}

DEFINE_PRIM(_TCP, w_tcp_init, _LOOP);
DEFINE_PRIM(_VOID, w_tcp_nodelay, _TCP _BOOL);
DEFINE_PRIM(_VOID, w_tcp_keepalive, _TCP _BOOL _I32);
DEFINE_PRIM(_TCP, w_tcp_accept, _LOOP _TCP);
DEFINE_PRIM(_VOID, w_tcp_bind_ipv4, _TCP _I32 _I32);
DEFINE_PRIM(_VOID, w_tcp_bind_ipv6, _TCP _BYTES _I32);
DEFINE_PRIM(_VOID, w_tcp_connect_ipv4, _TCP _I32 _I32 _CB);
DEFINE_PRIM(_VOID, w_tcp_connect_ipv6, _TCP _BYTES _I32 _CB);
DEFINE_PRIM(_VOID, w_tcp_read_stop, _TCP);

#define UV_TCP_CAST(name, basename, basetype, sign, call, ffi) \
	HL_PRIM void HL_NAME(name)(uv_tcp_t *stream, sign) { \
		basename((basetype *)stream, call); \
	} \
	DEFINE_PRIM(_VOID, name, _TCP ffi);

UV_TCP_CAST(w_tcp_listen, w_listen, uv_stream_t, int backlog COMMA vclosure *cb, backlog COMMA cb, _I32 _CB);
UV_TCP_CAST(w_tcp_write, w_write, uv_stream_t, uv_buf_t *buf COMMA vclosure *cb, buf COMMA cb, _BUF _CB);
UV_TCP_CAST(w_tcp_shutdown, HL_NAME(w_shutdown), uv_stream_t, vclosure *cb, cb, _CB);
UV_TCP_CAST(w_tcp_close, w_close, uv_handle_t, vclosure *cb, cb, _CB);
UV_TCP_CAST(w_tcp_read_start, w_read_start, uv_stream_t, vclosure *cb, cb, _CB_BYTES);

// ------------- UDP ------------------------------------------------

typedef struct {
	uv_w_handle_t w_handle;
	vclosure *cb_read;
} uv_w_udp_t;

static void handle_udp_cb_recv(uv_udp_t *handle, long int nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned int flags) {
	vclosure *cb = UV_HANDLE_DATA_SUB(handle, uv_w_udp_t)->cb_read;
	if (nread < 0)
		hl_call4(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(nread)), nread), vbyte *, NULL, int, 0, vdynamic *, NULL);
	else {
		vdynamic *w_addrport = NULL;
		if (addr != NULL) {
			vdynamic *w_addr;
			if (addr->sa_family == AF_INET) {
				w_addr = construct_addrinfo_ipv4(((struct sockaddr_in *)addr)->sin_addr.s_addr);
				w_addrport = construct_addrport(w_addr, ((struct sockaddr_in *)addr)->sin_port);
			} else if (addr->sa_family == AF_INET6) {
				w_addr = construct_addrinfo_ipv6(((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr);
				w_addrport = construct_addrport(w_addr, ((struct sockaddr_in6 *)addr)->sin6_port);
			}
		}
		hl_call4(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)buf->base, int, buf->len, vdynamic *, w_addrport);
	}
}

HL_PRIM uv_udp_t *HL_NAME(w_udp_init)(uv_loop_t *loop) {
	UV_ALLOC_CHECK(handle, uv_udp_t);
	UV_ERROR_CHECK_C(uv_udp_init(loop, handle), free(handle));
	UV_ALLOC_CHECK_C(data, uv_w_udp_t, free(handle));
	memset(data, 0, sizeof(uv_w_udp_t));
	UV_HANDLE_DATA(handle) = data;
	return handle;
}

HL_PRIM void HL_NAME(w_udp_bind_ipv4)(uv_udp_t *handle, int host, int port) {
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ERROR_CHECK(uv_udp_bind(handle, (const struct sockaddr *)&addr, 0));
}

HL_PRIM void HL_NAME(w_udp_bind_ipv6)(uv_udp_t *handle, vbyte *host, int port) {
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ERROR_CHECK(uv_udp_bind(handle, (const struct sockaddr *)&addr, 0));
}

HL_PRIM void HL_NAME(w_udp_send_ipv4)(uv_udp_t *handle, const uv_buf_t *buf, int host, int port, vclosure *cb) {
	// note: signature different due to no struct passing support in HL
	// currently only a single uv_buf_t can be passed at a time
	UV_SOCKADDR_IPV4(addr, host, port);
	UV_ALLOC_CHECK(req, uv_udp_send_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_udp_send(req, handle, buf, 1, (const struct sockaddr *)&addr, (void (*)(uv_udp_send_t *, int))handle_stream_cb), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM void HL_NAME(w_udp_send_ipv6)(uv_udp_t *handle, const uv_buf_t *buf, vbyte *host, int port, vclosure *cb) {
	// note: signature different due to no struct passing support in HL
	// currently only a single uv_buf_t can be passed at a time
	UV_SOCKADDR_IPV6(addr, host, port);
	UV_ALLOC_CHECK(req, uv_udp_send_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_udp_send(req, handle, buf, 1, (const struct sockaddr *)&addr, (void (*)(uv_udp_send_t *, int))handle_stream_cb), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM void HL_NAME(w_udp_recv_start)(uv_udp_t *handle, vclosure *cb) {
	UV_HANDLE_DATA_SUB(handle, uv_w_udp_t)->cb_read = cb;
	UV_ERROR_CHECK(uv_udp_recv_start(handle, handle_stream_cb_alloc, handle_udp_cb_recv));
}

HL_PRIM void HL_NAME(w_udp_recv_stop)(uv_udp_t *handle) {
	UV_HANDLE_DATA_SUB(handle, uv_w_udp_t)->cb_read = NULL;
	UV_ERROR_CHECK(uv_udp_recv_stop(handle));
}

DEFINE_PRIM(_UDP, w_udp_init, _LOOP);
DEFINE_PRIM(_VOID, w_udp_bind_ipv4, _UDP _I32 _I32);
DEFINE_PRIM(_VOID, w_udp_bind_ipv6, _UDP _BYTES _I32);
DEFINE_PRIM(_VOID, w_udp_send_ipv4, _UDP _BUF _I32 _I32 _CB);
DEFINE_PRIM(_VOID, w_udp_send_ipv6, _UDP _BUF _BYTES _I32 _CB);
DEFINE_PRIM(_VOID, w_udp_recv_start, _UDP _CB_UDP_RECV);
DEFINE_PRIM(_VOID, w_udp_recv_stop, _UDP);

// ------------- DNS ------------------------------------------------

static void handle_dns_gai(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call2(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(status)), status), varray *, NULL);
	else {
		int count = 0;
		struct addrinfo *cur;
		for (cur = res; cur != NULL; cur = cur->ai_next) {
			if (cur->ai_family == AF_INET || cur->ai_family == AF_INET6)
				count++;
		}
		varray *arr = hl_alloc_array(&hlt_dyn, count);
		cur = res;
		for (int i = 0; i < count; i++) {
			if (cur->ai_family == AF_INET)
				hl_aptr(arr, vdynamic *)[i] = construct_addrinfo_ipv4(((struct sockaddr_in *)cur->ai_addr)->sin_addr.s_addr);
			else if (cur->ai_family == AF_INET6)
				hl_aptr(arr, vdynamic *)[i] = construct_addrinfo_ipv6(((struct sockaddr_in6 *)cur->ai_addr)->sin6_addr.s6_addr);
			cur = cur->ai_next;
		}
		uv_freeaddrinfo(res);
		hl_call2(void, cb, vdynamic *, NULL, varray *, arr);
	}
	hl_remove_root(UV_REQ_DATA(req));
	free(req);
}

HL_PRIM void HL_NAME(w_getaddrinfo)(uv_loop_t *loop, vbyte *node, vbyte *service, int hint_flags, int hint_family, int hint_socktype, int hint_protocol, vclosure *cb) {
	struct addrinfo hints = {
		.ai_flags = hint_flags,
		.ai_family = hint_family,
		.ai_socktype = hint_socktype,
		.ai_protocol = hint_protocol,
		.ai_addrlen = 0,
		.ai_addr = NULL,
		.ai_canonname = NULL,
		.ai_next = NULL
	};
	UV_ALLOC_CHECK(req, uv_getaddrinfo_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_getaddrinfo(loop, req, handle_dns_gai, (char *)node, (char *)service, &hints), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

static void handle_dns_gni(uv_getnameinfo_t *req, int status, const char *hostname, const char *service) {
	vclosure *cb = UV_REQ_DATA(req);
	if (status < 0)
		hl_call3(void, cb, vdynamic *, construct_error((vbyte *)strdup(uv_strerror(status)), status), vbyte *, NULL, vbyte *, NULL);
	else 
		hl_call3(void, cb, vdynamic *, NULL, vbyte *, (vbyte *)hostname, vbyte *, (vbyte *)service);
	hl_remove_root(UV_REQ_DATA(req));
	free(req);
}

HL_PRIM void HL_NAME(w_getnameinfo_ipv4)(uv_loop_t *loop, int ip, int flags, vclosure *cb) {
	UV_SOCKADDR_IPV4(addr, ip, 0);
	UV_ALLOC_CHECK(req, uv_getnameinfo_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_getnameinfo(loop, req, handle_dns_gni, (const struct sockaddr *)&addr, flags), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

HL_PRIM void HL_NAME(w_getnameinfo_ipv6)(uv_loop_t *loop, vbyte *ip, int flags, vclosure *cb) {
	UV_SOCKADDR_IPV6(addr, ip, 0);
	UV_ALLOC_CHECK(req, uv_getnameinfo_t);
	UV_REQ_DATA(req) = (void *)cb;
	UV_ERROR_CHECK_C(uv_getnameinfo(loop, req, handle_dns_gni, (const struct sockaddr *)&addr, flags), free(req));
	hl_add_root(UV_REQ_DATA(req));
}

DEFINE_PRIM(_VOID, w_getaddrinfo, _LOOP _BYTES _BYTES _I32 _I32 _I32 _I32 _CB_GAI);
DEFINE_PRIM(_VOID, w_getnameinfo_ipv4, _LOOP _I32 _I32 _CB_GNI);
DEFINE_PRIM(_VOID, w_getnameinfo_ipv6, _LOOP _BYTES _I32 _CB_GNI);
