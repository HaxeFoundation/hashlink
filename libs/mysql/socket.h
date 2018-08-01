/*
 * Copyright (C)2005-2016 Haxe Foundation
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
#ifndef SOCKET_H
#define SOCKET_H

#include "osdef.h"

#ifdef OS_WINDOWS
#	include <winsock2.h>
	typedef SOCKET PSOCK;
#else
	typedef int PSOCK;
#	define INVALID_SOCKET (-1)
#endif

typedef unsigned int PHOST;

#define UNRESOLVED_HOST ((PHOST)-1)

typedef enum {
	PS_OK = 0,
	PS_ERROR = -1,
	PS_BLOCK = -2,
} SERR;

void psock_init();
PSOCK psock_create();
void psock_close( PSOCK s );
SERR psock_connect( PSOCK s, PHOST h, int port );
SERR psock_set_timeout( PSOCK s, double timeout );
SERR psock_set_blocking( PSOCK s, int block );
SERR psock_set_fastsend( PSOCK s, int fast );

int psock_send( PSOCK s, const char *buf, int size );
int psock_recv( PSOCK s, char *buf, int size );

PHOST phost_resolve( const char *hostname );

#endif
/* ************************************************************************ */
