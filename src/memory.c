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

/*
 * OS-backed memory allocation.
 */

#include "hl.h"

#define GC_PAGE_BITS	16
#define GC_PAGE_SIZE	(1 << GC_PAGE_BITS)

#ifdef HL_WIN
#	include <windows.h>
#	include <intrin.h>
#else
#	include <sys/types.h>
#	include <sys/mman.h>
#endif

#ifdef __APPLE__
#	ifndef MAP_ANONYMOUS
#		define MAP_ANONYMOUS MAP_ANON
#	endif
#endif

void *hl_alloc_executable_memory( int size ) {
#if defined(HL_WIN) && defined(HL_64)
	static char *jit_address = (char*)0x000076CA9F000000;
	void *ptr;
retry_jit_alloc:
	ptr = VirtualAlloc(jit_address,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	if( !ptr ) {
		jit_address = (char*)(((int_val)jit_address)>>1); // fix for Win7 - will eventually reach NULL
		goto retry_jit_alloc;
	}
	jit_address += size + ((-size) & (GC_PAGE_SIZE - 1));
	return ptr;
#elif defined(HL_WIN)
	void *ptr = VirtualAlloc(NULL,size,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	return ptr;
#elif defined(HL_CONSOLE)
	return NULL;
#else
	void *p;
	p = mmap(NULL,size,PROT_READ|PROT_WRITE|PROT_EXEC,(MAP_PRIVATE|MAP_ANONYMOUS),-1,0);
	return p;
#endif
}

void hl_free_executable_memory( void *c, int size ) {
#if defined(HL_WIN)
	VirtualFree(c,0,MEM_RELEASE);
#elif !defined(HL_CONSOLE)
	munmap(c, size);
#endif
}

#if defined(HL_CONSOLE)
void *sys_alloc_align( int size, int align );
void sys_free_align( void *ptr, int size );
#elif !defined(HL_WIN)
static void *base_addr = (void*)0x40000000;
#endif

void *gc_alloc_page_memory( int size ) {
#if defined(HL_WIN)
#	if defined(GC_DEBUG) && defined(HL_64)
#		define STATIC_ADDRESS
#	endif
#	ifdef STATIC_ADDRESS
	// force out of 32 bits addresses to check loss of precision
	static char *start_address = (char*)0x100000000;
#	else
	static void *start_address = NULL;
#	endif
	void *ptr = VirtualAlloc(start_address,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#	ifdef STATIC_ADDRESS
	if( ptr == NULL && start_address ) {
		start_address = NULL;
		return gc_alloc_page_memory(size);
	}
	start_address += size + ((-size) & (GC_PAGE_SIZE - 1));
#	endif
	return ptr;
#elif defined(HL_CONSOLE)
	return sys_alloc_align(size, GC_PAGE_SIZE);
#else
	/*
	int i = 0;
	while( gc_will_collide(base_addr,size) ) {
		base_addr = (char*)base_addr + GC_PAGE_SIZE;
		i++;
		// most likely our hashing creates too many collisions
		if( i >= 1 << (GC_LEVEL0_BITS + GC_LEVEL1_BITS + 2) )
			return NULL;
	}
	*/
	void *ptr = mmap(base_addr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
	if( ptr == (void*)-1 )
		return NULL;
	if( ((int_val)ptr) & (GC_PAGE_SIZE-1) ) {
		munmap(ptr,size);
		void *tmp;
		int tmp_size = (int)((int_val)ptr - (int_val)base_addr);
		if( tmp_size > 0 ) {
			base_addr = (void*)((((int_val)ptr) & ~(GC_PAGE_SIZE-1)) + GC_PAGE_SIZE);
			tmp = ptr;
		} else {
			base_addr = (void*)(((int_val)ptr) & ~(GC_PAGE_SIZE-1));
			tmp = NULL;
		}
		if( tmp ) tmp = mmap(tmp,tmp_size,PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
		ptr = gc_alloc_page_memory(size);
		if( tmp ) munmap(tmp,tmp_size);
		return ptr;
	}
	base_addr = (char*)ptr+size;
	return ptr;
#endif
}

void gc_free_page_memory( void *ptr, int size ) {
#ifdef HL_WIN
	VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(HL_CONSOLE)
	sys_free_align(ptr,size);
#else
	munmap(ptr,size);
#endif
}
