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
HL_API bool hl_socket_set_timeout( hl_socket *s, double t );

static hl_socket *debug_socket;

static void hl_debug_loop( hl_module *m ) {
	while( true ) {
		hl_socket *s = hl_socket_accept(debug_socket);
		printf("CONNECTED\n");
		hl_socket_close(s);
	}
}

bool hl_module_debug( hl_module *m, int port ) {
	hl_socket *s;
	hl_socket_init();
	s = hl_socket_new(false);
	if( s == NULL ) return false;
	if( !hl_socket_bind(s,0x0100007F/*127.0.0.1*/,port) || !hl_socket_listen(s, 10) ) {
		hl_socket_close(s);
		return false;
	}
	debug_socket = s;
	if( !hl_thread_start(hl_debug_loop, m, false) ) {
		hl_socket_close(s);
		return false;
	}
	return true;
}