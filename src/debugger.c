/*
 * Copyright (C)2015-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <hl.h>
#include <hlmodule.h>
#if defined(HL_THREADS) && !defined(HL_WIN)
#	include <pthread.h>
#endif

struct _hl_socket;
typedef struct _hl_socket hl_socket;
HL_API void hl_socket_init();
HL_API hl_socket *hl_socket_new( bool udp );
HL_API bool hl_socket_bind( hl_socket *s, int host, int port );
HL_API bool hl_socket_listen( hl_socket *s, int n );
HL_API void hl_socket_close( hl_socket *s );
HL_API hl_socket *hl_socket_accept( hl_socket *s );
HL_API int hl_socket_send( hl_socket *s, vbyte *buf, int pos, int len );
HL_API int hl_socket_recv( hl_socket *s, vbyte *buf, int pos, int len );
HL_API void hl_sys_sleep( double t );
HL_API int hl_sys_getpid();

static hl_socket *debug_socket = NULL;
static hl_socket *client_socket = NULL;
static bool debugger_connected = false;
static bool debugger_stopped = false;
static bool debugger_follows_modules = false;
static bool debug_lock = false;
#if !defined(HL_THREADS)
#elif defined(HL_WIN)
static CRITICAL_SECTION debug_cs;
#else
static pthread_mutex_t debug_cs = PTHREAD_MUTEX_INITIALIZER;
#endif

#define DEBUG_CMD_RUN			1
#define DEBUG_CMD_MODULES		2
#define DEBUG_CMD_MODULE_DEBUG	3
#define DEBUG_CMD_MODULE_BYTES	4
#define DEBUG_MSG_ERROR			-1
#define DEBUG_MSG_MODULE_ADDED	0x100

typedef struct {
	void *debug_data;
	void *raw_data;
	int debug_size;
	int raw_size;
	unsigned char build_id[HL_BUILD_ID_SIZE];
} hl_debug_module;

static struct {
	int count;
	int break_pending;
	hl_debug_module *modules;
} debug_registry = { 0, 0, NULL };

typedef struct {
	hl_socket *s;
	char *buf;
	int pos;
	int size;
} debug_out;

static void debug_write( debug_out *o, const void *ptr, int size ) {
	if( size <= 0 ) return;
	if( o->s ) {
		int pos = 0;
		while( pos < size ) {
			int r = hl_socket_send(o->s, (vbyte*)ptr, pos, size - pos);
			if( r <= 0 ) {
				o->s = NULL;
				return;
			}
			pos += r;
		}
		return;
	}
	if( o->pos + size > o->size ) {
		int nsize = o->size ? o->size : 4096;
		while( o->pos + size > nsize ) nsize <<= 1;
		char *nbuf = (char*)malloc(nsize);
		memcpy(nbuf, o->buf, o->pos);
		free(o->buf);
		o->buf = nbuf;
		o->size = nsize;
	}
	memcpy(o->buf + o->pos, ptr, size);
	o->pos += size;
}

static void debug_write_int( debug_out *o, int v ) {
	debug_write(o, &v, 4);
}

static void debug_write_ptr( debug_out *o, const void *p ) {
	debug_write(o, &p, sizeof(void*));
}

void hl_debug_lock( bool lock ) {
	if( !debug_lock ) return;
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	if( lock ) EnterCriticalSection(&debug_cs); else LeaveCriticalSection(&debug_cs);
#	else
	if( lock ) pthread_mutex_lock(&debug_cs); else pthread_mutex_unlock(&debug_cs);
#	endif
}

static void debug_write_functions( debug_out *o, hl_module *m ) {
	debug_write_ptr(o, m->globals_data);
	debug_write_ptr(o, m->jit_code);
	debug_write_int(o, m->codesize);
	debug_write_ptr(o, m->code->types);
	debug_write_int(o, m->code->nfunctions);
	for(int j=0;j<m->code->nfunctions;j++) {
		hl_function *f = m->code->functions + j;
		hl_debug_infos *d = m->jit_debug + j;
		struct {
			int nops;
			int start;
			int vars_size;
			unsigned char large;
		} fdata;
		int osize = d->large ? sizeof(int) : sizeof(unsigned short);
		fdata.nops = d->offsets ? f->nops : 0;
		fdata.start = d->start;
		fdata.vars_size = d->offsets ? d->vars_size : 0;
		fdata.large = (unsigned char)d->large;
		debug_write(o, &fdata, 13);
		if( d->offsets ) {
			debug_write(o, d->offsets, osize * (f->nops + 1));
			debug_write(o, d->vars, d->vars_size);
		} else {
			int zero = 0;
			debug_write(o, &zero, osize);
		}
	}
}

static void debug_write_extra( debug_out *o, hl_module *m ) {
	hl_code *c = m->code;
	int nglobals = m->globals_remap ? m->globals_remap_count : c->nglobals;
	debug_write(o, c->build_id, HL_BUILD_ID_SIZE);
	debug_write_ptr(o, c->raw_data);
	debug_write_int(o, c->raw_data ? c->raw_size : 0);
	debug_write_int(o, nglobals);
	for(int i=0;i<nglobals;i++)
		debug_write_int(o, m->globals_indexes[m->globals_remap ? m->globals_remap[i] : i]);
}

static void debug_register( hl_module *m, bool with_data ) {
	hl_debug_module e;
	memset(&e, 0, sizeof(e));
	memcpy(e.build_id, m->code->build_id, HL_BUILD_ID_SIZE);
	if( m->code->raw_data ) {
		e.raw_data = m->code->raw_data;
		e.raw_size = m->code->raw_size;
	}
	if( with_data ) {
		debug_out o = { NULL, NULL, 0, 0 };
		debug_write_functions(&o, m);
		debug_write_extra(&o, m);
		e.debug_data = o.buf;
		e.debug_size = o.pos;
	}
	int n = debug_registry.count;
	hl_debug_module *old = debug_registry.modules;
	hl_debug_module *mods = (hl_debug_module*)malloc(sizeof(hl_debug_module) * (n + 1));
	if( n ) memcpy(mods, old, sizeof(hl_debug_module) * n);
	mods[n] = e;
	debug_registry.modules = mods;
	debug_registry.count = n + 1;
	free(old);
}

static void debug_message( hl_socket *s, int code, debug_out *payload ) {
	debug_out o = { s, NULL, 0, 0 };
	debug_write_int(&o, code);
	debug_write_int(&o, payload ? payload->pos : 0);
	if( payload ) debug_write(&o, payload->buf, payload->pos);
}

void hl_debug_module_added( hl_module *m ) {
	if( !debug_lock ) return;
	debug_register(m, true);
	if( !client_socket ) return;
	hl_debug_module *e = debug_registry.modules + debug_registry.count - 1;
	debug_out p = { NULL, NULL, 0, 0 };
	debug_write_int(&p, debug_registry.count - 1);
	debug_write(&p, e->build_id, HL_BUILD_ID_SIZE);
	debug_write_ptr(&p, e->debug_data);
	debug_write_int(&p, e->debug_size);
	debug_write_ptr(&p, e->raw_data);
	debug_write_int(&p, e->raw_size);
	debug_message(client_socket, DEBUG_MSG_MODULE_ADDED, &p);
	free(p.buf);
	if( debugger_follows_modules ) debug_registry.break_pending = 1;
}

void hl_debug_module_break() {
	if( !debug_registry.break_pending ) return;
	hl_debug_break();
	debug_registry.break_pending = 0;
}

static bool debug_recv( hl_socket *s, void *buf, int size ) {
	int pos = 0;
	while( pos < size ) {
		int r = hl_socket_recv(s, (vbyte*)buf, pos, size - pos);
		if( r <= 0 ) return false;
		pos += r;
	}
	return true;
}

static bool debug_command( hl_socket *s, int cmd ) {
	int index = -1;
	if( (cmd == DEBUG_CMD_MODULE_DEBUG || cmd == DEBUG_CMD_MODULE_BYTES) && !debug_recv(s, &index, 4) )
		return false;
	debug_out p = { NULL, NULL, 0, 0 };
	int code = cmd;
	int nmodules;
	hl_debug_lock(true);
	hl_module **mods = hl_get_modules(&nmodules);
	switch( cmd ) {
	case DEBUG_CMD_MODULES:
		debug_write_int(&p, nmodules);
		for(int i=0;i<nmodules;i++) {
			debug_write(&p, mods[i]->code->build_id, HL_BUILD_ID_SIZE);
			debug_write_int(&p, mods[i]->code->nfunctions);
		}
		break;
	case DEBUG_CMD_MODULE_DEBUG:
		if( index < 0 || index >= nmodules ) {
			code = DEBUG_MSG_ERROR;
			break;
		}
		debug_write_functions(&p, mods[index]);
		debug_write_extra(&p, mods[index]);
		break;
	case DEBUG_CMD_MODULE_BYTES:
		if( index < 0 || index >= nmodules || !mods[index]->code->raw_data ) {
			code = DEBUG_MSG_ERROR;
			break;
		}
		debug_write(&p, mods[index]->code->raw_data, mods[index]->code->raw_size);
		break;
	default:
		code = DEBUG_MSG_ERROR;
		break;
	}
	if( code == DEBUG_MSG_ERROR ) {
		p.pos = 0;
		debug_write_int(&p, cmd);
	}
	debug_message(s, code, &p);
	hl_debug_lock(false);
	free(p.buf);
	return true;
}

static void hl_debug_loop() {
	void *inf_addr = hl_gc_threads_info();
	int flags = 0;
	int hl_ver = HL_VERSION;
	bool loop = false;
	int pid = hl_sys_getpid();
#	ifdef HL_64
	flags |= 1;
#	endif
	if( sizeof(bool) == 4 ) flags |= 2;
#	ifdef HL_THREADS
	flags |= 4;
	loop = true;
#	endif
#	ifdef HL_WIN_CALL
	flags |= 8;
#	endif
	flags |= 16;
	flags |= 32;
	hl_get_thread()->flags |= HL_THREAD_INVISIBLE;
	do {
		vbyte cmd;
		hl_socket *s = hl_socket_accept(debug_socket);
		if( s == NULL ) break;
		hl_debug_lock(true);
		debug_out o = { NULL, NULL, 0, 0 };
		debug_write(&o, "HLD2", 4);
		debug_write_int(&o, flags);
		debug_write_int(&o, hl_ver);
		debug_write_int(&o, pid);
		debug_write_ptr(&o, inf_addr);

		for(int i=1;i<=HBYTES;i++) {
			hl_type t = {(hl_type_kind)i};
			int k = 1 + hl_pad_struct(1,&t);
			debug_write_int(&o, k);
		}

		int nmodules;
		hl_module **mods = hl_get_modules(&nmodules);

		debug_write_int(&o, hl_jit_trampoline);

		debug_write_int(&o, nmodules);
		for(int i=0;i<nmodules;i++)
			debug_write_functions(&o, mods[i]);
		for(int i=0;i<nmodules;i++)
			debug_write_extra(&o, mods[i]);
		debug_write_ptr(&o, &debug_registry);
		debug_out so = { s, NULL, 0, 0 };
		debug_write(&so, o.buf, o.pos);
		free(o.buf);
		client_socket = s;
		hl_debug_lock(false);

		hl_setup.closure_stack_capture = 8;
		hl_setup.is_debugger_attached = true;

		// an old debugger closes the socket once attached
		while( hl_socket_recv(s,&cmd,0,1) == 1 ) {
			if( cmd == DEBUG_CMD_RUN ) {
				debugger_follows_modules = true;
				debugger_connected = true;
				continue;
			}
			if( !debug_command(s, cmd) ) break;
		}
		hl_debug_lock(true);
		client_socket = NULL;
		debugger_follows_modules = false;
		hl_debug_lock(false);
		hl_socket_close(s);
		debugger_connected = true;
	} while( loop );
	debugger_stopped = true;
}

h_bool hl_module_debug( hl_module *m, int port, h_bool wait ) {
	hl_socket *s;
	hl_socket_init();
	s = hl_socket_new(false);
	if( s == NULL ) return false;
	if( !hl_socket_bind(s,0x0100007F/*127.0.0.1*/,port) || !hl_socket_listen(s, 10) ) {
		hl_socket_close(s);
		return false;
	}
	debug_socket = s;
#	if defined(HL_THREADS) && defined(HL_WIN)
	InitializeCriticalSection(&debug_cs);
#	endif
	debug_lock = true;
	{
		int nmodules;
		hl_module **mods = hl_get_modules(&nmodules);
		for(int i=0;i<nmodules;i++)
			debug_register(mods[i], false);
	}
#	ifdef HL_THREADS
	hl_add_root(&debug_socket);
	hl_add_root(&client_socket);
	if( !hl_thread_start(hl_debug_loop, m, true) ) {
		hl_socket_close(s);
		return false;
	}
	if( wait ) {
		while( !debugger_connected )
			hl_sys_sleep(0.01);
	}
#	else
	// imply --debug-wait
	hl_debug_loop(m);
	hl_socket_close(debug_socket);
	debug_socket = NULL;
#	endif
	hl_setup.is_debugger_enabled = true;
	return true;
}

void hl_module_debug_stop() {
	if( !debug_socket ) return;
#	ifdef HL_THREADS
	hl_socket_close(debug_socket);
	while( !debugger_stopped )
		hl_sys_sleep(0.01);
	hl_remove_root(&debug_socket);
	hl_remove_root(&client_socket);
#	endif
}
