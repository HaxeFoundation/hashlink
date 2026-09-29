/*
 * Copyright (C)2015-2019 Haxe Foundation
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
#include "hlsystem.h"

#ifdef HL_LINUX
#include <semaphore.h>
#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if defined(HL_MAC)
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <dlfcn.h>
#include <objc/runtime.h>
#include <dispatch/dispatch.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#endif

#if defined(__GLIBC__)
#if __GLIBC_PREREQ(2, 30)
// tgkill is present
#else
// int tgkill(pid_t tgid, pid_t tid, int sig)
#define tgkill(tgid, tid, sig) syscall(SYS_tgkill, tgid, tid, sig)
#endif
#endif

#define MAX_STACK_SIZE (8 << 20)
#define MAX_STACK_COUNT 2048

#ifndef HL_THREADS
#	define PROFILE_BARRIER()
#elif defined(HL_VCC)
#	define PROFILE_BARRIER()	MemoryBarrier()
#else
#	define PROFILE_BARRIER()	__sync_synchronize()
#endif

HL_API double hl_sys_time( void );
int hl_module_capture_stack_walk( void *pc, void *fp, void *stack_base, void *stack_top, void *copy, void **out, int size );
uchar *hl_module_resolve_symbol_full( void *addr, uchar *out, int *outSize, int **r_debug_addr );

typedef struct _thread_handle thread_handle;
typedef struct _profile_data profile_data;

struct _thread_handle {
	int tid;
#	ifdef HL_WIN
	HANDLE h;
#	endif
	hl_thread_info *inf;
	char name[128];
	void **cached_stack;
	int cached_count;
	int cached_seq;
	bool has_cache;
	thread_handle *next;
};

struct _profile_data {
	int currentPos;
	int dataSize;
	unsigned char *data;
	profile_data *next;
};

typedef struct {
	profile_data *r;
	int pos;
} profile_reader;

static struct {
	int sample_count;
	volatile int profiling_pause;
	volatile bool stopLoop;
	volatile bool waitLoop;
	thread_handle *handles;
	thread_handle *olds;
	void **tmpMemory;
	void *stackOut[MAX_STACK_COUNT];
	profile_data *record;
	profile_data *first_record;
	hl_condition *waitCond;
} data = {0};

#ifdef HL_LINUX
static struct
{
	sem_t msg2;
	sem_t msg3;
	sem_t msg4;
	ucontext_t context;
} shared_context;

static void sigprof_handler(int sig, siginfo_t *info, void *ucontext)
{
	ucontext_t *ctx = ucontext;
	shared_context.context = *ctx;
	sem_post(&shared_context.msg2);
	sem_wait(&shared_context.msg3);
	sem_post(&shared_context.msg4);
}
#elif defined(HL_MAC)
static struct
{
	dispatch_semaphore_t msg2;
	dispatch_semaphore_t  msg3;
	dispatch_semaphore_t  msg4;
	ucontext_t context;
} shared_context;

static void sigprof_handler(int sig, siginfo_t *info, void *ucontext)
{
	ucontext_t *ctx = ucontext;
	shared_context.context = *ctx;
	dispatch_semaphore_signal(shared_context.msg2);
	dispatch_semaphore_wait(shared_context.msg3, DISPATCH_TIME_FOREVER);
	dispatch_semaphore_signal(shared_context.msg4);
}
#endif

static void *get_thread_stackptr( thread_handle *t, void **pc, void **fp ) {
	*fp = NULL;
#ifdef HL_WIN_DESKTOP
	CONTEXT c;
	c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
	if( !GetThreadContext(t->h,&c) ) return NULL;
	*pc = (void*)c.Rip;
	*fp = (void*)c.Rbp;
	return (void*)c.Rsp;
#elif defined(HL_LINUX) && defined(__x86_64__)
	*pc = (void*)shared_context.context.uc_mcontext.gregs[REG_RIP];
	*fp = (void*)shared_context.context.uc_mcontext.gregs[REG_RBP];
	return (void*)shared_context.context.uc_mcontext.gregs[REG_RSP];
#elif defined(HL_LINUX) && defined(__aarch64__)
	// Linux/Android ARM64: uc_mcontext has direct regs[] / sp / pc fields.
	*pc = (void*)shared_context.context.uc_mcontext.pc;
	*fp = (void*)shared_context.context.uc_mcontext.regs[29];
	return (void*)shared_context.context.uc_mcontext.sp;
#elif defined(HL_MAC) && defined(__x86_64__)
	struct __darwin_mcontext64 *mcontext = shared_context.context.uc_mcontext;
	if (mcontext != NULL) {
		*pc = (void*)mcontext->__ss.__rip;
		*fp = (void*)mcontext->__ss.__rbp;
		return (void*)mcontext->__ss.__rsp;
	}
	return NULL;
#elif defined(HL_MAC) && defined(__aarch64__)
	struct __darwin_mcontext64 *mcontext = shared_context.context.uc_mcontext;
	if (mcontext != NULL) {
		*pc = (void*)mcontext->__ss.__pc;
		*fp = (void*)mcontext->__ss.__fp;
		return (void*)mcontext->__ss.__sp;
	}
	return NULL;
#else
	return NULL;
#endif
}

static void thread_data_init( thread_handle *t ) {
#ifdef HL_WIN
	t->h = OpenThread(THREAD_ALL_ACCESS,FALSE, t->tid);
#endif
}

static void thread_data_free( thread_handle *t ) {
#ifdef HL_WIN
	CloseHandle(t->h);
#endif
	free(t->cached_stack);
	t->cached_stack = NULL;
	t->has_cache = false;
}

static bool pause_thread( thread_handle *t, bool b ) {
#ifdef HL_WIN
	if( b )
		return (int)SuspendThread(t->h) >= 0;
	else {
		ResumeThread(t->h);
		return true;
	}
#elif defined(HL_LINUX)
	if( b ) {
		tgkill(getpid(), t->tid, SIGPROF);
		return sem_wait(&shared_context.msg2) == 0;
	} else {
		sem_post(&shared_context.msg3);
		return sem_wait(&shared_context.msg4) == 0;
	}
#elif defined(HL_MAC)
	if( b ) {
		pthread_kill( t->inf->pthread_id, SIGPROF);
		return dispatch_semaphore_wait(shared_context.msg2, DISPATCH_TIME_FOREVER) == 0;
	} else {
		dispatch_semaphore_signal(shared_context.msg3);
		return dispatch_semaphore_wait(shared_context.msg4, DISPATCH_TIME_FOREVER) == 0;
	}
	return false;
#else
	return false;
#endif
}

static void record_data( void *ptr, int size ) {
	profile_data *r = data.record;
	if( !r || r->currentPos + size > r->dataSize ) {
		r = malloc(sizeof(profile_data));
		r->currentPos = 0;
		r->dataSize = 1 << 20;
		r->data = malloc(r->dataSize);
		r->next = NULL;
		if( data.record )
			data.record->next = r;
		else
			data.first_record = r;
		data.record = r;
		fflush(stdout);
	}
	memcpy(r->data + r->currentPos, ptr, size);
	r->currentPos += size;
}

static void record_sample( thread_handle *t, int count ) {
	int eventId = count | 0x80000000;
	double time = hl_sys_time();
	hl_threads_info *gc = hl_gc_threads_info();
	if( gc->stopping_world ) eventId |= 0x40000000;
	record_data(&time,sizeof(double));
	record_data(&t->tid,sizeof(int));
	record_data(&eventId,sizeof(int));
	record_data(data.stackOut,sizeof(void*)*count);
	if( *t->inf->thread_name && !*t->name )
		memcpy(t->name, t->inf->thread_name, sizeof(t->name));
}

static bool read_blocking_thread_locked( thread_handle *t, hl_thread_info *inf ) {
	void **stack_cur;
	int seq, count;
	if( inf->gc_blocking <= 0 ) return false;
	seq = inf->gc_ctx_seq;
	if( seq & 1 ) return false;
	if( t->has_cache && t->cached_seq == seq ) {
		memcpy(data.stackOut, t->cached_stack, t->cached_count * sizeof(void*));
		record_sample(t,t->cached_count);
		return true;
	}
	PROFILE_BARRIER();
	stack_cur = (void**)inf->stack_cur;
	if( stack_cur == NULL || (void*)stack_cur >= inf->stack_top ) return false;
	count = hl_module_capture_stack_walk(NULL, NULL, stack_cur, inf->stack_top, NULL, data.stackOut, MAX_STACK_COUNT);
	PROFILE_BARRIER();
	if( inf->gc_ctx_seq != seq || inf->gc_blocking <= 0 )
		return false;
	if( t->cached_stack == NULL )
		t->cached_stack = (void**)malloc(sizeof(void*) * MAX_STACK_COUNT);
	if( t->cached_stack ) {
		memcpy(t->cached_stack, data.stackOut, count * sizeof(void*));
		t->cached_count = count;
		t->cached_seq = seq;
		t->has_cache = true;
	}
	record_sample(t,count);
	return true;
}

// hl_unregister_thread() marks the thread as blocking, then frees its info and lets its
// stack go under the global lock : holding it keeps both alive while we read them.
static bool read_blocking_thread_data( thread_handle *t ) {
	hl_threads_info *gc = hl_gc_threads_info();
	hl_thread_info *inf = t->inf;
	bool ok = false;
	int i;
	if( inf->gc_blocking <= 0 || !hl_mutex_try_acquire(gc->global_lock) )
		return false;
	for(i=0;i<gc->count;i++)
		if( gc->threads[i] == inf ) {
			ok = read_blocking_thread_locked(t,inf);
			break;
		}
	hl_mutex_release(gc->global_lock);
	return ok;
}

static void read_thread_data( thread_handle *t ) {
	if( read_blocking_thread_data(t) )
		return;
	if( !pause_thread(t,true) )
		return;
	void *pc, *fp;
	void *stack = get_thread_stackptr(t,&pc,&fp);
	if( !stack ) {
		pause_thread(t,false);
		return;
	}

#if defined(HL_LINUX) || defined(HL_MAC)
	int count = hl_module_capture_stack_walk(pc, fp, stack, t->inf->stack_top, NULL, data.stackOut, MAX_STACK_COUNT);
	pause_thread(t, false);
#else
	int size = (int)((unsigned char*)t->inf->stack_top - (unsigned char*)stack);
	if( size < 0 ) size = 0;
	if( size > MAX_STACK_SIZE-32 ) size = MAX_STACK_SIZE-32;
#if defined(HL_WIN_DESKTOP) && defined(HL_VCC)
	// it seems we rarely can't make a first read on the thread stack, let's ignore errors and wait.
	__try {
#endif
		memcpy(data.tmpMemory,stack,size);
#if defined(HL_WIN_DESKTOP) && defined(HL_VCC)
	} __except(EXCEPTION_EXECUTE_HANDLER) {
	}
#endif
	pause_thread(t, false);
	int count = hl_module_capture_stack_walk(pc, fp, stack, (char*)stack + size, data.tmpMemory, data.stackOut, MAX_STACK_COUNT);
#endif
	record_sample(t,count);
}

static void profile_pause() {
	hl_condition_acquire(data.waitCond);
	data.profiling_pause++;
	hl_condition_broadcast(data.waitCond);
	hl_condition_release(data.waitCond);
}

static void profile_resume() {
	hl_condition_acquire(data.waitCond);
	data.profiling_pause--;
	hl_condition_broadcast(data.waitCond);
	hl_condition_release(data.waitCond);
}

static void hl_profile_loop( void *_ ) {
	double wait_time = 1. / data.sample_count;
	double next = hl_sys_time();
	data.tmpMemory = malloc(MAX_STACK_SIZE);
	data.waitLoop = false;
	while( !data.stopLoop ) {
		double t = hl_sys_time();
		hl_condition_acquire(data.waitCond);
		if( t < next || data.profiling_pause ) {
			if( !(t < next) ) next = t;
			if( data.profiling_pause ) {
				data.waitLoop = true;
				hl_condition_wait(data.waitCond);
				data.waitLoop = false;
			}
			hl_condition_release(data.waitCond);
			continue;
		}
		hl_condition_release(data.waitCond);
		hl_threads_info *threads = hl_gc_threads_info();
		int i;
		thread_handle *prev = NULL;
		thread_handle *cur = data.handles;
		for(i=0;i<threads->count;i++) {
			hl_thread_info *t = threads->threads[i];
			if( t->flags & HL_THREAD_INVISIBLE ) continue;

			if( !cur || cur->tid != t->thread_id ) {
				// have we lost a thread ?
				thread_handle *h = cur;
				thread_handle *hprev = prev;
				while( h ) {
					if( h->tid == t->thread_id ) {
						// remove from previous queue
						if( hprev ) {
							hprev->next = h->next;
						} else {
							data.handles = h->next;
						}
						// insert at current position
						if( prev ) {
							h->next = prev->next;
							prev->next = h;
						} else {
							h->next = data.handles;
							data.handles = h;
						}
						cur = h;
						break;
					}
					hprev = h;
					h = h->next;
				}
				if( !h ) {
					h = malloc(sizeof(thread_handle));
					memset(h,0,sizeof(thread_handle));
					h->tid = t->thread_id;
					h->inf = t;
					thread_data_init(h);
					h->next = cur;
					cur = h;
					if( prev == NULL ) data.handles = h; else prev->next = h;
				}
			}
			if( cur->inf != t ) {
				thread_data_free(cur);
				memset(cur->name, 0, sizeof(cur->name));
				cur->inf = t;
				thread_data_init(cur);
			}
			if( (t->flags & HL_THREAD_PROFILER_PAUSED) == 0 )
				read_thread_data(cur);
			prev = cur;
			cur = cur->next;
		}
		if( prev ) prev->next = NULL; else data.handles = NULL;
		while( cur != NULL ) {
			thread_handle *n;
			thread_data_free(cur);
			n = cur->next;
			if( *cur->name ) {
				cur->next = data.olds;
				data.olds = cur;
			} else
				free(cur);
			cur = n;
		}
		next += wait_time;
	}
	free(data.tmpMemory);
	data.tmpMemory = NULL;
	data.sample_count = 0;
	data.stopLoop = false;
}

static void profile_event( int code, vbyte *data, int dataLen );

void hl_profile_setup( int sample_count ) {
#	if defined(HL_THREADS) && (defined(HL_WIN_DESKTOP) || defined(HL_LINUX) || defined (HL_MAC))
	if( data.waitCond == NULL ) {
		data.waitCond = hl_condition_alloc();
		hl_add_root(&data.waitCond);
	}
	hl_setup.profile_event = profile_event;
	hl_setup.before_exit = hl_profile_end;
	if( data.sample_count ) return;
	if( sample_count < 0 ) {
		// was not started with --profile : pause until we get start event
		profile_pause();
		return;
	}
	data.sample_count = sample_count;
#	ifdef HL_LINUX
	sem_init(&shared_context.msg2, 0, 0);
	sem_init(&shared_context.msg3, 0, 0);
	sem_init(&shared_context.msg4, 0, 0);
	struct sigaction action = {0};
	action.sa_sigaction = sigprof_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGPROF, &action, NULL);
#	elif defined(HL_MAC)
	shared_context.context.uc_mcontext = NULL;
	shared_context.msg2 = dispatch_semaphore_create(0);
	shared_context.msg3 = dispatch_semaphore_create(0);
	shared_context.msg4 = dispatch_semaphore_create(0);
	struct sigaction action = {0};
	action.sa_sigaction = sigprof_handler;
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	sigaction(SIGPROF, &action, NULL);
#	endif
	hl_thread_start(hl_profile_loop,NULL,false);
#	endif
}

static bool read_profile_data( profile_reader *r, void *ptr, int size ) {
	while( size ) {
		if( r->r == NULL ) return false;
		int bytes = r->r->currentPos - r->pos;
		if( bytes > size ) bytes = size;
		if( ptr ) memcpy(ptr, r->r->data + r->pos, bytes);
		size -= bytes;
		r->pos += bytes;
		if( r->pos == r->r->currentPos ) {
			r->r = r->r->next;
			r->pos = 0;
		}
	}
	return true;
}

static int write_names( thread_handle *h, FILE *f ) {
	int count = 0;
	while( h ) {
		if( *h->name ) {
			if( f ) {
				int len = (int)strlen(h->name);
				fwrite(&h->tid,1,4,f);
				fwrite(&len,1,4,f);
				fwrite(h->name,1,len,f);
			} else
				count++;
		}
		h = h->next;
	}
	return count;
}

static void profile_dump( vbyte* ptr ) {
	if( !data.first_record ) return;

	profile_pause();
	printf("Writing profiling data...\n");
	fflush(stdout);

	char* filename = ptr == NULL ? "hlprofile.dump" : hl_to_utf8((uchar*)ptr);
	FILE *f = fopen(filename,"wb");
	if( f == NULL ) {
		profile_resume();
		printf("Failed to open file %s, 0 profile samples saved\n", filename);
		hl_error("Failed to open file");
		return;
	}
	int version = HL_VERSION;
	fwrite("PROF",1,4,f);
	fwrite(&version,1,4,f);
	fwrite(&data.sample_count,1,4,f);
	profile_reader r;
	r.r = data.first_record;
	r.pos = 0;
	int samples = 0;
	while( true ) {
		double time;
		int i, tid, eventId;
		if( !read_profile_data(&r,&time, sizeof(double)) ) break;
		read_profile_data(&r,&tid,sizeof(int));
		read_profile_data(&r,&eventId,sizeof(int));
		fwrite(&time,1,8,f);
		fwrite(&tid,1,4,f);
		fwrite(&eventId,1,4,f);
		if( eventId < 0 ) {
			int count = eventId & 0x3FFFFFFF;
			read_profile_data(&r,data.stackOut,sizeof(void*)*count);
			for(i=0;i<count;i++) {
				uchar outStr[256];
				int outSize = 256;
				int *debug_addr = NULL;
				hl_module_resolve_symbol_full(data.stackOut[i],outStr,&outSize,&debug_addr);
				if( debug_addr == NULL ) {
					int bad = -1;
					fwrite(&bad,1,4,f);
				} else {
					fwrite(debug_addr,1,8,f);
					if( (debug_addr[0] & 0x80000000) == 0 ) {
						debug_addr[0] |= 0x80000000;
						fwrite(&outSize,1,4,f);
						fwrite(outStr,1,outSize*sizeof(uchar),f);
					}
				}
			}
			samples++;
		} else {
			int size;
			read_profile_data(&r,&size, sizeof(int));
			fwrite(&size,1,4,f);
			while( size ) {
				int k = size > MAX_STACK_SIZE ? MAX_STACK_SIZE : size;
				read_profile_data(&r,data.tmpMemory,k);
				fwrite(data.tmpMemory,1,k,f);
				size -= k;
			}
		}
	}
	double tend = -1;
	fwrite(&tend,1,8,f);

	// reset debug_addr flags (allow further dumps)
	r.r = data.first_record;
	r.pos = 0;
	while( true ) {
		int i, eventId;
		if( !read_profile_data(&r,NULL, sizeof(double) + sizeof(int)) ) break;
		read_profile_data(&r,&eventId,sizeof(int));
		if( eventId < 0 ) {
			int count = eventId & 0x3FFFFFFF;
			read_profile_data(&r,data.stackOut,sizeof(void*)*count);
			for(i=0;i<count;i++) {
				int *debug_addr = NULL;
				hl_module_resolve_symbol_full(data.stackOut[i],NULL,NULL,&debug_addr);
				if( debug_addr )
					debug_addr[0] &= 0x7FFFFFFF;
			}
		} else {
			int size;
			read_profile_data(&r,&size,sizeof(int));
			read_profile_data(&r,NULL,size);
		}
	}
	// dump threads names
	int names_count = write_names(data.handles,NULL) + write_names(data.olds,NULL);
	fwrite(&names_count,1,4,f);
	write_names(data.handles,f);
	write_names(data.olds,f);
	// done
	fclose(f);
	printf("%d profile samples saved to %s\n", samples, filename);
	profile_resume();
}

void hl_profile_end() {
	profile_dump(NULL);
	if( !data.sample_count ) return;
	data.stopLoop = true;
	hl_condition_acquire(data.waitCond);
	data.profiling_pause = 0;
	hl_condition_broadcast(data.waitCond);
	hl_condition_release(data.waitCond);
	while( data.stopLoop ) {};
}

static void profile_event( int code, vbyte *ptr, int dataLen ) {
	switch( code ) {
	case -1:
		hl_get_thread()->flags |= HL_THREAD_PROFILER_PAUSED;
		break;
	case -2:
		hl_get_thread()->flags &= ~HL_THREAD_PROFILER_PAUSED;
		break;
	case -3:
		profile_pause();
		while( !data.waitLoop ) {}
		profile_data *d = data.first_record;
		while( d ) {
			profile_data *n = d->next;
			free(d->data);
			free(d);
			d = n;
		}
		data.first_record = NULL;
		data.record = NULL;
		profile_resume();
		break;
	case -4:
		profile_pause();
		break;
	case -5:
		profile_resume();
		break;
	case -6:
		profile_dump(ptr);
		break;
	case -7:
		{
			uchar *end = NULL;
			hl_profile_setup( ptr ? utoi((uchar*)ptr,&end) : 1000);
		}
		break;
	case -8:
		hl_get_thread()->flags |= HL_THREAD_INVISIBLE;
		break;
	default:
		if( code < 0 ) return;
		if( data.profiling_pause || (code != 0 && (hl_get_thread()->flags & HL_THREAD_PROFILER_PAUSED)) ) return;
		profile_pause();
		while( !data.waitLoop ) {}
		double time = hl_sys_time();
		record_data(&time,sizeof(double));
		record_data(&hl_get_thread()->thread_id,sizeof(int));
		record_data(&code,sizeof(int));
		record_data(&dataLen,sizeof(int));
		record_data(ptr,dataLen);
		profile_resume();
		break;
	}
}
