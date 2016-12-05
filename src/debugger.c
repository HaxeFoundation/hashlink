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

static hl_socket *debug_socket = NULL;
static hl_socket *client_socket = NULL;
static bool debugger_connected = false;

#define send hl_send_data
static void send( void *ptr, int size ) {
	hl_socket_send(client_socket, ptr, 0, size);
}

static void hl_debug_loop( hl_module *m ) {
	void *stack_top = hl_module_stack_top();
	int flags = 0;
#	ifdef HL_64
	flags |= 1;
#	endif
	if( sizeof(bool) == 4 ) flags |= 2;
	while( true ) {
		int i;
		vbyte cmd;
		hl_socket *s = hl_socket_accept(debug_socket);
		client_socket = s;
		send("HLD0",4);
		send(&flags,4);
		send(&stack_top,sizeof(void*));
		send(&m->jit_code,sizeof(void*));
		send(&m->codesize,4);
		send(&m->code->nfunctions,4);

		for(i=0;i<m->code->nfunctions;i++) {
			hl_function *f = m->code->functions + i;
			hl_debug_infos *d = m->jit_debug + i;
			struct {
				int nops;
				int start;
				unsigned char large;
			} fdata;
			fdata.nops = f->nops;
			fdata.start = d->start;
			fdata.large = (unsigned char)d->large;
			send(&fdata,9);
			send(d->offsets,(d->large ? sizeof(int) : sizeof(unsigned short)) * (f->nops + 1));
		}

		// wait answer
		hl_socket_recv(s,&cmd,0,1);
		hl_socket_close(s);
		debugger_connected = true;
		client_socket = NULL;
	}
}

bool hl_module_debug( hl_module *m, int port, bool wait ) {
	hl_socket *s;
	hl_socket_init();
	s = hl_socket_new(false);
	if( s == NULL ) return false;
	if( !hl_socket_bind(s,0x0100007F/*127.0.0.1*/,port) || !hl_socket_listen(s, 10) ) {
		hl_socket_close(s);
		return false;
	}
	hl_add_root(&debug_socket);
	hl_add_root(&client_socket);
	debug_socket = s;
	if( !hl_thread_start(hl_debug_loop, m, false) ) {
		hl_socket_close(s);
		return false;
	}
	if( wait ) {
		while( !debugger_connected )
			hl_sys_sleep(0.01);
	}
	return true;
}
