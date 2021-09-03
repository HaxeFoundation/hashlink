#define HL_NAME(n) uv_##n
#ifdef _WIN32
#	include <uv.h>
#	include <hl.h>
#else
#	include <hl.h>
#	include <uv.h>
#endif

#if (UV_VERSION_MAJOR <= 0 || UV_VERSION_MINOR < 42)
#	error "libuv 1.42 or newer required"
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
#define _BUF_ARRAY			_ABSTRACT(uv_buf_t_arr)
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
#define _CPU_TIMES			_ABSTRACT(uv_cpu_times_t_star)
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

typedef struct uv_cpu_times_s uv_cpu_times_t;

typedef struct sockaddr uv_sockaddr;
typedef struct sockaddr_in uv_sockaddr_in;
typedef struct sockaddr_in6 uv_sockaddr_in6;
typedef struct sockaddr_storage uv_sockaddr_storage;

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

#define DEFINE_PRIM_C_FIELD_REF(hl_return,c_return,hl_struct,c_struct,field) \
	HL_PRIM c_return HL_NAME(c_struct##_##field)( struct c_struct *s ) { \
		return &s->field; \
	} \
	DEFINE_PRIM(hl_return, c_struct##_##field, hl_struct);

#define DEFINE_PRIM_UV_FIELD(hl_return,c_return,hl_struct,uv_name,field) \
	HL_PRIM c_return HL_NAME(uv_name##_##field)( uv_##uv_name##_t *s ) { \
		return (c_return)s->field; \
	} \
	DEFINE_PRIM(hl_return, uv_name##_##field, hl_struct);

#define DEFINE_PRIM_UV_FIELD_REF(hl_return,c_return,hl_struct,uv_name,field) \
	HL_PRIM c_return HL_NAME(uv_name##_##field)( uv_##uv_name##_t *s ) { \
		return (c_return)&s->field; \
	} \
	DEFINE_PRIM(hl_return, uv_name##_##field, hl_struct);


#define DEFINE_PRIM_OF_POINTER(hl_type,uv_name) \
	HL_PRIM uv_##uv_name##_t *HL_NAME(pointer_to_##uv_name)( void *ptr ) { \
		return ptr; \
	} \
	DEFINE_PRIM(hl_type, pointer_to_##uv_name, _POINTER);

#define DEFINE_PRIM_TO_POINTER(hl_type,uv_name) \
	HL_PRIM void *HL_NAME(pointer_of_##uv_name)( uv_##uv_name##_t *v ) { \
		return v; \
	} \
	DEFINE_PRIM(_POINTER, pointer_of_##uv_name, hl_type);

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

HL_PRIM void *HL_NAME(pointer_of_bytes)( vbyte *bytes ) {
	return bytes;
}
DEFINE_PRIM(_POINTER, pointer_of_bytes, _BYTES);

DEFINE_PRIM_FREE(_REF(_BYTES), char_array);

// Buf

HL_PRIM uv_buf_t *HL_NAME(alloc_buf)( vbyte *bytes, int length ) {
	uv_buf_t *buf = UV_ALLOC(uv_buf_t);
	buf->base = (char *)bytes;
	buf->len = length;
	return buf;
}
DEFINE_PRIM(_BUF_ARRAY, alloc_buf, _BYTES _I32);

HL_PRIM void HL_NAME(buf_set)( uv_buf_t *buf, vbyte *bytes, int length ) { // TODO: change `length` to `int64`
	buf->base = (char *)bytes;
	buf->len = length;
}
DEFINE_PRIM(_VOID, buf_set, _BUF_ARRAY _BYTES _I32);

DEFINE_PRIM_FREE(_BUF_ARRAY, buf);
DEFINE_PRIM_UV_FIELD(_BYTES, vbyte *, _BUF_ARRAY, buf, base);
DEFINE_PRIM_UV_FIELD(_U64, int64, _BUF_ARRAY, buf, len);

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

DEFINE_PRIM_C_FIELD(_I32, int, _SOCKADDR_STORAGE, sockaddr_storage, ss_family);
DEFINE_PRIM_C_FIELD(_I32, int, _SOCKADDR_IN, sockaddr_in, sin_port);
DEFINE_PRIM_C_FIELD(_I32, int, _SOCKADDR_IN6, sockaddr_in6, sin6_port);

HL_PRIM struct sockaddr_storage *HL_NAME(alloc_sockaddr_storage)() {
	return UV_ALLOC(struct sockaddr_storage);
}
DEFINE_PRIM(_SOCKADDR_STORAGE, alloc_sockaddr_storage, _NO_ARG);

HL_PRIM int HL_NAME(sockaddr_storage_size)() {
	return sizeof(struct sockaddr_storage);
}
DEFINE_PRIM(_I32, sockaddr_storage_size, _NO_ARG);

#define DEFINE_PRIM_OF_SOCKADDR_STORAGE(hl_type, name) \
	HL_PRIM struct name *HL_NAME(name##_of_storage)( struct sockaddr_storage *addr ) { \
		return (struct name *)addr; \
	} \
	DEFINE_PRIM(hl_type, name##_of_storage, _SOCKADDR_STORAGE);

DEFINE_PRIM_OF_SOCKADDR_STORAGE(_SOCKADDR, sockaddr);
DEFINE_PRIM_OF_SOCKADDR_STORAGE(_SOCKADDR_IN, sockaddr_in);
DEFINE_PRIM_OF_SOCKADDR_STORAGE(_SOCKADDR_IN6, sockaddr_in6);

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

HL_PRIM int HL_NAME(address_family_of_af)( int af_family ) {
	switch( af_family ) {
		case AF_UNSPEC: return HL_UV_UNSPEC;
		case AF_INET: return HL_UV_INET;
		case AF_INET6: return HL_UV_INET6;
		default: return af_family;
	}
}
DEFINE_PRIM(_I32, address_family_of_af, _I32)

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

DEFINE_PRIM_OF_POINTER(_STATFS, statfs);
DEFINE_PRIM_OF_POINTER(_DIR, dir);
DEFINE_PRIM_FREE(_DIR, dir);

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
DEFINE_PRIM_FREE(_DIRENT, dirent);

HL_PRIM int HL_NAME(translate_to_sys_file_open_flag)( int hx_flag ) {
	switch( hx_flag ) {
		case 0: return UV_FS_O_APPEND;
		case 1: return UV_FS_O_CREAT;
		case 2: return UV_FS_O_DIRECT;
		case 3: return UV_FS_O_DIRECTORY;
		case 4: return UV_FS_O_DSYNC;
		case 5: return UV_FS_O_EXCL;
		case 6: return UV_FS_O_EXLOCK;
		case 7: return UV_FS_O_FILEMAP;
		case 8: return UV_FS_O_NOATIME;
		case 9: return UV_FS_O_NOCTTY;
		case 10: return UV_FS_O_NOFOLLOW;
		case 11: return UV_FS_O_NONBLOCK;
		case 12: return UV_FS_O_RANDOM;
		case 13: return UV_FS_O_RDONLY;
		case 14: return UV_FS_O_RDWR;
		case 15: return UV_FS_O_SEQUENTIAL;
		case 16: return UV_FS_O_SHORT_LIVED;
		case 17: return UV_FS_O_SYMLINK;
		case 18: return UV_FS_O_SYNC;
		case 19: return UV_FS_O_TEMPORARY;
		case 20: return UV_FS_O_TRUNC;
		case 21: return UV_FS_O_WRONLY;
		default: hl_error("Unknown file open flag index: %d", hx_flag);
	}
}
DEFINE_PRIM(_I32, translate_to_sys_file_open_flag, _I32);

HL_PRIM varray *HL_NAME(statfs_f_spare)( uv_statfs_t *stat ) {
	varray *a = hl_alloc_array(&hlt_i64, 4);
	hl_aptr(a,double)[0] = stat->f_spare[0];
	hl_aptr(a,double)[1] = stat->f_spare[1];
	hl_aptr(a,double)[2] = stat->f_spare[2];
	hl_aptr(a,double)[3] = stat->f_spare[3];
	return a;
}
DEFINE_PRIM(_ARR, statfs_f_spare, _STATFS);

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

// Misc

DEFINE_PRIM_FREE(_RUSAGE, rusage);
DEFINE_PRIM_FREE(_TIMEVAL64, timeval64);

HL_PRIM uv_cpu_info_t *HL_NAME(cpu_info_get)( uv_cpu_info_t *infos, int index ) {
	return &infos[index];
}
DEFINE_PRIM(_CPU_INFO, cpu_info_get, _CPU_INFO _I32);

HL_PRIM uv_interface_address_t *HL_NAME(interface_address_get)( uv_interface_address_t *addresses, int index ) {
	return &addresses[index];
}
DEFINE_PRIM(_INTERFACE_ADDRESS, interface_address_get, _INTERFACE_ADDRESS _I32);

#define DEFINE_PRIM_INTERFACE_ADDRESS_ADDR(field) \
	HL_PRIM uv_sockaddr_storage *HL_NAME(interface_address_##field)( uv_interface_address_t *addr ) { \
		uv_sockaddr_storage *result = UV_ALLOC(uv_sockaddr_storage); \
		memcpy(result, &addr->field, sizeof(addr->field)); \
		return result; \
	} \
	DEFINE_PRIM(_SOCKADDR_STORAGE, interface_address_##field, _INTERFACE_ADDRESS);

DEFINE_PRIM_INTERFACE_ADDRESS_ADDR(address);
DEFINE_PRIM_INTERFACE_ADDRESS_ADDR(netmask);

HL_PRIM char *HL_NAME(interface_address_phys_addr)( uv_interface_address_t *addr ) {
	return addr->phys_addr;
}
DEFINE_PRIM(_BYTES, interface_address_phys_addr, _INTERFACE_ADDRESS);

HL_PRIM varray *HL_NAME(loadavg_array)() {
	double avg[3];
	uv_loadavg(avg);
	varray *a = hl_alloc_array(&hlt_f64, 3);
	hl_aptr(a,double)[0] = avg[0];
	hl_aptr(a,double)[1] = avg[1];
	hl_aptr(a,double)[2] = avg[2];
	return a;
}
DEFINE_PRIM(_ARR, loadavg_array, _NO_ARG);

DEFINE_PRIM_ALLOC(_UTSNAME, utsname);
DEFINE_PRIM_FREE(_UTSNAME, utsname);
DEFINE_PRIM_UV_FIELD_REF(_BYTES, vbyte *, _UTSNAME, utsname, sysname);
DEFINE_PRIM_UV_FIELD_REF(_BYTES, vbyte *, _UTSNAME, utsname, release);
DEFINE_PRIM_UV_FIELD_REF(_BYTES, vbyte *, _UTSNAME, utsname, version);
DEFINE_PRIM_UV_FIELD_REF(_BYTES, vbyte *, _UTSNAME, utsname, machine);

DEFINE_PRIM_ALLOC(_RANDOM, random);

static void on_uv_random_cb( uv_random_t* r, int status, void* buf, size_t buflen ) {
	vclosure *c = DATA(uv_req_cb_data_t *, r)->callback;
	hl_call1(void, c, int, status);
}

// auto-generated libuv bindings
#include "uv_generated.c"