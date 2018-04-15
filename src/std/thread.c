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

#if !defined(HL_THREADS)

struct _hl_mutex {
	void *_unused;
};

struct _hl_tls {
	void *value;
};

struct _hl_spinlock {
	void *value;
};

#elif defined(HL_WIN)

struct _hl_mutex {
	CRITICAL_SECTION cs;
};

struct _hl_spinlock {
	void *value;
};

#else

#	include <pthread.h>
#	include <unistd.h>
#	include <sys/syscall.h>

struct _hl_mutex {
	pthread_mutex_t lock;
};

struct _hl_tls {
	pthread_key_t key;
};

struct _hl_spinlock {
	pthread_mutex_t lock;
	void *value;
};

#endif

// ----------------- ALLOC

HL_PRIM hl_mutex *hl_mutex_alloc() {
#	if !defined(HL_THREADS)
	return (hl_mutex*)1;
#	elif defined(HL_WIN)
	hl_mutex *l = (hl_mutex*)malloc(sizeof(hl_mutex));
	InitializeCriticalSection(&l->cs);
	return l;
#	else
	hl_mutex *l = (hl_mutex*)malloc(sizeof(hl_mutex));
	pthread_mutexattr_t a;
	pthread_mutexattr_init(&a);
	pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&l->lock,&a);
	pthread_mutexattr_destroy(&a);
	return l;
#	endif
}

HL_PRIM void hl_mutex_acquire( hl_mutex *l ) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	EnterCriticalSection(&l->cs);
#	else
	pthread_mutex_lock(&l->lock);
#	endif
}

HL_PRIM bool hl_mutex_try_acquire( hl_mutex *l ) {
#if	!defined(HL_THREADS)
	return true;
#	elif defined(HL_WIN)
	return (bool)TryEnterCriticalSection(&l->cs);
#	else
	return pthread_mutex_trylock(&l->lock) == 0;
#	endif
}

HL_PRIM void hl_mutex_release( hl_mutex *l ) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	LeaveCriticalSection(&l->cs);
#	else
	pthread_mutex_unlock(&l->lock);
#	endif
}

HL_PRIM void hl_mutex_free( hl_mutex *l ) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	DeleteCriticalSection(&l->cs);
	free(l);
#	else
	pthread_mutex_destroy(&l->lock);
	free(l);
#	endif
}

// ----------------- THREAD LOCAL

HL_PRIM hl_tls *hl_tls_alloc() {
#	if !defined(HL_THREADS)
	hl_tls *l = malloc(sizeof(hl_tls));
	l->value = NULL;
	return l;
#	elif defined(HL_WIN)
	DWORD t = TlsAlloc();
	TlsSetValue(t,NULL);
	return (hl_tls*)(int_val)t;
#	else
	hl_tls *l = malloc(sizeof(hl_tls));
	pthread_key_create(&l->key,NULL);
	return l;
#	endif
}

HL_PRIM void hl_tls_free( hl_tls *l ) {
#	if !defined(HL_THREADS)
	free(l);
#	elif defined(HL_WIN)
	TlsFree((DWORD)(int_val)l);
#	else
	pthread_key_delete(l->key);
	free(l);
#	endif
}

HL_PRIM void hl_tls_set( hl_tls *l, void *v ) {
#	if !defined(HL_THREADS)
	l->value = v;
#	elif defined(HL_WIN)
	TlsSetValue((DWORD)(int_val)l,v);
#	else
	pthread_setspecific(l->key,v);
#	endif
}

HL_PRIM void *hl_tls_get( hl_tls *l ) {
#	if !defined(HL_THREADS)
	return l->value;
#	elif defined(HL_WIN)
	return (void*)TlsGetValue((DWORD)(int_val)l);
#	else
	return pthread_getspecific(l->key);
#	endif
}

// ----------------- THREAD

HL_PRIM hl_thread *hl_thread_current() {
#if !defined(HL_THREADS)
	return NULL;
#elif defined(HL_WIN)
	return (hl_thread*)(int_val)GetCurrentThreadId();
#else
	return (hl_thread*)pthread_self();
#endif
}

HL_PRIM void hl_thread_yield() {
#if !defined(Hl_THREADS)
	// nothing
#elif defined(HL_WIN)
	Sleep(0);
#else
	pthread_yield();
#endif
}


HL_PRIM int hl_thread_id() {
#if !defined(HL_THREADS)
	return 0;
#elif defined(HL_WIN)
	return (int)GetCurrentThreadId();
#elif defined(SYS_gettid) && !defined(HL_TVOS)
	return syscall(SYS_gettid);
#else
	hl_error("hl_thread_id() not available for this platform");
	return -1;
#endif
}

typedef struct {
	void (*callb)( void *);
	void *param;
} thread_start;

static void gc_thread_entry( thread_start *_s ) {
	thread_start s = *_s;
	hl_register_thread(&s);
	hl_remove_root(&_s->param);
	free(_s);
	s.callb(s.param);
	hl_unregister_thread();
}

HL_PRIM hl_thread *hl_thread_start( void *callback, void *param, bool withGC ) {
#ifdef HL_THREADS
	if( withGC ) {
		thread_start *s = (thread_start*)malloc(sizeof(thread_start));
		s->callb = callback;
		s->param = param;
		hl_add_root(&s->param);
		callback = gc_thread_entry;
		param = s;
	}
#endif
#if !defined(HL_THREADS)
	hl_error("Threads support is disabled");
	return NULL;
#elif defined(HL_WIN)
	DWORD tid;
	HANDLE h = CreateThread(NULL,0,callback,param,0,&tid);
	if( h == NULL )
		return NULL;
	CloseHandle(h);
	return (hl_thread*)(int_val)tid;
#else
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
#endif
}

static void hl_run_thread( vclosure *c ) {
	bool isExc;
	varray *a;
	int i;
	vdynamic *exc = hl_dyn_call_safe(c,NULL,0,&isExc);
	if( !isExc )
		return;
	a = hl_exception_stack();
	uprintf(USTR("Uncaught exception: %s\n"), hl_to_string(exc));
	for(i=0;i<a->size;i++)
		uprintf(USTR("Called from %s\n"), hl_aptr(a,uchar*)[i]);
}

HL_PRIM hl_thread *hl_thread_create( vclosure *c ) {
	return hl_thread_start(hl_run_thread,c,true);
}

#define _THREAD _ABSTRACT(hl_thread)
DEFINE_PRIM(_THREAD, thread_current, _NO_ARG);
DEFINE_PRIM(_THREAD, thread_create, _FUN(_VOID,_NO_ARG));
