#include "hl.h"
#include "gc.h"

#ifdef HL_WIN
#	include <windows.h>
#else
#	include <sys/types.h>
#	include <sys/mman.h>
#endif


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#ifdef __APPLE__
#	ifndef MAP_ANONYMOUS
#		define MAP_ANONYMOUS MAP_ANON
#	endif
#endif

#ifdef HL_THREADS
#error "no threads with gc for now"
#endif

#define GC_STATIC

// utilities --------------------------------------------------------

// branch optimisation
#define LIKELY(c) __builtin_expect((c), 1)
#define UNLIKELY(c) __builtin_expect((c), 0)

// bit access
#define GET_BIT(bmp, bit) ((bmp)[(bit) >> 3] & (1 << ((bit) & 7)))
#define SET_BIT0(bmp, bit) (bmp)[(bit) >> 3] &= 0xFF ^ (1 << ((bit) & 7));
#define SET_BIT1(bmp, bit) (bmp)[(bit) >> 3] |= 1 << ((bit) & 7);
#define SET_BIT(bmp, bit, val) (bmp)[(bit) >> 3] = ((bmp)[(bit) >> 3] & (0xFF ^ (1 << ((bit) & 7)))) | ((val) << ((bit) & 7));

// doubly-linked lists
#define DLL_UNLINK(obj) do { \
		(obj)->next->prev = (obj)->prev; \
		(obj)->prev->next = (obj)->next; \
		(obj)->next = NULL; \
		(obj)->prev = NULL; \
	} while (0)
#define DLL_INSERT(obj, head) do { \
		(obj)->next = (head)->next; \
		(obj)->prev = (head); \
		(head)->next = (obj); \
		(obj)->next->prev = (obj); \
	} while (0)

// OS page management ----------------------------------------------

static void *base_addr;

// allocates a page-aligned region of memory from the OS
GC_STATIC void *gc_alloc_os_memory(int size) {
	GC_DEBUG_DUMP1("gc_alloc_os_memory.enter", size);
	if (gc_stats.total_memory + size >= gc_config.memory_limit) {
		GC_DEBUG_DUMP0("gc_alloc_os_memory.fail.oom");
		GC_DEBUG(fatal, "using %lu / %lu bytes, need %d more", gc_stats.total_memory, gc_config.memory_limit, size);
		GC_FATAL("OOM: memory limit hit");
	}
	void *ptr = mmap(base_addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == (void *)-1) {
		GC_DEBUG_DUMP0("gc_alloc_os_memory.fail.other");
		GC_FATAL("failed to allocate page");
	}
	gc_stats.total_memory += size;
	if (((int_val)ptr) & (GC_PAGE_SIZE - 1)) {
		// re-align
		munmap(ptr,size);
		void *tmp;
		int tmp_size = (int)((int_val)ptr - (int_val)base_addr);
		if (tmp_size > 0) {
			base_addr = (void *)((((int_val)ptr) & ~(GC_PAGE_SIZE - 1)) + GC_PAGE_SIZE);
			tmp = ptr;
		} else {
			base_addr = (void *)(((int_val)ptr) & ~(GC_PAGE_SIZE - 1));
			tmp = NULL;
		}
		if (tmp)
			tmp = mmap(tmp, tmp_size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		// TODO: too many retries ???
		ptr = gc_alloc_os_memory(size);
		if (tmp)
			munmap(tmp, tmp_size);
		return ptr;
	}
	base_addr = (char *)ptr + size;
	GC_DEBUG_DUMP2("gc_alloc_os_memory.success", ptr, size);
	GC_DEBUG(os, "allocated OS page: %p", ptr);
	return ptr;
}

// returns a region of memory back to the OS
GC_STATIC void gc_free_os_memory(void *ptr, int size) {
	munmap(ptr, size);
	gc_stats.total_memory -= size;
	GC_DEBUG_DUMP2("gc_free_os_memory.success", ptr, size);
	GC_DEBUG(os, "reclaimed OS page: %p", ptr);
}

// GC page management ----------------------------------------------

static void *gc_min_allocated;
static void *gc_max_allocated;
static gc_page_header_t *gc_last_allocated_page;
static gc_page_header_t *gc_last_allocated_huge_page;
static gc_page_hash_t gc_page_hash;
static hl_mutex *gc_mutex_pool;
static gc_block_header_t *gc_pool;
static int gc_pool_count;
// TODO: pre-allocate page (or pages) to be used for fast allocation (refresh during collection)
// TODO: mutexes ...

// allocates and initialises a GC page
GC_STATIC void gc_add_page(gc_page_header_t *header) {
	if ((char *)header < (char *)gc_min_allocated) {
		gc_min_allocated = (char *)header;
	}
	if ((char *)header + GC_PAGE_SIZE > (char *)gc_max_allocated) {
		gc_max_allocated = (char *)header + GC_PAGE_SIZE;
	}

	gc_stats.total_pages++;
	gc_page_hash.total_pages++;
	double load_factor = gc_page_hash.total_pages / gc_page_hash.bucket_count;
	if (load_factor > 0.75) { // TODO: limit the growth of buckets
		gc_page_hash.bucket_count <<= 1;
		free(gc_page_hash.buckets);
		gc_page_hash.buckets = calloc(sizeof(gc_page_header_t *), gc_page_hash.bucket_count);
		for (gc_page_header_t *cur = gc_last_allocated_page; cur != NULL; cur = cur->next_page) {
			int bucket = GC_HASH(cur) % gc_page_hash.bucket_count;
			cur->next_page_bucket = gc_page_hash.buckets[bucket];
			gc_page_hash.buckets[bucket] = cur;
		}
		for (gc_page_header_t *cur = gc_last_allocated_huge_page; cur != NULL; cur = cur->next_page) {
			int bucket = GC_HASH(cur) % gc_page_hash.bucket_count;
			cur->next_page_bucket = gc_page_hash.buckets[bucket];
			gc_page_hash.buckets[bucket] = cur;
		}
	}
	int bucket = GC_HASH(header) % gc_page_hash.bucket_count;
	header->next_page_bucket = gc_page_hash.buckets[bucket];
	gc_page_hash.buckets[bucket] = header;
	GC_DEBUG(page, "page %p in bucket %d", header, bucket);
}

// allocates a normal page (containing immix blocks)
GC_STATIC gc_page_header_t *gc_alloc_page_normal(void) {
	gc_page_header_t *header = (gc_page_header_t *)gc_alloc_os_memory(GC_PAGE_SIZE);
	if (header == NULL) {
		// TODO: OOM here rather than in gc_pop_block ?
		return NULL;
	}

	header->size = GC_PAGE_SIZE;
	header->free_blocks = GC_BLOCKS_PER_PAGE;
	header->kind = GC_PAGE_NORMAL;
	header->next_page = gc_last_allocated_page;
	gc_last_allocated_page = header;

	gc_add_page(header);

	gc_block_header_t *blocks = GC_PAGE_BLOCK(header, 0);
	for (int i = 0; i < GC_BLOCKS_PER_PAGE; i++) {
		blocks[i].kind = GC_BLOCK_FREE;
		blocks[i].next = &blocks[i + 1];
	}
	hl_mutex_acquire(gc_mutex_pool);
	blocks[GC_BLOCKS_PER_PAGE - 1].next = gc_pool;
	gc_pool = &blocks[0];
	gc_pool_count += GC_BLOCKS_PER_PAGE;
	hl_mutex_release(gc_mutex_pool);

	GC_DEBUG_DUMP1("gc_alloc_page_normal.success", header);
	return header;
}

// allocates a huge page (containing a single object)
GC_STATIC gc_page_header_t *gc_alloc_page_huge(int size) {
	gc_page_header_t *header = (gc_page_header_t *)gc_alloc_os_memory(sizeof(gc_page_header_t) + size);
	if (header == NULL) {
		GC_FATAL("OOM: huge page");
	}

	header->size = sizeof(gc_page_header_t) + size;
	header->kind = GC_PAGE_HUGE;
	header->next_page = gc_last_allocated_huge_page;
	gc_last_allocated_huge_page = header;

	gc_add_page(header);

	GC_DEBUG_DUMP2("gc_alloc_page_huge.success", header, size);
	return header;
}

// frees a GC page
GC_STATIC void gc_free_page_memory(gc_page_header_t *header) {
	// TODO: make page list a DLL
	gc_page_header_t **last = header->kind == GC_PAGE_HUGE ? &gc_last_allocated_huge_page : &gc_last_allocated_page;
	for (gc_page_header_t *cur = *last; cur != NULL; cur = cur->next_page) {
		if (cur == header) {
			*last = cur->next_page;
			break;
		}
		last = &cur->next_page;
	}
	int bucket = GC_HASH(header) % gc_page_hash.bucket_count;
	last = &gc_page_hash.buckets[bucket];
	for (gc_page_header_t *cur = gc_page_hash.buckets[bucket]; cur != NULL; cur = cur->next_page_bucket) {
		if (cur == header) {
			*last = cur->next_page_bucket;
			break;
		}
		last = &cur->next_page;
	}
	// TODO: maybe rearrange if load factor is low?
	gc_page_hash.total_pages--;
	gc_stats.total_pages--;
	gc_free_os_memory(header, header->size);
	GC_DEBUG_DUMP1("gc_free_page_memory.success", header);
}

// roots -----------------------------------------------------------

static hl_mutex *gc_mutex_roots;
static void ***gc_roots;
static int gc_root_count;
static int gc_root_capacity;

// adds a root to the GC root list
// a root is a mutable pointer to GC-allocated memory (or NULL), which is
// dereferenced and followed when starting the GC marking phase
HL_PRIM void hl_add_root(void **p) {
	if (p == NULL) {
		return;
	}
	hl_mutex_acquire(gc_mutex_roots);
	if (gc_root_count >= gc_root_capacity) {
		int new_capacity = gc_root_capacity << 1;
		if (gc_root_capacity == 0) {
			new_capacity = 128;
		}
		void ***new_roots = (void ***)calloc(new_capacity, sizeof(void **));
		if (gc_roots != NULL) {
			memcpy(new_roots, gc_roots, sizeof(void **) * gc_root_capacity);
		}
		gc_roots = new_roots;
		gc_root_capacity = new_capacity;
	}
	gc_roots[gc_root_count++] = p;
	hl_mutex_release(gc_mutex_roots);
	GC_DEBUG_DUMP1("hl_add_root.success", p);
}

// removes the given root from the GC root list
HL_PRIM void hl_remove_root(void **p) {
	hl_mutex_acquire(gc_mutex_roots);
	for (int i = 0; i < gc_root_count; i++) {
		if (gc_roots[i] == p) {
			gc_roots[i] = gc_roots[--gc_root_count];
			break;
		}
	}
	hl_mutex_release(gc_mutex_roots);
	GC_DEBUG_DUMP1("hl_remove_root.success", p);
}

// thread setup ----------------------------------------------------

HL_THREAD_STATIC_VAR hl_thread_info *current_thread;
static int gc_mark_polarity;
static int gc_mark_total;
static gc_mark_stack_t gc_mark_stack;
static gc_mark_stack_t gc_mark_stack_next;
static gc_mark_stack_t *gc_mark_stack_active;

// registers a thread with the GC
// a registered thread can use GC allocation and must be stopped when
// collection is happening
HL_API void hl_register_thread(void *stack_top) {
	if (hl_get_thread())
		hl_fatal("thread already registered");

	hl_thread_info *t = (hl_thread_info*)calloc(sizeof(hl_thread_info), 1);
	t->thread_id = hl_thread_id();
	t->stack_top = stack_top;
	t->flags = HL_TRACK_MASK << HL_TREAD_TRACK_SHIFT;

	gc_block_header_t *block = gc_pop_block();
	t->lines_block = block;
	t->lines_start = &block->lines[0];
	t->lines_limit = &block->lines[GC_LINES_PER_BLOCK];

	gc_block_dummy_t *sentinels = (gc_block_dummy_t *)calloc(sizeof(gc_block_dummy_t), 4);
	sentinels[0].next = (gc_block_header_t *)&sentinels[1];
	sentinels[1].prev = (gc_block_header_t *)&sentinels[0];
	sentinels[2].next = (gc_block_header_t *)&sentinels[3];
	sentinels[3].prev = (gc_block_header_t *)&sentinels[2];
	t->sentinels = sentinels;

	t->full_blocks = (gc_block_header_t *)&sentinels[0];
	t->recyclable_blocks = (gc_block_header_t *)&sentinels[2];

	current_thread = t;
	hl_add_root((void **)&t->exc_value);
	hl_add_root((void **)&t->exc_handler);

	/*
	// TODO: thread list
	gc_global_lock(true);
	hl_thread_info **all = (hl_thread_info**)malloc(sizeof(void*) * (gc_threads.count + 1));
	memcpy(all,gc_threads.threads,sizeof(void*)*gc_threads.count);
	gc_threads.threads = all;
	all[gc_threads.count++] = t;
	gc_global_lock(false);
	*/
}

// unregisters a thread from the GC
HL_API void hl_unregister_thread(void) {
	hl_thread_info *t = hl_get_thread();
	if (!t)
		hl_fatal("thread not registered");
	hl_remove_root((void **)&t->exc_value);
	hl_remove_root((void **)&t->exc_handler);

	// TODO: release blocks into zombie stack
	free(t->sentinels);
	free(t);
	current_thread = NULL;

	/*
	gc_global_lock(true);
	for(int i = 0;i < gc_threads.count; i++)
		if( gc_threads.threads[i] == t ) {
			memmove(gc_threads.threads + i, gc_threads.threads + i + 1, sizeof(void*) * (gc_threads.count - i - 1));
			gc_threads.count--;
			break;
		}
	free(t);
	current_thread = NULL;
	// don't use gc_global_lock(false)
	hl_mutex_release(gc_threads.global_lock);
	*/
}

HL_API void *hl_gc_threads_info() {
	return NULL;
}

GC_STATIC void gc_save_context(hl_thread_info *t) {
	setjmp(t->gc_regs);
	t->stack_cur = &t;
}

GC_STATIC void gc_stop_world( bool b ) {
#	ifdef HL_THREADS
	if( b ) {
		int i;
		gc_threads.stopping_world = true;
		for(i=0;i<gc_threads.count;i++) {
			hl_thread_info *t = gc_threads.threads[i];
			while( t->gc_blocking == 0 ) {}; // spinwait
		}
	} else {
		// releasing global lock will release all threads
		gc_threads.stopping_world = false;
	}
#	else
	if (b) {
		gc_save_context(current_thread);
	}
#	endif
}

// thread-local allocator <-> global GC ----------------------------

// acquires a block for the current thread
// the block may come from the thread's recycle list, or from the global
// free block pool (which incurs a locking penalty)
GC_STATIC gc_block_header_t *gc_pop_block(void) {
	hl_mutex_acquire(gc_mutex_pool); // TODO: mutexes need to be re-entrant, or lock later
	if (UNLIKELY(gc_pool_count == 0)) {
		// if (gc_stats.total_memory * 1.2 >= gc_config.memory_limit) {
		if (gc_stats.live_blocks > 0) {
			GC_DEBUG(alloc, "using %lu / %lu bytes", gc_stats.total_memory, gc_config.memory_limit);
			// GC_FATAL("OOM: memory limit hit");
			GC_DEBUG(alloc, "memory limit hit, triggering major ...");
			hl_gc_major();
		}
		if (gc_pool_count == 0 && gc_alloc_page_normal() == NULL) {
			GC_FATAL("OOM: page memory");
		}
	}
	gc_block_header_t *block = gc_pool;
	if ((((int_val)block->next) & (int_val)(0x10000 - 1)) != 0) {
		GC_FATAL("pool corrupted in pop");
	}
	gc_pool = block->next;
	gc_page_header_t *page = GC_BLOCK_PAGE(block);
	// TODO: mutex per page?
	page->free_blocks--;
	SET_BIT1(page->block_bmp, GC_BLOCK_ID(block));
	gc_pool_count--;
	hl_mutex_release(gc_mutex_pool);
	block->kind = GC_BLOCK_NEW;
	block->owner_thread = hl_thread_id();
	GC_DEBUG(alloc, "got block %p", block);
	GC_DEBUG_DUMP1("gc_pop_block.success", block);
	gc_stats.live_blocks++;
	return block;
}

// returns a thread-owned block to the global free pool
GC_STATIC void gc_push_block(gc_block_header_t *block) {
	if (block->kind == GC_BLOCK_FULL || block->kind == GC_BLOCK_RECYCLED) {
		DLL_UNLINK(block);
	} // TODO: condition should always be true, until zombie blocks are introduced ?
	block->kind = GC_BLOCK_FREE; // TODO: zombie ?
	block->owner_thread = -1;
	memset(block->line_marks, 0, GC_LINES_PER_BLOCK);
	gc_page_header_t *page = GC_BLOCK_PAGE(block);
	// TODO: mutex per page?
	hl_mutex_acquire(gc_mutex_pool);
	page->free_blocks++;
	SET_BIT0(page->block_bmp, GC_BLOCK_ID(block));
	// TODO: manage disorder in GC pool (prefer address order)
	block->next = gc_pool;
	if ((((int_val)block) & (int_val)(0x10000 - 1)) != 0) {
		GC_FATAL("pool corrupted in push");
	}
	gc_pool = block;
	gc_pool_count++;
	gc_stats.live_blocks--;
	hl_mutex_release(gc_mutex_pool);
	GC_DEBUG_DUMP1("gc_push_block.success", block);
}

// acquires a huge object
// this always allocates an OS page
GC_STATIC void *gc_pop_huge(int size) {
	gc_page_header_t *header = gc_alloc_page_huge(8192 + size);
	GC_DEBUG(alloc, "allocated huge %d at %p", size, header);
	GC_DEBUG_DUMP2("gc_pop_huge.success", header, size);
	gc_block_header_t *block = (gc_block_header_t *)header;
	block->huge_sized = 1;
	return &block->lines[0];
}

// frees a huge object
GC_STATIC void gc_push_huge(void *huge) {
	gc_page_header_t *header = GC_BLOCK_PAGE(huge);
	GC_DEBUG(alloc, "freed huge %p", huge);
	GC_DEBUG_DUMP1("gc_push_huge.success", header);
	gc_free_os_memory(header, header->size);
}

// thread <-> thread-local allocator -------------------------------

// bump allocate an object with the current bump cursor
// should only be called when there is definitely enough space to allocate
// the object
// this also initialises the object header
GC_STATIC gc_object_t *gc_alloc_bump(hl_type *t, int size, int words, int flags) {
	gc_object_t *ret = (gc_object_t *)current_thread->lines_start;
	gc_metadata_t *meta = GC_METADATA(ret);

	// clear stale data
	memset(ret, 0, sizeof(void *) * words);

	ret->t = t;
	meta->flags = 0;
	meta->marked = !gc_mark_polarity;
	meta->words = words & 15;
	if (flags & GC_ALLOC_DYNAMIC) {
		meta->dynamic_mark = 1;
	}
	if (flags & GC_ALLOC_NOPTR) {
		meta->no_ptr = 1;
	}
	// int line_id = GC_LINE_ID(ret);
	// gc_block_header_t *block = GC_LINE_BLOCK(ret);
	// GC_DEBUG("pre-allocation: %p - %p", current_thread->lines_start, current_thread->lines_limit);
	current_thread->lines_start = (void *)(((char *)current_thread->lines_start) + size);
	if (LIKELY(size <= GC_LINE_SIZE)) {
		// GC_DEBUG("allocated small %p in block %p (%d bytes, %d words)", ret, block, size, words);
	} else {
		meta->medium_sized = 1;
		gc_metadata_ext_t *meta_ext = (gc_metadata_ext_t *)meta;
		meta_ext->words_ext = words >> 4;
		// int last_id = GC_LINE_ID((void *)ret + size);
		// ret->line_span = (last_id - line_id) + 1;
		// GC_DEBUG("allocated medium %p in block %p (%d bytes, %d lines, %d words)", ret, block, size, ret->line_span, words);
	}

	GC_DEBUG_DUMP3("gc_alloc_bump.success", ret, size, flags);
	return ret;
}

// finds the next line gap in the active block of the current thread
// returns true if a gap was found
GC_STATIC bool gc_find_gap(void) {
	if (GC_LINE_BLOCK(current_thread->lines_start) != current_thread->lines_block)
		return false;
	// TODO: optimise
	int line = GC_LINE_ID(current_thread->lines_start);
	bool last_free = (line == 0);
	for (; line < GC_LINES_PER_BLOCK; line++) {
		if (current_thread->lines_block->line_marks[line] == 0) {
			// skip line gap immediately after a marked line
			if (last_free) {
				break;
			} else {
				last_free = true;
			}
		} else {
			last_free = false;
		}
	}
	if (line >= GC_LINES_PER_BLOCK)
		return false;
	current_thread->lines_start = &current_thread->lines_block->lines[line];
	// GC_DEBUG("gap search in %p; line %d, %p", current_thread->lines_block, line, current_thread->lines_start);
	// found a gap, increase limit as far as possible
	// TODO: optimise
	while (current_thread->lines_block->line_marks[line] == 0) {
		line++;
	}
	current_thread->lines_limit = &current_thread->lines_block->lines[line];
	return true;
}

// generic allocation function
HL_API void *hl_gc_alloc_gen(hl_type *t, int size, int flags) {
	// align to words
	int words = (size + (sizeof(void *) - 1)) / sizeof(void *);
	size = words * sizeof(void *);

	if (LIKELY((char *)current_thread->lines_start + size <= (char *)current_thread->lines_limit)) {
		return gc_alloc_bump(t, size, words, flags);
	}

	// TODO: list operations should not be interrupted by the GC, mutex per list

	if (LIKELY(size <= GC_LINE_SIZE)) {
		// find next line gap (based on mark bits from previous collection) in current block
		if (gc_find_gap()) {
			return gc_alloc_bump(t, size, words, flags);
		}

		// add current block to full blocks
		current_thread->lines_block->kind = GC_BLOCK_FULL;
		DLL_INSERT(current_thread->lines_block, current_thread->full_blocks);

		// no gap found, get next recyclable block
		if (current_thread->recyclable_blocks->next->next != NULL) {
			current_thread->lines_block = current_thread->recyclable_blocks->next;
			current_thread->lines_start = &current_thread->lines_block->lines[0];
			DLL_UNLINK(current_thread->lines_block);
			current_thread->lines_block->kind = GC_BLOCK_NEW; // TODO: maybe a separate kind
			if (gc_find_gap()) {
				return gc_alloc_bump(t, size, words, flags);
			}
			GC_FATAL("unreachable");
		}

		// if none, get fresh block
		current_thread->lines_block = gc_pop_block();
		if (current_thread->lines_block == NULL) {
			hl_gc_major();
			current_thread->lines_block = gc_pop_block();
			if (current_thread->lines_block == NULL) {
				GC_FATAL("OOM: alloc");
			}
		}
		current_thread->lines_start = &current_thread->lines_block->lines[0];
		current_thread->lines_limit = &current_thread->lines_block->lines[GC_LINES_PER_BLOCK];
		return gc_alloc_bump(t, size, words, flags);
	} else if (size <= GC_MEDIUM_SIZE) {
		current_thread->lines_block->kind = GC_BLOCK_FULL;
		DLL_INSERT(current_thread->lines_block, current_thread->full_blocks);

		// get empty block to avoid expensive search
		current_thread->lines_block = gc_pop_block();
		if (current_thread->lines_block == NULL) {
			hl_gc_major();
			current_thread->lines_block = gc_pop_block();
			if (current_thread->lines_block == NULL) {
				GC_FATAL("OOM: alloc");
			}
		}
		current_thread->lines_start = &current_thread->lines_block->lines[0];
		current_thread->lines_limit = &current_thread->lines_block->lines[GC_LINES_PER_BLOCK];
		return gc_alloc_bump(t, size, words, flags);
	} else {
		gc_object_t *obj = gc_pop_huge(size);
		obj->t = t;
		gc_metadata_t *meta = GC_METADATA(obj);
		meta->flags = 0;
		meta->marked = !gc_mark_polarity;
		if (flags & GC_ALLOC_DYNAMIC) {
			meta->dynamic_mark = 1;
		}
		if (flags & GC_ALLOC_NOPTR) {
			meta->no_ptr = 1;
		}
		return obj;
	}
}

// triggers a major GC collection
HL_API void hl_gc_major(void) {
	GC_DEBUG_DUMP0("hl_gc_major.entry");
	gc_stop_world(true);
	gc_mark();
	gc_sweep();
	gc_stop_world(false);
	GC_DEBUG_DUMP0("hl_gc_major.success");
}

// GC internals ----------------------------------------------------

// push a pointer (and its incoming reference) to the mark stack
GC_STATIC void gc_push(void *p, void *ref) {
	if (p == NULL)
		return;
	if (UNLIKELY(gc_mark_stack_active->pos >= gc_mark_stack_active->capacity)) {
		if (gc_mark_stack_active == &gc_mark_stack) {
			GC_DEBUG(mark, "mark stack growth alloc");
			// ran out of space on original stack, allocate a new one and start using it
			gc_mark_stack_next.capacity = gc_mark_stack.capacity << 1;
			gc_mark_stack_next.data = malloc(sizeof(gc_mark_stack_entry_t) * gc_mark_stack_next.capacity);
			gc_mark_stack_active = &gc_mark_stack_next;
		} else {
			GC_DEBUG(mark, "mark stack panic realloc");
			// ran out of space on both stacks, allocate a new one and copy into it
			int new_capacity = gc_mark_stack_active->capacity << 1; // TODO: OOM if too large
			gc_mark_stack_entry_t *new_stack = (gc_mark_stack_entry_t *)malloc(sizeof(gc_mark_stack_entry_t) * new_capacity);
			memcpy(new_stack, gc_mark_stack.data, sizeof(gc_mark_stack_entry_t) * gc_mark_stack.pos);
			memcpy(&new_stack[gc_mark_stack.pos], gc_mark_stack_next.data, sizeof(gc_mark_stack_entry_t) * gc_mark_stack_next.pos);
			free(gc_mark_stack.data);
			free(gc_mark_stack_next.data);
			gc_mark_stack.data = new_stack;
			gc_mark_stack_next.data = NULL;
			gc_mark_stack.pos = gc_mark_stack.pos + gc_mark_stack_next.pos;
			gc_mark_stack.capacity = new_capacity;
			gc_mark_stack_active = &gc_mark_stack;
		}
	}
	if (gc_get_block(p) != NULL) {
		gc_object_t *obj = p;
		gc_metadata_t *meta = GC_METADATA(obj);
		if (meta->marked != gc_mark_polarity) {
			meta->marked = gc_mark_polarity;
			gc_mark_total++;
			gc_mark_stack_active->data[gc_mark_stack_active->pos].object = p;
			gc_mark_stack_active->data[gc_mark_stack_active->pos++].reference = ref;
			GC_DEBUG(mark, "pushed %p -> %p to %p", ref, p, gc_mark_stack_active->data);
			GC_DEBUG_DUMP2("gc_push.success", p, ref);
		}
	}
}

// get the next pointer (and its incoming reference) from the mark stack
GC_STATIC gc_object_t *gc_pop(void) {
	void *p = gc_mark_stack.data[--gc_mark_stack.pos].object;
	void *ref = gc_mark_stack.data[gc_mark_stack.pos].reference;
	// GC_DEBUG("[M] popped %p from %p", p, gc_mark_stack.data);
	if (gc_mark_stack.pos <= 0 && gc_mark_stack_active != &gc_mark_stack) {
		GC_DEBUG(mark, "mark stack shrunk");
		// emptied original stack, switch
		free(gc_mark_stack.data);
		gc_mark_stack.data = gc_mark_stack_next.data;
		gc_mark_stack.pos = gc_mark_stack_next.pos;
		gc_mark_stack.capacity = gc_mark_stack_next.capacity;
		gc_mark_stack_next.data = NULL;
		gc_mark_stack_active = &gc_mark_stack;
	}
	GC_DEBUG_DUMP2("gc_pop.success", p, ref);
	return p;
}

HL_PRIM vbyte* hl_type_name( hl_type *t );

// marks live objects in the heap
GC_STATIC void gc_mark(void) {
	GC_DEBUG(mark, "start");
	GC_DEBUG(mark, "polarity: %d", gc_mark_polarity);

	// TODO: only if debug
	// reset stats
	gc_stats.live_objects = 0;

	// reset line marks
	// TODO: is there a better place to do this?
	for (gc_page_header_t *page = gc_last_allocated_page; page != NULL; page = page->next_page) {
		for (int block_id = 0; block_id < GC_BLOCKS_PER_PAGE; block_id++) {
			if (GET_BIT(page->block_bmp, block_id)) {
				gc_block_header_t *block = GC_PAGE_BLOCK(page, block_id);
				memset(block->line_marks, 0, GC_LINES_PER_BLOCK);
			}
		}
	}

	// reset stacks
	gc_mark_total = 0;
	gc_mark_stack.pos = 0;
	gc_mark_stack_next.pos = 0;
	gc_mark_stack_active = &gc_mark_stack;

	GC_DEBUG_DUMP0("gc_mark.roots");

	// mark roots
	for (int i = 0; i < gc_root_count; i++) {
		GC_DEBUG(mark, "root: %p", gc_roots[i]);
		gc_push(*gc_roots[i], gc_roots[i]);
	}

	GC_DEBUG_DUMP0("gc_mark.threads");

	// scan threads stacks & registers
	// TODO: multiple threads ...
	{
		hl_thread_info *t = current_thread;
		void **cur = t->stack_cur;
		void **top = t->stack_top;
		while (cur < top) {
			gc_push(*cur++, NULL);
		}
		cur = (void **)(&(t->gc_regs));
		top = (void **)(((char *)&(t->gc_regs)) + sizeof(jmp_buf));
		while (cur < top) {
			gc_push(*cur++, NULL);
		}
	}

	GC_DEBUG_DUMP0("gc_mark.propagate");

	// propagate
	while (gc_mark_total-- > 0) {
		gc_stats.live_objects++;
		gc_object_t *p = gc_pop();
		if (p == NULL)
			GC_FATAL("popped null");
		gc_metadata_t *meta = GC_METADATA(p);

		// mark line(s)
		int line_id = GC_LINE_ID(p);
		int words = meta->words;
		gc_block_header_t *block = GC_LINE_BLOCK(p);
		if (meta->medium_sized) {
			gc_metadata_ext_t *meta_ext = (gc_metadata_ext_t *)meta;
			words |= (meta_ext->words_ext << 4);
			int last_id = GC_LINE_ID((char *)p + words * 8);
			int line_span = (last_id - line_id);
			GC_DEBUG(mark, "marking medium, %d lines", line_span);
			memset(&block->line_marks[line_id], 1, line_span);
		} else {
			// for huge objects this does nothing useful
			block->line_marks[line_id] = 1;
		}

		if (meta->no_ptr) {
			continue;
		}

		// scan object
		void **data = (void **)((void *)p + sizeof(gc_object_t));
		if (block->huge_sized) {
			words = (GC_BLOCK_PAGE(p)->size - 8192) / 8;
			GC_DEBUG(mark, "scanning huge, %d words", words);
		}
		// printf("%p %p %p %p -- type %s, %d words\n", p, meta, p->t, p->t->mark_bits, hl_type_name(p->t), words);
		// printf("mark_bits 1 %d\n", p->t->mark_bits[0]);
		// TODO: p->t->mark_bits should only be null with dynamic_mark
		if (true) { //meta->dynamic_mark || p->t == NULL || p->t->mark_bits == NULL) {
			for (int i = 0; i < words; i++) {
				gc_push(data[i], &data[i]);
			}
		} else {
			unsigned int *mark_bits = p->t->mark_bits;
			for (int i = 0; i < words; i++) {
				if (GET_BIT(mark_bits, i)) {
					gc_push(data[i], &data[i]);
				}
			}
		}
	}

	GC_DEBUG_DUMP0("gc_mark.cleanup");

	// clean up stacks
	if (gc_mark_stack_active == &gc_mark_stack_next) {
		free(gc_mark_stack.data);
		gc_mark_stack.data = gc_mark_stack_next.data;
		gc_mark_stack.capacity = gc_mark_stack_next.capacity;
		gc_mark_stack_next.data = NULL;
		gc_mark_stack_active = &gc_mark_stack;
	}
}

// performs a heap sweep
GC_STATIC void gc_sweep(void) {
	GC_DEBUG(sweep, "start");

	// iterate through pages
	// in each page, iterate occupied (block_bmp) pages
	// for each block
	// - count the number of used lines
	// - finalise unmarked lines ?
	// - if compactable and fragmented, try compacting
	// - if empty, move into global free pool
	// - if not full but in full list, move into thread's recycle list

	// normal page with blocks
	for (gc_page_header_t *page = gc_last_allocated_page; page != NULL; page = page->next_page) {
		for (int block_id = 0; block_id < GC_BLOCKS_PER_PAGE; block_id++) {
			if (GET_BIT(page->block_bmp, block_id)) {
				gc_block_header_t *block = GC_PAGE_BLOCK(page, block_id);
				int lines_used = 0;
				for (int line_id = 0; line_id < GC_LINES_PER_BLOCK; line_id++) {
					if (block->line_marks[line_id]) {
						lines_used++;
					}
				}
				GC_DEBUG_DUMP2("gc_sweep.block", block, block->line_marks);
				GC_DEBUG(sweep, "block %p uses %d lines", block, lines_used);
				if (lines_used == 0) {
					gc_push_block(block);
				} else if (lines_used < GC_LINES_PER_BLOCK && block->kind == GC_BLOCK_FULL) {
					DLL_UNLINK(block);
					block->kind = GC_BLOCK_RECYCLED;
					// TODO: get thread by block->owner_thread
					DLL_INSERT(block, current_thread->recyclable_blocks);
				}
			}
		}
	}

	// huge pages with one object
	gc_page_header_t *next_page;
	for (gc_page_header_t *page = gc_last_allocated_huge_page; page != NULL; page = next_page) {
		next_page = page->next_page;
		gc_object_t *obj = (gc_object_t *)(&((gc_block_header_t *)page)->lines[0]);
		gc_metadata_t *meta = GC_METADATA(obj);
		GC_DEBUG(sweep, "huge %p mark: %d %d", obj, meta->marked, gc_mark_polarity);
		if (meta->marked != gc_mark_polarity) {
			//gc_free_page_memory(page);
			gc_push_huge(obj);
		}
	}

	// change polarity for the next cycle
	gc_mark_polarity = 1 - gc_mark_polarity;

	gc_stats.cycles++;
}

GC_STATIC gc_block_header_t *gc_get_block(void *p) {
	if (p < gc_min_allocated || p >= gc_max_allocated || ((int_val)p & 7) != 0) {
		return NULL;
	}
	int bucket = GC_HASH(p) % gc_page_hash.bucket_count;
	for (gc_page_header_t *cur = gc_page_hash.buckets[bucket]; cur != NULL; cur = cur->next_page_bucket) {
		if ((void *)p >= (void *)cur && (void *)p < (void *)((char *)cur + cur->size)) {
			gc_block_header_t *ret = GC_LINE_BLOCK(p);
			if ((int_val)p - (int_val)ret < 64 * GC_LINE_SIZE) {
				// block header
				return NULL;
			}
			return ret;
		}
	}
	return NULL;
}

HL_API void hl_blocking(bool b) {
	// hl_fatal("hl_blocking not implemented");
}

HL_API hl_thread_info *hl_get_thread() {
	return current_thread;
}

HL_API bool hl_is_gc_ptr(void *ptr) {
	return (gc_get_block(ptr) != NULL);
}

// HL_API varray *hl_alloc_array( hl_type *t, int size ) { return NULL; }

HL_API vdynamic *hl_alloc_dynamic( hl_type *t ) {
	return (vdynamic*)hl_gc_alloc_gen(t, sizeof(vdynamic), hl_is_ptr(t) ? GC_ALLOC_DYNAMIC : GC_ALLOC_NOPTR);
}

static const vdynamic vdyn_true = { &hlt_bool, {true} };
static const vdynamic vdyn_false = { &hlt_bool, {false} };

vdynamic *hl_alloc_dynbool( bool b ) {
	return (vdynamic*)(b ? &vdyn_true : &vdyn_false);
}

HL_API vdynamic *hl_alloc_obj( hl_type *t ) {
	vobj *o;
	int size;
	int i;
	hl_runtime_obj *rt = t->obj->rt;
	if( rt == NULL || rt->methods == NULL ) rt = hl_get_obj_proto(t);
	size = rt->size;
	if( size & (HL_WSIZE-1) ) size += HL_WSIZE - (size & (HL_WSIZE-1));
	if( t->kind == HSTRUCT ) {
		o = (vobj*)hl_gc_alloc_gen(t, size, rt->hasPtr ? GC_ALLOC_RAW : GC_ALLOC_NOPTR);
		o->t = NULL;
	} else {
		o = (vobj*)hl_gc_alloc_gen(t, size, rt->hasPtr ? GC_ALLOC_DYNAMIC : GC_ALLOC_NOPTR);
		o->t = t;
	}
	for(i=0;i<rt->nbindings;i++) {
		hl_runtime_binding *b = rt->bindings + i;
		*(void**)(((char*)o) + rt->fields_indexes[b->fid]) = b->closure ? hl_alloc_closure_ptr(b->closure,b->ptr,o) : b->ptr;
	}
	return (vdynamic*)o;
}

// HL_API venum *hl_alloc_enum( hl_type *t, int index ) { return NULL; }

HL_API vvirtual *hl_alloc_virtual( hl_type *t ) {
	vvirtual *v = (vvirtual*)hl_gc_alloc(t, t->virt->dataSize + sizeof(vvirtual) + sizeof(void*) * t->virt->nfields);
	void **fields = (void**)(v + 1);
	char *vdata = (char*)(fields + t->virt->nfields);
	int i;
	v->value = NULL;
	v->next = NULL;
	for(i=0;i<t->virt->nfields;i++)
		fields[i] = (char*)v + t->virt->indexes[i];
	memset(vdata, 0, t->virt->dataSize);
	return v;
}

HL_API vdynobj *hl_alloc_dynobj(void) {
	return (vdynobj*)hl_gc_alloc_gen(&hlt_dynobj,sizeof(vdynobj), GC_ALLOC_DYNAMIC);
}

// HL_API vbyte *hl_alloc_bytes( int size ) { return NULL; }

static hl_types_dump gc_types_dump = NULL;
HL_API void hl_gc_set_dump_types( hl_types_dump tdump ) {
	gc_types_dump = tdump;
}

// GC initialisation -----------------------------------------------

GC_STATIC void gc_init(void) {
	gc_mutex_roots = hl_mutex_alloc(false);
	gc_mutex_pool = hl_mutex_alloc(false);

	base_addr = (void *)0x40000000;
	gc_min_allocated = (void *)0xFFFFFFFFFFFFFFFF;
	gc_max_allocated = 0;
	gc_last_allocated_page = NULL;
	gc_last_allocated_huge_page = NULL;
	gc_page_hash.bucket_count = 128;
	gc_page_hash.buckets = calloc(sizeof(gc_page_header_t *), gc_page_hash.bucket_count);
	gc_page_hash.total_pages = 0;
	gc_pool = NULL;
	gc_pool_count = 0;
	gc_roots = NULL;
	gc_root_count = 0;
	gc_root_capacity = 0;
	gc_mark_polarity = 1;
	gc_mark_stack.capacity = 256;
	gc_mark_stack.data = malloc(sizeof(gc_mark_stack_entry_t) * gc_mark_stack.capacity);
	gc_mark_stack.pos = 0;
	gc_mark_stack_next.data = NULL;
	gc_mark_stack_next.pos = 0;
	gc_mark_stack_next.capacity = 0;
	gc_mark_stack_active = NULL;
	gc_stats.live_objects = 0;
	gc_stats.live_blocks = 0;
	gc_stats.total_memory = 0;
	gc_stats.cycles = 0;
	gc_stats.total_pages = 0;
	// gc_config.memory_limit = 500 * 1024 * 1024;
	gc_config.memory_limit = 100 * 1024 * 1024;
	gc_config.debug_fatal = true;
	gc_config.debug_os = false;
	gc_config.debug_page = false;
	gc_config.debug_alloc = false;
	gc_config.debug_mark = false;
	gc_config.debug_sweep = false;
	gc_config.debug_other = false;
	gc_config.debug_dump = true;
	gc_config.dump_file = fopen("gc.dump", "w");
}

GC_STATIC void gc_deinit(void) {
	hl_mutex_free(gc_mutex_roots);
	hl_mutex_free(gc_mutex_pool);
	while (gc_last_allocated_page != NULL) {
		gc_free_page_memory(gc_last_allocated_page);
	}
	while (gc_last_allocated_huge_page != NULL) {
		gc_free_page_memory(gc_last_allocated_huge_page);
	}
	free(gc_page_hash.buckets);
	free(gc_mark_stack.data);
}

void hl_cache_free();
void hl_cache_init();

void hl_global_init() {
	gc_init();
	hl_cache_init();
}

void hl_global_free() {
	gc_deinit();
	hl_cache_free();
}
