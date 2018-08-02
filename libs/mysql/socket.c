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
#include "socket.h"
#include <string.h>

#ifdef OS_WINDOWS
	static int init_done = 0;
	static WSADATA init_data;
#	define POSIX_LABEL(x)
#	define HANDLE_EINTR(x)

#else
#	define _GNU_SOURCE
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <sys/time.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <arpa/inet.h>
#	include <unistd.h>
#	include <netdb.h>
#	include <fcntl.h>
#	include <errno.h>
#	include <stdio.h>
#	include <poll.h>
#	define closesocket close
#	define SOCKET_ERROR (-1)
#	define POSIX_LABEL(x)	x:
#	define HANDLE_EINTR(x)	if( errno == EINTR ) goto x
#endif

#if defined(OS_WINDOWS) || defined(OS_MAC)
#	define MSG_NOSIGNAL 0
#endif

static int block_error() {
#ifdef OS_WINDOWS
	int err = WSAGetLastError();
	if( err == WSAEWOULDBLOCK || err == WSAEALREADY )
#else
	if( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS || errno == EALREADY )
#endif
		return PS_BLOCK;
	return PS_ERROR;
}

void psock_init() {
#ifdef OS_WINDOWS
	if( !init_done ) {
		WSAStartup(MAKEWORD(2,0),&init_data);
		init_done = 1;
	}
#endif
}

PSOCK psock_create() {
	PSOCK s = socket(AF_INET,SOCK_STREAM,0);
#	if defined(OS_MAC) || defined(OS_BSD)
	if( s != INVALID_SOCKET )
		setsockopt(s,SOL_SOCKET,SO_NOSIGPIPE,NULL,0);
#	endif
#	ifdef OS_POSIX
	// we don't want sockets to be inherited in case of exec
	{
		int old = fcntl(s,F_GETFD,0);
		if( old >= 0 ) fcntl(s,F_SETFD,old|FD_CLOEXEC);
	}
#	endif
	return s;
}

void psock_close( PSOCK s ) {
	POSIX_LABEL(close_again);
	if( closesocket(s) ) {
		HANDLE_EINTR(close_again);
	}
}

int psock_send( PSOCK s, const char *buf, int size ) {
	int ret;
	POSIX_LABEL(send_again);
	ret = send(s,buf,size,MSG_NOSIGNAL);
	if( ret == SOCKET_ERROR ) {
		HANDLE_EINTR(send_again);
		return block_error();
	}
	return ret;
}

int psock_recv( PSOCK s, char *buf, int size ) {
	int ret;
	POSIX_LABEL(recv_again);
	ret = recv(s,buf,size,MSG_NOSIGNAL);
	if( ret == SOCKET_ERROR ) {
		HANDLE_EINTR(recv_again);
		return block_error();
	}
	return ret;
}

PHOST phost_resolve( const char *host ) {
	PHOST ip = inet_addr(host);
	if( ip == INADDR_NONE ) {
		struct hostent *h;
#	if defined(OS_WINDOWS) || defined(OS_MAC) || defined(OS_CYGWIN)
		h = gethostbyname(host);
#	else
		struct hostent hbase;
		char buf[1024];
		int errcode;
		gethostbyname_r(host,&hbase,buf,1024,&h,&errcode);
#	endif
		if( h == NULL )
			return UNRESOLVED_HOST;
		ip = *((unsigned int*)h->h_addr_list[0]);
	}
	return ip;
}

SERR psock_connect( PSOCK s, PHOST host, int port ) {
	struct sockaddr_in addr;
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	*(int*)&addr.sin_addr.s_addr = host;
	if( connect(s,(struct sockaddr*)&addr,sizeof(addr)) != 0 )
		return block_error();
	return PS_OK;
}

SERR psock_set_timeout( PSOCK s, double t ) {
#ifdef OS_WINDOWS
	int time = (int)(t * 1000);
#else
	struct timeval time;
	time.tv_usec = (int)((t - (int)t)*1000000);
	time.tv_sec = (int)t;
#endif
	if( setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,(char*)&time,sizeof(time)) != 0 )
		return PS_ERROR;
	if( setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,(char*)&time,sizeof(time)) != 0 )
		return PS_ERROR;
	return PS_OK;
}


SERR psock_set_blocking( PSOCK s, int block ) {
#ifdef OS_WINDOWS
	{
		unsigned long arg = !block;
		if( ioctlsocket(s,FIONBIO,&arg) != 0 )
			return PS_ERROR;
	}
#else
	{
		int rights = fcntl(s,F_GETFL);
		if( rights == -1 )
			return PS_ERROR;
		if( block )
			rights &= ~O_NONBLOCK;
		else
			rights |= O_NONBLOCK;
		if( fcntl(s,F_SETFL,rights) == -1 )
			return PS_ERROR;
	}
#endif
	return PS_OK;
}

SERR psock_set_fastsend( PSOCK s, int fast ) {
	if( setsockopt(s,IPPROTO_TCP,TCP_NODELAY,(char*)&fast,sizeof(fast)) )
		return block_error();
	return PS_OK;
}

void psock_wait( PSOCK s ) {
#	ifdef OS_WINDOWS
	fd_set set;
	FD_ZERO(&set);
	FD_SET(s,&set);
	select((int)s+1,&set,NULL,NULL,NULL);
#	else
	struct pollfd fds;
	POSIX_LABEL(poll_again);
	fds.fd = s;
	fds.events = POLLIN;
	fds.revents = 0;
	if( poll(&fds,1,-1) < 0 ) {
		HANDLE_EINTR(poll_again);
	}
#	endif
}

/* ************************************************************************ */
