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

typedef struct _hl_semaphore hl_semaphore;
typedef struct _hl_condition hl_condition;

#if !defined(HL_THREADS)

struct _hl_mutex {
	void (*free)( hl_mutex * );
	void *_unused;
};

struct _hl_semaphore {
  void (*free)(hl_semaphore *);
};

struct _hl_condition {
  void (*free)(hl_condition *);
};

struct _hl_tls {
	void (*free)( hl_tls * );
	void *value;
};

#elif defined(HL_WIN)

struct _hl_mutex {
	void (*free)( hl_mutex * );
	CRITICAL_SECTION cs;
	bool is_gc;
};

struct _hl_semaphore {
	void (*free)(hl_semaphore *);
	HANDLE sem;
};

struct _hl_condition {
	void (*free)(hl_condition *);
	CRITICAL_SECTION cs;
	CONDITION_VARIABLE cond;
};

struct _hl_tls {
	void (*free)( hl_tls * );
	DWORD tid;
};

#else

#	include <pthread.h>
#	include <unistd.h>
#	include <sys/syscall.h>
#	include <sys/time.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <semaphore.h>
#endif

struct _hl_mutex {
	void (*free)( hl_mutex * );
	pthread_mutex_t lock;
	bool is_gc;
};

struct _hl_semaphore {
	void (*free)(hl_semaphore *);
#	ifdef __APPLE__
	dispatch_semaphore_t sem;
#	else
	sem_t sem;
#endif
};

struct _hl_condition {
	void (*free)(hl_condition *);
	pthread_mutex_t mutex;
	pthread_cond_t cond;
};

struct _hl_tls {
	void (*free)( hl_tls * );
	pthread_key_t key;
};

#endif

// ----------------- ALLOC

HL_PRIM hl_mutex *hl_mutex_alloc( bool gc_thread ) {
#	if !defined(HL_THREADS)
	static struct _hl_mutex null_mutex = {0};
	return (hl_mutex*)&null_mutex;
#	elif defined(HL_WIN)
	hl_mutex *l = (hl_mutex*)hl_gc_alloc_finalizer(sizeof(hl_mutex));
	l->free = hl_mutex_free;
	l->is_gc = gc_thread;
	InitializeCriticalSection(&l->cs);
	return l;
#	else
	hl_mutex *l = (hl_mutex*)hl_gc_alloc_finalizer(sizeof(hl_mutex));
	l->free = hl_mutex_free;
	l->is_gc = gc_thread;
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
	if( l->is_gc ) hl_blocking(true);
	EnterCriticalSection(&l->cs);
	if( l->is_gc ) hl_blocking(false);
#	else
	if( l->is_gc ) hl_blocking(true);
	pthread_mutex_lock(&l->lock);
	if( l->is_gc ) hl_blocking(false);
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
	if( l->free ) {
		DeleteCriticalSection(&l->cs);
		l->free = NULL;
	}
#	else
	if( l->free ) {
		pthread_mutex_destroy(&l->lock);
		l->free = NULL;
	}
#	endif
}

#define _MUTEX _ABSTRACT(hl_mutex)
DEFINE_PRIM(_MUTEX, mutex_alloc, _BOOL);
DEFINE_PRIM(_VOID, mutex_acquire, _MUTEX);
DEFINE_PRIM(_BOOL, mutex_try_acquire, _MUTEX);
DEFINE_PRIM(_VOID, mutex_release, _MUTEX);
DEFINE_PRIM(_VOID, mutex_free, _MUTEX);

// ------------------ SEMAPHORE

HL_PRIM hl_semaphore *hl_semaphore_alloc(int value) {
#	if !defined(HL_THREADS)
	static struct _hl_semaphore null_semaphore = {0};
	return (hl_semaphore *)&null_semaphore;
#	elif defined(HL_WIN)
	hl_semaphore *sem =
	    (hl_semaphore *)hl_gc_alloc_finalizer(sizeof(hl_semaphore));
	sem->free = hl_semaphore_free;
	sem->sem = CreateSemaphoreW(NULL, value, 0x7FFFFFFF, NULL);
	return sem;
#	else
	hl_semaphore *sem =
	    (hl_semaphore *)hl_gc_alloc_finalizer(sizeof(hl_semaphore));
	sem->free = hl_semaphore_free;
#	ifdef __APPLE__
	sem->sem = dispatch_semaphore_create(value);
#	else
	sem_init(&sem->sem, false, value);
#	endif
	return sem;
#	endif
}

HL_PRIM void hl_semaphore_acquire(hl_semaphore *sem) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	WaitForSingleObject(sem->sem, INFINITE);
#	else
#	ifdef __APPLE__
	dispatch_semaphore_wait(sem->sem, DISPATCH_TIME_FOREVER);
#	else
	sem_wait(&sem->sem);
#	endif
#	endif
}

HL_PRIM bool hl_semaphore_try_acquire(hl_semaphore *sem, vdynamic *timeout) {
#	if !defined(HL_THREADS)
	return true;
#	elif defined(HL_WIN)
	return WaitForSingleObject(sem->sem,
	                           timeout ? (DWORD)((FLOAT)timeout->v.d * 1000.0)
	                                   : 0) == WAIT_OBJECT_0;
#	else
#	ifdef __APPLE__
	return dispatch_semaphore_wait(
	           sem->sem, dispatch_time(DISPATCH_TIME_NOW,
	                              (int64_t)((timeout ? timeout->v.d : 0) *
	                                        1000 * 1000 * 1000))) == 0;
#	else
	if (timeout) {
		struct timeval tv;
		struct timespec t;
		double delta = timeout->v.d;
		int idelta = (int)delta, idelta2;
		delta -= idelta;
		delta *= 1.0e9;
		gettimeofday(&tv, NULL);
		delta += tv.tv_usec * 1000.0;
		idelta2 = (int)(delta / 1e9);
		delta -= idelta2 * 1e9;
		t.tv_sec = tv.tv_sec + idelta + idelta2;
		t.tv_nsec = (long)delta;
		return sem_timedwait(&sem->sem, &t) == 0;
	} else {

		return sem_trywait(&sem->sem) == 0;
	}
#	endif
#	endif
}

HL_PRIM void hl_semaphore_release(hl_semaphore *sem) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	ReleaseSemaphore(sem->sem, 1, NULL);
#	else
#	ifdef __APPLE__
	dispatch_semaphore_signal(sem->sem);
#	else
	sem_post(&sem->sem);
#	endif
#	endif
}

HL_PRIM void hl_semaphore_free(hl_semaphore *sem) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	if (sem->free) {
		CloseHandle(sem->sem);
		sem->free = NULL;
	}
#	else
	if (sem->free) {
#ifndef __APPLE__
		sem_destroy(&sem->sem);
#endif
		sem->free = NULL;
	}
#	endif
}

#define _SEMAPHORE _ABSTRACT(hl_semaphore)
DEFINE_PRIM(_SEMAPHORE, semaphore_alloc, _I32);
DEFINE_PRIM(_VOID, semaphore_acquire, _SEMAPHORE);
DEFINE_PRIM(_BOOL, semaphore_try_acquire, _SEMAPHORE _NULL(_F64));
DEFINE_PRIM(_VOID, semaphore_release, _SEMAPHORE);
DEFINE_PRIM(_VOID, semaphore_free, _SEMAPHORE);
// ------------------ CONDITION

HL_PRIM hl_condition *hl_condition_alloc() {
#	if !defined(HL_THREADS)
	static struct _hl_condition null_condition = {0};
	return (hl_condition *)&null_condition;
#	elif defined(HL_WIN)
	hl_condition *cond =
	    (hl_condition *)hl_gc_alloc_finalizer(sizeof(hl_condition));
	cond->free = hl_condition_free;
	InitializeCriticalSection(&cond->cs);
	InitializeConditionVariable(&cond->cond);
	return cond;
#	else
	hl_condition *cond =
	    (hl_condition *)hl_gc_alloc_finalizer(sizeof(hl_condition));
	cond->free = hl_condition_free;
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_cond_init(&cond->cond, &attr);
	pthread_condattr_destroy(&attr);
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&cond->mutex, &mutexattr);
	pthread_mutexattr_destroy(&mutexattr);
	return cond;
#	endif
}
HL_PRIM void hl_condition_acquire(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	EnterCriticalSection(&cond->cs);
#	else
	pthread_mutex_lock(&cond->mutex);
#	endif
}

HL_PRIM bool hl_condition_try_acquire(hl_condition *cond) {
#	if !defined(HL_THREADS)
	return true;
#	elif defined(HL_WIN)
	return (bool)TryEnterCriticalSection(&cond->cs);
#	else
	return pthread_mutex_trylock(&cond->mutex) == 0;
#	endif
}

HL_PRIM void hl_condition_release(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	LeaveCriticalSection(&cond->cs);
#	else
	pthread_mutex_unlock(&cond->mutex);
#	endif
}
HL_PRIM void hl_condition_wait(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	SleepConditionVariableCS(&cond->cond, &cond->cs, INFINITE);
#	else
	pthread_cond_wait(&cond->cond, &cond->mutex);
#	endif
}

HL_PRIM bool hl_condition_timed_wait(hl_condition *cond, double timeout) {
#	if !defined(HL_THREADS)
	return true;
#	elif defined(HL_WIN)
	SleepConditionVariableCS(&cond->cond, &cond->cs,
	                         (DWORD)((FLOAT)timeout * 1000.0));
	return true;
#	else
	struct timeval tv;
	struct timespec t;
	double delta = timeout;
	int idelta = (int)delta, idelta2;
	delta -= idelta;
	delta *= 1.0e9;
	gettimeofday(&tv, NULL);
	delta += tv.tv_usec * 1000.0;
	idelta2 = (int)(delta / 1e9);
	delta -= idelta2 * 1e9;
	t.tv_sec = tv.tv_sec + idelta + idelta2;
	t.tv_nsec = (long)delta;
	return pthread_cond_timedwait(&cond->cond, &cond->mutex, &t) == 0;
#	endif
}

HL_PRIM void hl_condition_signal(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	WakeConditionVariable(&cond->cond);
#	else
	pthread_cond_signal(&cond->cond);
#	endif
}
HL_PRIM void hl_condition_broadcast(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	WakeAllConditionVariable(&cond->cond);
#	else
	pthread_cond_broadcast(&cond->cond);
#	endif
}
HL_PRIM void hl_condition_free(hl_condition *cond) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	if (cond->free) {
		DeleteCriticalSection(&cond->cs);
		cond->free = NULL;
	}
#	else
	if (cond->free) {
		pthread_cond_destroy(&cond->cond);
		pthread_mutex_destroy(&cond->mutex);
		cond->free = NULL;
	}
#	endif
}

#define _CONDITION _ABSTRACT(hl_condition)
DEFINE_PRIM(_CONDITION, condition_alloc, _NO_ARG)
DEFINE_PRIM(_VOID, condition_acquire, _CONDITION)
DEFINE_PRIM(_BOOL, condition_try_acquire, _CONDITION)
DEFINE_PRIM(_VOID, condition_release, _CONDITION)
DEFINE_PRIM(_VOID, condition_wait, _CONDITION)
DEFINE_PRIM(_BOOL, condition_timed_wait, _CONDITION _F64)
DEFINE_PRIM(_VOID, condition_signal, _CONDITION)
DEFINE_PRIM(_VOID, condition_broadcast, _CONDITION)

// ----------------- THREAD LOCAL

HL_PRIM hl_tls *hl_tls_alloc( bool gc_value ) {
#	if !defined(HL_THREADS)
	hl_tls *l = (hl_tls*)hl_gc_alloc_finalizer(sizeof(hl_tls));
	l->free = hl_tls_free;
	l->value = NULL;
	return l;
#	elif defined(HL_WIN)
	hl_tls *l = (hl_tls*)hl_gc_alloc_finalizer(sizeof(hl_tls));
	l->free = hl_tls_free;
	l->tid = TlsAlloc();
	TlsSetValue(l->tid,NULL);
	return l;
#	else
	hl_tls *l = (hl_tls*)hl_gc_alloc_finalizer(sizeof(hl_tls));
	l->free = hl_tls_free;
	pthread_key_create(&l->key,NULL);
	return l;
#	endif
}

HL_PRIM void hl_tls_free( hl_tls *l ) {
#	if !defined(HL_THREADS)
	free(l);
#	elif defined(HL_WIN)
	if( l->free ) {
		TlsFree(l->tid);
		l->free = NULL;
	}
#	else
	if( l->free ) {
		pthread_key_delete(l->key);
		l->free = NULL;
	}
#	endif
}

HL_PRIM void hl_tls_set( hl_tls *l, void *v ) {
#	if !defined(HL_THREADS)
	l->value = v;
#	elif defined(HL_WIN)
	TlsSetValue(l->tid,v);
#	else
	pthread_setspecific(l->key,v);
#	endif
}

HL_PRIM void *hl_tls_get( hl_tls *l ) {
#	if !defined(HL_THREADS)
	return l->value;
#	elif defined(HL_WIN)
	return (void*)TlsGetValue(l->tid);
#	else
	return pthread_getspecific(l->key);
#	endif
}

#define _TLS _ABSTRACT(hl_tls)
DEFINE_PRIM(_TLS, tls_alloc, _BOOL);
DEFINE_PRIM(_DYN, tls_get, _TLS);
DEFINE_PRIM(_VOID, tls_set, _TLS _DYN);

// ----------------- DEQUE

typedef struct _tqueue {
	vdynamic *msg;
	struct _tqueue *next;
} tqueue;

struct _hl_deque;
typedef struct _hl_deque hl_deque;

struct _hl_deque {
	void (*free)( hl_deque * );
	tqueue *first;
	tqueue *last;
#ifdef HL_THREADS
#	ifdef HL_WIN
	CRITICAL_SECTION lock;
	HANDLE wait;
#	else
	pthread_mutex_t lock;
	pthread_cond_t wait;
#	endif
#endif
};

#if !defined(HL_THREADS)
#	define LOCK(l)
#	define UNLOCK(l)
#	define SIGNAL(l)
#elif defined(HL_WIN)
#	define LOCK(l)		EnterCriticalSection(&(l))
#	define UNLOCK(l)	LeaveCriticalSection(&(l))
#	define SIGNAL(l)	ReleaseSemaphore(l,1,NULL)
#else
#	define LOCK(l)		pthread_mutex_lock(&(l))
#	define UNLOCK(l)	pthread_mutex_unlock(&(l))
#	define SIGNAL(l)	pthread_cond_signal(&(l))
#endif

static void hl_deque_free( hl_deque *q ) {
	hl_remove_root(&q->first);
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	DeleteCriticalSection(&q->lock);
	CloseHandle(q->wait);
#	else
	pthread_mutex_destroy(&q->lock);
	pthread_cond_destroy(&q->wait);
#	endif
}

HL_PRIM hl_deque *hl_deque_alloc() {
	hl_deque *q = (hl_deque*)hl_gc_alloc_finalizer(sizeof(hl_deque));
	q->free = hl_deque_free;
	q->first = NULL;
	q->last = NULL;
	hl_add_root(&q->first);
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	q->wait = CreateSemaphore(NULL,0,(1 << 30),NULL);
	InitializeCriticalSection(&q->lock);
#	else
	pthread_mutex_init(&q->lock,NULL);
	pthread_cond_init(&q->wait,NULL);
#	endif
	return q;
}

HL_PRIM void hl_deque_add( hl_deque *q, vdynamic *msg ) {
	tqueue *t = (tqueue*)hl_gc_alloc_raw(sizeof(tqueue));
	t->msg = msg;
	t->next = NULL;
	LOCK(q->lock);
	if( q->last == NULL )
		q->first = t;
	else
		q->last->next = t;
	q->last = t;
	SIGNAL(q->wait);
	UNLOCK(q->lock);
}

HL_PRIM void hl_deque_push( hl_deque *q, vdynamic *msg ) {
	tqueue *t = (tqueue*)hl_gc_alloc_raw(sizeof(tqueue));
	t->msg = msg;
	LOCK(q->lock);
	t->next = q->first;
	q->first = t;
	if( q->last == NULL )
		q->last = t;
	SIGNAL(q->wait);
	UNLOCK(q->lock);
}

HL_PRIM vdynamic *hl_deque_pop( hl_deque *q, bool block ) {
	vdynamic *msg;
	hl_blocking(true);
	LOCK(q->lock);
	while( q->first == NULL )
		if( block ) {
#			if !defined(HL_THREADS)
#			elif defined(HL_WIN)
			UNLOCK(q->lock);
			WaitForSingleObject(q->wait,INFINITE);
			LOCK(q->lock);
#			else
			pthread_cond_wait(&q->wait,&q->lock);
#			endif
		} else {
			UNLOCK(q->lock);
			hl_blocking(false);
			return NULL;
		}
	msg = q->first->msg;
	q->first = q->first->next;
	if( q->first == NULL )
		q->last = NULL;
	else
		SIGNAL(q->wait);
	UNLOCK(q->lock);
	hl_blocking(false);
	return msg;
}


#define _DEQUE _ABSTRACT(hl_deque)
DEFINE_PRIM(_DEQUE, deque_alloc, _NO_ARG);
DEFINE_PRIM(_VOID, deque_add, _DEQUE _DYN);
DEFINE_PRIM(_VOID, deque_push, _DEQUE _DYN);
DEFINE_PRIM(_DYN, deque_pop, _DEQUE _BOOL);

// ----------------- LOCK

struct _hl_lock;
typedef struct _hl_lock hl_lock;

struct _hl_lock {
	void (*free)( hl_lock * );
#if !defined(HL_THREADS)
	int counter;
#elif defined(HL_WIN)
	HANDLE wait;
#else
	pthread_mutex_t lock;
	pthread_cond_t wait;
	int counter;
#endif
};

static void hl_lock_free( hl_lock *l ) {
#	if !defined(HL_THREADS)
#	elif defined(HL_WIN)
	CloseHandle(l->wait);
#	else
	pthread_mutex_destroy(&l->lock);
	pthread_cond_destroy(&l->wait);
#	endif
}

HL_PRIM hl_lock *hl_lock_create() {
	hl_lock *l = (hl_lock*)hl_gc_alloc_finalizer(sizeof(hl_lock));
	l->free = hl_lock_free;
#	if !defined(HL_THREADS)
	l->counter = 0;
#	elif defined(HL_WIN)
	l->wait = CreateSemaphore(NULL,0,(1 << 30),NULL);
#	else
	l->counter = 0;
	pthread_mutex_init(&l->lock,NULL);
	pthread_cond_init(&l->wait,NULL);
#	endif
	return l;
}

HL_PRIM void hl_lock_release( hl_lock *l ) {
#	if !defined(HL_THREADS)
	l->counter++;
#	elif defined(HL_WIN)
	ReleaseSemaphore(l->wait,1,NULL);
#	else
	pthread_mutex_lock(&l->lock);
	l->counter++;
	pthread_cond_signal(&l->wait);
	pthread_mutex_unlock(&l->lock);
#	endif
}

HL_PRIM bool hl_lock_wait( hl_lock *l, vdynamic *timeout ) {
#	if !defined(HL_THREADS)
	if( l->counter == 0 ) return false;
	l->counter--;
	return true;
#	elif defined(HL_WIN)
	DWORD ret;
	hl_blocking(true);
	ret = WaitForSingleObject(l->wait, timeout?(DWORD)((FLOAT)timeout->v.d * 1000.0):INFINITE);
	hl_blocking(false);
	switch( ret ) {
	case WAIT_ABANDONED:
	case WAIT_OBJECT_0:
		return true;
	case WAIT_TIMEOUT:
		return false;
	default:
		hl_error("Lock wait error");
	}
#	else
	{
		hl_blocking(true);
		pthread_mutex_lock(&l->lock);
		while( l->counter == 0 ) {
			if( timeout ) {
				struct timeval tv;
				struct timespec t;
				double delta = timeout->v.d;
				int idelta = (int)delta, idelta2;
				delta -= idelta;
				delta *= 1.0e9;
				gettimeofday(&tv,NULL);
				delta += tv.tv_usec * 1000.0;
				idelta2 = (int)(delta / 1e9);
				delta -= idelta2 * 1e9;
				t.tv_sec = tv.tv_sec + idelta + idelta2;
				t.tv_nsec = (long)delta;
				if( pthread_cond_timedwait(&l->wait,&l->lock,&t) != 0 ) {
					pthread_mutex_unlock(&l->lock);
					hl_blocking(false);
					return false;
				}
			} else
				pthread_cond_wait(&l->wait,&l->lock);
		}
		l->counter--;
		if( l->counter > 0 )
			pthread_cond_signal(&l->wait);
		pthread_mutex_unlock(&l->lock);
		hl_blocking(false);
		return true;
	}
#	endif
}

#define _LOCK _ABSTRACT(hl_lock)
DEFINE_PRIM(_LOCK, lock_create, _NO_ARG);
DEFINE_PRIM(_VOID, lock_release, _LOCK);
DEFINE_PRIM(_BOOL, lock_wait, _LOCK _NULL(_F64));

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
#elif defined(HL_MAC)
	uint64_t tid64;
	pthread_threadid_np(NULL, &tid64);
	return (pid_t)tid64;
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

#ifdef HL_THREADS
static void gc_thread_entry( thread_start *_s ) {
	thread_start s = *_s;
	hl_register_thread(&s);
	hl_remove_root(&_s->param);
	free(_s);
	s.callb(s.param);
	hl_unregister_thread();
}
#endif

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
	fflush(stdout);
}

HL_PRIM hl_thread *hl_thread_create( vclosure *c ) {
	return hl_thread_start(hl_run_thread,c,true);
}

#define _THREAD _ABSTRACT(hl_thread)
DEFINE_PRIM(_THREAD, thread_current, _NO_ARG);
DEFINE_PRIM(_THREAD, thread_create, _FUN(_VOID,_NO_ARG));
