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

// Common macros

#define _U64	_I64
#define _U32	_I32

#define _POINTER			_ABSTRACT(void_pointer)
#define _HANDLE				_ABSTRACT(uv_handle_t_star)
#define _REQ				_ABSTRACT(uv_req_t_star)
#define _LOOP				_ABSTRACT(uv_loop_t_star)
#define _ASYNC				_HANDLE
#define _CHECK				_HANDLE
#define _TIMER				_HANDLE
#define _SOCKADDR			_ABSTRACT(sockaddr_star)
#define _SOCKADDR_IN		_ABSTRACT(sockaddr_in_star)
#define _SOCKADDR_IN6		_ABSTRACT(sockaddr_in6_star)
#define _SOCKADDR_STORAGE	_ABSTRACT(sockaddr_storage_star)
#define _GETADDRINFO		_REQ
#define _GETNAMEINFO		_REQ
#define _ADDRINFO			_ABSTRACT(addrinfo_star)
#define _HANDLE_TYPE		_I32
#define _OS_FD				_ABSTRACT(uv_os_fd)
#define _FS					_REQ
#define _UID_T				_I32
#define _GID_T				_I32
#define _FILE				_I32
#define _BUF				_ABSTRACT(uv_buf_t)
#define _BUF_ARR			_ABSTRACT(uv_buf_t_arr)
#define _DIR				_ABSTRACT(uv_dir_t_star)
#define _FS_POLL			_HANDLE
#define _IDLE				_HANDLE
#define _RANDOM				_REQ
#define _PIPE				_HANDLE
#define _CONNECT			_REQ
#define _PREPARE			_HANDLE
#define _PROCESS			_HANDLE
#define _SIGNAL				_HANDLE
#define _SHUTDOWN			_REQ
#define _STREAM				_HANDLE
#define _WRITE				_REQ
#define _TCP				_HANDLE
#define _TTY				_HANDLE
#define _UDP				_HANDLE
#define _UDP_SEND			_REQ
#define _DIRENT				_ABSTRACT(uv_dirent_t_star)
#define _STAT				_ABSTRACT(uv_stat_t_star)
#define _STATFS				_ABSTRACT(uv_statfs_t_star)
#define _TIMESPEC			_ABSTRACT(uv_timespec_t_star)
#define _RUSAGE				_ABSTRACT(uv_rusage_t_star)
#define _CPU_INFO			_ABSTRACT(uv_cpu_info_t_star)
#define _INTERFACE_ADDRESS	_ABSTRACT(uv_interface_address_t_star)
#define _PASSWD				_ABSTRACT(uv_passwd_t_star)
#define _UTSNAME			_ABSTRACT(uv_utsname_t_star)
#define _TIMEVAL			_ABSTRACT(uv_timeval_t_star)
#define _TIMEVAL64			_ABSTRACT(uv_timeval64_t_star)
#define _STDIO_CONTAINER	_ABSTRACT(uv_stdio_container_t_star)
#define _PROCESS_OPTIONS	_ABSTRACT(uv_process_options_t_star)
#define _REQ_TYPE			_I32
#define _TTY_MODE_T			_I32
#define _TTY_VTERMSTATE_T	_I32
#define _TTY_VTERMSTATE		_REF(_TTY_VTERMSTATE_T)
#define _MEMBERSHIP			_I32
#define _FS_TYPE			_I32
#define _FS_EVENT			_HANDLE

typedef struct sockaddr uv_sockaddr;
typedef struct sockaddr_in uv_sockaddr_in;
typedef struct sockaddr_in6 uv_sockaddr_in6;
typedef struct sockaddr_storage uv_sockaddr_storage;

// TODO {
	static void on_uv_walk_cb( uv_handle_t* handle, void* arg ) {
	}

	static void on_uv_random_cb( uv_random_t* r, int status, void* buf, size_t buflen ) {
	}
// }

#define UV_ALLOC(t)	((t*)malloc(sizeof(t)))
#define DATA(t,h)	((t)h->data)

#define UV_CHECK_NULL(v,fail_return) \
	if( !v ) { \
		hl_null_access(); \
		return fail_return; \
	}

#define DEFINE_PRIM_ALLOC(hl_type,uv_name) \
	HL_PRIM uv_##uv_name##_t *HL_NAME(alloc_##uv_name)() { \
		return UV_ALLOC(uv_##uv_name##_t); \
	} \
	DEFINE_PRIM(hl_type, alloc_##uv_name, _NO_ARG);

#define DEFINE_PRIM_FREE(hl_type, name) \
	HL_PRIM void HL_NAME(free_##name)( void *ptr ) { \
		free(ptr); \
	} \
	DEFINE_PRIM(_VOID, free_##name, hl_type);

#define DEFINE_PRIM_C_FIELD(hl_return,c_return,hl_struct,c_struct,field) \
	HL_PRIM c_return HL_NAME(c_struct##_##field)( struct c_struct *s ) { \
		return (c_return)s->field; \
	} \
	DEFINE_PRIM(hl_return, c_struct##_##field, hl_struct);

#define DEFINE_PRIM_UV_FIELD(hl_return,c_return,hl_struct,uv_name,field) \
	HL_PRIM c_return HL_NAME(uv_name##_##field)( uv_##uv_name##_t *s ) { \
		return (c_return)s->field; \
	} \
	DEFINE_PRIM(hl_return, uv_name##_##field, hl_struct);

#define DEFINE_PRIM_OF_POINTER(hl_type,uv_name) \
	HL_PRIM uv_##uv_name##_t *HL_NAME(pointer_to_##uv_name)( void *ptr ) { \
		return ptr; \
	} \
	DEFINE_PRIM(hl_type, pointer_to_##uv_name, _POINTER);

#define UV_SET_DATA(h,new_data) \
	if( h->data != new_data ) { \
		if( h->data ) \
			hl_remove_root(h->data); \
		if( new_data ) \
			hl_add_root(new_data); \
		h->data = new_data; \
	}

// Errors

#define HL_UV_NOERR 0
#define HL_UV_E2BIG 1
#define HL_UV_EACCES 2
#define HL_UV_EADDRINUSE 3
#define HL_UV_EADDRNOTAVAIL 4
#define HL_UV_EAFNOSUPPORT 5
#define HL_UV_EAGAIN 6
#define HL_UV_EAI_ADDRFAMILY 7
#define HL_UV_EAI_AGAIN 8
#define HL_UV_EAI_BADFLAGS 9
#define HL_UV_EAI_BADHINTS 10
#define HL_UV_EAI_CANCELED 11
#define HL_UV_EAI_FAIL 12
#define HL_UV_EAI_FAMILY 13
#define HL_UV_EAI_MEMORY 14
#define HL_UV_EAI_NODATA 15
#define HL_UV_EAI_NONAME 16
#define HL_UV_EAI_OVERFLOW 17
#define HL_UV_EAI_PROTOCOL 18
#define HL_UV_EAI_SERVICE 19
#define HL_UV_EAI_SOCKTYPE 20
#define HL_UV_EALREADY 21
#define HL_UV_EBADF 22
#define HL_UV_EBUSY 23
#define HL_UV_ECANCELED 24
#define HL_UV_ECHARSET 25
#define HL_UV_ECONNABORTED 26
#define HL_UV_ECONNREFUSED 27
#define HL_UV_ECONNRESET 28
#define HL_UV_EDESTADDRREQ 29
#define HL_UV_EEXIST 30
#define HL_UV_EFAULT 31
#define HL_UV_EFBIG 32
#define HL_UV_EHOSTUNREACH 33
#define HL_UV_EINTR 34
#define HL_UV_EINVAL 35
#define HL_UV_EIO 36
#define HL_UV_EISCONN 37
#define HL_UV_EISDIR 38
#define HL_UV_ELOOP 39
#define HL_UV_EMFILE 40
#define HL_UV_EMSGSIZE 41
#define HL_UV_ENAMETOOLONG 42
#define HL_UV_ENETDOWN 43
#define HL_UV_ENETUNREACH 44
#define HL_UV_ENFILE 45
#define HL_UV_ENOBUFS 46
#define HL_UV_ENODEV 47
#define HL_UV_ENOENT 48
#define HL_UV_ENOMEM 49
#define HL_UV_ENONET 50
#define HL_UV_ENOPROTOOPT 51
#define HL_UV_ENOSPC 52
#define HL_UV_ENOSYS 53
#define HL_UV_ENOTCONN 54
#define HL_UV_ENOTDIR 55
#define HL_UV_ENOTEMPTY 56
#define HL_UV_ENOTSOCK 57
#define HL_UV_ENOTSUP 58
#define HL_UV_EOVERFLOW 59
#define HL_UV_EPERM 60
#define HL_UV_EPIPE 61
#define HL_UV_EPROTO 62
#define HL_UV_EPROTONOSUPPORT 63
#define HL_UV_EPROTOTYPE 64
#define HL_UV_ERANGE 65
#define HL_UV_EROFS 66
#define HL_UV_ESHUTDOWN 67
#define HL_UV_ESPIPE 68
#define HL_UV_ESRCH 69
#define HL_UV_ETIMEDOUT 70
#define HL_UV_ETXTBSY 71
#define HL_UV_EXDEV 72
#define HL_UV_UNKNOWN 73
#define HL_UV_EOF 74
#define HL_UV_ENXIO 75
#define HL_UV_EMLINK 76
#define HL_UV_ENOTTY 77
#define HL_UV_EFTYPE 78
#define HL_UV_EILSEQ 79
#define HL_UV_ESOCKTNOSUPPORT 80

static int errno_uv2hl( int uv_errno ) {
	switch(uv_errno) {
		case 0: return HL_UV_NOERR;
		case UV_E2BIG: return HL_UV_E2BIG;
		case UV_EACCES: return HL_UV_EACCES;
		case UV_EADDRINUSE: return HL_UV_EADDRINUSE;
		case UV_EADDRNOTAVAIL: return HL_UV_EADDRNOTAVAIL;
		case UV_EAFNOSUPPORT: return HL_UV_EAFNOSUPPORT;
		case UV_EAGAIN: return HL_UV_EAGAIN;
		case UV_EAI_ADDRFAMILY: return HL_UV_EAI_ADDRFAMILY;
		case UV_EAI_AGAIN: return HL_UV_EAI_AGAIN;
		case UV_EAI_BADFLAGS: return HL_UV_EAI_BADFLAGS;
		case UV_EAI_BADHINTS: return HL_UV_EAI_BADHINTS;
		case UV_EAI_CANCELED: return HL_UV_EAI_CANCELED;
		case UV_EAI_FAIL: return HL_UV_EAI_FAIL;
		case UV_EAI_FAMILY: return HL_UV_EAI_FAMILY;
		case UV_EAI_MEMORY: return HL_UV_EAI_MEMORY;
		case UV_EAI_NODATA: return HL_UV_EAI_NODATA;
		case UV_EAI_NONAME: return HL_UV_EAI_NONAME;
		case UV_EAI_OVERFLOW: return HL_UV_EAI_OVERFLOW;
		case UV_EAI_PROTOCOL: return HL_UV_EAI_PROTOCOL;
		case UV_EAI_SERVICE: return HL_UV_EAI_SERVICE;
		case UV_EAI_SOCKTYPE: return HL_UV_EAI_SOCKTYPE;
		case UV_EALREADY: return HL_UV_EALREADY;
		case UV_EBADF: return HL_UV_EBADF;
		case UV_EBUSY: return HL_UV_EBUSY;
		case UV_ECANCELED: return HL_UV_ECANCELED;
		case UV_ECHARSET: return HL_UV_ECHARSET;
		case UV_ECONNABORTED: return HL_UV_ECONNABORTED;
		case UV_ECONNREFUSED: return HL_UV_ECONNREFUSED;
		case UV_ECONNRESET: return HL_UV_ECONNRESET;
		case UV_EDESTADDRREQ: return HL_UV_EDESTADDRREQ;
		case UV_EEXIST: return HL_UV_EEXIST;
		case UV_EFAULT: return HL_UV_EFAULT;
		case UV_EFBIG: return HL_UV_EFBIG;
		case UV_EHOSTUNREACH: return HL_UV_EHOSTUNREACH;
		case UV_EINTR: return HL_UV_EINTR;
		case UV_EINVAL: return HL_UV_EINVAL;
		case UV_EIO: return HL_UV_EIO;
		case UV_EISCONN: return HL_UV_EISCONN;
		case UV_EISDIR: return HL_UV_EISDIR;
		case UV_ELOOP: return HL_UV_ELOOP;
		case UV_EMFILE: return HL_UV_EMFILE;
		case UV_EMSGSIZE: return HL_UV_EMSGSIZE;
		case UV_ENAMETOOLONG: return HL_UV_ENAMETOOLONG;
		case UV_ENETDOWN: return HL_UV_ENETDOWN;
		case UV_ENETUNREACH: return HL_UV_ENETUNREACH;
		case UV_ENFILE: return HL_UV_ENFILE;
		case UV_ENOBUFS: return HL_UV_ENOBUFS;
		case UV_ENODEV: return HL_UV_ENODEV;
		case UV_ENOENT: return HL_UV_ENOENT;
		case UV_ENOMEM: return HL_UV_ENOMEM;
		case UV_ENONET: return HL_UV_ENONET;
		case UV_ENOPROTOOPT: return HL_UV_ENOPROTOOPT;
		case UV_ENOSPC: return HL_UV_ENOSPC;
		case UV_ENOSYS: return HL_UV_ENOSYS;
		case UV_ENOTCONN: return HL_UV_ENOTCONN;
		case UV_ENOTDIR: return HL_UV_ENOTDIR;
		case UV_ENOTEMPTY: return HL_UV_ENOTEMPTY;
		case UV_ENOTSOCK: return HL_UV_ENOTSOCK;
		case UV_ENOTSUP: return HL_UV_ENOTSUP;
		case UV_EOVERFLOW: return HL_UV_EOVERFLOW;
		case UV_EPERM: return HL_UV_EPERM;
		case UV_EPIPE: return HL_UV_EPIPE;
		case UV_EPROTO: return HL_UV_EPROTO;
		case UV_EPROTONOSUPPORT: return HL_UV_EPROTONOSUPPORT;
		case UV_EPROTOTYPE: return HL_UV_EPROTOTYPE;
		case UV_ERANGE: return HL_UV_ERANGE;
		case UV_EROFS: return HL_UV_EROFS;
		case UV_ESHUTDOWN: return HL_UV_ESHUTDOWN;
		case UV_ESPIPE: return HL_UV_ESPIPE;
		case UV_ESRCH: return HL_UV_ESRCH;
		case UV_ETIMEDOUT: return HL_UV_ETIMEDOUT;
		case UV_ETXTBSY: return HL_UV_ETXTBSY;
		case UV_EXDEV: return HL_UV_EXDEV;
		case UV_UNKNOWN: return HL_UV_UNKNOWN;
		case UV_EOF: return HL_UV_EOF;
		case UV_ENXIO: return HL_UV_ENXIO;
		case UV_EMLINK: return HL_UV_EMLINK;
		case UV_ENOTTY: return HL_UV_ENOTTY;
		case UV_EFTYPE: return HL_UV_EFTYPE;
		case UV_EILSEQ: return HL_UV_EILSEQ;
		case UV_ESOCKTNOSUPPORT: return HL_UV_ESOCKTNOSUPPORT;
		default: return HL_UV_UNKNOWN;
	}
}

static int errno_hl2uv( int uv_errno ) {
	switch(uv_errno) {
		case HL_UV_E2BIG: return UV_E2BIG;
		case HL_UV_EACCES: return UV_EACCES;
		case HL_UV_EADDRINUSE: return UV_EADDRINUSE;
		case HL_UV_EADDRNOTAVAIL: return UV_EADDRNOTAVAIL;
		case HL_UV_EAFNOSUPPORT: return UV_EAFNOSUPPORT;
		case HL_UV_EAGAIN: return UV_EAGAIN;
		case HL_UV_EAI_ADDRFAMILY: return UV_EAI_ADDRFAMILY;
		case HL_UV_EAI_AGAIN: return UV_EAI_AGAIN;
		case HL_UV_EAI_BADFLAGS: return UV_EAI_BADFLAGS;
		case HL_UV_EAI_BADHINTS: return UV_EAI_BADHINTS;
		case HL_UV_EAI_CANCELED: return UV_EAI_CANCELED;
		case HL_UV_EAI_FAIL: return UV_EAI_FAIL;
		case HL_UV_EAI_FAMILY: return UV_EAI_FAMILY;
		case HL_UV_EAI_MEMORY: return UV_EAI_MEMORY;
		case HL_UV_EAI_NODATA: return UV_EAI_NODATA;
		case HL_UV_EAI_NONAME: return UV_EAI_NONAME;
		case HL_UV_EAI_OVERFLOW: return UV_EAI_OVERFLOW;
		case HL_UV_EAI_PROTOCOL: return UV_EAI_PROTOCOL;
		case HL_UV_EAI_SERVICE: return UV_EAI_SERVICE;
		case HL_UV_EAI_SOCKTYPE: return UV_EAI_SOCKTYPE;
		case HL_UV_EALREADY: return UV_EALREADY;
		case HL_UV_EBADF: return UV_EBADF;
		case HL_UV_EBUSY: return UV_EBUSY;
		case HL_UV_ECANCELED: return UV_ECANCELED;
		case HL_UV_ECHARSET: return UV_ECHARSET;
		case HL_UV_ECONNABORTED: return UV_ECONNABORTED;
		case HL_UV_ECONNREFUSED: return UV_ECONNREFUSED;
		case HL_UV_ECONNRESET: return UV_ECONNRESET;
		case HL_UV_EDESTADDRREQ: return UV_EDESTADDRREQ;
		case HL_UV_EEXIST: return UV_EEXIST;
		case HL_UV_EFAULT: return UV_EFAULT;
		case HL_UV_EFBIG: return UV_EFBIG;
		case HL_UV_EHOSTUNREACH: return UV_EHOSTUNREACH;
		case HL_UV_EINTR: return UV_EINTR;
		case HL_UV_EINVAL: return UV_EINVAL;
		case HL_UV_EIO: return UV_EIO;
		case HL_UV_EISCONN: return UV_EISCONN;
		case HL_UV_EISDIR: return UV_EISDIR;
		case HL_UV_ELOOP: return UV_ELOOP;
		case HL_UV_EMFILE: return UV_EMFILE;
		case HL_UV_EMSGSIZE: return UV_EMSGSIZE;
		case HL_UV_ENAMETOOLONG: return UV_ENAMETOOLONG;
		case HL_UV_ENETDOWN: return UV_ENETDOWN;
		case HL_UV_ENETUNREACH: return UV_ENETUNREACH;
		case HL_UV_ENFILE: return UV_ENFILE;
		case HL_UV_ENOBUFS: return UV_ENOBUFS;
		case HL_UV_ENODEV: return UV_ENODEV;
		case HL_UV_ENOENT: return UV_ENOENT;
		case HL_UV_ENOMEM: return UV_ENOMEM;
		case HL_UV_ENONET: return UV_ENONET;
		case HL_UV_ENOPROTOOPT: return UV_ENOPROTOOPT;
		case HL_UV_ENOSPC: return UV_ENOSPC;
		case HL_UV_ENOSYS: return UV_ENOSYS;
		case HL_UV_ENOTCONN: return UV_ENOTCONN;
		case HL_UV_ENOTDIR: return UV_ENOTDIR;
		case HL_UV_ENOTEMPTY: return UV_ENOTEMPTY;
		case HL_UV_ENOTSOCK: return UV_ENOTSOCK;
		case HL_UV_ENOTSUP: return UV_ENOTSUP;
		case HL_UV_EOVERFLOW: return UV_EOVERFLOW;
		case HL_UV_EPERM: return UV_EPERM;
		case HL_UV_EPIPE: return UV_EPIPE;
		case HL_UV_EPROTO: return UV_EPROTO;
		case HL_UV_EPROTONOSUPPORT: return UV_EPROTONOSUPPORT;
		case HL_UV_EPROTOTYPE: return UV_EPROTOTYPE;
		case HL_UV_ERANGE: return UV_ERANGE;
		case HL_UV_EROFS: return UV_EROFS;
		case HL_UV_ESHUTDOWN: return UV_ESHUTDOWN;
		case HL_UV_ESPIPE: return UV_ESPIPE;
		case HL_UV_ESRCH: return UV_ESRCH;
		case HL_UV_ETIMEDOUT: return UV_ETIMEDOUT;
		case HL_UV_ETXTBSY: return UV_ETXTBSY;
		case HL_UV_EXDEV: return UV_EXDEV;
		case HL_UV_UNKNOWN: return UV_UNKNOWN;
		case HL_UV_EOF: return UV_EOF;
		case HL_UV_ENXIO: return UV_ENXIO;
		case HL_UV_EMLINK: return UV_EMLINK;
		case HL_UV_ENOTTY: return UV_ENOTTY;
		case HL_UV_EFTYPE: return UV_EFTYPE;
		case HL_UV_EILSEQ: return UV_EILSEQ;
		case HL_UV_ESOCKTNOSUPPORT: return UV_ESOCKTNOSUPPORT;
		default: return UV_UNKNOWN;
	}
}

static int hl_uv_errno( int result ) {
	return result < 0 ? errno_uv2hl(result) : 0;
}

HL_PRIM int HL_NAME(translate_uv_error)( int uv_errno ) {
	return uv_errno < 0 ? errno_uv2hl(uv_errno) : 0;
}
DEFINE_PRIM(_I32, translate_uv_error, _I32);

HL_PRIM int HL_NAME(translate_to_uv_error)( int hl_errno ) {
	return errno_hl2uv(hl_errno);
}
DEFINE_PRIM(_I32, translate_to_uv_error, _I32);

// Various utils

DEFINE_PRIM_FREE(_BYTES, bytes);

HL_PRIM vbyte **HL_NAME(alloc_char_array)( int length ) {
	return malloc(sizeof(vbyte *) * length);
}
DEFINE_PRIM(_REF(_BYTES), alloc_char_array, _I32);

DEFINE_PRIM_FREE(_REF(_BYTES), char_array);

// Buf

HL_PRIM uv_buf_t *HL_NAME(alloc_buf)( vbyte *bytes, int length ) {
	uv_buf_t *buf = UV_ALLOC(uv_buf_t);
	buf->base = (char *)bytes;
	buf->len = length;
	return buf;
}
DEFINE_PRIM(_BUF_ARR, alloc_buf, _BYTES _I32);

HL_PRIM void HL_NAME(buf_set)( uv_buf_t *buf, vbyte *bytes, int length ) { // TODO: change `length` to `int64`
	buf->base = (char *)bytes;
	buf->len = length;
}
DEFINE_PRIM(_VOID, buf_set, _BUF_ARR _BYTES _I32);

DEFINE_PRIM_FREE(_BUF_ARR, buf);
DEFINE_PRIM_UV_FIELD(_BYTES, vbyte *, _BUF_ARR, buf, base);
DEFINE_PRIM_UV_FIELD(_U64, int64, _BUF_ARR, buf, len);

// Handle

#define HANDLE_DATA_FIELDS \
	hl_type *t; \
	uv_handle_t *_h; \
	vclosure *onClose;

#define HANDLE_DATA_WITH_ALLOC_FIELDS \
	HANDLE_DATA_FIELDS; \
	vclosure *onAlloc;

typedef struct {
	HANDLE_DATA_FIELDS;
} uv_handle_data_t;

typedef struct {
	HANDLE_DATA_FIELDS;
	vclosure *callback;
} uv_handle_cb_data_t;

typedef struct {
	HANDLE_DATA_WITH_ALLOC_FIELDS;
} uv_handle_data_with_alloc_t;

#define _HANDLE_DATA	_OBJ(_HANDLE _FUN(_VOID,_NO_ARG))

DEFINE_PRIM_FREE(_HANDLE, handle);

HL_PRIM void HL_NAME(handle_set_data_with_gc)( uv_handle_t *h, uv_handle_data_t *new_data ) {
	UV_SET_DATA(h, new_data);
}
DEFINE_PRIM(_VOID, handle_set_data_with_gc, _HANDLE _HANDLE_DATA);

static void on_uv_close_cb( uv_handle_t *h ) {
	uv_handle_data_t *data = DATA(uv_handle_data_t *, h);
	hl_call0(void, data->onClose);
}

static void on_uv_alloc_cb( uv_handle_t* h, size_t size, uv_buf_t *buf ) {
	vclosure *c = DATA(uv_handle_data_with_alloc_t *, h)->onAlloc;
	hl_call2(void, c, uv_buf_t *, buf, int, (int)size);
	if( buf->base )
		hl_add_root(buf->base);
}

// Request

#define REQ_DATA_FIELDS \
	hl_type *t; \
	uv_req_t *_r;

typedef struct {
	REQ_DATA_FIELDS
} uv_req_data_t;

typedef struct {
	REQ_DATA_FIELDS
	vclosure *callback;
} uv_req_cb_data_t;

#define _REQ_DATA	_OBJ(_REQ)

DEFINE_PRIM_FREE(_REQ,req);

HL_PRIM void HL_NAME(req_set_data_with_gc)( uv_req_t *r, uv_req_data_t *new_data ) {
	UV_SET_DATA(r, new_data);
}
DEFINE_PRIM(_VOID, req_set_data_with_gc, _REQ _REQ_DATA);

// Async

DEFINE_PRIM_ALLOC(_ASYNC, async);

static void on_uv_async_cb( uv_async_t *h ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call1(void, c, uv_async_t *, h);
}

// Check

DEFINE_PRIM_ALLOC(_CHECK, check);

static void on_uv_check_cb( uv_check_t *h ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call0(void, c);
}

// Prepare

DEFINE_PRIM_ALLOC(_PREPARE, prepare);

static void on_uv_prepare_cb( uv_prepare_t *h ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call0(void, c);
}

// Idle

DEFINE_PRIM_ALLOC(_IDLE, idle);

static void on_uv_idle_cb( uv_idle_t *h ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call0(void, c);
}

// Timer

DEFINE_PRIM_ALLOC(_TIMER, timer);

static void on_uv_timer_cb( uv_timer_t *h ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call0(void, c);
}

// Loop

#define _RUN_MODE _I32

DEFINE_PRIM_ALLOC(_LOOP, loop);
DEFINE_PRIM_FREE(_LOOP, loop);

// SockAddr

DEFINE_PRIM_FREE(_SOCKADDR_STORAGE, sockaddr_storage);

HL_PRIM struct sockaddr_storage *HL_NAME(alloc_sockaddr_storage)() {
	return UV_ALLOC(struct sockaddr_storage);
}
DEFINE_PRIM(_SOCKADDR_STORAGE, alloc_sockaddr_storage, _NO_ARG);

HL_PRIM int HL_NAME(sockaddr_storage_size)() {
	return sizeof(struct sockaddr_storage);
}
DEFINE_PRIM(_I32, sockaddr_storage_size, _NO_ARG);

HL_PRIM struct sockaddr *HL_NAME(sockaddr_of_storage)( struct sockaddr_storage *addr ) {
	return (struct sockaddr *)addr;
}
DEFINE_PRIM(_SOCKADDR, sockaddr_of_storage, _SOCKADDR_STORAGE);

HL_PRIM struct sockaddr_storage *HL_NAME(sockaddr_to_storage)( struct sockaddr *addr ) {
	if( !addr )
		return NULL;
	struct sockaddr_storage *storage = UV_ALLOC(struct sockaddr_storage);
	memcpy(storage, addr, sizeof(struct sockaddr_storage));
	return storage;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, sockaddr_to_storage, _SOCKADDR);

// DNS

DEFINE_PRIM_ALLOC(_GETADDRINFO, getaddrinfo);
DEFINE_PRIM_ALLOC(_GETNAMEINFO, getnameinfo);

//see hl.uv.Dns.AddrInfoFlags
#define HL_UV_AI_PASSIVE		1
#define HL_UV_AI_CANONNAME		2
#define HL_UV_AI_NUMERICHOST	4
#define HL_UV_AI_V4MAPPED		8
#define HL_UV_AI_ALL			16
#define HL_UV_AI_ADDRCONFIG		32
#define HL_UV_AI_NUMERICSERV	64

//see hl.uv.Dns.NameInfoFlags
#define HL_UV_NI_NUMERICHOST	1
#define HL_UV_NI_NUMERICSERV	2
#define HL_UV_NI_NOFQDN			4
#define HL_UV_NI_NAMEREQD		8
#define HL_UV_NI_DGRAM			16

HL_PRIM int HL_NAME(nameinfo_flags_to_native)( int flags ) {
	int native = 0;
	if( flags & HL_UV_NI_NUMERICHOST ) native |= NI_NUMERICHOST;
	if( flags & HL_UV_NI_NUMERICSERV ) native |= NI_NUMERICSERV;
	if( flags & HL_UV_NI_NOFQDN ) native |= NI_NOFQDN;
	if( flags & HL_UV_NI_NAMEREQD ) native |= NI_NAMEREQD;
	if( flags & HL_UV_NI_DGRAM ) native |= NI_DGRAM;
	return native;
}
DEFINE_PRIM(_I32, nameinfo_flags_to_native, _I32);

//see hl.uv.SockAddr.AddressFamily
#define HL_UV_UNSPEC	-1
#define HL_UV_INET		-2
#define HL_UV_INET6		-3

HL_PRIM int HL_NAME(address_family_to_pf)( int family ) {
	switch( family ) {
		case HL_UV_UNSPEC: return PF_UNSPEC;
		case HL_UV_INET: return PF_INET;
		case HL_UV_INET6: return PF_INET6;
		default: return family;
	}
}
DEFINE_PRIM(_I32, address_family_to_pf, _I32)

HL_PRIM int HL_NAME(address_family_to_af)( int family ) {
	switch( family ) {
		case HL_UV_UNSPEC: return AF_UNSPEC;
		case HL_UV_INET: return AF_INET;
		case HL_UV_INET6: return AF_INET6;
		default: return family;
	}
}
DEFINE_PRIM(_I32, address_family_to_af, _I32)

//see hl.uv.SockAddr.SocketType
#define HL_UV_STREAM	-1
#define HL_UV_DGRAM		-2
#define HL_UV_RAW		-3

HL_PRIM struct addrinfo *HL_NAME(alloc_addrinfo)( int flags, int family, int socktype, int protocol ) {
	struct addrinfo *info = UV_ALLOC(struct addrinfo);

	info->ai_flags = 0;
	if( flags & HL_UV_AI_PASSIVE )		info->ai_flags |= AI_PASSIVE;
	if( flags & HL_UV_AI_CANONNAME )	info->ai_flags |= AI_CANONNAME;
	if( flags & HL_UV_AI_NUMERICHOST )	info->ai_flags |= AI_NUMERICHOST;
	if( flags & HL_UV_AI_V4MAPPED )		info->ai_flags |= AI_V4MAPPED;
	if( flags & HL_UV_AI_ALL )			info->ai_flags |= AI_ALL;
	if( flags & HL_UV_AI_ADDRCONFIG )	info->ai_flags |= AI_ADDRCONFIG;
	if( flags & HL_UV_AI_NUMERICSERV )	info->ai_flags |= AI_NUMERICSERV;

	info->ai_family = uv_address_family_to_pf(family);

	switch( socktype ) {
		case HL_UV_STREAM: info->ai_socktype = SOCK_STREAM; break;
		case HL_UV_DGRAM: info->ai_socktype = SOCK_DGRAM; break;
		case HL_UV_RAW: info->ai_socktype = SOCK_RAW; break;
		default: info->ai_socktype = socktype; break;
	}

	info->ai_protocol = protocol;
	return info;
}
DEFINE_PRIM(_ADDRINFO, alloc_addrinfo, _I32 _I32 _I32 _I32);

HL_PRIM int HL_NAME(addrinfo_ai_family)( struct addrinfo *ai ) {
	switch( ai->ai_family ) {
		case PF_UNSPEC: return 0;
		case PF_INET: return -1;
		case PF_INET6: return -2;
		default: return ai->ai_family;
	}
}
DEFINE_PRIM(_I32, addrinfo_ai_family, _ADDRINFO);

HL_PRIM int HL_NAME(addrinfo_ai_socktype)( struct addrinfo *ai ) {
	switch( ai->ai_socktype ) {
		case SOCK_STREAM: return -1;
		case SOCK_DGRAM: return -2;
		case SOCK_RAW: return -3;
		default: return ai->ai_socktype;
	}
}
DEFINE_PRIM(_I32, addrinfo_ai_socktype, _ADDRINFO);

HL_PRIM uv_sockaddr_storage *HL_NAME(addrinfo_ai_addr)( struct addrinfo *ai ) {
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	memcpy(addr, ai->ai_addr, ai->ai_addrlen);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, addrinfo_ai_addr, _ADDRINFO);

DEFINE_PRIM_C_FIELD(_I32, int, _ADDRINFO, addrinfo, ai_protocol);
DEFINE_PRIM_C_FIELD(_BYTES, vbyte *, _ADDRINFO, addrinfo, ai_canonname);
DEFINE_PRIM_C_FIELD(_ADDRINFO, struct addrinfo *, _ADDRINFO, addrinfo, ai_next);

static void on_uv_getaddrinfo_cb( uv_getaddrinfo_t *r, int status, struct addrinfo *res ) {
	vclosure *c = DATA(uv_req_cb_data_t *,r)->callback;
	hl_call2(void,c,int,status,struct addrinfo *,res);
}

static void on_uv_getnameinfo_cb( uv_getnameinfo_t *r, int status, const char *hostname, const char *service ) {
	vclosure *c = DATA(uv_req_cb_data_t *,r)->callback;
	hl_call3(void,c,int,status,const char *,hostname,const char *,service);
}

// Stream

typedef struct {
	HANDLE_DATA_WITH_ALLOC_FIELDS;
	vclosure *onConnection;
	vclosure *onRead;
} uv_stream_data_t;

DEFINE_PRIM_ALLOC(_WRITE, write);
DEFINE_PRIM_ALLOC(_CONNECT, connect);
DEFINE_PRIM_ALLOC(_SHUTDOWN, shutdown);

static void on_uv_write_cb( uv_write_t *r, int status ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, int, status);
}

static void on_uv_connect_cb( uv_connect_t *r, int status ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, int, status);
}

static void on_uv_shutdown_cb( uv_shutdown_t *r, int status ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, int, status);
}

static void on_uv_connection_cb( uv_stream_t *h, int status ) {
	vclosure *c = DATA(uv_stream_data_t *, h)->onConnection;
	hl_call1(void, c, int, status);
}

static void on_uv_read_cb( uv_stream_t *h, ssize_t nread, const uv_buf_t *buf ) {
	vclosure *c = DATA(uv_stream_data_t *, h)->onRead;
	if( buf->base )
		hl_remove_root(buf->base);
	hl_call2(void, c, int64, nread, const uv_buf_t *, buf);
}

// TCP

DEFINE_PRIM_ALLOC(_TCP, tcp);

// Pipe

DEFINE_PRIM_ALLOC(_PIPE, pipe);

// UDP

typedef struct {
	HANDLE_DATA_WITH_ALLOC_FIELDS;
	vclosure *onRecv;
} uv_udp_data_t;

DEFINE_PRIM_ALLOC(_UDP, udp);
DEFINE_PRIM_ALLOC(_UDP_SEND, udp_send);

static void on_uv_udp_send_cb( uv_udp_send_t *r, int status ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, int, status);
}

static void on_uv_udp_recv_cb( uv_udp_t *h, ssize_t nread, const uv_buf_t *buf, const uv_sockaddr *src_addr, unsigned flags ) {
	vclosure *c = DATA(uv_udp_data_t *, h)->onRecv;
	if( (nread <= 0 && !src_addr) || (flags & UV_UDP_MMSG_FREE) )
		hl_remove_root(buf->base);
	hl_call4(void, c, int64, nread, const uv_buf_t *, buf, const uv_sockaddr *, src_addr, unsigned, flags);
}

// Signal

HL_PRIM int HL_NAME(translate_to_sys_signal)( int hx ) {
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
DEFINE_PRIM(_I32, translate_to_sys_signal, _I32);

HL_PRIM int HL_NAME(translate_sys_signal)( int uv ) {
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
DEFINE_PRIM(_I32, translate_sys_signal, _I32);

// Process

typedef struct {
	HANDLE_DATA_FIELDS;
	vclosure *onExit;
} uv_process_data_t;

DEFINE_PRIM_ALLOC(_PROCESS, process);

static void on_uv_exit_cb(uv_process_t *h, int64_t exit_status, int term_signal) {
	vclosure *c = DATA(uv_process_data_t *, h)->onExit;
	if( c )
		hl_call3(void, c, uv_process_data_t *, h->data, int64, exit_status, int, uv_translate_sys_signal(term_signal));
}

// Constructors of hl.uv.Process.ProcessStdio enum
typedef struct { //ProcessStdio.FD
	hl_type *t;
	int index;
	int fd;
} stdio_fd;

typedef struct { //ProcessStdio.PIPE
	hl_type *t;
	int index;
	uv_pipe_t *pipe;
	int permissions;
	vdynamic *nonBlock;
} stdio_pipe;

typedef struct { //ProcessStdio.STREAM
	hl_type *t;
	int index;
	uv_stream_t *stream;
} stdio_stream;

HL_PRIM uv_stdio_container_t *HL_NAME(alloc_stdio_container)( varray *stdio, int count ) {
	uv_stdio_container_t *container = malloc(sizeof(uv_stdio_container_t) * stdio->size);
	for (int i = 0; i < count; i++) {
		venum *io = hl_aptr(stdio, venum *)[i];
		if( !io ) {
			container[i].flags = UV_IGNORE;
			continue;
		}
		// On Haxe side: enum ProcessStdio
		stdio_pipe *cfg;
		switch( io->index ) {
			case 0: // IGNORE
				container[i].flags = UV_IGNORE;
				break;
			case 1: // INHERIT
				container[i].flags = UV_INHERIT_FD;
				container[i].data.fd = i;
				break;
			case 2: // FD(fd:StdioFd)
				container[i].flags = UV_INHERIT_FD;
				container[i].data.fd = ((stdio_fd *)io)->fd;
				break;
			case 3: // PIPE
				cfg = (stdio_pipe *)io;
				UV_CHECK_NULL(cfg->pipe,NULL);
				container[i].flags = UV_CREATE_PIPE;
				container[i].flags |= cfg->permissions;
				// switch( cfg->permissions ) {
				// 	case 1: container[i].flags |= UV_READABLE_PIPE; break;
				// 	case 2: container[i].flags |= UV_WRITABLE_PIPE; break;
				// 	case 3:
				// 	default: container[i].flags |= UV_READABLE_PIPE | UV_WRITABLE_PIPE; break;
				// }
				if( cfg->nonBlock && cfg->nonBlock->v.b )
					container[i].flags |= UV_OVERLAPPED_PIPE;
				container[i].data.stream = (uv_stream_t *)cfg->pipe;
				break;
			case 4: // STREAM
				UV_CHECK_NULL(((stdio_stream *)io)->stream,NULL);
				container[i].flags = UV_INHERIT_STREAM;
				container[i].data.stream = ((stdio_stream *)io)->stream;
				break;
			default:
				container[i].flags = UV_IGNORE;
				break;
		}
	}
	return container;
}
DEFINE_PRIM(_STDIO_CONTAINER, alloc_stdio_container, _ARR _I32);
DEFINE_PRIM_FREE(_STDIO_CONTAINER, stdio_container);

HL_PRIM uv_process_options_t *HL_NAME(alloc_process_options)( vbyte *file, vbyte **args, vbyte **env,
	vbyte *cwd, int flags, int stdio_count, uv_stdio_container_t *stdio, uv_uid_t uid, uv_gid_t gid ) {

	uv_process_options_t *options = UV_ALLOC(uv_process_options_t);
	options->file = (char *)file;
	options->exit_cb = on_uv_exit_cb;
	options->args = (char **)args;
	options->env = (char **)env;
	options->cwd = (char *)cwd;
	options->flags = flags;
	options->uid = uid;
	options->gid = gid;
	options->stdio_count = stdio_count;
	options->stdio = stdio;
	return options;
}
DEFINE_PRIM(_PROCESS_OPTIONS, alloc_process_options, _BYTES _REF(_BYTES) _REF(_BYTES) _BYTES _I32 _I32 _STDIO_CONTAINER _I32 _I32);
DEFINE_PRIM_FREE(_PROCESS_OPTIONS, process_options);

// File system

DEFINE_PRIM_ALLOC(_FS, fs);

static void on_uv_fs_cb( uv_fs_t *r ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, uv_fs_t *, r);
}

DEFINE_PRIM_OF_POINTER(_DIR,dir);
DEFINE_PRIM_FREE(_DIR,dir);

HL_PRIM void HL_NAME(dir_init)( uv_dir_t *dir, int num_entries ) {
	dir->nentries = num_entries;
	dir->dirents = malloc(sizeof(uv_dirent_t) * num_entries);
}
DEFINE_PRIM(_VOID, dir_init, _DIR _I32);

HL_PRIM uv_dirent_t *HL_NAME(dir_dirent)( uv_dir_t *dir, int index ) {
	return &dir->dirents[index];
}
DEFINE_PRIM(_DIRENT, dir_dirent, _DIR _I32);

DEFINE_PRIM_UV_FIELD(_I32, int, _DIR, dir, nentries);
DEFINE_PRIM_UV_FIELD(_BYTES, vbyte *, _DIRENT, dirent, name);
DEFINE_PRIM_UV_FIELD(_I32, int, _DIRENT, dirent, type);
DEFINE_PRIM_FREE(_DIRENT, dirent);

DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_dev);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_mode);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_nlink);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_uid);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_gid);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_rdev);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_ino);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_size);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_blksize);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_blocks);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_flags);
DEFINE_PRIM_UV_FIELD(_U64, int64, _STAT, stat, st_gen);

#define DEFINE_PRIM_STAT_TIME(field) \
	HL_PRIM uv_timespec_t *HL_NAME(stat_##field)( uv_stat_t *stat ) { \
		return &stat->field; \
	} \
	DEFINE_PRIM(_TIMESPEC, stat_##field, _STAT);

DEFINE_PRIM_STAT_TIME(st_atim);
DEFINE_PRIM_STAT_TIME(st_mtim);
DEFINE_PRIM_STAT_TIME(st_ctim);
DEFINE_PRIM_STAT_TIME(st_birthtim);

DEFINE_PRIM_UV_FIELD(_I64, int64, _TIMESPEC, timespec, tv_sec);
DEFINE_PRIM_UV_FIELD(_I64, int64, _TIMESPEC, timespec, tv_nsec);

// Tty

DEFINE_PRIM_ALLOC(_TTY, tty);

// Fs event

static void on_uv_fs_event_cb( uv_fs_event_t *h, const char *filename, int events, int status ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call3(void, c, int, status, vbyte *, (vbyte *)filename, int, events);
}

DEFINE_PRIM_ALLOC(_FS_EVENT, fs_event);

// Fs poll

static void on_uv_fs_poll_cb( uv_fs_poll_t *h, int status, const uv_stat_t *prev, const uv_stat_t *curr ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call3(void, c, int, status, const uv_stat_t *, prev, const uv_stat_t *, curr);
}

DEFINE_PRIM_ALLOC(_FS_POLL, fs_poll);

// Signal

static void on_uv_signal_cb( uv_signal_t *h, int signum ) {
	vclosure *c = DATA(uv_handle_cb_data_t *, h)->callback;
	hl_call1(void, c, int, signum);
}

DEFINE_PRIM_ALLOC(_SIGNAL, signal);
DEFINE_PRIM_UV_FIELD(_I32, int, _SIGNAL, signal, signum);


// version

#define DEFINE_PRIM_VERSION(name, value, hl_type, c_type) \
	HL_PRIM c_type HL_NAME(version_##name)() { \
		return (c_type)value; \
	} \
	DEFINE_PRIM(hl_type, version_##name, _NO_ARG); \

HL_PRIM vbyte *HL_NAME(version_string_wrap)() {
	const char *v = uv_version_string();
	return hl_copy_bytes((vbyte *)v, strlen(v));
}
DEFINE_PRIM(_BYTES, version_string_wrap, _NO_ARG);

DEFINE_PRIM_VERSION(major, UV_VERSION_MAJOR, _I32, int);
DEFINE_PRIM_VERSION(minor, UV_VERSION_MINOR, _I32, int);
DEFINE_PRIM_VERSION(patch, UV_VERSION_PATCH, _I32, int);
DEFINE_PRIM_VERSION(hex, UV_VERSION_HEX, _I32, int);
DEFINE_PRIM_VERSION(is_release, UV_VERSION_IS_RELEASE, _BOOL, bool);
DEFINE_PRIM_VERSION(suffix, UV_VERSION_SUFFIX, _BYTES, vbyte *);

// auto-generated libuv bindings
#include "uv_generated.c"



// TODO: remove everything below

#define _CALLB		_FUN(_VOID,_NO_ARG)
#define _TIMESPEC_OBJ	_OBJ(_I64 _I64)

#define EVT_CLOSE	1

#define EVT_STREAM_READ		0
#define EVT_STREAM_LISTEN	2

#define EVT_CONNECT	0	// connect_t

#define EVT_MAX		2 // !!!!!!!!!!!!!!

typedef struct {
	vclosure *events[EVT_MAX + 1];
	void *data;
} events_data;

#define UV_DATA(h)		((events_data*)((h)->data))
#define UV_ALLOC_REQ(t,r,c) \
	t *r = UV_ALLOC(t); \
	req_init_hl_data((uv_req_t *)r); \
	if( c ) \
		req_register_callback((uv_req_t *)r,c,0);
#define UV_CHECK_ERROR(action,cleanup,fail_return) \
	int __result__ = action; \
	if(__result__ < 0) { \
		cleanup; \
		hx_error(__result__); \
		return fail_return; \
	}
#define UV_GET_CLOSURE(c,holder,callback_idx,fail_msg) \
	events_data *__ev__ = UV_DATA(holder); \
	vclosure *c = __ev__ ? __ev__->events[callback_idx] : NULL; \
	if( !c ) \
		hl_fatal(fail_msg);
#define UV_COPY_DATA(buf,holder,buffer,length) \
	events_data *__ev__ = UV_DATA(holder); \
	__ev__->data = malloc(length); \
	memcpy(__ev__->data,buffer,length); \
	uv_buf_t buf = uv_buf_init(__ev__->data, length);

hl_type hlt_sockaddr = { HABSTRACT, {USTR("uv_sockaddr_storage")} };

static void dyn_set_i64( vdynamic *obj, int field, int64 value ) {
	vdynamic *v = hl_alloc_dynamic(&hlt_i64);
	v->v.i64 = value;
	hl_dyn_setp(obj, field, &hlt_dyn, v);
}

static int bytes_geti32( vbyte *b, int pos ) {
	return b[pos] | (b[pos + 1] << 8) | (b[pos + 2] << 16) | (b[pos + 3] << 24);
}

// exceptions

static vclosure *c_exception;

HL_PRIM void HL_NAME(init_exception)( vclosure *c ) {
	c_exception = c;
}
DEFINE_PRIM(_VOID, init_exception, _FUN(_DYN, _I32));

static void hx_error(int uv_errno) {
	if( c_exception ) {
		vdynamic *exc = hl_call1(vdynamic *, c_exception, int, errno_uv2hl(uv_errno));
		hl_throw(exc);
	} else {
		hl_error("%s", hl_to_utf16(uv_err_name(uv_errno)));
	}
}

// Request

static events_data *req_init_hl_data( uv_req_t *r ) {
	events_data *d = hl_gc_alloc_raw(sizeof(events_data));
	memset(d,0,sizeof(events_data));
	hl_add_root(&r->data);
	r->data = d;
	return d;
}

static void req_register_callback( uv_req_t *r, vclosure *c, int event_kind ) {
	if( !r )
		hl_fatal("Missing req");
	if( !r->data )
		hl_fatal("Missing req data");
	UV_DATA(r)->events[event_kind] = c;
}

static void req_clear_callback( uv_req_t *r, int event_kind ) {
	req_register_callback(r,NULL,event_kind);
}

static void free_req( uv_req_t *r ) {
	events_data *ev = UV_DATA(r);
	if( ev ) {
		req_clear_callback(r, 0);
		if( ev->data ) {
			free(ev->data);
			hl_remove_root(&r->data);
		}
		r->data = NULL;
	}
	free(r);
}

static void free_fs_req( uv_fs_t *r ) {
	events_data *ev = UV_DATA(r);
	if( ev ) {
		req_clear_callback((uv_req_t *)r, 0);
		if( ev->data ) {
			free(ev->data);
			hl_remove_root(&r->data);
		}
		r->data = NULL;
	}
	uv_fs_req_cleanup(r);
}

HL_PRIM void HL_NAME(cancel_wrap)( uv_req_t *r ) {
	UV_CHECK_NULL(r,);
	UV_CHECK_ERROR(uv_cancel(r),,);
	free_req(r);
}
DEFINE_PRIM(_VOID, cancel_wrap, _REQ);

// HANDLE

static events_data *handle_init_hl_data( uv_handle_t *h ) {
	events_data *d = hl_gc_alloc_raw(sizeof(events_data));
	memset(d,0,sizeof(events_data));
	hl_add_root(&h->data);
	h->data = d;
	return d;
}

static void handle_register_callback( uv_handle_t *h, vclosure *c, int event_kind ) {
	if( !h )
		hl_fatal("Missing handle");
	if( !h->data )
		hl_fatal("Missing handle data");
	UV_DATA(h)->events[event_kind] = c;
}

static void handle_clear_callback( uv_handle_t *h, int event_kind ) {
	handle_register_callback(h,NULL,event_kind);
}

static void on_close( uv_handle_t *h ) {
	events_data *ev = UV_DATA(h);
	if( ev ) {
		vclosure *c = ev ? ev->events[EVT_CLOSE] : NULL;
		if( c ) {
			hl_call0(void, c);
			handle_clear_callback(h, EVT_CLOSE);
		}
		if( ev->data ) {
			free(ev->data);
			hl_remove_root(&h->data);
		}
		h->data = NULL;
	}
	free(h);
}

static void free_handle( uv_handle_t *h ) {
	uv_close(h, on_close);
}

HL_PRIM void HL_NAME(close_wrap)( uv_handle_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	handle_register_callback(h, c, EVT_CLOSE);
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
	UV_GET_CLOSURE(c,r,0,"No callback in shutdown request");
	hl_call1(void, c, int, errno_uv2hl(status));
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(shutdown_wrap)( uv_stream_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_shutdown_t,r,c)
	UV_CHECK_ERROR(uv_shutdown(r, h, on_shutdown),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, shutdown_wrap, _HANDLE _FUN(_VOID,_I32));

static void on_listen( uv_stream_t *h, int status ) {
	UV_GET_CLOSURE(c,h,EVT_STREAM_LISTEN,"No listen callback in stream handle");
	hl_call1(void, c, int, errno_uv2hl(status));
}

HL_PRIM void HL_NAME(listen_wrap)( uv_stream_t *h, int backlog, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,EVT_STREAM_LISTEN);
	UV_CHECK_ERROR(uv_listen(h, backlog, on_listen),handle_clear_callback((uv_handle_t*)h,EVT_STREAM_LISTEN),);
}
DEFINE_PRIM(_VOID, listen_wrap, _HANDLE _I32 _FUN(_VOID,_I32));

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
	UV_GET_CLOSURE(c,h,EVT_STREAM_READ,"No listen callback in stream handle");
	if( nread < 0 ) {
		hl_call3(void, c, int, errno_uv2hl(nread), vbyte *, NULL, int, 0);
	} else {
		hl_call3(void, c, int, 0, vbyte *, (vbyte *)buf->base, int, nread);
	}
	free(buf->base);
}

HL_PRIM void HL_NAME(read_start_wrap)( uv_stream_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,EVT_STREAM_READ);
	UV_CHECK_ERROR(uv_read_start(h,on_alloc,on_read), handle_clear_callback((uv_handle_t*)h,EVT_STREAM_READ),);
}
DEFINE_PRIM(_VOID, read_start_wrap, _HANDLE _FUN(_VOID,_I32 _BYTES _I32));

HL_PRIM void HL_NAME(read_stop_wrap)( uv_stream_t *h ) {
	UV_CHECK_NULL(h,);
	handle_clear_callback((uv_handle_t*)h,EVT_STREAM_READ);
	uv_read_stop(h);
}
DEFINE_PRIM(_VOID, read_stop_wrap, _HANDLE);

static void on_write( uv_write_t *r, int status ) {
	UV_GET_CLOSURE(c,r,0,"No callback in write request");
	hl_call1(void, c, int, hl_uv_errno(status));
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(write_wrap)( uv_stream_t *h, vbyte *b, int length, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(b,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_write_t,r,c);
	UV_COPY_DATA(buf,r,b,length);
	UV_CHECK_ERROR(uv_write(r,h,&buf,1,on_write),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, write_wrap, _HANDLE _BYTES _I32 _FUN(_VOID,_I32));

HL_PRIM int HL_NAME(try_write_wrap)( uv_stream_t *h, vbyte *b, int length ) {
	UV_CHECK_NULL(h,0);
	UV_CHECK_NULL(b,0);
	uv_buf_t buf = uv_buf_init((char *)b, length);
	UV_CHECK_ERROR(uv_try_write(h,&buf,1),,0);
	return __result__;
}
DEFINE_PRIM(_I32, try_write_wrap, _HANDLE _BYTES _I32);

HL_PRIM void HL_NAME(write2_wrap)( uv_stream_t *h, vbyte *b, int length, uv_stream_t *send_handle, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(b,);
	UV_CHECK_NULL(send_handle,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_write_t,r,c);
	UV_COPY_DATA(buf,r,b,length);
	UV_CHECK_ERROR(uv_write2(r,h,&buf,1,send_handle,on_write),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, write2_wrap, _HANDLE _BYTES _I32 _HANDLE _FUN(_VOID,_I32));

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
	handle_init_hl_data((uv_handle_t*)t);
	return t;
}
DEFINE_PRIM(_HANDLE, timer_init_wrap, _LOOP);

static void on_timer( uv_timer_t *h ) {
	UV_GET_CLOSURE(c,h,0,"No callback in timer handle");
	hl_call0(void, c);
}

// TODO: change `timeout` and `repeat` to uint64
HL_PRIM void HL_NAME(timer_start_wrap)(uv_timer_t *t, vclosure *c, int timeout, int repeat) {
	UV_CHECK_NULL(t,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)t,c,0);
	UV_CHECK_ERROR(
		uv_timer_start(t,on_timer, (uint64_t)timeout, (uint64_t)repeat),
		handle_clear_callback((uv_handle_t*)t,0),
	);
}
DEFINE_PRIM(_VOID, timer_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG) _I32 _I32);

HL_PRIM void HL_NAME(timer_stop_wrap)(uv_timer_t *t) {
	UV_CHECK_NULL(t,);
	handle_clear_callback((uv_handle_t*)t,0);
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

static void on_async( uv_async_t *h ) {
	UV_GET_CLOSURE(c,h,0,"No callback in async handle");
	hl_call1(void, c, uv_async_t *, h);
}

HL_PRIM uv_async_t *HL_NAME(async_init_wrap)( uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,NULL);
	UV_CHECK_NULL(c,NULL);
	uv_async_t *a = UV_ALLOC(uv_async_t);
	UV_CHECK_ERROR(uv_async_init(loop,a,on_async),free(a),NULL);
	handle_init_hl_data((uv_handle_t*)a);
	handle_register_callback((uv_handle_t*)a,c,0);
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
	UV_GET_CLOSURE(c,h,0,"No callback in idle handle");
	hl_call0(void, c);
}

HL_PRIM uv_idle_t *HL_NAME(idle_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_idle_t *h = UV_ALLOC(uv_idle_t);
	UV_CHECK_ERROR(uv_idle_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, idle_init_wrap, _LOOP);

HL_PRIM void HL_NAME(idle_start_wrap)( uv_idle_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
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
	UV_GET_CLOSURE(c,h,0,"No callback in prepare handle");
	hl_call0(void, c);
}

HL_PRIM uv_prepare_t *HL_NAME(prepare_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_prepare_t *h = UV_ALLOC(uv_prepare_t);
	UV_CHECK_ERROR(uv_prepare_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, prepare_init_wrap, _LOOP);

HL_PRIM void HL_NAME(prepare_start_wrap)( uv_prepare_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
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
	UV_GET_CLOSURE(c,h,0,"No callback in check handle");
	hl_call0(void, c);
}

HL_PRIM uv_check_t *HL_NAME(check_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_check_t *h = UV_ALLOC(uv_check_t);
	UV_CHECK_ERROR(uv_check_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, check_init_wrap, _LOOP);

HL_PRIM void HL_NAME(check_start_wrap)( uv_check_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
	uv_check_start(h, on_check);
}
DEFINE_PRIM(_VOID, check_start_wrap, _HANDLE _FUN(_VOID,_NO_ARG));

HL_PRIM void HL_NAME(check_stop_wrap)( uv_check_t *h ) {
	UV_CHECK_NULL(h,);
	uv_check_stop(h);
}
DEFINE_PRIM(_VOID, check_stop_wrap, _HANDLE);

// Signal

static void on_signal( uv_signal_t *h, int signum ) {
	UV_GET_CLOSURE(c,h,0,"No callback in signal handle");
	hl_call1(void, c, int, uv_translate_sys_signal(signum));
}

static void on_signal_oneshot( uv_signal_t *h, int signum ) {
	UV_GET_CLOSURE(c,h,0,"No callback in signal handle");
	handle_clear_callback((uv_handle_t *)h,0);
	hl_call1(void, c, int, uv_translate_sys_signal(signum));
}

HL_PRIM uv_signal_t *HL_NAME(signal_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_signal_t *h = UV_ALLOC(uv_signal_t);
	UV_CHECK_ERROR(uv_signal_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, signal_init_wrap, _LOOP);

HL_PRIM void HL_NAME(signal_start_wrap)( uv_signal_t *h, int signum, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
	UV_CHECK_ERROR(uv_signal_start(h, on_signal, uv_translate_to_sys_signal(signum)),handle_clear_callback((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, signal_start_wrap, _HANDLE _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(signal_start_oneshot_wrap)( uv_signal_t *h, int signum, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
	UV_CHECK_ERROR(uv_signal_start_oneshot(h, on_signal_oneshot, uv_translate_to_sys_signal(signum)),handle_clear_callback((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, signal_start_oneshot_wrap, _HANDLE _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(signal_stop_wrap)( uv_signal_t *h ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_signal_stop(h),,);
}
DEFINE_PRIM(_VOID, signal_stop_wrap, _HANDLE);

HL_PRIM int HL_NAME(signal_get_sigNum_wrap)(uv_signal_t *h) {
	UV_CHECK_NULL(h,0);
	return uv_translate_sys_signal(h->signum);
}
DEFINE_PRIM(_I32, signal_get_sigNum_wrap, _HANDLE);

// Sockaddr

/**
 * Convert `hl.uv.SockAddr.AddressFamily` values to corresponding AF_* values
 */
static int address_family(int hx_address_family) {
	switch( hx_address_family ) {
		case -1: return AF_UNSPEC;
		case -2: return AF_INET;
		case -3: return AF_INET6;
		default: return hx_address_family;
	}
}

HL_PRIM uv_sockaddr_storage *HL_NAME(ip4_addr_wrap)( vstring *ip, int port ) {
	UV_CHECK_NULL(ip,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage); //register in hl gc?
	UV_CHECK_ERROR(uv_ip4_addr(hl_to_utf8(ip->bytes), port, (uv_sockaddr_in *)addr),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, ip4_addr_wrap, _STRING _I32);

HL_PRIM uv_sockaddr_storage *HL_NAME(ip6_addr_wrap)( vstring *ip, int port ) {
	UV_CHECK_NULL(ip,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_ip6_addr(hl_to_utf8(ip->bytes), port, (uv_sockaddr_in6 *)addr),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, ip6_addr_wrap, _STRING _I32);

HL_PRIM vdynamic *HL_NAME(sockaddr_get_port)( uv_sockaddr_storage *addr ) {
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
DEFINE_PRIM(_NULL(_I32), sockaddr_get_port, _SOCKADDR_STORAGE);

HL_PRIM uv_sockaddr_storage *HL_NAME(sockaddr_cast_ptr)( vdynamic *ptr ) {
	UV_CHECK_NULL(ptr,NULL);
	if( ptr->t->kind != HABSTRACT || 0 != memcmp(ptr->t->abs_name,USTR("uv_sockaddr_storage"),38) )
		hl_error("Invalid usage of hl.uv.SockAddr.castPtr()");
	return (uv_sockaddr_storage *)ptr->v.ptr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, sockaddr_cast_ptr, _DYN);

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
DEFINE_PRIM(_BYTES, ip_name_wrap, _SOCKADDR_STORAGE);

// TCP

HL_PRIM uv_tcp_t *HL_NAME(tcp_init_wrap)( uv_loop_t *loop, vdynamic *domain ) {
	UV_CHECK_NULL(loop,NULL);
	uv_tcp_t *h = UV_ALLOC(uv_tcp_t);
	if( !domain ) {
		UV_CHECK_ERROR(uv_tcp_init(loop,h),free(h),NULL);
	} else {
		int d = address_family(domain->v.i);
		UV_CHECK_ERROR(uv_tcp_init_ex(loop,h,d),free_handle((uv_handle_t *)h),NULL);
	}
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, tcp_init_wrap, _LOOP _NULL(_I32));

HL_PRIM void HL_NAME(tcp_nodelay_wrap)( uv_tcp_t *h, bool enable ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_tcp_nodelay(h,enable?1:0),,);
}
DEFINE_PRIM(_VOID, tcp_nodelay_wrap, _HANDLE _BOOL);

HL_PRIM void HL_NAME(tcp_keepalive_wrap)( uv_tcp_t *h, bool enable, int delay ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_tcp_keepalive(h,enable?1:0,delay),,);
}
DEFINE_PRIM(_VOID, tcp_keepalive_wrap, _HANDLE _BOOL _I32);

HL_PRIM void HL_NAME(tcp_simultaneous_accepts_wrap)( uv_tcp_t *h, bool enable ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_tcp_simultaneous_accepts(h,enable?1:0),,);
}
DEFINE_PRIM(_VOID, tcp_simultaneous_accepts_wrap, _HANDLE _BOOL);

HL_PRIM void HL_NAME(tcp_bind_wrap)( uv_tcp_t *h, uv_sockaddr_storage *addr, vdynamic *ipv6_only ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(addr,);
	int flags = ipv6_only && ipv6_only->v.b ? UV_TCP_IPV6ONLY : 0;
	UV_CHECK_ERROR(uv_tcp_bind(h,(uv_sockaddr *)addr,flags),,);
}
DEFINE_PRIM(_VOID, tcp_bind_wrap, _HANDLE _SOCKADDR_STORAGE _NULL(_BOOL));

HL_PRIM uv_sockaddr_storage *HL_NAME(tcp_getsockname_wrap)( uv_tcp_t *h ) {
	UV_CHECK_NULL(h,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	int size = sizeof(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_tcp_getsockname(h,(uv_sockaddr *)addr,&size),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, tcp_getsockname_wrap, _HANDLE);

HL_PRIM uv_sockaddr_storage *HL_NAME(tcp_getpeername_wrap)( uv_tcp_t *h ) {
	UV_CHECK_NULL(h,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	int size = sizeof(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_tcp_getpeername(h,(uv_sockaddr *)addr,&size),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, tcp_getpeername_wrap, _HANDLE);

static void on_connect( uv_connect_t *r, int status ) {
	UV_GET_CLOSURE(c,r,0,"No callback in connect request");
	hl_call1(void, c, int, errno_uv2hl(status));
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(tcp_connect_wrap)( uv_tcp_t *h, uv_sockaddr_storage *addr, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(addr,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_connect_t,r,c);
	UV_CHECK_ERROR(uv_tcp_connect(r, h,(uv_sockaddr *)addr,on_connect),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, tcp_connect_wrap, _HANDLE _SOCKADDR_STORAGE _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(tcp_close_reset_wrap)( uv_tcp_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	handle_register_callback((uv_handle_t *)h, c, EVT_CLOSE);
	uv_tcp_close_reset(h, on_close);
}
DEFINE_PRIM(_VOID, tcp_close_reset_wrap, _HANDLE _CALLB);

// PIPE

HL_PRIM uv_pipe_t *HL_NAME(pipe_init_wrap)( uv_loop_t *loop, vdynamic *ipc ) {
	UV_CHECK_NULL(loop,NULL);
	uv_pipe_t *h = UV_ALLOC(uv_pipe_t);
	UV_CHECK_ERROR(uv_pipe_init(loop,h,(ipc&&ipc->v.b)),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, pipe_init_wrap, _LOOP _NULL(_BOOL));

HL_PRIM void HL_NAME(pipe_bind_wrap)( uv_pipe_t *h, vstring *name ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(name,);
	UV_CHECK_ERROR(uv_pipe_bind(h,hl_to_utf8(name->bytes)),,);
}
DEFINE_PRIM(_VOID, pipe_bind_wrap, _HANDLE _STRING);

static vbyte *pipe_getname( uv_pipe_t *h, int (*fn)(const uv_pipe_t *h, char *buf, size_t *size) ) {
	UV_CHECK_NULL(h,NULL);
	size_t size = 256;
	char *buf = NULL;
	int result = UV_ENOBUFS;
	while (result == UV_ENOBUFS) {
		if( buf )
			free(buf);
		buf = malloc(size);
		result = fn(h,buf,&size);
	}
	vbyte *name = hl_alloc_bytes(size);
	memcpy(name,buf,size);
	free(buf);
	UV_CHECK_ERROR(result,,NULL); //free bytes?
	return name;
}

HL_PRIM vbyte *HL_NAME(pipe_getsockname_wrap)( uv_pipe_t *h ) {
	return pipe_getname(h, uv_pipe_getsockname);
}
DEFINE_PRIM(_BYTES, pipe_getsockname_wrap, _HANDLE);

HL_PRIM vbyte *HL_NAME(pipe_getpeername_wrap)( uv_pipe_t *h ) {
	return pipe_getname(h, uv_pipe_getpeername);
}
DEFINE_PRIM(_BYTES, pipe_getpeername_wrap, _HANDLE);

HL_PRIM void HL_NAME(pipe_connect_wrap)( uv_pipe_t *h, vstring *name, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(name,);
	UV_CHECK_NULL(c,);
	uv_connect_t *r = UV_ALLOC(uv_connect_t);
	req_init_hl_data((uv_req_t *)r);
	req_register_callback((uv_req_t *)r,c,0);
	uv_pipe_connect(r, h,hl_to_utf8(name->bytes),on_connect);
}
DEFINE_PRIM(_VOID, pipe_connect_wrap, _HANDLE _STRING _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(pipe_pending_instances_wrap)( uv_pipe_t *h, int count ) {
	UV_CHECK_NULL(h,);
	uv_pipe_pending_instances(h, count);
}
DEFINE_PRIM(_VOID, pipe_pending_instances_wrap, _HANDLE _I32);

HL_PRIM int HL_NAME(pipe_pending_count_wrap)( uv_pipe_t *h ) {
	UV_CHECK_NULL(h,0);
	UV_CHECK_ERROR(uv_pipe_pending_count(h),,0);
	return __result__;
}
DEFINE_PRIM(_I32, pipe_pending_count_wrap, _HANDLE);

HL_PRIM int HL_NAME(pipe_pending_type_wrap)( uv_pipe_t *h ) {
	UV_CHECK_NULL(h,0);
	UV_CHECK_ERROR(uv_pipe_pending_type(h),,0);
	switch( __result__ ) {
		case UV_ASYNC: return 1;
		case UV_CHECK: return 2;
		case UV_FS_EVENT: return 3;
		case UV_FS_POLL: return 4;
		case UV_HANDLE: return 5;
		case UV_IDLE: return 6;
		case UV_NAMED_PIPE: return 7;
		case UV_POLL: return 8;
		case UV_PREPARE: return 9;
		case UV_PROCESS: return 10;
		case UV_STREAM: return 11;
		case UV_TCP: return 12;
		case UV_TIMER: return 13;
		case UV_TTY: return 14;
		case UV_UDP: return 15;
		case UV_SIGNAL: return 16;
		case UV_FILE: return 17;
		case UV_UNKNOWN_HANDLE:
		default:
			return 0;
	}
}
DEFINE_PRIM(_I32, pipe_pending_type_wrap, _HANDLE);

// Process

static void on_process_exit(uv_process_t *h, int64_t exit_status, int term_signal) {
	events_data *ev = UV_DATA(h);
	vclosure *c = ev ? ev->events[0] : NULL;
	if( c ) {
		hl_call3(void, c, uv_process_t *, h, int64, exit_status, int, uv_translate_sys_signal(term_signal));
		handle_clear_callback((uv_handle_t *)h, 0);
	}
}

HL_PRIM uv_process_t *HL_NAME(spawn_wrap)( uv_loop_t *loop, vstring *file, varray *args,
	vclosure *on_exit, varray *stdio, varray *env, vstring *cwd, vdynamic *uid,
	vdynamic *gid, vdynamic *detached, vdynamic *windowsVerbatimArguments, vdynamic *windowsHide,
	vdynamic *windowsHideConsole, vdynamic *windowsHideGui ) {
	UV_CHECK_NULL(loop,NULL);
	UV_CHECK_NULL(file,NULL);
	UV_CHECK_NULL(args,NULL);

	uv_process_t *h = UV_ALLOC(uv_process_t);

	uv_process_options_t options = {0};
	options.file = hl_to_utf8(file->bytes);
	options.exit_cb = on_process_exit;

	options.args = malloc(sizeof(char *) * (args->size + 1));
	for (int i = 0; i < args->size; i++)
		options.args[i] = hl_to_utf8(hl_aptr(args, vstring *)[i]->bytes);
	options.args[args->size] = NULL;

	if( env ) {
		options.env = malloc(sizeof(char *) * (env->size + 1));
		for (int i = 0; i < env->size; i++)
			options.env[i] = hl_to_utf8(hl_aptr(env, vstring *)[i]->bytes);
		options.env[env->size] = NULL;
	}
	if( stdio ) {
		options.stdio_count = stdio->size;
		options.stdio = malloc(sizeof(uv_stdio_container_t) * stdio->size);
		for (int i = 0; i < stdio->size; i++) {
			venum *io = hl_aptr(stdio, venum *)[i];
			if( !io ) {
				options.stdio[i].flags = UV_IGNORE;
				continue;
			}
			// On Haxe side: enum ProcessStdio
			stdio_pipe *cfg;
			switch( io->index ) {
				case 0: // IGNORE
					options.stdio[i].flags = UV_IGNORE;
					break;
				case 1: // INHERIT
					options.stdio[i].flags = UV_INHERIT_FD;
					options.stdio[i].data.fd = i;
					break;
				case 2: // FD(fd:StdioFd)
					options.stdio[i].flags = UV_INHERIT_FD;
					options.stdio[i].data.fd = ((stdio_fd *)io)->fd;
					break;
				case 3: // PIPE
					cfg = (stdio_pipe *)io;
					UV_CHECK_NULL(cfg->pipe,NULL);
					options.stdio[i].flags = UV_CREATE_PIPE;
					switch( cfg->permissions ) {
						case 1: options.stdio[i].flags |= UV_READABLE_PIPE; break;
						case 2: options.stdio[i].flags |= UV_WRITABLE_PIPE; break;
						case 3:
						default: options.stdio[i].flags |= UV_READABLE_PIPE | UV_WRITABLE_PIPE; break;
					}
					if( cfg->nonBlock && cfg->nonBlock->v.b )
						options.stdio[i].flags |= UV_OVERLAPPED_PIPE;
					options.stdio[i].data.stream = (uv_stream_t *)cfg->pipe;
					break;
				case 4: // STREAM
					UV_CHECK_NULL(((stdio_stream *)io)->stream,NULL);
					options.stdio[i].flags = UV_INHERIT_STREAM;
					options.stdio[i].data.stream = ((stdio_stream *)io)->stream;
					break;
				default:
					options.stdio[i].flags = UV_IGNORE;
					break;
			}
		}
	}

	if( cwd )
		options.cwd = hl_to_utf8(cwd->bytes);
	if(uid) {
		options.uid = uid->v.i;
		options.flags |= UV_PROCESS_SETUID;
	}
	if(gid) {
		options.gid = gid->v.i;
		options.flags |= UV_PROCESS_SETGID;
	}
	if(detached && detached->v.b)
		options.flags |= UV_PROCESS_DETACHED;
	if(windowsVerbatimArguments && windowsVerbatimArguments->v.b)
		options.flags |= UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS;
	if(windowsHide && windowsHide->v.b)
		options.flags |= UV_PROCESS_WINDOWS_HIDE;
	if(windowsHideConsole && windowsHideConsole->v.b)
		options.flags |= UV_PROCESS_WINDOWS_HIDE_CONSOLE;
	if(windowsHideGui && windowsHideGui->v.b)
		options.flags |= UV_PROCESS_WINDOWS_HIDE_GUI;

	UV_CHECK_ERROR(uv_spawn(loop,h,&options),free(h),NULL); // free options?
	handle_init_hl_data((uv_handle_t*)h);
	if( on_exit )
		handle_register_callback((uv_handle_t *)h, on_exit, 0);
	return h;
}
DEFINE_PRIM(_HANDLE, spawn_wrap, _LOOP _STRING _ARR _FUN(_VOID, _HANDLE _I64 _I32) _ARR _ARR _STRING _NULL(_I32) _NULL(_I32) _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL));

HL_PRIM int HL_NAME(process_pid)( uv_process_t *h ) {
	return h->pid;
}
DEFINE_PRIM(_I32, process_pid, _HANDLE);

HL_PRIM void HL_NAME(process_kill_wrap)( uv_process_t *h, int signum ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_process_kill(h, uv_translate_to_sys_signal(signum)),,);
}
DEFINE_PRIM(_VOID, process_kill_wrap, _HANDLE _I32);

HL_PRIM void HL_NAME(kill_wrap)( int pid, int signum ) {
	UV_CHECK_ERROR(uv_kill(pid, uv_translate_to_sys_signal(signum)),,);
}
DEFINE_PRIM(_VOID, kill_wrap, _I32 _I32);

// UDP

HL_PRIM uv_udp_t *HL_NAME(udp_init_wrap)( uv_loop_t *loop, vdynamic *domain, vdynamic *recvmmsg ) {
	UV_CHECK_NULL(loop,NULL);
	uv_udp_t *h = UV_ALLOC(uv_udp_t);
	if( !domain && !recvmmsg ) {
		UV_CHECK_ERROR(uv_udp_init(loop,h),free(h),NULL);
	} else {
		int flags = 0;
		if( domain )
			flags |= address_family(domain->v.i);
		if( recvmmsg && recvmmsg->v.b )
			flags |= UV_UDP_RECVMMSG;
		UV_CHECK_ERROR(uv_udp_init_ex(loop,h,flags),free_handle((uv_handle_t *)h),NULL);
	}
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, udp_init_wrap, _LOOP _NULL(_I32) _NULL(_BOOL));

HL_PRIM void HL_NAME(udp_bind_wrap)( uv_udp_t *h, uv_sockaddr_storage *addr, vdynamic *ipv6_only, vdynamic *reuse_addr ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(addr,);
	int flags = 0;
	if( ipv6_only && ipv6_only->v.b )
		flags |= UV_UDP_IPV6ONLY;
	if( reuse_addr && reuse_addr->v.b )
		flags |= UV_UDP_REUSEADDR;
	UV_CHECK_ERROR(uv_udp_bind(h,(uv_sockaddr *)addr,flags),,);
}
DEFINE_PRIM(_VOID, udp_bind_wrap, _HANDLE _SOCKADDR_STORAGE _NULL(_BOOL) _NULL(_BOOL));

HL_PRIM void HL_NAME(udp_connect_wrap)( uv_udp_t *h, uv_sockaddr_storage *addr ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_connect(h,(uv_sockaddr *)addr),,);
}
DEFINE_PRIM(_VOID, udp_connect_wrap, _HANDLE _SOCKADDR_STORAGE _NULL(_BOOL));

HL_PRIM uv_sockaddr_storage *HL_NAME(udp_getsockname_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	int size = sizeof(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_udp_getsockname(h,(uv_sockaddr *)addr,&size),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, udp_getsockname_wrap, _HANDLE);

HL_PRIM uv_sockaddr_storage *HL_NAME(udp_getpeername_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,NULL);
	uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
	int size = sizeof(uv_sockaddr_storage);
	UV_CHECK_ERROR(uv_udp_getpeername(h,(uv_sockaddr *)addr,&size),free(addr),NULL);
	return addr;
}
DEFINE_PRIM(_SOCKADDR_STORAGE, udp_getpeername_wrap, _HANDLE);

static uv_membership udp_membership( int hx_membership ) {
	switch( hx_membership ) {
		case 0: return UV_LEAVE_GROUP;
		case 1: return UV_JOIN_GROUP;
		default:
			hl_fatal("Unexpected UdpMembership value");
			return hx_membership;
	}
}

HL_PRIM void HL_NAME(udp_set_membership_wrap)( uv_udp_t *h, vstring *multicast_addr, vstring *interface_addr, int membership ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(multicast_addr,);
	UV_CHECK_NULL(interface_addr,);
	char *m_addr = hl_to_utf8(multicast_addr->bytes);
	char *i_addr = hl_to_utf8(interface_addr->bytes);
	uv_membership m = udp_membership(membership);
	UV_CHECK_ERROR(uv_udp_set_membership(h,m_addr,i_addr,m),,);
}
DEFINE_PRIM(_VOID, udp_set_membership_wrap, _HANDLE _STRING _STRING _STRING _I32);

HL_PRIM void HL_NAME(udp_set_source_membership_wrap)( uv_udp_t *h, vstring *multicast_addr, vstring *interface_addr, vstring *source_addr, int membership ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(multicast_addr,);
	UV_CHECK_NULL(interface_addr,);
	UV_CHECK_NULL(source_addr,);
	char *m_addr = hl_to_utf8(multicast_addr->bytes);
	char *i_addr = hl_to_utf8(interface_addr->bytes);
	char *s_addr = hl_to_utf8(source_addr->bytes);
	uv_membership m = udp_membership(membership);
	UV_CHECK_ERROR(uv_udp_set_source_membership(h,m_addr,i_addr,s_addr,m),,);
}
DEFINE_PRIM(_VOID, udp_set_source_membership_wrap, _HANDLE _STRING _STRING _STRING _I32);

HL_PRIM void HL_NAME(udp_set_multicast_loop_wrap)( uv_udp_t *h, bool on ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_set_multicast_loop(h,on),,);
}
DEFINE_PRIM(_VOID, udp_set_multicast_loop_wrap, _HANDLE _BOOL);

HL_PRIM void HL_NAME(udp_set_multicast_ttl_wrap)( uv_udp_t *h, int ttl ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_set_multicast_ttl(h,ttl),,);
}
DEFINE_PRIM(_VOID, udp_set_multicast_ttl_wrap, _HANDLE _I32);

HL_PRIM void HL_NAME(udp_set_multicast_interface_wrap)( uv_udp_t *h, vstring *interface_addr ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(interface_addr,);
	UV_CHECK_ERROR(uv_udp_set_multicast_interface(h,hl_to_utf8(interface_addr->bytes)),,);
}
DEFINE_PRIM(_VOID, udp_set_multicast_interface_wrap, _HANDLE _STRING);

HL_PRIM void HL_NAME(udp_set_broadcast_wrap)( uv_udp_t *h, bool on ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_set_broadcast(h,on),,);
}
DEFINE_PRIM(_VOID, udp_set_broadcast_wrap, _HANDLE _BOOL);

HL_PRIM void HL_NAME(udp_set_ttl_wrap)( uv_udp_t *h, int ttl ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_set_ttl(h,ttl),,);
}
DEFINE_PRIM(_VOID, udp_set_ttl_wrap, _HANDLE _I32);

static void on_udp_send( uv_udp_send_t *r, int status ) {
	UV_GET_CLOSURE(c,r,0,"No callback in udp send request");
	hl_call1(void, c, int, hl_uv_errno(status));
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(udp_send_wrap)( uv_udp_t *h, vbyte *data, int length, uv_sockaddr_storage *addr, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_udp_send_t,r,c);
	events_data *d = UV_DATA(r);
	uv_buf_t buf;
	d->data = malloc(length);
	memcpy(d->data,data,length);
	buf.base = d->data;
	buf.len = length;
	UV_CHECK_ERROR(uv_udp_send(r,h,&buf,1,(uv_sockaddr *)addr,on_udp_send),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, udp_send_wrap, _HANDLE _BYTES _I32 _SOCKADDR_STORAGE _FUN(_VOID,_I32));

HL_PRIM int HL_NAME(udp_try_send_wrap)( uv_udp_t *h, vbyte *data, int length, uv_sockaddr_storage *addr ) {
	UV_CHECK_NULL(h,0);
	uv_buf_t buf = uv_buf_init((char *)data, length);
	UV_CHECK_ERROR(uv_udp_try_send(h,&buf,1,(uv_sockaddr *)addr),,0);
	return __result__;
}
DEFINE_PRIM(_I32, udp_try_send_wrap, _HANDLE _BYTES _I32 _SOCKADDR_STORAGE);

static void on_udp_recv( uv_udp_t *h, ssize_t nread, const uv_buf_t *buf, const uv_sockaddr *src_addr, unsigned flags ) {
	UV_GET_CLOSURE(c,h,0,"No recv callback in udp handle");

	uv_sockaddr_storage *addr = NULL;
	if( src_addr ) {
		addr = UV_ALLOC(uv_sockaddr_storage);
		memcpy(addr, src_addr, sizeof(uv_sockaddr_storage));
	}

	vdynamic *hx_flags = (vdynamic*)hl_alloc_dynobj();
	hl_dyn_seti(hx_flags, hl_hash_utf8("mmsgChunk"), &hlt_bool, 0 != (flags & UV_UDP_MMSG_CHUNK));
	hl_dyn_seti(hx_flags, hl_hash_utf8("mmsgFree"), &hlt_bool, 0 != (flags & UV_UDP_MMSG_FREE));
	hl_dyn_seti(hx_flags, hl_hash_utf8("partial"), &hlt_bool, 0 != (flags & UV_UDP_PARTIAL));

	if( nread < 0 ) {
		hl_call5(void, c, int, errno_uv2hl(nread), vbyte *, NULL, int, 0, uv_sockaddr_storage *, addr, vdynamic *, hx_flags);
	} else {
		hl_call5(void, c, int, 0, vbyte *, (vbyte *)buf->base, int, nread, uv_sockaddr_storage *, addr, vdynamic *, hx_flags);
	}
	if( (nread <= 0 && !addr) || (flags & UV_UDP_MMSG_FREE) )
		free(buf->base);
}

HL_PRIM void HL_NAME(udp_recv_start_wrap)( uv_udp_t *h, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t *)h,c,0);
	UV_CHECK_ERROR(uv_udp_recv_start(h,on_alloc,on_udp_recv),handle_clear_callback((uv_handle_t*)h,0),);
}
DEFINE_PRIM(_VOID, udp_recv_start_wrap, _HANDLE _FUN(_VOID,_I32 _BYTES _I32 _SOCKADDR_STORAGE _DYN));

HL_PRIM bool HL_NAME(udp_using_recvmmsg_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,false);
	UV_CHECK_ERROR(uv_udp_using_recvmmsg(h),,false);
	return __result__  == 1;
}
DEFINE_PRIM(_BOOL, udp_using_recvmmsg_wrap, _HANDLE);

HL_PRIM void HL_NAME(udp_recv_stop_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_udp_recv_stop(h),,);
}
DEFINE_PRIM(_VOID, udp_recv_stop_wrap, _HANDLE);

HL_PRIM int HL_NAME(udp_get_send_queue_size_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,0);
	return uv_udp_get_send_queue_size(h);
}
DEFINE_PRIM(_I32, udp_get_send_queue_size_wrap, _HANDLE);

HL_PRIM int HL_NAME(udp_get_send_queue_count_wrap)( uv_udp_t *h ) {
	UV_CHECK_NULL(h,0);
	return uv_udp_get_send_queue_count(h);
}
DEFINE_PRIM(_I32, udp_get_send_queue_count_wrap, _HANDLE);

// DNS

static void on_getaddrinfo( uv_getaddrinfo_t *r, int status, struct addrinfo *res ) {
	UV_GET_CLOSURE(c,r,0,"No callback in getaddrinfo request");

	int count = 0;
	struct addrinfo *current = res;
	while( current ) {
		++count;
		current = current->ai_next;
	}

	varray *addresses = hl_alloc_array(&hlt_dyn, count);
	current = res;
	int hfamily = hl_hash_utf8("family");
	int hsockType = hl_hash_utf8("sockType");
	int hprotocol = hl_hash_utf8("protocol");
	int haddr = hl_hash_utf8("addr");
	int hcanonName = hl_hash_utf8("canonName");
	int i = 0;
	vdynamic *entry;
	while( current ) {
		entry = (vdynamic *)hl_alloc_dynobj();
		uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
		memcpy(addr, current->ai_addr, current->ai_addrlen);
		hl_dyn_setp(entry,haddr,&hlt_sockaddr,addr);
		hl_dyn_seti(entry,hprotocol,&hlt_i32,current->ai_protocol);
		if( current->ai_canonname ) {
			vbyte *canonname = hl_copy_bytes((vbyte *)current->ai_canonname, strlen(current->ai_canonname));
			hl_dyn_setp(entry,hcanonName,&hlt_bytes,canonname);
		}
		int family;
		switch( current->ai_family ) {
			case PF_UNSPEC: family = -1; break;
			case PF_INET: family = -2; break;
			case PF_INET6: family = -3; break;
			default: family = current->ai_family; break;
		}
		hl_dyn_seti(entry,hfamily,&hlt_i32,family);
		int socktype;
		switch( current->ai_socktype ) {
			case SOCK_STREAM: socktype = -1; break;
			case SOCK_DGRAM: socktype = -2; break;
			case SOCK_RAW: socktype = -3; break;
			default: socktype = current->ai_socktype; break;
		}
		hl_dyn_seti(entry,hsockType,&hlt_i32,socktype);
		hl_aptr(addresses, vdynamic*)[i] = entry;
		current = current->ai_next;
		++i;
	}
	freeaddrinfo(res);

	hl_call2(void,c,int,errno_uv2hl(status),varray *,addresses);
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(getaddrinfo_wrap)( uv_loop_t *l, vstring *name, vstring *service, vdynamic *flags,
	vdynamic *family, vdynamic *socktype, vdynamic *protocol, vclosure *c ) {
	UV_CHECK_NULL(l,);
	UV_CHECK_NULL(( name || service ),);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_getaddrinfo_t,r,c);
	char *c_name = name ? hl_to_utf8(name->bytes) : NULL;
	char *c_service = service ? hl_to_utf8(service->bytes) : NULL;
	struct addrinfo hints = {0};
	if( family )
		switch( family->v.i ) {
			case -1: hints.ai_family = PF_UNSPEC; break;
			case -2: hints.ai_family = PF_INET; break;
			case -3: hints.ai_family = PF_INET6; break;
			default: hints.ai_family = family->v.i; break;
		}
	if( socktype )
		switch( socktype->v.i ) {
			case -1: hints.ai_socktype = SOCK_STREAM; break;
			case -2: hints.ai_socktype = SOCK_DGRAM; break;
			case -3: hints.ai_socktype = SOCK_RAW; break;
			default: hints.ai_socktype = socktype->v.i; break;
		}
	if( protocol )
		hints.ai_protocol = protocol->v.i;
	if( flags ) {
		if( hl_dyn_geti(flags,hl_hash_utf8("passive"),&hlt_bool) )
			hints.ai_flags |= AI_PASSIVE;
		if( hl_dyn_geti(flags,hl_hash_utf8("canonName"),&hlt_bool) )
			hints.ai_flags |= AI_CANONNAME;
		if( hl_dyn_geti(flags,hl_hash_utf8("numericHost"),&hlt_bool) )
			hints.ai_flags |= AI_NUMERICHOST;
		if( hl_dyn_geti(flags,hl_hash_utf8("numericServ"),&hlt_bool) )
			hints.ai_flags |= AI_NUMERICSERV;
		if( hl_dyn_geti(flags,hl_hash_utf8("v4Mapped"),&hlt_bool) )
			hints.ai_flags |= AI_V4MAPPED;
		if( hl_dyn_geti(flags,hl_hash_utf8("all"),&hlt_bool) )
			hints.ai_flags |= AI_ALL;
		if( hl_dyn_geti(flags,hl_hash_utf8("addrConfig"),&hlt_bool) )
			hints.ai_flags |= AI_ADDRCONFIG;
	}
	UV_CHECK_ERROR(uv_getaddrinfo(l,r,on_getaddrinfo,c_name,c_service,&hints),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, getaddrinfo_wrap, _LOOP _STRING _STRING _DYN _NULL(_I32) _NULL(_I32) _NULL(_I32) _FUN(_VOID,_I32 _ARR));

static void on_getnameinfo( uv_getnameinfo_t *r, int status, const char *hostname, const char *service ) {
	UV_GET_CLOSURE(c,r,0,"No callback in getnameinfo request");
	vbyte * bhost = NULL;
	if( hostname )
		bhost = hl_copy_bytes((const vbyte *)hostname, strlen(hostname));
	vbyte * bservice = NULL;
	if( service )
		bservice = hl_copy_bytes((const vbyte *)service, strlen(service));
	hl_call3(void,c,int,errno_uv2hl(status),vbyte *,bhost,vbyte *,bservice);
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(getnameinfo_wrap)( uv_loop_t *l, uv_sockaddr_storage *addr, vdynamic *namereqd, vdynamic *dgram,
	vdynamic *nofqdn, vdynamic *numerichost, vdynamic *numericserv, vclosure *c ) {
	UV_CHECK_NULL(l,);
	UV_CHECK_NULL(addr,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_getnameinfo_t,r,c);
	int flags = 0;
	if( namereqd && namereqd->v.b )
		flags |= NI_NAMEREQD;
	if( dgram && dgram->v.b )
		flags |= NI_DGRAM;
	if( nofqdn && nofqdn->v.b )
		flags |= NI_NOFQDN;
	if( numerichost && numerichost->v.b )
		flags |= NI_NUMERICHOST;
	if( numericserv && numericserv->v.b )
		flags |= NI_NUMERICSERV;
	UV_CHECK_ERROR(uv_getnameinfo(l,r,on_getnameinfo,(uv_sockaddr *)addr,flags),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, getnameinfo_wrap, _LOOP _SOCKADDR_STORAGE _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL) _NULL(_BOOL) _FUN(_VOID,_I32 _BYTES _BYTES));

// loop

HL_PRIM uv_loop_t *HL_NAME(loop_init_wrap)( ) {
	uv_loop_t *loop = UV_ALLOC(uv_loop_t);
	UV_CHECK_ERROR(uv_loop_init(loop),free(loop),NULL);
	return loop;
}
DEFINE_PRIM(_LOOP, loop_init_wrap, _NO_ARG);

// DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);

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

// File system

static void on_fs_open( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs_open request");
	hl_call2(void, c, int, hl_uv_errno(r->result), int, r->result);
	free_req((uv_req_t *)r);
}

HL_PRIM void HL_NAME(fs_open_wrap)( uv_loop_t *loop, vstring *path, varray *flags, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(flags,);
	UV_CHECK_NULL(c,);

	int i_flags = 0;
	int mode = 0;
	for( int i=0; i<flags->size; i++ ){
		venum *flag = hl_aptr(flags, venum*)[i];
		if( !flag ) {
			continue;
		}
		switch( flag->index ) {
			case 0: i_flags |= UV_FS_O_APPEND; break;
			case 1:
				i_flags |= UV_FS_O_CREAT;
				mode = ((struct {hl_type *t;int index;int mode;} *)flag)->mode;
				break;
			case 2: i_flags |= UV_FS_O_DIRECT; break;
			case 3: i_flags |= UV_FS_O_DIRECTORY; break;
			case 4: i_flags |= UV_FS_O_DSYNC; break;
			case 5: i_flags |= UV_FS_O_EXCL; break;
			case 6: i_flags |= UV_FS_O_EXLOCK; break;
			case 7: i_flags |= UV_FS_O_FILEMAP; break;
			case 8: i_flags |= UV_FS_O_NOATIME; break;
			case 9: i_flags |= UV_FS_O_NOCTTY; break;
			case 10: i_flags |= UV_FS_O_NOFOLLOW; break;
			case 11: i_flags |= UV_FS_O_NONBLOCK; break;
			case 12: i_flags |= UV_FS_O_RANDOM; break;
			case 13: i_flags |= UV_FS_O_RDONLY; break;
			case 14: i_flags |= UV_FS_O_RDWR; break;
			case 15: i_flags |= UV_FS_O_SEQUENTIAL; break;
			case 16: i_flags |= UV_FS_O_SHORT_LIVED; break;
			case 17: i_flags |= UV_FS_O_SYMLINK; break;
			case 18: i_flags |= UV_FS_O_SYNC; break;
			case 19: i_flags |= UV_FS_O_TEMPORARY; break;
			case 20: i_flags |= UV_FS_O_TRUNC; break;
			case 21: i_flags |= UV_FS_O_WRONLY; break;
		}
	}
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_open(loop,r,hl_to_utf8(path->bytes),i_flags,mode,on_fs_open),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, fs_open_wrap, _LOOP _STRING _ARR _FUN(_VOID,_I32 _I32));

static void on_fs_common( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs request");
	hl_call1(void,c,int,hl_uv_errno(r->result));
	free_fs_req(r);
}

static void on_fs_bytes_handled( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs request");
	hl_call2(void,c,int,hl_uv_errno(r->result),int64,r->result<0?0:r->result);
	free_fs_req(r);
}

static void on_fs_path( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs request");
	hl_call2(void,c,int,hl_uv_errno(r->result),vbyte *,(vbyte *)(r->result<0?NULL:r->path));
	free_fs_req(r);
}

static void on_fs_bytes( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs request");
	hl_call2(void,c,int,hl_uv_errno(r->result),vbyte *,(vbyte *)(r->result<0?NULL:r->ptr));
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_close_wrap)( uv_file file, uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_close(loop,r,file,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_close_wrap, _I32 _LOOP _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_write_wrap)( uv_file file, uv_loop_t *loop, vbyte *data, int length, int64 offset, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(data,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_COPY_DATA(buf,r,data,length);
	UV_CHECK_ERROR(uv_fs_write(loop,r,file,&buf,1,offset,on_fs_bytes_handled),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_write_wrap, _I32 _LOOP _BYTES _I32 _I64 _FUN(_VOID,_I32 _I64));

HL_PRIM void HL_NAME(fs_read_wrap)( uv_file file, uv_loop_t *loop, vbyte *buf, int length, int64 offset, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(buf,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	uv_buf_t b = uv_buf_init((char *)buf, length);
	UV_CHECK_ERROR(uv_fs_read(loop,r,file,&b,1,offset,on_fs_bytes_handled),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_read_wrap, _I32 _LOOP _BYTES _I32 _I64 _FUN(_VOID,_I32 _I64));

HL_PRIM void HL_NAME(fs_unlink_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_unlink(loop,r,hl_to_utf8(path->bytes),on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_unlink_wrap, _LOOP _STRING _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_mkdir_wrap)( uv_loop_t *loop, vstring *path, int mode, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_mkdir(loop,r,hl_to_utf8(path->bytes),mode,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_mkdir_wrap, _LOOP _STRING _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_mkdtemp_wrap)( uv_loop_t *loop, vstring *tpl, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(tpl,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_mkdtemp(loop,r,hl_to_utf8(tpl->bytes),on_fs_path),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_mkdtemp_wrap, _LOOP _STRING _FUN(_VOID,_I32 _BYTES));

static void on_fs_mkstemp( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs_mkstemp request");
	hl_call3(void,c,int,hl_uv_errno(r->result),int,r->result,vbyte *,(vbyte *)(r->result<0?NULL:r->path));
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_mkstemp_wrap)( uv_loop_t *loop, vstring *tpl, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(tpl,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_mkstemp(loop,r,hl_to_utf8(tpl->bytes),on_fs_mkstemp),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_mkstemp_wrap, _LOOP _STRING _FUN(_VOID,_I32 _I32 _BYTES));

HL_PRIM void HL_NAME(fs_rmdir_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_rmdir(loop,r,hl_to_utf8(path->bytes),on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_rmdir_wrap, _LOOP _STRING _FUN(_VOID,_I32));

static void on_fs_opendir( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs_opendir request");
	hl_call2(void,c,int,hl_uv_errno(r->result),uv_dir_t *,(r->result<0?NULL:r->ptr));
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_opendir_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_opendir(loop,r,hl_to_utf8(path->bytes),on_fs_opendir),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_opendir_wrap, _LOOP _STRING _FUN(_VOID,_I32 _DIR));

HL_PRIM void HL_NAME(fs_closedir_wrap)( uv_dir_t *dir, uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(dir,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_closedir(loop,r,dir,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_closedir_wrap, _DIR _LOOP _FUN(_VOID,_I32));

static void on_fs_readdir( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs_readdir request");
	if( r->result < 0 ) {
		hl_call2(void,c,int,hl_uv_errno(r->result),vdynamic *,NULL);
	} else {
		uv_dir_t *dir = r->ptr;
		varray *entries = hl_alloc_array(&hlt_dyn, r->result);
		int hash_type = hl_hash_utf8("type");
		int hash_name = hl_hash_utf8("name");
		for(int i = 0; i < r->result; i++) {
			vdynamic *entry = (vdynamic*)hl_alloc_dynobj();

			int entry_type = 0;
			switch( dir->dirents[i].type ) {
				case UV_DIRENT_UNKNOWN: entry_type = 1; break;
				case UV_DIRENT_FILE: entry_type = 2; break;
				case UV_DIRENT_DIR: entry_type = 3; break;
				case UV_DIRENT_LINK: entry_type = 4; break;
				case UV_DIRENT_FIFO: entry_type = 5; break;
				case UV_DIRENT_SOCKET: entry_type = 6; break;
				case UV_DIRENT_CHAR: entry_type = 7; break;
				case UV_DIRENT_BLOCK: entry_type = 8; break;
			}
			hl_dyn_seti(entry, hash_type, &hlt_i32, entry_type);

			const char *c_name = dir->dirents[i].name;
			vbyte *name = hl_copy_bytes((const vbyte *)c_name, strlen(c_name));
			hl_dyn_setp(entry, hash_name, &hlt_bytes, name);

			hl_aptr(entries,vdynamic *)[i] = entry;
		}
		hl_call2(void,c,int,0,varray *,entries);
	}
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_readdir_wrap)( uv_dir_t *dir, uv_loop_t *loop, int num_entries, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(dir,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	dir->nentries = num_entries;
	dir->dirents = malloc(sizeof(uv_dirent_t) * num_entries);
	UV_CHECK_ERROR(uv_fs_readdir(loop,r,dir,on_fs_readdir),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_readdir_wrap, _DIR _LOOP _I32 _FUN(_VOID,_I32 _ARR));

static vdynamic *alloc_timespec_dyn( const uv_timespec_t *spec ) {
	vdynamic *obj = (vdynamic*)hl_alloc_dynobj();
	hl_dyn_seti(obj, hl_hash_utf8("sec"), &hlt_i64, (int64)spec->tv_sec);
	hl_dyn_seti(obj, hl_hash_utf8("nsec"), &hlt_i64, (int64)spec->tv_nsec);
	return obj;
}

static vdynamic *alloc_stat_dyn( const uv_stat_t *stat ) {
	vdynamic *obj = (vdynamic*)hl_alloc_dynobj();
	dyn_set_i64(obj, hl_hash_utf8("dev"), stat->st_dev);
	dyn_set_i64(obj, hl_hash_utf8("mode"), stat->st_mode);
	dyn_set_i64(obj, hl_hash_utf8("nlink"), stat->st_nlink);
	dyn_set_i64(obj, hl_hash_utf8("uid"), stat->st_uid);
	dyn_set_i64(obj, hl_hash_utf8("gid"), stat->st_gid);
	dyn_set_i64(obj, hl_hash_utf8("rdev"), stat->st_rdev);
	dyn_set_i64(obj, hl_hash_utf8("ino"), stat->st_ino);
	dyn_set_i64(obj, hl_hash_utf8("size"), stat->st_size);
	dyn_set_i64(obj, hl_hash_utf8("blksize"), stat->st_blksize);
	dyn_set_i64(obj, hl_hash_utf8("blocks"), stat->st_blocks);
	dyn_set_i64(obj, hl_hash_utf8("flags"), stat->st_flags);
	dyn_set_i64(obj, hl_hash_utf8("gen"), stat->st_gen);
	hl_dyn_setp(obj, hl_hash_utf8("atim"), &hlt_dyn, alloc_timespec_dyn(&stat->st_atim));
	hl_dyn_setp(obj, hl_hash_utf8("mtim"), &hlt_dyn, alloc_timespec_dyn(&stat->st_mtim));
	hl_dyn_setp(obj, hl_hash_utf8("ctim"), &hlt_dyn, alloc_timespec_dyn(&stat->st_ctim));
	hl_dyn_setp(obj, hl_hash_utf8("birthtim"), &hlt_dyn, alloc_timespec_dyn(&stat->st_birthtim));
	return obj;
}

static void on_fs_stat( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in fs stat request");
	if( r->result < 0 ) {
		hl_call2(void,c,int,hl_uv_errno(r->result),vdynamic *,NULL);
	} else {
		hl_call2(void,c,int,0,vdynamic *,alloc_stat_dyn(&r->statbuf));
	}
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_stat_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_stat(loop,r,hl_to_utf8(path->bytes),on_fs_stat),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_stat_wrap, _LOOP _STRING _FUN(_VOID,_I32 _DYN));

HL_PRIM void HL_NAME(fs_fstat_wrap)( uv_file file, uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_fstat(loop,r,file,on_fs_stat),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_fstat_wrap, _I32 _LOOP _FUN(_VOID,_I32 _DYN));

HL_PRIM void HL_NAME(fs_lstat_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_lstat(loop,r,hl_to_utf8(path->bytes),on_fs_stat),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_lstat_wrap, _LOOP _STRING _FUN(_VOID,_I32 _DYN));

static void on_fs_statfs( uv_fs_t *r ) {
	UV_GET_CLOSURE(c,r,0,"No callback in statfs request");
	events_data *ev = UV_DATA(r);
	if( r->result < 0 ) {
		hl_call2(void,c,int,hl_uv_errno(r->result),vdynamic *,NULL);
	} else {
		uv_statfs_t *stat = r->ptr;
		vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
		dyn_set_i64(obj, hl_hash_utf8("type"), stat->f_type);
		dyn_set_i64(obj, hl_hash_utf8("bsize"), stat->f_bsize);
		dyn_set_i64(obj, hl_hash_utf8("blocks"), stat->f_blocks);
		dyn_set_i64(obj, hl_hash_utf8("bfree"), stat->f_bfree);
		dyn_set_i64(obj, hl_hash_utf8("bavail"), stat->f_bavail);
		dyn_set_i64(obj, hl_hash_utf8("files"), stat->f_files);
		dyn_set_i64(obj, hl_hash_utf8("ffree"), stat->f_ffree);

		varray *spare = hl_alloc_array(&hlt_i64, 4);
		for (int i = 0; i < 4; i++)
			hl_aptr(spare,int64)[i] = stat->f_spare[i];
		hl_dyn_setp(obj, hl_hash_utf8("spare"), &hlt_array, spare);

		hl_call2(void,c,int,0,vdynamic *,obj);
	}
	ev->data = NULL;
	free_fs_req(r);
}

HL_PRIM void HL_NAME(fs_statfs_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_statfs(loop,r,hl_to_utf8(path->bytes),on_fs_statfs),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_statfs_wrap, _LOOP _STRING _FUN(_VOID,_I32 _DYN));

HL_PRIM void HL_NAME(fs_rename_wrap)( uv_loop_t *loop, vstring *path, vstring *new_path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(new_path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_rename(loop,r,hl_to_utf8(path->bytes),hl_to_utf8(new_path->bytes),on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_rename_wrap, _LOOP _STRING _STRING _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_fsync_wrap)( uv_file file, uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_fsync(loop,r,file,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_fsync_wrap, _I32 _LOOP _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_fdatasync_wrap)( uv_file file, uv_loop_t *loop, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_fdatasync(loop,r,file,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_fdatasync_wrap, _I32 _LOOP _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_ftruncate_wrap)( uv_file file, uv_loop_t *loop, int64 offset, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_ftruncate(loop,r,file,offset,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_ftruncate_wrap, _I32 _LOOP _I64 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_copyfile_wrap)( uv_loop_t *loop, vstring *path, vstring *new_path, varray_bytes *flags, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(new_path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	int i_flags = 0;
	if( flags ) {
		for(int i = 0; i < flags->length; i++) {
			switch( bytes_geti32(flags->bytes, i * 4) ) {
				case 1: i_flags |= UV_FS_COPYFILE_EXCL; break;
				case 2: i_flags |= UV_FS_COPYFILE_FICLONE; break;
				case 3: i_flags |= UV_FS_COPYFILE_FICLONE_FORCE; break;
			}
		}
	}
	UV_CHECK_ERROR(uv_fs_copyfile(loop,r,hl_to_utf8(path->bytes),hl_to_utf8(new_path->bytes),i_flags,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_copyfile_wrap, _LOOP _STRING _STRING _ARRBYTES _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_sendfile_wrap)( uv_file src, uv_loop_t *loop, uv_file dst, int64 in_offset, int64 length, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_sendfile(loop,r,src,dst,in_offset,length,on_fs_bytes_handled),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_sendfile_wrap, _I32 _LOOP _I32 _I64 _I64 _FUN(_VOID,_I32 _I64));

HL_PRIM void HL_NAME(fs_access_wrap)( uv_loop_t *loop, vstring *path, varray_bytes *mode, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(mode,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	int i_mode = 0;
	if( mode ) {
		for(int i = 0; i < mode->length; i++) {
			switch( bytes_geti32(mode->bytes, i * 4) ) {
				case 0: i_mode |= F_OK; break;
				case 1: i_mode |= R_OK; break;
				case 2: i_mode |= W_OK; break;
				case 3: i_mode |= X_OK; break;
			}
		}
	}
	UV_CHECK_ERROR(uv_fs_access(loop,r,hl_to_utf8(path->bytes),i_mode,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_access_wrap, _LOOP _STRING _ARRBYTES _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_chmod_wrap)( uv_loop_t *loop, vstring *path, int mode, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_chmod(loop,r,hl_to_utf8(path->bytes),mode,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_chmod_wrap, _LOOP _STRING _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_fchmod_wrap)( uv_file file, uv_loop_t *loop, int mode, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_fchmod(loop,r,file,mode,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_fchmod_wrap, _I32 _LOOP _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_utime_wrap)( uv_loop_t *loop, vstring *path, double atime, double mtime, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_utime(loop,r,hl_to_utf8(path->bytes),atime,mtime,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_utime_wrap, _LOOP _STRING _F64 _F64 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_futime_wrap)( uv_file file, uv_loop_t *loop, double atime, double mtime, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_futime(loop,r,file,atime,mtime,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_futime_wrap, _I32 _LOOP _F64 _F64 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_lutime_wrap)( uv_loop_t *loop, vstring *path, double atime, double mtime, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_lutime(loop,r,hl_to_utf8(path->bytes),atime,mtime,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_lutime_wrap, _LOOP _STRING _F64 _F64 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_link_wrap)( uv_loop_t *loop, vstring *path, vstring *link, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(link,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_link(loop,r,hl_to_utf8(path->bytes),hl_to_utf8(link->bytes),on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_link_wrap, _LOOP _STRING _STRING _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_symlink_wrap)( uv_loop_t *loop, vstring *path, vstring *link, varray_bytes *flags, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(link,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	int i_flags = 0;
	if( flags ) {
		for(int i = 0; i < flags->length; i++) {
			switch( bytes_geti32(flags->bytes, i * 4) ) {
				case 0: i_flags |= UV_FS_SYMLINK_DIR; break;
				case 1: i_flags |= UV_FS_SYMLINK_JUNCTION; break;
			}
		}
	}
	UV_CHECK_ERROR(uv_fs_symlink(loop,r,hl_to_utf8(path->bytes),hl_to_utf8(link->bytes),i_flags,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_symlink_wrap, _LOOP _STRING _STRING _ARRBYTES _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_readlink_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_readlink(loop,r,hl_to_utf8(path->bytes),on_fs_bytes),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_readlink_wrap, _LOOP _STRING _FUN(_VOID,_I32 _BYTES));

HL_PRIM void HL_NAME(fs_realpath_wrap)( uv_loop_t *loop, vstring *path, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_realpath(loop,r,hl_to_utf8(path->bytes),on_fs_bytes),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_realpath_wrap, _LOOP _STRING _FUN(_VOID,_I32 _BYTES));

HL_PRIM void HL_NAME(fs_chown_wrap)( uv_loop_t *loop, vstring *path, int uid, int gid, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_chown(loop,r,hl_to_utf8(path->bytes),uid,gid,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_chown_wrap, _LOOP _STRING _I32 _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_fchown_wrap)( uv_file file, uv_loop_t *loop, int uid, int gid, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_fchown(loop,r,file,uid,gid,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_fchown_wrap, _I32 _LOOP _I32 _I32 _FUN(_VOID,_I32));

HL_PRIM void HL_NAME(fs_lchown_wrap)( uv_loop_t *loop, vstring *path, int uid, int gid, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_fs_t,r,c);
	UV_CHECK_ERROR(uv_fs_lchown(loop,r,hl_to_utf8(path->bytes),uid,gid,on_fs_common),free_fs_req(r),);
}
DEFINE_PRIM(_VOID, fs_lchown_wrap, _LOOP _STRING _I32 _I32 _FUN(_VOID,_I32));

// Fs event

HL_PRIM uv_fs_event_t *HL_NAME(fs_event_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_fs_event_t *h = UV_ALLOC(uv_fs_event_t);
	UV_CHECK_ERROR(uv_fs_event_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, fs_event_init_wrap, _LOOP);

static void on_fs_event( uv_fs_event_t *h, const char *filename, int events, int status ) {
	UV_GET_CLOSURE(c,h,0,"No callback in fs_event handle");
	if( status < 0 ) {
		hl_call3(void,c,int,errno_uv2hl(status),vbyte *,NULL,varray *,NULL);
	} else {
		int size = (0 != (UV_RENAME & events)) + (0 != (UV_CHANGE & events));
		varray *a_events = hl_alloc_array(&hlt_i32, size);
		int i = 0;
		if( UV_RENAME & events )
			hl_aptr(a_events,int)[i++] = UV_RENAME;
		if( UV_CHANGE & events )
			hl_aptr(a_events,int)[i++] = UV_CHANGE;
		vbyte *path = hl_copy_bytes((const vbyte *)filename, strlen(filename));
		hl_call3(void,c,int,0,vbyte *,path,varray *,a_events);
	}
}

HL_PRIM void HL_NAME(fs_event_start_wrap)( uv_fs_event_t *h, vstring *path, varray_bytes *flags, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
	unsigned int i_flags = 0;
	if( flags ) {
		for(int i = 0; i < flags->length; i++) {
			switch( bytes_geti32(flags->bytes, i * 4) ) {
				case 1: i_flags |= UV_FS_EVENT_WATCH_ENTRY; break;
				case 2: i_flags |= UV_FS_EVENT_STAT; break;
				case 3: i_flags |= UV_FS_EVENT_RECURSIVE; break;
			}
		}
	}
	UV_CHECK_ERROR(uv_fs_event_start(h,on_fs_event,hl_to_utf8(path->bytes),i_flags),handle_clear_callback((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, fs_event_start_wrap, _HANDLE _STRING _ARRBYTES _FUN(_VOID,_I32 _BYTES _ARR));

HL_PRIM void HL_NAME(fs_event_stop_wrap)( uv_fs_event_t *h ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_fs_event_stop(h),,);
}
DEFINE_PRIM(_VOID, fs_event_stop_wrap, _HANDLE);

// Fs poll

HL_PRIM uv_fs_poll_t *HL_NAME(fs_poll_init_wrap)( uv_loop_t *loop ) {
	UV_CHECK_NULL(loop,NULL);
	uv_fs_poll_t *h = UV_ALLOC(uv_fs_poll_t);
	UV_CHECK_ERROR(uv_fs_poll_init(loop,h),free(h),NULL);
	handle_init_hl_data((uv_handle_t*)h);
	return h;
}
DEFINE_PRIM(_HANDLE, fs_poll_init_wrap, _LOOP);

static void on_fs_poll( uv_fs_poll_t *h, int status, const uv_stat_t *prev, const uv_stat_t *curr ) {
	UV_GET_CLOSURE(c,h,0,"No callback in fs_poll handle");
	hl_call3(void,c,int,0,vdynamic *,(prev?alloc_stat_dyn(prev):NULL),vdynamic *,(curr?alloc_stat_dyn(curr):NULL));
}

HL_PRIM void HL_NAME(fs_poll_start_wrap)( uv_fs_poll_t *h, vstring *path, int interval, vclosure *c ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_NULL(path,);
	UV_CHECK_NULL(c,);
	handle_register_callback((uv_handle_t*)h,c,0);
	UV_CHECK_ERROR(uv_fs_poll_start(h,on_fs_poll,hl_to_utf8(path->bytes),interval),handle_clear_callback((uv_handle_t *)h,0),);
}
DEFINE_PRIM(_VOID, fs_poll_start_wrap, _HANDLE _STRING _I32 _FUN(_VOID,_I32 _DYN _DYN));

HL_PRIM void HL_NAME(fs_poll_stop_wrap)( uv_fs_poll_t *h ) {
	UV_CHECK_NULL(h,);
	UV_CHECK_ERROR(uv_fs_poll_stop(h),,);
}
DEFINE_PRIM(_VOID, fs_poll_stop_wrap, _HANDLE);

// Miscellaneous

HL_PRIM int64 HL_NAME(resident_set_memory_wrap)() {
	size_t rss;
	UV_CHECK_ERROR(uv_resident_set_memory(&rss),,0);
	return rss;
}
DEFINE_PRIM(_I64, resident_set_memory_wrap, _NO_ARG);

HL_PRIM double HL_NAME(uptime_wrap)() {
	double uptime;
	UV_CHECK_ERROR(uv_uptime(&uptime),,0);
	return uptime;
}
DEFINE_PRIM(_F64, uptime_wrap, _NO_ARG);

static vdynamic *alloc_timeval_dyn( const uv_timeval_t *tv ) {
	vdynamic *obj = (vdynamic*)hl_alloc_dynobj();
	hl_dyn_seti(obj, hl_hash_utf8("sec"), &hlt_i64, (int64)tv->tv_sec);
	hl_dyn_seti(obj, hl_hash_utf8("usec"), &hlt_i64, (int64)tv->tv_usec);
	return obj;
}

HL_PRIM vdynamic *HL_NAME(getrusage_wrap)() {
	uv_rusage_t rusage = {0};
	UV_CHECK_ERROR(uv_getrusage(&rusage),,NULL);
	vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
	hl_dyn_setp(obj, hl_hash_utf8("utime"), &hlt_dyn, alloc_timeval_dyn(&rusage.ru_utime));
	hl_dyn_setp(obj, hl_hash_utf8("stime"), &hlt_dyn, alloc_timeval_dyn(&rusage.ru_stime));
	dyn_set_i64(obj, hl_hash_utf8("maxrss"), rusage.ru_maxrss);
	dyn_set_i64(obj, hl_hash_utf8("ixrss"), rusage.ru_ixrss);
	dyn_set_i64(obj, hl_hash_utf8("idrss"), rusage.ru_idrss);
	dyn_set_i64(obj, hl_hash_utf8("isrss"), rusage.ru_isrss);
	dyn_set_i64(obj, hl_hash_utf8("minflt"), rusage.ru_minflt);
	dyn_set_i64(obj, hl_hash_utf8("majflt"), rusage.ru_majflt);
	dyn_set_i64(obj, hl_hash_utf8("nswap"), rusage.ru_nswap);
	dyn_set_i64(obj, hl_hash_utf8("inblock"), rusage.ru_inblock);
	dyn_set_i64(obj, hl_hash_utf8("oublock"), rusage.ru_oublock);
	dyn_set_i64(obj, hl_hash_utf8("msgsnd"), rusage.ru_msgsnd);
	dyn_set_i64(obj, hl_hash_utf8("msgrcv"), rusage.ru_msgrcv);
	dyn_set_i64(obj, hl_hash_utf8("nsignals"), rusage.ru_nsignals);
	dyn_set_i64(obj, hl_hash_utf8("nvcsw"), rusage.ru_nvcsw);
	dyn_set_i64(obj, hl_hash_utf8("nivcsw"), rusage.ru_nivcsw);
	return obj;
}
DEFINE_PRIM(_DYN, getrusage_wrap, _NO_ARG);

HL_PRIM varray *HL_NAME(cpu_info_wrap)() {
	uv_cpu_info_t *infos;
	int count;
	UV_CHECK_ERROR(uv_cpu_info(&infos, &count),,NULL);
	int hash_user = hl_hash_utf8("user");
	int hash_nice = hl_hash_utf8("nice");
	int hash_sys = hl_hash_utf8("sys");
	int hash_idle = hl_hash_utf8("idle");
	int hash_irq = hl_hash_utf8("irq");
	int hash_model = hl_hash_utf8("model");
	int hash_speed = hl_hash_utf8("speed");
	int hash_cpuTimes = hl_hash_utf8("cpuTimes");
	varray *a = hl_alloc_array(&hlt_dyn, count);
	for(int i = 0; i < count; i++) {
		vdynamic *times = (vdynamic *)hl_alloc_dynobj();
		dyn_set_i64(times, hash_user, infos[i].cpu_times.user);
		dyn_set_i64(times, hash_nice, infos[i].cpu_times.nice);
		dyn_set_i64(times, hash_sys, infos[i].cpu_times.sys);
		dyn_set_i64(times, hash_idle, infos[i].cpu_times.idle);
		dyn_set_i64(times, hash_irq, infos[i].cpu_times.irq);

		vdynamic *info = (vdynamic *)hl_alloc_dynobj();
		hl_dyn_setp(info, hash_model, &hlt_bytes, hl_copy_bytes((vbyte *)infos[i].model, strlen(infos[i].model) + 1));
		hl_dyn_seti(info, hash_speed, &hlt_i32, infos[i].speed);
		hl_dyn_setp(info, hash_cpuTimes, &hlt_dyn, times);

		hl_aptr(a,vdynamic *)[i] = info;
	}
	uv_free_cpu_info(infos, count);
	return a;
}
DEFINE_PRIM(_ARR, cpu_info_wrap, _NO_ARG);

HL_PRIM varray *HL_NAME(interface_addresses_wrap)() {
	uv_interface_address_t *addresses;
	int count;
	UV_CHECK_ERROR(uv_interface_addresses(&addresses, &count),,NULL);
	int hash_name = hl_hash_utf8("name");
	int hash_physAddr = hl_hash_utf8("physAddr");
	int hash_isInternal = hl_hash_utf8("isInternal");
	int hash_address = hl_hash_utf8("address");
	int hash_netmask = hl_hash_utf8("netmask");
	varray *a = hl_alloc_array(&hlt_dyn, count);
	for(int i = 0; i < count; i++) {
		vdynamic *info = (vdynamic *)hl_alloc_dynobj();

		hl_dyn_setp(info, hash_name, &hlt_bytes, hl_copy_bytes((vbyte *)addresses[i].name, strlen(addresses[i].name)));
		hl_dyn_setp(info, hash_physAddr, &hlt_bytes, hl_copy_bytes((vbyte *)addresses[i].phys_addr, 6));
		hl_dyn_seti(info, hash_isInternal, &hlt_bool, addresses[i].is_internal);

		uv_sockaddr_storage *addr = UV_ALLOC(uv_sockaddr_storage);
		memcpy(addr, &addresses[i].address, sizeof(addresses[i].address));
		hl_dyn_setp(info, hash_address, &hlt_sockaddr, addr);

		uv_sockaddr_storage *mask = UV_ALLOC(uv_sockaddr_storage);
		memcpy(mask, &addresses[i].netmask, sizeof(addresses[i].netmask));
		hl_dyn_setp(info, hash_netmask, &hlt_sockaddr, mask);

		hl_aptr(a,vdynamic *)[i] = info;
	}
	uv_free_interface_addresses(addresses, count);
	return a;
}
DEFINE_PRIM(_ARR, interface_addresses_wrap, _NO_ARG);

HL_PRIM varray *HL_NAME(loadavg_wrap)() {
	double avg[3];
	uv_loadavg(avg);
	varray *a = hl_alloc_array(&hlt_f64, 3);
	hl_aptr(a,double)[0] = avg[0];
	hl_aptr(a,double)[1] = avg[1];
	hl_aptr(a,double)[2] = avg[2];
	return a;
}
DEFINE_PRIM(_ARR, loadavg_wrap, _NO_ARG);

static vbyte *os_str( int (*fn)(char *buf, size_t *size) ) {
	size_t size = 256;
	char *buf = NULL;
	int result = UV_ENOBUFS;
	while (result == UV_ENOBUFS) {
		if( buf )
			free(buf);
		buf = malloc(size);
		result = fn(buf,&size);
	}
	vbyte *path = hl_alloc_bytes(size);
	memcpy(path,buf,size);
	free(buf);
	UV_CHECK_ERROR(result,,NULL); // free bytes?
	return path;
}

HL_PRIM vbyte *HL_NAME(os_homedir_wrap)() {
	return os_str(uv_os_homedir);
}
DEFINE_PRIM(_BYTES, os_homedir_wrap, _NO_ARG);

HL_PRIM vbyte *HL_NAME(os_tmpdir_wrap)() {
	return os_str(uv_os_tmpdir);
}
DEFINE_PRIM(_BYTES, os_tmpdir_wrap, _NO_ARG);

HL_PRIM vdynamic *HL_NAME(os_getpasswd_wrap)() {
	uv_passwd_t p;
	UV_CHECK_ERROR(uv_os_get_passwd(&p),,NULL);
	vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
	hl_dyn_setp(obj, hl_hash_utf8("username"), &hlt_bytes, hl_copy_bytes((vbyte *)p.username, strlen(p.username)));
	hl_dyn_setp(obj, hl_hash_utf8("homedir"), &hlt_bytes, hl_copy_bytes((vbyte *)p.homedir, strlen(p.homedir)));
	dyn_set_i64(obj, hl_hash_utf8("uid"), p.uid);
	dyn_set_i64(obj, hl_hash_utf8("gid"), p.gid);
	if( p.shell )
		hl_dyn_setp(obj, hl_hash_utf8("shell"), &hlt_bytes, hl_copy_bytes((vbyte *)p.shell, strlen(p.shell)));
	uv_os_free_passwd(&p);
	return obj;
}
DEFINE_PRIM(_DYN, os_getpasswd_wrap, _NO_ARG);

HL_PRIM vbyte *HL_NAME(os_gethostname_wrap)() {
	return os_str(uv_os_gethostname);
}
DEFINE_PRIM(_BYTES, os_gethostname_wrap, _NO_ARG);

HL_PRIM int HL_NAME(os_getpriority_wrap)( int pid ) {
	int priority;
	UV_CHECK_ERROR(uv_os_getpriority(pid,&priority),,0);
	return priority;
}
DEFINE_PRIM(_I32, os_getpriority_wrap, _I32);

HL_PRIM void HL_NAME(os_setpriority_wrap)( int pid, int priority ) {
	UV_CHECK_ERROR(uv_os_setpriority(pid,priority),,);
}
DEFINE_PRIM(_VOID, os_setpriority_wrap, _I32 _I32);

HL_PRIM vdynamic *HL_NAME(os_uname_wrap)() {
	uv_utsname_t u;
	UV_CHECK_ERROR(uv_os_uname(&u),,NULL);
	vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
	hl_dyn_setp(obj, hl_hash_utf8("machine"), &hlt_bytes, hl_copy_bytes((vbyte *)u.machine, strlen(u.machine)));
	hl_dyn_setp(obj, hl_hash_utf8("release"), &hlt_bytes, hl_copy_bytes((vbyte *)u.release, strlen(u.release)));
	hl_dyn_setp(obj, hl_hash_utf8("sysname"), &hlt_bytes, hl_copy_bytes((vbyte *)u.sysname, strlen(u.sysname)));
	hl_dyn_setp(obj, hl_hash_utf8("version"), &hlt_bytes, hl_copy_bytes((vbyte *)u.version, strlen(u.version)));
	return obj;
}
DEFINE_PRIM(_DYN, os_uname_wrap, _NO_ARG);

HL_PRIM vdynamic *HL_NAME(gettimeofday_wrap)() {
	uv_timeval64_t t;
	UV_CHECK_ERROR(uv_gettimeofday(&t),,NULL);
	vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
	dyn_set_i64(obj, hl_hash_utf8("sec"), t.tv_sec);
	dyn_set_i64(obj, hl_hash_utf8("usec"), t.tv_usec);
	return obj;
}
DEFINE_PRIM(_DYN, gettimeofday_wrap, _NO_ARG);

static void on_random( uv_random_t* r, int status, void* buf, size_t buflen ) {
	UV_GET_CLOSURE(c,r,0,"No callback in random request");
	hl_call1(void,c,int,errno_uv2hl(status));
}

HL_PRIM void HL_NAME(random_wrap)( uv_loop_t *loop, vbyte *buf, int length, int flags, vclosure *c ) {
	UV_CHECK_NULL(loop,);
	UV_CHECK_NULL(buf,);
	UV_CHECK_NULL(c,);
	UV_ALLOC_REQ(uv_random_t,r,c);
	UV_CHECK_ERROR(uv_random(loop,r,buf,length,flags,on_random),free_req((uv_req_t *)r),);
}
DEFINE_PRIM(_VOID, random_wrap, _LOOP _BYTES _I32 _I32 _FUN(_VOID,_I32));

// Tty

HL_PRIM uv_tty_t *HL_NAME(tty_init_wrap)( uv_loop_t *loop, int fd ) {
	UV_CHECK_NULL(loop,NULL);
	uv_tty_t *h = UV_ALLOC(uv_tty_t);
	UV_CHECK_ERROR(uv_tty_init(loop,h,fd,0),free_handle((uv_handle_t *)h),NULL);
	handle_init_hl_data((uv_handle_t *)h);
	return h;
}
DEFINE_PRIM(_HANDLE, tty_init_wrap, _LOOP _I32);

HL_PRIM void HL_NAME(tty_set_mode_wrap)( uv_tty_t *h, int mode ) {
	UV_CHECK_NULL(h,);
	uv_tty_mode_t uv_mode = UV_TTY_MODE_NORMAL;
	switch( mode ) {
		case 0: uv_mode = UV_TTY_MODE_NORMAL; break;
		case 1: uv_mode = UV_TTY_MODE_RAW; break;
		case 2: uv_mode = UV_TTY_MODE_IO; break;
	}
	UV_CHECK_ERROR(uv_tty_set_mode(h,uv_mode),,);
}
DEFINE_PRIM(_VOID, tty_set_mode_wrap, _HANDLE _I32);

HL_PRIM void HL_NAME(tty_reset_mode_wrap)() {
	UV_CHECK_ERROR(uv_tty_reset_mode(),,);
}
DEFINE_PRIM(_VOID, tty_reset_mode_wrap, _NO_ARG);

HL_PRIM vdynamic *HL_NAME(tty_get_winsize_wrap)( uv_tty_t *h ) {
	UV_CHECK_NULL(h,NULL);
	int width;
	int height;
	UV_CHECK_ERROR(uv_tty_get_winsize(h,&width,&height),,NULL);
	vdynamic *obj = (vdynamic *)hl_alloc_dynobj();
	hl_dyn_seti(obj, hl_hash_utf8("width"), &hlt_i32, width);
	hl_dyn_seti(obj, hl_hash_utf8("height"), &hlt_i32, height);
	return obj;
}
DEFINE_PRIM(_DYN, tty_get_winsize_wrap, _HANDLE);

HL_PRIM void HL_NAME(tty_set_vterm_state_wrap)( int state ) {
	uv_tty_vtermstate_t uv_state = UV_TTY_SUPPORTED;
	switch( state ) {
		case 0: uv_state = UV_TTY_SUPPORTED; break;
		case 1: uv_state = UV_TTY_UNSUPPORTED; break;
	}
	uv_tty_set_vterm_state(uv_state);
}
DEFINE_PRIM(_VOID, tty_set_vterm_state_wrap, _I32);

HL_PRIM int HL_NAME(tty_get_vterm_state_wrap)() {
	uv_tty_vtermstate_t state;
	UV_CHECK_ERROR(uv_tty_get_vterm_state(&state),,UV_TTY_SUPPORTED);
	switch( state ) {
		case UV_TTY_SUPPORTED: return 0;
		case UV_TTY_UNSUPPORTED:
		default: return 1;
	}
}
DEFINE_PRIM(_I32, tty_get_vterm_state_wrap, _NO_ARG);
