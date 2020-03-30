#include "hl.h"

#ifdef HL_WIN
#	include <windows.h>
#else
#	include <sys/types.h>
#	include <sys/mman.h>
#endif

struct hl_alloc_block {
	int size;
	hl_alloc_block *next;
	unsigned char *p;
};

void hl_alloc_init( hl_alloc *a ) {
	a->cur = NULL;
}

void *hl_malloc( hl_alloc *a, int size ) {
	hl_alloc_block *b = a->cur;
	void *p;
	if( !size ) return NULL;
	size += hl_pad_size(size,&hlt_dyn);
	if( b == NULL || b->size <= size ) {
		int alloc = size < 4096-sizeof(hl_alloc_block) ? 4096-sizeof(hl_alloc_block) : size;
		b = (hl_alloc_block *)malloc(sizeof(hl_alloc_block) + alloc);
		if( b == NULL ) hl_fatal("Out of memory (malloc)");
		b->p = ((unsigned char*)b) + sizeof(hl_alloc_block);
		b->size = alloc;
		b->next = a->cur;
		a->cur = b;
	}
	p = b->p;
	b->p += size;
	b->size -= size;
	return p;
}

void *hl_zalloc( hl_alloc *a, int size ) {
	void *p = hl_malloc(a,size);
	if( p ) memset(p, 0, size);
	return p;
}

void hl_free( hl_alloc *a ) {
	hl_alloc_block *b = a->cur;
	int_val prev = 0;
	int size = 0;
	while( b ) {
		hl_alloc_block *n = b->next;
		size = (int)(b->p + b->size - ((unsigned char*)b));
		prev = (int_val)b;
		free(b);
		b = n;
	}
	// check if our allocator was not part of the last free block
	if( (int_val)a < prev || (int_val)a > prev+size )
		a->cur = NULL;
}

HL_PRIM void *hl_alloc_executable_memory( int size ) {
#ifdef __APPLE__
#  	ifndef MAP_ANONYMOUS
#     		define MAP_ANONYMOUS MAP_ANON
#       endif
#endif
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

HL_PRIM void hl_free_executable_memory( void *c, int size ) {
#if defined(HL_WIN)
	VirtualFree(c,0,MEM_RELEASE);
#elif !defined(HL_CONSOLE)
	munmap(c, size);
#endif
}
