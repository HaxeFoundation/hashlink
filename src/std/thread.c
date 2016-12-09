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
#	include <sys/syscall.h>
#endif

HL_PRIM hl_thread *hl_thread_current() {
#	ifdef HL_WIN
	return (hl_thread*)(int_val)GetCurrentThreadId();
#	else
	return (hl_thread*)pthread_self();
#	endif
}

HL_PRIM int hl_thread_id() {
#	ifdef HL_WIN
	return (int)GetCurrentThreadId();
#	else
#	ifdef SYS_gettid
	return syscall(SYS_gettid);
#	else
	hl_error("hl_thread_id() not available for this platform");
	return -1;
#	endif
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
	return (hl_thread*)(int_val)tid;
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

HL_PRIM bool hl_thread_pause( hl_thread *t, bool pause ) {
#	ifdef HL_WIN
	bool ret;
	HANDLE h = OpenThread(THREAD_ALL_ACCESS,FALSE,(DWORD)(int_val)t);
	if( pause ) 
		ret = ((int)SuspendThread(h)) >= 0;
	else {
		int r;
		while( (r = (int)ResumeThread(h)) > 0 ) {
		}
		ret = r == 0;
	}
	CloseHandle(h);
	return ret;
#	else
	// TODO : use libthread_db
	return false;
#	endif
}

HL_PRIM int hl_thread_context_size() {
#	ifdef HL_WIN
	return (sizeof(CONTEXT) + sizeof(int_val) - 1) / sizeof(int_val);
#	else
	return 0;
#	endif
}

HL_API int hl_thread_context_index( const char *name ) {
#	ifdef HL_WIN
	CONTEXT *c = NULL;
#	define _ADDR(__r) (int)(((int_val)&c->__r) / sizeof(int_val))
#	ifdef HL_64
#		define ADDR(_,r) _ADDR(r)
#	else
#		define ADDR(r,_) _ADDR(r)
#	endif
	if( strcmp(name,"eip") == 0 )
		return ADDR(Eip,Rip);
	if( strcmp(name,"esp") == 0 )
		return ADDR(Esp,Rsp);
	return -1;
#	else
	return -1;
#	endif
#	undef ADDR
#	undef _ADDR
}

HL_API bool hl_thread_get_context( hl_thread *t, hl_thread_registers *regs ) {
#	ifdef HL_WIN
	HANDLE h = OpenThread(THREAD_ALL_ACCESS,FALSE,(DWORD)(int_val)t);
	bool ret;
	CONTEXT *c = (CONTEXT*)regs;
	c->ContextFlags = CONTEXT_FULL;
	ret = (bool)GetThreadContext(h,c);
	CloseHandle(h);
	return ret;
#	else
	return false;
#	endif
}

HL_API bool hl_thread_set_context( hl_thread *t, hl_thread_registers *regs ) {
#	ifdef HL_WIN
	HANDLE h = OpenThread(THREAD_ALL_ACCESS,FALSE,(DWORD)(int_val)t);
	bool ret;
	CONTEXT *c = (CONTEXT*)regs;
	c->ContextFlags = CONTEXT_FULL;
	ret = (bool)SetThreadContext(h,c);
	CloseHandle(h);
	return ret;
#	else
	return false;
#	endif
}
