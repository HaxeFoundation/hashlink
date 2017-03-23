#define HL_NAME(n) uv_##n
#include <uv.h>
#include <hl.h>

// loop

#define _LOOP _ABSTRACT(uv_loop)
DEFINE_PRIM(_LOOP, default_loop, _NO_ARG);
DEFINE_PRIM(_I32, loop_close, _LOOP);
DEFINE_PRIM(_I32, run, _LOOP _I32);
DEFINE_PRIM(_I32, loop_alive, _LOOP);
DEFINE_PRIM(_VOID, stop, _LOOP);

DEFINE_PRIM(_BYTES, strerror, _I32);

// HANDLE

#define _HANDLE _ABSTRACT(uv_handle)
//DEFINE_PRIM(_VOID, close, _HANDLE);

// TCP

#define _TCP _ABSTRACT(uv_tcp)

HL_PRIM uv_tcp_t *uv_tcp_new( uv_loop_t *loop ) {
	uv_tcp_t *t = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	if( uv_tcp_init(loop,t) < 0 )
		return NULL;
	return t;
}

DEFINE_PRIM(_TCP, tcp_new, _LOOP);