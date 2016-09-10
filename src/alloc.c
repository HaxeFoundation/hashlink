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
#include "hl.h"
#ifdef HL_WIN
#	include <windows.h>
#	include <intrin.h>
static unsigned int __inline TRAILING_ONES( unsigned int x ) {
	DWORD msb = 0;
	if( _BitScanForward( &msb, ~x ) )
		return msb;
	return 32;
}
static unsigned int __inline TRAILING_ZEROES( unsigned int x ) {
	DWORD msb = 0;
	if( _BitScanForward( &msb, x ) )
		return msb;
	return 32;
}
#else
#	include <sys/types.h>
#	include <sys/mman.h>
#	define TRAILING_ONES(x)		(~(x)?__builtin_ctz(~(x)):0)
#	define TRAILING_ZEROES(x)	(x?__builtin_ctz(x):32)
#endif
#	define MZERO(ptr,size)		memset(ptr,0,size)	

// GC

#define	GC_ALIGN_BITS	2
#define	GC_ALIGN		(1 << GC_ALIGN_BITS)
#define GC_ALL_PAGES	(GC_PARTITIONS << PAGE_KIND_BITS)

#define GC_PAGE_BITS	16
#define GC_PAGE_SIZE	(1 << GC_PAGE_BITS)

#ifndef HL_64
#	define GC_MASK_BITS			16
#	define GC_LEVEL0_BITS		8
#	define GC_LEVEL1_BITS		8
#	define GC_GET_LEVEL1(ptr)	hl_gc_page_map[((unsigned int)(ptr))>>(GC_MASK_BITS+GC_LEVEL1_BITS)]
#	define GC_GET_PAGE(ptr)		GC_GET_LEVEL1(ptr)[(((unsigned int)(ptr))>>GC_MASK_BITS)&GC_LEVEL1_MASK]
#endif
#define GC_LEVEL1_MASK		((1 << GC_LEVEL1_BITS) - 1)

#define PAGE_KIND_BITS		2
#define PAGE_KIND_MASK		((1 << PAGE_KIND_BITS) - 1)

#ifdef HL_DEBUG
//#	define HL_TRACK_ALLOC
#endif

typedef struct _gc_pheader gc_pheader;

struct _gc_pheader {
	// const
	int page_size;
	int page_kind;
	int block_size;
	int max_blocks;
	int first_block;
	// mutable
	int next_block;
	int free_blocks;
	unsigned char *sizes;
	unsigned char *bmp;
	gc_pheader *next_page;
#ifdef HL_TRACK_ALLOC
	int *alloc_hashes;
#endif
#ifdef HL_DEBUG
	int page_id;
#endif
};

#define GC_PARTITIONS	9
#define GC_PART_BITS	4
#define GC_FIXED_PARTS	5
static const int GC_SBITS[GC_PARTITIONS] = {0,0,0,0,0,		3,6,14,22};
static const int GC_SIZES[GC_PARTITIONS] = {4,8,12,16,20,	8,64,1<<14,1<<22};

static gc_pheader *gc_pages[GC_ALL_PAGES] = {NULL};
static gc_pheader *gc_free_pages[GC_ALL_PAGES] = {NULL};
static gc_pheader *gc_level1_null[1<<GC_LEVEL1_BITS] = {NULL};
HL_PRIM gc_pheader **hl_gc_page_map[1<<GC_LEVEL0_BITS] = {NULL};

static struct { 
	int64 total_requested;
	int64 total_allocated;
	int64 last_mark;
	int64 pages_total_memory;
	int64 debug;
	int64 allocation_count;
	int pages_count;
	int pages_allocated;
	int mark_bytes;
} gc_stats = {0};

// -------------------------  ROOTS ----------------------------------------------------------

static void ***gc_roots = NULL;
static int gc_roots_count = 0;
static int gc_roots_max = 0;

HL_PRIM void hl_add_root( void **r ) {
	if( gc_roots_count == gc_roots_max ) {
		int nroots = gc_roots_max ? (gc_roots_max << 1) : 16;
		void ***roots = (void***)malloc(sizeof(void*)*nroots);
		memcpy(roots,gc_roots,sizeof(void*)*gc_roots_count);
		free(gc_roots);
		gc_roots = roots;
		gc_roots_max = nroots;
	}
	gc_roots[gc_roots_count++] = r;
}

HL_PRIM void hl_remove_root( void **v ) {
	int i;
	for(i=0;i<gc_roots_count;i++)
		if( gc_roots[i] == v ) {
			gc_roots_count--;
			memcpy(gc_roots + i, gc_roots + (i+1), gc_roots_count - i);
			break;
		}
}

// -------------------------  ALLOCATOR ----------------------------------------------------------

static void *gc_alloc_page_memory( int size );
static void gc_free_page_memory( void *ptr, int size );

static bool is_zero( void *ptr, int size ) {
	static char ZEROMEM[256] = {0};
	unsigned char *p = (unsigned char*)ptr;
	while( size>>8 ) {
		if( memcmp(p,ZEROMEM,256) ) return false;
		p += 256;
		size -= 256;
	}
	return memcmp(p,ZEROMEM,size) == 0;
}

static void gc_flush_empty_pages() {
	int i;
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *p = gc_pages[i];
		gc_pheader *prev = NULL;
		while( p ) {
			gc_pheader *next = p->next_page;
			if( p->bmp && is_zero(p->bmp+(p->first_block>>3),((p->max_blocks+7)>>3) - (p->first_block>>3)) ) {
				int j;
				gc_stats.pages_count--;
				gc_stats.pages_total_memory -= p->page_size;
				gc_stats.mark_bytes -= (p->max_blocks + 7) >> 3;
				if( prev )
					prev->next_page = next;
				else
					gc_pages[i] = next;
				if( gc_free_pages[i] == p )
					gc_free_pages[i] = next;
				for(j=0;j<p->page_size>>GC_MASK_BITS;j++) {
					void *ptr = (unsigned char*)p + (j<<GC_MASK_BITS);
					GC_GET_PAGE(ptr) = NULL;
				}
				gc_free_page_memory(p,p->page_size);
			} else
				prev = p;
			p = next;
		}
	}
}

static int PAGE_ID = 0;

static gc_pheader *gc_alloc_new_page( int block, int size, bool varsize ) {
	int m, i;
	unsigned char *base;
	gc_pheader *p;
	int start_pos;
	base = (unsigned char*)gc_alloc_page_memory(size);
	p = (gc_pheader*)base;
	if( !base ) {
#		ifdef HL_DEBUG
		hl_gc_dump();
#		endif
		hl_fatal("Out of memory");
	}
#	ifdef HL_DEBUG
	memset(base,0xDD,size);
	p->page_id = PAGE_ID++;
#	endif
	if( ((int_val)base) & ((1<<GC_MASK_BITS) - 1) )
		hl_fatal("Page memory is not correctly aligned");
	p->page_size = size;
	p->block_size = block;
	p->max_blocks = size / block;
	p->sizes = NULL;
	p->bmp = NULL;
	start_pos = sizeof(gc_pheader);
	if( varsize ) {
		p->sizes = base + start_pos;
		start_pos += p->max_blocks;
		MZERO(p->sizes,p->max_blocks);
	}
#	ifdef HL_TRACK_ALLOC
	p->alloc_hashes = (int*)malloc(sizeof(int) * p->max_blocks);
	MZERO(p->alloc_hashes,p->max_blocks * sizeof(int));
#	endif
	m = start_pos % block;
	if( m ) start_pos += block - m;
	p->first_block = start_pos / block;
	p->next_block = p->first_block;
	p->free_blocks = p->max_blocks - p->first_block;
	// update stats
	gc_stats.pages_count++;
	gc_stats.pages_allocated++;
	gc_stats.pages_total_memory += size;
	gc_stats.mark_bytes += (p->max_blocks + 7) >> 3;

	// register page in page map
	for(i=0;i<size>>GC_MASK_BITS;i++) {
		void *ptr = (unsigned char*)p + (i<<GC_MASK_BITS);
		if( GC_GET_LEVEL1(ptr) == gc_level1_null ) {
			gc_pheader **level = (gc_pheader**)malloc(sizeof(void*) * (1<<GC_LEVEL1_BITS));
			MZERO(level,sizeof(void*) * (1<<GC_LEVEL1_BITS));
			GC_GET_LEVEL1(ptr) = level;
		}
		GC_GET_PAGE(ptr) = p;
	}
	return p;
}

static void *gc_alloc_fixed( int part, int kind ) {
	int pid = (part << PAGE_KIND_BITS) | kind;
	gc_pheader *p = gc_free_pages[pid];
	unsigned char *ptr;
	while( p ) {
		if( p->bmp ) {
			int next = p->next_block;
			while( true ) {
				unsigned int fetch_bits = ((unsigned int*)p->bmp)[next >> 5];
				int ones = TRAILING_ONES(fetch_bits >> (next&31));
				next += ones;
				if( (next&31) == 0 && ones ) {
					if( next >= p->max_blocks ) {
						p->next_block = next;
						break;
					}
					continue;
				}
				p->next_block = next;
				if( next >= p->max_blocks )
					break;
				goto alloc_fixed;
			}
		} else if( p->next_block < p->max_blocks )
			break;
		p = p->next_page;
		gc_free_pages[pid] = p;
	}
	if( p == NULL ) {
		p = gc_alloc_new_page(GC_SIZES[part], GC_PAGE_SIZE, false);
		p->page_kind = kind;
		p->next_page = gc_pages[pid];
		gc_pages[pid] = p;
		gc_free_pages[pid] = p;
	}
alloc_fixed:
	ptr = (unsigned char*)p + p->next_block * p->block_size;
#	ifdef HL_DEBUG
	{
		int i;
		if( p->next_block < p->first_block || p->next_block >= p->max_blocks )
			hl_fatal("assert");
		if( p->bmp && (p->bmp[p->next_block>>3]&(1<<(p->next_block&7))) != 0 )
			hl_fatal("Alloc on marked bit");
		for(i=0;i<p->block_size;i++)
			if( ptr[i] != 0xDD )
				hl_fatal("assert");
	}
#	endif
#	ifdef HL_TRACK_ALLOC
	p->alloc_hashes[p->next_block] = hl_get_stack_hash();
#	endif
	p->next_block++;
	gc_stats.total_allocated += p->block_size;
	return ptr;
}

static void *gc_alloc_var( int part, int size, int kind ) {
	int pid = (part << PAGE_KIND_BITS) | kind;
	gc_pheader *p = gc_pages[pid];
	unsigned char *ptr;
	int nblocks = size >> GC_SBITS[part];
loop:
	while( p ) {
		if( p->bmp ) {
			int next, avail = 0;
			if( p->free_blocks >= nblocks ) {
				p->next_block = p->first_block;
				p->free_blocks = 0;
			}
			next = p->next_block;
			if( next + nblocks > p->max_blocks ) {
				gc_stats.debug++;
				p = p->next_page;
				continue;
			}
			while( true ) {
				int fid = next >> 5;
				unsigned int fetch_bits = ((unsigned int*)p->bmp)[fid];
				int bits;
resume:
				bits = TRAILING_ONES(fetch_bits >> (next&31));
				if( bits ) {
					if( avail > p->free_blocks ) p->free_blocks = avail;
					avail = 0;
					next += bits - 1;
#					ifdef HL_DEBUG
					if( p->sizes[next] == 0 ) hl_fatal("assert");
#					endif
					next += p->sizes[next];
					if( (next>>5) != fid ) {
						if( next + nblocks > p->max_blocks ) {
							p->next_block = next;
							p = p->next_page;
							goto loop;
						}
						continue;
					}
					goto resume;
				}
				bits = TRAILING_ZEROES( (next & 31) ? (fetch_bits >> (next&31)) | (1<<(32-(next&31))) : fetch_bits );
				avail += bits;
				next += bits;
				if( next > p->max_blocks ) {
					avail -= next - p->max_blocks;
					next = p->max_blocks;
					if( avail < nblocks ) break;
				}
				if( avail >= nblocks ) {
					p->next_block = next - avail;
					goto alloc_var;
				}
				if( next & 31 ) goto resume;
			}
			if( avail > p->free_blocks ) p->free_blocks = avail;
			p->next_block = next;
		} else if( p->next_block + nblocks <= p->max_blocks )
			break;
		p = p->next_page;
	}
	if( p == NULL ) {
		int psize = GC_PAGE_SIZE;
		while( psize < size + 1024 )
			psize <<= 1;
		p = gc_alloc_new_page(GC_SIZES[part], psize, true);
		p->page_kind = kind;
		p->next_page = gc_pages[pid];
		gc_pages[pid] = p;
	}
alloc_var:
	ptr = (unsigned char*)p + p->next_block * p->block_size;
#	ifdef HL_DEBUG
	{
		int i;
		if( p->next_block < p->first_block || p->next_block + nblocks > p->max_blocks )
			hl_fatal("assert");
		for(i=0;i<size;i++)
			if( ptr[i] != 0xDD )
				hl_fatal("assert");
	}
#	endif
	if( p->bmp ) {
		int i;
		int bid = p->next_block;
		for(i=0;i<nblocks;i++) {
#			ifdef HL_DEBUG
			if( (p->bmp[bid>>3]&(1<<(bid&7))) != 0 ) hl_fatal("Alloc on marked block");
#			endif
			p->bmp[bid>>3] &= ~(1<<(bid&7));
			bid++;
		}
		bid = p->next_block;
		p->bmp[bid>>3] |= 1<<(bid&7);
	}
#	ifdef HL_TRACK_ALLOC
	p->alloc_hashes[p->next_block] = hl_get_stack_hash();
#	endif
	if( nblocks > 1 ) MZERO(p->sizes + p->next_block, nblocks);
	p->sizes[p->next_block] = (unsigned char)nblocks;
	p->next_block += nblocks;
	gc_stats.total_allocated += size + 1;
	return ptr;
}

static void *gc_alloc_gen( int size, int flags ) {
	int m = size & (GC_ALIGN - 1);
	int p;
	gc_stats.allocation_count++;
	gc_stats.total_requested += size;
	if( m ) size += GC_ALIGN - m;
	if( size <= 0 )
		return NULL;
	if( size <= GC_SIZES[GC_FIXED_PARTS-1] && (flags & MEM_ALIGN_DOUBLE) == 0 )
		return gc_alloc_fixed( (size >> GC_ALIGN_BITS) - 1, flags & PAGE_KIND_MASK);
	for(p=GC_FIXED_PARTS;p<GC_PARTITIONS;p++) {
		int block = GC_SIZES[p];
		int m = size & (block - 1);
		if( m ) size += block - m;
		if( size < block * 255 )
			return gc_alloc_var(p, size, flags & PAGE_KIND_MASK);
	}
	hl_error("Required memory allocation too big");
	return NULL;
}

static void gc_check_mark();

//#define HL_BUMP_ALLOC

#ifdef HL_BUMP_ALLOC
static unsigned char *alloc_all = NULL;
static unsigned char *alloc_end = NULL;
#endif

void *hl_gc_alloc_gen( int size, int flags ) {
	void *ptr;
#ifdef HL_BUMP_ALLOC
	if( !alloc_all ) {
		int tot = 3<<29;
		alloc_all = gc_alloc_page_memory(tot);
		if( !alloc_all ) hl_fatal("Failed to allocate bump memory");
		alloc_end = alloc_all + tot;
	}
	ptr = alloc_all;
	alloc_all += size;
	if( alloc_all > alloc_end ) hl_fatal("Out of memory");
#else
	gc_check_mark();
	ptr = gc_alloc_gen(size, flags);
#	ifdef HL_DEBUG
	memset(ptr,0xCD,size);
#	endif
#endif
	if( flags & MEM_ZERO ) memset(ptr,0,size);
	return ptr;
}

// -------------------------  MARKING ----------------------------------------------------------

static float gc_mark_threshold = 0.5;
static void *gc_stack_top = NULL;
static int mark_size = 0;
static unsigned char *mark_data = NULL;
static void **cur_mark_stack = NULL;
static void **mark_stack_end = NULL;
static int mark_stack_size = 0;

#define GC_PUSH_GEN(ptr,page,bid) \
	if( MEM_HAS_PTR((page)->page_kind) ) { \
		if( mark_stack == mark_stack_end ) mark_stack = hl_gc_mark_grow(mark_stack); \
		*mark_stack++ = ptr; \
		*mark_stack++ = ((unsigned char*)page) + bid; \
	}

HL_PRIM void **hl_gc_mark_grow( void **stack ) {
	int nsize = mark_stack_size ? (((mark_stack_size * 3) >> 1) & ~1) : 256;
	void **nstack = (void**)malloc(sizeof(void**) * nsize);
	void **base_stack = mark_stack_end - mark_stack_size;
	cur_mark_stack = stack;
	memcpy(nstack, base_stack, (unsigned char*)cur_mark_stack - (unsigned char*)base_stack);
	free(base_stack);
	mark_stack_size = nsize;
	mark_stack_end = nstack + nsize;
	cur_mark_stack = nstack + (cur_mark_stack - base_stack);
	if( base_stack == NULL ) {
		*cur_mark_stack++ = 0;
		*cur_mark_stack++ = 0;
	}
	return cur_mark_stack;
}

static void gc_flush_mark() {
	register void **mark_stack = cur_mark_stack;
	while( true ) {
		unsigned char *page_bid = *--mark_stack;
		void **block = (void**)*--mark_stack;
		gc_pheader *page = (gc_pheader*)((int_val)page_bid & ~(GC_PAGE_SIZE - 1));
		int bid = ((int)(int_val)page_bid) & (GC_PAGE_SIZE - 1);
		int size, nwords;
		if( !block ) {
			mark_stack += 2;
			break;
		}
		size = page->sizes ? page->sizes[bid] * page->block_size : page->block_size;
		nwords = size / HL_WSIZE;
		while( nwords-- ) {
			void *p = *block++;
			page = GC_GET_PAGE(p);
			if( !page || ((((unsigned char*)p - (unsigned char*)page))%page->block_size) != 0 ) continue;
			bid = ((unsigned char*)p - (unsigned char*)page) / page->block_size;
			if( page->sizes && page->sizes[bid] == 0 ) continue;
			if( (page->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
				page->bmp[bid>>3] |= 1<<(bid&7);
				GC_PUSH_GEN(p,page,bid);
			}
		}
	}
	cur_mark_stack = mark_stack;
}

#ifdef HL_DEBUG
static void gc_clear_unmarked_mem() {
	int i;
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *p = gc_pages[i];
		while( p ) {
			int bid;
			for(bid=p->first_block;bid<p->max_blocks;bid++) {
				if( p->sizes && !p->sizes[bid] ) continue;
				if( (p->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
					int size = p->sizes ? p->sizes[bid] * p->block_size : p->block_size;
					unsigned char *ptr = (unsigned char*)p + bid * p->block_size;
					if( bid * p->block_size + size > p->page_size ) hl_fatal("invalid block size"); 
					memset(ptr,0xDD,size);
					if( p->sizes ) p->sizes[bid] = 0;
				}
			}
			p = p->next_page;
		}
	}
}
#endif

static void gc_mark() {
	jmp_buf regs;
	void **stack_head;
	void **stack_top = (void**)gc_stack_top;
	void **mark_stack = cur_mark_stack;
	int mark_bytes = gc_stats.mark_bytes;
	int pid, i;
	unsigned char *mark_cur;
	// save registers
	setjmp(regs);
	// prepare mark bits
	if( mark_bytes > mark_size ) {
		gc_free_page_memory(mark_data, mark_size);
		if( mark_size == 0 ) mark_size = GC_PAGE_SIZE;
		while( mark_size < mark_bytes )
			mark_size <<= 1;
		mark_data = gc_alloc_page_memory(mark_size);
	}
	mark_cur = mark_data;
	MZERO(mark_data,mark_bytes);
	for(pid=0;pid<GC_ALL_PAGES;pid++) {
		gc_pheader *p = gc_pages[pid];
		gc_free_pages[pid] = p;
		while( p ) {
			p->bmp = mark_cur;
			p->next_block = p->first_block;
			p->free_blocks = 0;
			mark_cur += (p->max_blocks + 7) >> 3;
			p = p->next_page;
		}
	}
	// push roots
	for(i=0;i<gc_roots_count;i++) {
		void *p = *gc_roots[i];
		gc_pheader *page;
		if( !p ) continue;
		page = GC_GET_PAGE(p);
		// don't check if valid ptr : it's a manual added root, so should be valid
		int bid = ((unsigned char*)p - (unsigned char*)page) / page->block_size;
		if( (page->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
			page->bmp[bid>>3] |= 1<<(bid&7);
			GC_PUSH_GEN(p,page,bid);
		}
	}
	// scan stack
	stack_head = (void**)&stack_head;
	if( stack_head > (void**)&regs ) stack_head = (void**)&regs; // fix for compilers that might inverse variables
	while( stack_head <= stack_top ) {
		void *p = *stack_head++;
		gc_pheader *page = GC_GET_PAGE(p);
		int bid;
		if( !page || (((unsigned char*)p - (unsigned char*)page)%page->block_size) != 0 ) continue;
		bid = ((unsigned char*)p - (unsigned char*)page) / page->block_size;
		if( page->sizes && !page->sizes[bid] ) continue; // inner pointer
		if( (page->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
			page->bmp[bid>>3] |= 1<<(bid&7);
			GC_PUSH_GEN(p,page,bid);
		}
	}
	cur_mark_stack = mark_stack;
	if( mark_stack ) gc_flush_mark();
#	ifdef HL_DEBUG
	gc_clear_unmarked_mem();
#	endif
	gc_flush_empty_pages();
}

HL_API void hl_gc_dump() {
#	ifdef HL_TRACK_ALLOC
	int i;
	hl_field_lookup *hashes = NULL;
	int h_count = 0, h_size = 0;
	FILE *outFile;
	
	int tot_blocks = 0;
	int64 tot_size = 0;

	hl_gc_major();
	outFile = fopen("gcDump.txt","w");
	fprintf(outFile,"%dMB GC pages memory\n",(int)(gc_stats.pages_total_memory>>20));

	// Count currently allocated
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *p = gc_pages[i];
		while( p ) {
			int bid;
			for(bid=p->first_block;bid<p->max_blocks;bid++)
				if( p->bmp[bid>>3]&(1<<(bid&7)) ) {
					tot_blocks++;
					tot_size += p->sizes ? p->sizes[bid] * p->block_size : p->block_size;
				}
			p = p->next_page;
		}
	}
	fprintf(outFile,"%d Live Blocks, %dMB total size\n",tot_blocks,(int)(tot_size>>20));

	fprintf(outFile,"%d Pages\n",gc_stats.pages_count);
	// Dump pages stats
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *p = gc_pages[i];
		while( p ) {
			static const char *PDESC[] = {"dyn","raw","noptr","finalizers"};
			int bid;
			int tot_blocks = 0;
			int64 tot_size = 0;
			for(bid=p->first_block;bid<p->max_blocks;bid++)
				if( p->bmp[bid>>3]&(1<<(bid&7)) ) {
					tot_blocks++;
					tot_size += p->sizes ? p->sizes[bid] * p->block_size : p->block_size;
				}
			fprintf(outFile,"\tpage %.8X %d block %s : %dKB, %d/%d live blocks, %.2f%% used\n", (int)(int_val)p, p->block_size, PDESC[p->page_kind], p->page_size>>10, tot_blocks, p->max_blocks-p->first_block, (tot_size * 100. / ((p->max_blocks-p->first_block)*p->block_size)));
			p = p->next_page;
		}
	}
	fprintf(outFile,"%d Live Blocks, %dMB total size\n",tot_blocks,(int)(tot_size>>20));

	// Count blocks per stack hash
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *p = gc_pages[i];
		while( p ) {
			int bid;
			for(bid=p->first_block;bid<p->max_blocks;bid++)
				if( p->bmp[bid>>3]&(1<<(bid&7)) ) {
					int hash = p->alloc_hashes[bid];
					hl_field_lookup *f = hl_lookup_find(hashes,h_count,hash);
					if( f == NULL ) {
						if( h_count == h_size ) {
							int nsize = h_size ? h_size << 1 : 128;
							hl_field_lookup *ns = (hl_field_lookup*)malloc(sizeof(hl_field_lookup)*nsize);
							memcpy(ns,hashes,h_size*sizeof(hl_field_lookup));
							free(hashes);
							hashes = ns;
							h_size = nsize;
						}
						f = hl_lookup_insert(hashes,h_count++,hash,NULL,0);
					}
					// total mem size
					f->t = (hl_type*)((int_val)f->t + (p->sizes ? p->sizes[bid] * p->block_size : p->block_size));
					f->field_index++;
				}
			p = p->next_page;
		}
	}
	// sort by mem size
	for(i=0;i<h_count;i++) {
		int j;
		hl_field_lookup *a = hashes + i;
		for(j=i+1;j<h_count;j++) {
			hl_field_lookup *b = hashes + j;
			if( a->t < b->t ) {
				hl_field_lookup tmp = *a;
				*a = *b;
				*b = tmp;
			}
		}
	}
	// dump output
	for(i=0;i<h_count;i++) {
		hl_field_lookup *l = hashes + i;
		int scount;
		int j;
		void **stack = hl_stack_from_hash(l->hashed_name,&scount);
		uchar tmp[512];
		char stmp[512];
		fprintf(outFile, "%.8X %d blocks, %d KB mem size\n",l->hashed_name,l->field_index,(int)(((int_val)l->t)>>10));
		for(j=0;j<scount;j++) {
			int size = 512;
			uchar *sym = hl_resolve_symbol(stack[j],tmp,&size);
			if( sym ) {
				utostr(stmp,512,sym);
				fprintf(outFile, "\t%s", stmp);
			}
		}
	}
	fclose(outFile);
	free(hashes);
#	endif
}

HL_API void hl_gc_major() {
	gc_stats.last_mark = gc_stats.total_allocated;
	gc_mark();
}

HL_API bool hl_is_gc_ptr( void *ptr ) {
	gc_pheader *page = GC_GET_PAGE(ptr);
	int bid;
	if( !page ) return false;
	if( ((unsigned char*)ptr - (unsigned char*)page) % page->block_size != 0 ) return false;
	bid = ((unsigned char*)ptr - (unsigned char*)page) / page->block_size;
	if( bid < page->first_block ) return false;
	if( page->sizes && page->sizes[bid] == 0 ) return false;
	return true;
}

static void gc_check_mark() {
	int64 m = gc_stats.total_allocated - gc_stats.last_mark;
	if( m > gc_stats.pages_total_memory * gc_mark_threshold ) hl_gc_major();
}

static void hl_gc_init( void *stack_top ) {
	int i;
	gc_stack_top = stack_top;
	for(i=0;i<1<<GC_LEVEL0_BITS;i++)
		hl_gc_page_map[i] = gc_level1_null;
	if( TRAILING_ONES(0x080003FF) != 10 || TRAILING_ONES(0) != 0 || TRAILING_ONES(0xFFFFFFFF) != 32 )
		hl_fatal("Invalid builtin tl1");
	if( TRAILING_ZEROES((unsigned)~0x080003FF) != 10 || TRAILING_ZEROES(0) != 32 || TRAILING_ZEROES(0xFFFFFFFF) != 0 )
		hl_fatal("Invalid builtin tl0");
}

// ---- UTILITIES ----------------------

void hl_global_init( void *stack_top ) {
	hl_gc_init(stack_top);
}

void hl_cache_free();

void hl_global_free() {
	hl_cache_free();
}

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
	if( b == NULL || b->size <= size ) {
		int alloc = size < 4096-sizeof(hl_alloc_block) ? 4096-sizeof(hl_alloc_block) : size;
		b = (hl_alloc_block *)malloc(sizeof(hl_alloc_block) + alloc);
		if( b == NULL ) {
			printf("Out of memory");
			exit(99);
		}
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
	if( p ) MZERO(p,size);
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
#ifdef HL_WIN
	return VirtualAlloc(NULL,size,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
#else
	void *p;
	p = mmap(NULL,size,PROT_READ|PROT_WRITE|PROT_EXEC,(MAP_PRIVATE|MAP_ANON),-1,0);
	return p;
#endif
}

HL_PRIM void hl_free_executable_memory( void *c, int size ) {
#ifdef HL_WIN
	VirtualFree(c,0,MEM_RELEASE);
#else
	munmap(c, size);
#endif
}

static void *gc_alloc_page_memory( int size ) {
#ifdef HL_WIN
	return VirtualAlloc(NULL,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#else
	return malloc(size);
#endif
}

static void gc_free_page_memory( void *ptr, int size ) {
#ifdef HL_WIN
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	free(ptr);
#endif
}

vdynamic *hl_alloc_dynamic( hl_type *t ) {
	vdynamic *d = (vdynamic*) (hl_is_ptr(t) ? hl_gc_alloc(sizeof(vdynamic)) : hl_gc_alloc_noptr(sizeof(vdynamic)));
	d->t = t;
	d->v.ptr = NULL;
#	ifdef HL_DEBUG
	if( t->kind == HVOID )
		hl_error("alloc_dynamic(VOID)");
#	endif
	return d;
}

vdynamic *hl_alloc_obj( hl_type *t ) {
	vobj *o;
	int size;
	hl_runtime_obj *rt = t->obj->rt;
	if( rt == NULL || rt->methods == NULL ) rt = hl_get_obj_proto(t);
	size = rt->size;
	if( size & (HL_WSIZE-1) ) size += HL_WSIZE - (size & (HL_WSIZE-1));
	o = (vobj*)hl_gc_alloc(size);
	MZERO(o,size);
	o->t = t;
	return (vdynamic*)o;
}

vdynobj *hl_alloc_dynobj() {
	vdynobj *o = (vdynobj*)hl_gc_alloc(sizeof(vdynobj));
	o->dproto = (vdynobj_proto*)&hlt_dynobj;
	o->nfields = 0;
	o->dataSize = 0;
	o->fields_data = NULL;
	o->virtuals = NULL;
	return o;
}

vvirtual *hl_alloc_virtual( hl_type *t ) {
	vvirtual *v = (vvirtual*)hl_gc_alloc(t->virt->dataSize + sizeof(vvirtual) + sizeof(void*) * t->virt->nfields);
	void **fields = (void**)(v + 1);
	char *vdata = (char*)(fields + t->virt->nfields);
	int i;
	v->t = t;
	v->value = NULL;
	v->next = NULL;
	for(i=0;i<t->virt->nfields;i++)
		fields[i] = (char*)v + t->virt->indexes[i];
	MZERO(vdata,t->virt->dataSize);
	return v;
}
