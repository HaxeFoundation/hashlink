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
HL_API void hl_sys_sleep( double t );
HL_API int hl_socket_send( hl_socket *s, vbyte *buf, int pos, int len );
HL_API int hl_socket_recv( hl_socket *s, vbyte *buf, int pos, int len );

static hl_thread *main_thread = NULL;
static hl_socket *debug_socket = NULL;
static hl_socket *client_socket = NULL;
static bool debugger_connected = false;
static bool kill_on_debug_exit = false;

typedef enum {
	Run = 0,
	Pause = 1,
	Resume = 2,
	Stop = 3,
	Stack = 4,
} Command;

static void dbg_send( Command cmd ) {
	vbyte c = (vbyte)cmd;
	hl_socket_send(client_socket, &c, 0, 1);
}

#define STACK_SIZE 256

static void hl_debug_loop( hl_module *m ) {
	vbyte cmd;
	void *stack[STACK_SIZE];
	hl_thread_registers *regs = (hl_thread_registers*)malloc(sizeof(int_val) * hl_thread_context_size());
	int i_esp = hl_thread_context_index("esp");
//	int eip = hl_thread_context_index("eip");
	while( true ) {
		hl_socket *s = hl_socket_accept(debug_socket);
		client_socket = s;
		while( true ) {
			if( hl_socket_recv(s,&cmd,0,1) != 1 ) {
				hl_socket_close(s);
				if( kill_on_debug_exit ) exit(-9);
				break;
			}
			switch( cmd ) {
			case Run:
				debugger_connected = true;
				break;
			case Pause:
				hl_thread_pause(main_thread, true);
				break;
			case Resume:
				hl_thread_pause(main_thread, false);
				break;
			case Stack:
				if( !hl_thread_get_context(main_thread, regs) )
					exit(-10);
				{
					int i;
					int size = hl_module_capture_stack(stack,STACK_SIZE,(void**)regs[i_esp]);
					hl_socket_send(s,(vbyte*)&size,0,4);
					for(i=0;i<size;i++) {
						struct {
							int fidx;
							int fpos;
						} inf;
						if( !hl_module_resolve_pos(stack[i],&inf.fidx,&inf.fpos) ) {
							inf.fidx = -1;
							inf.fpos = -1;
						}
						hl_socket_send(s,(vbyte*)&inf,0,8);
					}
				}
				break;
			case Stop:
				exit(-9);
				break;
			default:
				fprintf(stderr,"Unknown debug command [%d]\n",cmd);
				break;
			}
		}
		hl_socket_close(s);
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
	main_thread = hl_thread_current();
	debug_socket = s;
	if( wait ) kill_on_debug_exit = true;
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