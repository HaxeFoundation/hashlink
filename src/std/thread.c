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
#include <hl.h>

#ifndef HL_WIN
#	include <pthread.h>
#endif

HL_PRIM hl_thread *hl_thread_current() {
#	ifdef HL_WIN
	return (hl_thread*)GetCurrentThreadId();
#	else
	return (hl_thread*)pthread_self();
#	endif
}

HL_PRIM hl_thread *hl_thread_start( void *callback, void *param, bool withGC ) {
	if( withGC ) hl_error("Threads with garbage collector are currently not supported");
#	ifdef HL_WIN
	DWORD tid;
	HANDLE h = CreateThread(NULL,0,callback,param,0,&tid);
	if( h == NULL )
		return NULL;
	CloseHandle(h);
	return (hl_thread*)tid;
#	else
	pthread_t t;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if( pthread_create(&t,&attr,callback,param) != 0 ) {
		pthread_attr_destroy(&attr);
		return NULL;
	}
	pthread_attr_destroy(&attr);
	return (hl_thread*)t;
#	endif
}