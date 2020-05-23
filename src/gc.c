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

// utilities --------------------------------------------------------

// branch optimisation
#define LIKELY(c) __builtin_expect((c), 1)
#define UNLIKELY(c) __builtin_expect((c), 0)

// bit access
#define GET_BIT(bmp, bit) ((bmp)[(bit) >> 3] & (1 << ((bit) & 7)))
#define GET_BIT32(bmp, bit) ((bmp)[(bit) >> 5] & (1 << ((bit) & 31)))
#define SET_BIT0(bmp, bit) (bmp)[(bit) >> 3] &= 0xFF ^ (1 << ((bit) & 7));
#define SET_BIT1(bmp, bit) (bmp)[(bit) >> 3] |= 1 << ((bit) & 7);
#define SET_BIT(bmp, bit, val) (bmp)[(bit) >> 3] = ((bmp)[(bit) >> 3] & (0xFF ^ (1 << ((bit) & 7)))) | ((val) << ((bit) & 7));

// singly-linked lists
#define SLL_POP_P(obj, list, next_field) do { \
		GC_ASSERT(list != NULL); \
		(obj) = (list); \
		(list) = (obj)->next_field; \
		(obj)->next_field = NULL; \
	} while (0)
#define SLL_POP(obj, list) SLL_POP_P(obj, list, next)

#define SLL_PUSH_P(obj, list, next_field) do { \
		(obj)->next_field = list; \
		list = (obj); \
	} while (0)
#define SLL_PUSH(obj, list) SLL_PUSH_P(obj, list, next)

// doubly-linked lists
#define DLL_UNLINK(obj) do { \
		GC_ASSERT((obj) != NULL); \
		GC_ASSERT_M((obj)->next != NULL, "%p", (obj)); \
		GC_ASSERT_M((obj)->prev != NULL, "%p", (obj)); \
		(obj)->next->prev = (obj)->prev; \
		(obj)->prev->next = (obj)->next; \
		(obj)->next = NULL; \
		(obj)->prev = NULL; \
	} while (0)
#define DLL_INSERT(obj, head) do { \
		GC_ASSERT((obj) != NULL); \
		GC_ASSERT((head) != NULL); \
		(obj)->next = (head)->next; \
		(obj)->prev = (head); \
		(head)->next = (obj); \
		(obj)->next->prev = (obj); \
	} while (0)

// state structs

static gc_stats_t *gc_stats;
static gc_config_t *gc_config;

static bool gc_blocked;

// OS page management ----------------------------------------------

static void *base_addr;
static int page_counter = 0;
static int huge_counter = 0;
static gc_page_header_t *tracked_page_addr[500] = {0};
static gc_page_header_t *tracked_huge_addr[500] = {0};

#define ID_TRACK(page_num, low) \
	(gc_object_t *)((int_val)tracked_page_addr[page_num] | ((int_val)low & 0x3FFFFF))

static FILE *dump_file;
/*void dump_live(void) {
	fwrite(&(gc_stats->live_memory), 8, 1, dump_file);
}*/
#define dump_live(...)

// allocates a page-aligned region of memory from the OS
GC_STATIC void *gc_alloc_os_memory(int size) {
	GC_DEBUG_DUMP1("gc_alloc_os_memory.enter", size);
	/*
	if (gc_stats->total_memory + size >= gc_config->memory_limit) {
		GC_DEBUG_DUMP0("gc_alloc_os_memory.fail.oom");
		GC_DEBUG(fatal, "using %lu / %lu bytes, need %d more", gc_stats->total_memory, gc_config->memory_limit, size);
		GC_FATAL("OOM: memory limit hit");
	}
	*/
	void *ptr = mmap(base_addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == (void *)-1) {
		GC_DEBUG_DUMP0("gc_alloc_os_memory.fail.other");
		GC_FATAL("failed to allocate page");
	}
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
	gc_stats->total_memory += size;
	base_addr = (char *)ptr + size;
	// align up
	base_addr = (void *)((((int_val)base_addr) & ~(GC_PAGE_SIZE - 1)) + GC_PAGE_SIZE);
//printf("PAGE ALLOC %d: %p (%d bytes)\n", page_counter++, ptr, size);
	GC_DEBUG_DUMP2("gc_alloc_os_memory.success", ptr, size);
	GC_DEBUG(os, "allocated OS page: %p", ptr);
	return ptr;
}

// returns a region of memory back to the OS
GC_STATIC void gc_free_os_memory(void *ptr, int size) {
	munmap(ptr, size);
	gc_stats->total_memory -= size;
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
	if ((char *)header + header->size > (char *)gc_max_allocated) {
		gc_max_allocated = (char *)header + header->size;
	}

	gc_stats->total_pages++;
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

	gc_stats->total_pages_normal++;
	gc_add_page(header);

if (page_counter < 500)
	tracked_page_addr[page_counter++] = header;

	gc_block_header_t *blocks = GC_PAGE_BLOCK(header, 0);
	for (int i = 0; i < GC_BLOCKS_PER_PAGE; i++) {
		blocks[i].kind = GC_BLOCK_FREE;
		blocks[i].next = &blocks[i + 1];
		blocks[i].debug_objects = (char *)calloc(1, 896);
		if (blocks[i].debug_objects == NULL)
			GC_FATAL("cannot alloc debug bitmap?");
	}
	hl_mutex_acquire(gc_mutex_pool);
	blocks[GC_BLOCKS_PER_PAGE - 1].next = gc_pool;
	gc_pool = &blocks[0];
	gc_pool_count += GC_BLOCKS_PER_PAGE;
	hl_mutex_release(gc_mutex_pool);
gc_debug_verify_pool("page");

	GC_DEBUG_DUMP1("gc_alloc_page_normal.success", header);
	return header;
}

// allocates a huge page (containing a single object)
GC_STATIC gc_page_header_t *gc_alloc_page_huge(int size) {
gc_debug_verify_pool("huge before");

	gc_page_header_t *header = (gc_page_header_t *)gc_alloc_os_memory(sizeof(gc_page_header_t) + size);
	if (header == NULL) {
		GC_FATAL("OOM: huge page");
	}

	header->size = sizeof(gc_page_header_t) + size;
	header->kind = GC_PAGE_HUGE;
	header->next_page = gc_last_allocated_huge_page;
	gc_last_allocated_huge_page = header;

	gc_add_page(header);

if (huge_counter < 500)
	tracked_huge_addr[huge_counter++] = header;

gc_debug_verify_pool("huge after");

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
	for (gc_page_header_t *cur = *last; cur != NULL; cur = cur->next_page_bucket) {
		if (cur == header) {
			*last = cur->next_page_bucket;
			break;
		}
		last = &cur->next_page_bucket;
	}
	// TODO: maybe rearrange if load factor is low?
	gc_page_hash.total_pages--;
	gc_stats->total_pages--;
	gc_free_os_memory(header, header->size);
	GC_DEBUG_DUMP1("gc_free_page_memory.success", header);
}

GC_STATIC void gc_grow_heap(int count) {
//printf("GROWTH %d\n", count);
	for (int i = 0; i < count; i++) {
		gc_alloc_page_normal();
	}
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

	gc_block_header_t *block = gc_pop_block();
	t->lines_block = block;
	t->lines_start = &block->lines[0];
	t->lines_limit = &block->lines[GC_LINES_PER_BLOCK];

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
// TODO: recycle list is only touched in alloc?
GC_STATIC gc_block_header_t *gc_pop_block(void) {
gc_debug_verify_pool("pop before");
	hl_mutex_acquire(gc_mutex_pool); // TODO: mutexes need to be re-entrant, or lock later
	if (UNLIKELY(gc_pool_count == 0)) {
		// if (gc_stats->total_memory * 1.2 >= gc_config->memory_limit) {
		if (gc_stats->live_blocks > 0) {
			GC_DEBUG(alloc, "using %lu / %lu bytes, triggering major ...", gc_stats->total_memory, gc_config->memory_limit);
			hl_gc_major();
		}
		if (gc_pool_count == 0 && gc_alloc_page_normal() == NULL) {
			GC_FATAL("OOM: page memory");
		}
	}
	gc_block_header_t *block;
	SLL_POP(block, gc_pool);
	gc_page_header_t *page = GC_BLOCK_PAGE(block);
	// TODO: mutex per page?
	page->free_blocks--;
	SET_BIT1(page->block_bmp, GC_BLOCK_ID(block));
	gc_pool_count--;
	hl_mutex_release(gc_mutex_pool);
	block->kind = GC_BLOCK_BRAND_NEW;
	block->owner_thread = hl_thread_id();
	GC_DEBUG(alloc, "got block %p", block);
	GC_DEBUG_DUMP1("gc_pop_block.success", block);
	gc_stats->live_blocks++;
gc_debug_verify_pool("pop after");
	GC_ASSERT(block != NULL);
	return block;
}

// returns a thread-owned block to the global free pool
GC_STATIC void gc_push_block(gc_block_header_t *block) {
	GC_ASSERT(block != NULL);
	GC_ASSERT(block->kind == GC_BLOCK_FULL);
	block->kind = GC_BLOCK_FREE; // TODO: zombie ?
	block->owner_thread = -1;
	memset(block->line_marks, 0, GC_LINES_PER_BLOCK);
	gc_page_header_t *page = GC_BLOCK_PAGE(block);
	// TODO: mutex per page?
	hl_mutex_acquire(gc_mutex_pool);
	page->free_blocks++;
	SET_BIT0(page->block_bmp, GC_BLOCK_ID(block));
	// TODO: manage disorder in GC pool (prefer address order)
	SLL_PUSH(block, gc_pool);
	gc_pool_count++;
	gc_stats->live_blocks--;
	hl_mutex_release(gc_mutex_pool);
gc_debug_verify_pool("push");
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
	GC_ASSERT(huge != NULL);

	gc_page_header_t *header = GC_BLOCK_PAGE(huge);
	GC_DEBUG(alloc, "freed huge %p", huge);
	GC_DEBUG_DUMP1("gc_push_huge.success", header);
	gc_free_page_memory(header);
}

// thread <-> thread-local allocator -------------------------------

// bump allocate an object with the current bump cursor
// should only be called when there is definitely enough space to allocate
// the object
// this also initialises the object header
GC_STATIC gc_object_t *gc_alloc_bump(hl_type *t, int size, int words, int flags) {
	GC_ASSERT(words > 0);
	GC_ASSERT(size > 0);

	gc_object_t *ret = (gc_object_t *)current_thread->lines_start;
	gc_block_header_t *block = GC_LINE_BLOCK(ret);
	gc_metadata_t *meta = GC_METADATA(ret);

	// clear stale data
	memset(ret, 0, sizeof(void *) * words);

	// initialise type and metadata
	ret->t = t;
	meta->flags = 0;
	meta->marked = !gc_mark_polarity;
	int sub_words = words - 1;
	meta->words = sub_words & 15;
	if (flags & MEM_KIND_RAW) {
		meta->raw = 1;
	}
	if (flags & MEM_KIND_NOPTR) {
		meta->no_ptr = 1;
	}
	if (UNLIKELY(size > GC_LINE_SIZE)) {
		meta->medium_sized = 1;
		GC_METADATA_EXT(ret) = sub_words >> 4;
	}

	// bump cursor
	current_thread->lines_start = (void *)(((char *)current_thread->lines_start) + size);

	GC_DEBUG_DUMP3("gc_alloc_bump.success", ret, size, flags);
	gc_stats->live_memory += size;
	gc_stats->live_memory_normal += size;
	dump_live();

	int obj_id = ((int_val)ret - (int_val)&block->lines[0]) / 8;
//	if (GET_BIT(block->debug_objects, obj_id)) {
//printf("%p (block %p, size %d, obj %d)\n", ret, block, size, obj_id);
//gc_debug_block(block);
//		GC_FATAL("overriding existing object");
//	}
	SET_BIT1(block->debug_objects, obj_id);

	return ret;
}

// finds the next line gap in the active block of the current thread
// returns true if a gap was found
GC_STATIC bool gc_find_gap(void) {
	GC_ASSERT(
		current_thread->lines_block->kind == GC_BLOCK_BRAND_NEW
		|| current_thread->lines_block->kind == GC_BLOCK_NEW
		|| current_thread->lines_block->kind == GC_BLOCK_RECYCLED
		);

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
	GC_ASSERT(!gc_blocked);

	if (size < sizeof(void *)) {
		size = sizeof(void *);
	}

	// align to words
	int words = (size + (sizeof(void *) - 1)) / sizeof(void *);

	size = words * sizeof(void *);

	if (LIKELY((char *)current_thread->lines_start + size <= (char *)current_thread->lines_limit)) {
		return gc_alloc_bump(t, size, words, flags);
	}

	// align start to next cache line (for gc_find_gap)
	current_thread->lines_start = (int_val)((char *)current_thread->lines_start + 127) & 0xFFFFFFFFFFFFFF80;

	// TODO: list operations should not be interrupted by the GC, mutex per list

	if (LIKELY(size <= GC_LINE_SIZE)) {
		// find next line gap (based on mark bits from previous collection) in current block
		if (gc_find_gap()) {
			return gc_alloc_bump(t, size, words, flags);
		}

		// add current block to full blocks
		current_thread->lines_block->kind = GC_BLOCK_FULL;
		DLL_INSERT(current_thread->lines_block, current_thread->full_blocks);
		gc_debug_verify_pool("full insert small");

		// no gap found, get next recyclable block
		if (current_thread->recyclable_blocks->next->next != NULL) {
			current_thread->lines_block = current_thread->recyclable_blocks->next;
			current_thread->lines_start = &current_thread->lines_block->lines[0];
			DLL_UNLINK(current_thread->lines_block);
			current_thread->lines_block->kind = GC_BLOCK_NEW;
			if (gc_find_gap()) {
				return gc_alloc_bump(t, size, words, flags);
			}
			gc_debug_block(current_thread->lines_block);
			GC_FATAL("unreachable");
		}

		// if none, get fresh block
		current_thread->lines_block = gc_pop_block();
		current_thread->lines_start = &current_thread->lines_block->lines[0];
		current_thread->lines_limit = &current_thread->lines_block->lines[GC_LINES_PER_BLOCK];
		return gc_alloc_bump(t, size, words, flags);
	} else if (size <= GC_MEDIUM_SIZE) {
		current_thread->lines_block->kind = GC_BLOCK_FULL;
		DLL_INSERT(current_thread->lines_block, current_thread->full_blocks);
		gc_debug_verify_pool("full insert medium");

		// get empty block to avoid expensive search
		current_thread->lines_block = gc_pop_block();
		current_thread->lines_start = &current_thread->lines_block->lines[0];
		current_thread->lines_limit = &current_thread->lines_block->lines[GC_LINES_PER_BLOCK];
		return gc_alloc_bump(t, size, words, flags);
	} else {
		// TODO: (statically) separate path for huge objects
		if ((flags & MEM_KIND_FINALIZER) == MEM_KIND_FINALIZER) {
			// TODO: huge finalizer objects
			GC_FATAL("cannot alloc huge finalizer");
		}
		gc_object_t *obj = gc_pop_huge(size);
		obj->t = t;
		gc_metadata_t *meta = GC_METADATA(obj);
		meta->flags = 0;
		meta->marked = !gc_mark_polarity;
		if (flags & MEM_KIND_RAW) {
			meta->raw = 1;
		}
		if (flags & MEM_KIND_NOPTR) {
			meta->no_ptr = 1;
		}
		gc_stats->live_memory += size;
		dump_live();
		return obj;
	}
}

HL_API void *hl_gc_alloc_finalizer(int size) {
	// align to cache lines
	int words = (size + (sizeof(void *) - 1)) / sizeof(void *);
	words = (((size + 127) / 128) * 128) / sizeof(void *);
	current_thread->lines_start = (int_val)((char *)current_thread->lines_start + 127) & 0xFFFFFFFFFFFFFF80;
	size = words * sizeof(void *);

	void *ret = hl_gc_alloc_gen(&hlt_abstract, size, MEM_KIND_FINALIZER);

	// mark as finaliser
	gc_block_header_t *block = GC_LINE_BLOCK(ret);
	block->line_finalize[GC_LINE_ID(ret)] = 1;

	return ret;
}

#ifdef __clang__
// this only works with clang
static void **get_stack_bottom(void) {
	void *x;
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wreturn-stack-address"
	return &x;
	#pragma clang diagnostic pop
}
#endif

// spills registers and triggers a major GC collection
HL_API void hl_gc_major(void) {
	puts("major cycle...");
	// TODO: figure this out for the MT case (see master branch)
	//gc_stop_world(true);
	jmp_buf env;
	setjmp(env);
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wincompatible-pointer-types"
	void *(*volatile f)(void) = get_stack_bottom;
	#pragma clang diagnostic pop
	current_thread->stack_cur = f();
#else
	current_thread->stack_cur = __builtin_frame_address(0);
#endif
	GC_DEBUG_DUMP0("hl_gc_major.entry");
	gc_mark();
	gc_sweep();
	// gc_stop_world(false);
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
			gc_mark_stack_next.pos = 0;
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
	// void *ref = gc_mark_stack.data[gc_mark_stack.pos].reference;
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

	// reset stats
	gc_stats->live_objects = 0;
	gc_stats->live_memory = 0;
	gc_stats->live_memory_normal = 0;

	// reset line marks
	// TODO: is there a better place to do this?
	// TODO: use a rolling 8-bit polarity, only reset every 256 cycles
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
			gc_push(*cur, NULL);
			cur++;
		}
	}

	GC_DEBUG_DUMP0("gc_mark.propagate");

	// propagate
	while (gc_mark_total-- > 0) {
		gc_stats->live_objects++;
		gc_object_t *p = gc_pop();
		gc_metadata_t *meta = GC_METADATA(p);
		gc_block_header_t *block = GC_LINE_BLOCK(p);

		// mark line(s)
		int line_id = GC_LINE_ID_IN(p, block);
		int obj_id = ((int_val)p - (int_val)&block->lines[0]) / 8;
		int words = meta->words;

		if (GC_BLOCK_PAGE(p)->kind != GC_PAGE_HUGE && !GET_BIT(block->debug_objects, obj_id)) {
//gc_debug_block(block);
//		printf("popped: %p\n", p); fflush(stdout);
//gc_debug_obj(p);
//GC_FATAL("not an existing object!");
			// TODO: these objects probably come from a gc_major triggered from
			// within gc_pop_block (or similar); in theory, interior pointers should
			// not get here - then this check and debug_objects can be removed
			printf("non-ex %p\n", p);
			continue;
		}

		if (block->huge_sized) {
			words = (GC_BLOCK_PAGE(p)->size - 8192) / 8;
			GC_DEBUG(mark, "scanning huge, %d words", words);
		} else if (meta->medium_sized) {
			words |= GC_METADATA_EXT(p) << 4;
			words++;
			int last_id = GC_LINE_ID_IN((char *)p + words * 8, block);
			int line_span = (last_id - line_id);
			GC_DEBUG(mark, "marking medium, %d lines", line_span);
			memset(&block->line_marks[line_id], 1, line_span);
		} else if (!block->huge_sized) {
			words++;
			block->line_marks[line_id] = 1;
		}

		gc_stats->live_memory_normal += words * 8;

		// scan object
		void **data = (void **)p;
		if (meta->no_ptr) {
			continue;
		} else if (meta->raw || p->t == NULL || p->t->mark_bits == NULL || p->t->kind == HFUN) {
			for (int i = 0; i < words; i++) {
				gc_push(data[i], p); // &data[i]);
			}
		} else {
			int pos = 0;
			int *mark_bits = p->t->mark_bits;
			if (p->t->kind == HENUM) {
				venum *e = (venum *)p;
				mark_bits = p->t->mark_bits + e->index;
				data += 2;
				words -= 2;
			} else {
				data++;
				pos++;
			}
			for (; pos < words; pos++) {
				if (GET_BIT32(mark_bits, pos)) {
					gc_push(*data, p); // &data[i]);
				}
				data++;
			}
		}
	}

	gc_stats->live_memory += gc_stats->live_memory_normal;

	GC_DEBUG_DUMP0("gc_mark.cleanup");
	dump_live();

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
	// - finalise unmarked lines
	// - if compactable and fragmented, try compacting
	// - if empty, move into global free pool
	// - if not full but in full list, move into thread's recycle list

	// normal pages with blocks
	for (gc_page_header_t *page = gc_last_allocated_page; page != NULL; page = page->next_page) {
		for (int block_id = 0; block_id < GC_BLOCKS_PER_PAGE; block_id++) {
			if (GET_BIT(page->block_bmp, block_id)) {
				gc_block_header_t *block = GC_PAGE_BLOCK(page, block_id);
				int lines_used = (block->line_marks[0] ? 1 : 0);
				for (int line_id = 0; line_id < GC_LINES_PER_BLOCK; line_id++) {
					gc_object_t *p = (gc_object_t *)&block->lines[line_id];
					gc_metadata_t *meta = &block->metadata[line_id * 16];
					if (!block->line_marks[line_id] && block->line_finalize[line_id]) {
						void **f = p;
						void (*finalize)(void *) = *((void (**)(void *))p);
						if (finalize != NULL) {
							finalize((void *)&block->lines[line_id]);
							*f = NULL;
						}
						block->line_finalize[line_id] = 0;
					}
					if (!block->line_marks[line_id] && block->debug_objects) {
						for (int i = 0; i < 16; i++) {
							int obj_id = line_id * 16 + i;
							SET_BIT0(block->debug_objects, obj_id);
						}
					}
					if (line_id == 0)
						continue;
					if (block->line_marks[line_id - 1] || block->line_marks[line_id]) {
						lines_used++;
					}
				}
				GC_DEBUG_DUMP2("gc_sweep.block", block, block->line_marks);
				GC_DEBUG(sweep, "block %p uses %d lines", block, lines_used);
				if (block->kind == GC_BLOCK_FULL) {
					if (lines_used == 0) {
						DLL_UNLINK(block);
						gc_push_block(block);
					} else if (lines_used < GC_LINES_PER_BLOCK) {
						DLL_UNLINK(block);
						block->kind = GC_BLOCK_RECYCLED;
						// TODO: get thread by block->owner_thread
						DLL_INSERT(block, current_thread->recyclable_blocks);
					}
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
			gc_push_huge(obj);
		}
	}

	// change polarity for the next cycle
	gc_mark_polarity = 1 - gc_mark_polarity;

	// grow heap if needed
//printf("live memory normal: %lu\n", gc_stats->live_memory_normal);
//printf("total pages normal: %lu\n", gc_stats->total_pages_normal);
	if (gc_stats->total_pages_normal < 90 && (float)(gc_stats->live_memory_normal / GC_PAGE_SIZE) > (float)gc_stats->total_pages_normal * gc_config->min_grow_usage) {
		int growth = (int)((float)gc_stats->total_pages_normal * gc_config->heap_growth);
		gc_grow_heap(growth);
	}

	gc_stats->cycles++;
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
	// TODO: threads
	gc_blocked = b;
}

HL_API hl_thread_info *hl_get_thread() {
	return current_thread;
}

HL_API bool hl_is_gc_ptr(void *ptr) {
	return (gc_get_block(ptr) != NULL);
}

HL_API vdynamic *hl_alloc_dynamic( hl_type *t ) {
	return (vdynamic*)hl_gc_alloc_gen(t, sizeof(vdynamic), hl_is_ptr(t) ? MEM_KIND_DYNAMIC : MEM_KIND_NOPTR);
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
		o = (vobj*)hl_gc_alloc_gen(NULL, size, rt->hasPtr ? MEM_KIND_RAW : MEM_KIND_NOPTR);
	} else {
		o = (vobj*)hl_gc_alloc_gen(t, size, rt->hasPtr ? MEM_KIND_DYNAMIC : MEM_KIND_NOPTR);
	}
	for(i=0;i<rt->nbindings;i++) {
		hl_runtime_binding *b = rt->bindings + i;
		*(void**)(((char*)o) + rt->fields_indexes[b->fid]) = b->closure ? hl_alloc_closure_ptr(b->closure,b->ptr,o) : b->ptr;
	}
	return (vdynamic*)o;
}

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
	return (vdynobj*)hl_gc_alloc_gen(&hlt_dynobj,sizeof(vdynobj), MEM_KIND_DYNAMIC);
}

// alloc functions defined elsewhere:
//   varray *hl_alloc_array(hl_type *t, int size);
//   venum *hl_alloc_enum(hl_type *t, int index);
//   vbyte *hl_alloc_bytes(int size);

static hl_types_dump gc_types_dump = NULL;
HL_API void hl_gc_set_dump_types( hl_types_dump tdump ) {
	gc_types_dump = tdump;
}

// GC initialisation -----------------------------------------------

#include <signal.h>
GC_STATIC void gc_handle_signal(int signum) {
	signal(signum, SIG_DFL);
	printf("SIGNAL %d\n", signum);
	// hl_dump_stack();
	gc_debug_interactive();
	raise(signum);
}

GC_STATIC gc_stats_t *gc_init(void) {
	puts("new GC init...");
	gc_mutex_roots = hl_mutex_alloc(false);
	gc_mutex_pool = hl_mutex_alloc(false);
	gc_blocked = false;

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
	gc_stats = (gc_stats_t *)calloc(sizeof(gc_stats_t), 1);
	gc_stats->live_objects = 0;
	gc_stats->live_blocks = 0;
	gc_stats->total_memory = 0;
	gc_stats->cycles = 0;
	gc_stats->live_memory = 0;
	gc_stats->live_memory_normal = 0;
	gc_stats->total_pages = 0;
	gc_stats->total_pages_normal = 0;
	gc_config = (gc_config_t *)calloc(sizeof(gc_config_t), 1);
	gc_config->min_heap_pages = 10;
	gc_config->memory_limit = 500 * 1024 * 1024;
	gc_config->heap_growth = 0.35f;
	gc_config->min_grow_usage = 0.2f;
	gc_config->debug_fatal = true;
	gc_config->debug_os = false;
	gc_config->debug_page = false;
	gc_config->debug_alloc = false;
	gc_config->debug_mark = false;
	gc_config->debug_sweep = false;
	gc_config->debug_other = false;
	gc_config->debug_dump = false;
	gc_config->dump_file = fopen("gc.dump", "w");
	//dump_file = fopen("gc.live", "w");
	gc_grow_heap(gc_config->min_heap_pages);

	//struct sigaction act;
	//act.sa_sigaction = NULL;
	//act.sa_handler = gc_handle_signal;
	//act.sa_flags = 0;
	//sigemptyset(&act.sa_mask);
	//sigaction(SIGILL,&act,NULL);
	//sigaction(SIGSEGV,&act,NULL);

	return gc_stats;
}

GC_STATIC void gc_deinit(void) {
	hl_mutex_free(gc_mutex_roots);
	hl_mutex_free(gc_mutex_pool);
	gc_page_header_t *page = gc_last_allocated_page;
	gc_page_header_t *next = NULL;
	while (page != NULL) {
		next = page->next_page;
		gc_free_os_memory(page, page->size);
		page = next;
	}
	page = gc_last_allocated_huge_page;
	while (page != NULL) {
		next = page->next_page;
		gc_free_os_memory(page, page->size);
		page = next;
	}
	free(gc_page_hash.buckets);
	free(gc_mark_stack.data);
	free(gc_stats);
	free(gc_config);
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

#if GC_ENABLE_DEBUG

void gc_debug_block(gc_block_header_t *block) {
	static char *kind_names[] = {
		"(not initialised)",
		"free",
		"new",
		"full",
		"recycled",
		"zombie",
		"brand new"
	};
	if (block == NULL) {
		puts("block: NULL");
		return;
	}
	printf("block: %p\n", block);
	gc_page_header_t *page = (gc_page_header_t *)block;
	printf("  (P) next_page: %p\n", page->next_page);
	printf("  (P) next_page_bucket: %p\n", page->next_page_bucket);
	printf("  (P) block_bmp: %lx\n", *((long int *)(&page->block_bmp)));
	printf("  (P) size: %d\n", page->size);
	printf("  (P) free_blocks: %d\n", (int)page->free_blocks);
	printf("  (P) kind: %d\n", (int)page->kind);
	printf("  (B) prev: %p\n", block->prev);
	printf("  (B) next: %p\n", block->next);
	printf("  (B) kind: %s\n", kind_names[block->kind]);
	printf("  (B) owner_thread: %d\n", block->owner_thread);
	printf("  (B) huge_sized: %d\n", block->huge_sized);

	bool last_used = false;
	for (int line = 0; line < GC_LINES_PER_BLOCK; line++) {
		if (line % 128 == 0)
			printf("  ");
		if (block->line_marks[line])
			printf("X");
		else
			printf(".");
		if (line % 128 == 127)
			puts("");
	}
	/*
	puts("");
	if (block->debug_objects != NULL) {
		for (int obj = 0; obj < 448 * 16; obj++) {
			if (obj % 200 == 0)
				printf(" ");
			if (GET_BIT(block->debug_objects, obj))
				printf("X");
			else
				printf(".");
		if (obj % 200 == 199)
			puts("");
		}
	}*/
	puts("");
	int lines_used = (block->line_marks[0] ? 1 : 0);
	for (int line_id = 1; line_id < GC_LINES_PER_BLOCK; line_id++) {
		if (block->line_marks[line_id - 1] || block->line_marks[line_id]) {
			lines_used++;
		}
	}
	printf("lines used: %d\n", lines_used);
}

void gc_debug_obj_id(void **p) {
	gc_block_header_t *block = GC_LINE_BLOCK(p);
	gc_page_header_t *page = GC_BLOCK_PAGE(block);
	for (int i = 0; i < page_counter && i < 500; i++) {
		if (page == tracked_page_addr[i]) {
			printf("%p ID_TRACK(%d, 0x%x)\n", p, i, (int_val)p & 0x3FFFFF); fflush(stdout);
			return;
		}
	}
	for (int i = 0; i < huge_counter && i < 500; i++) {
		if (page == tracked_huge_addr[i]) {
			printf("%p HUGE ID_TRACK(%d, 0x%x)\n", p, i, (int_val)p & 0x3FFFFF); fflush(stdout);
			return;
		}
	}
	printf("%p ID_???\n", p); fflush(stdout);
}

void gc_debug_obj(void **p) {
	gc_debug_obj_id(p);
	if (p != NULL) {
		gc_block_header_t *block = GC_LINE_BLOCK(p);
		gc_metadata_t *meta = GC_METADATA(p);
		puts("meta:");
		int real_words = meta->words;
		printf("  words:        %d (+ 1)\n", meta->words);
		printf("  marked:       %d\n", meta->marked);
		printf("  medium_sized: %d\n", meta->medium_sized);
		printf("  raw:          %d\n", meta->raw);
		printf("  no_ptr:       %d\n", meta->no_ptr);
		if (meta->medium_sized) {
			printf("  (ext) words:  %d\n", GC_METADATA_EXT(p));
			real_words |= GC_METADATA_EXT(p) << 4;
		}
		if ((int_val)p & 0xFFFFFFFFFFFFFF80 == (int_val)p) {
			printf("finalize: %d\n", block->line_finalize[GC_LINE_ID(p)]);
		}
		real_words++;
		hl_type *t = *p;
		printf("type: %p\n", t);
		/*
		void **data = (void **)p;
		for (int i = 0; i < real_words; i++) {
			printf("  - %d (at %p): %p\n", i, &data[i], data[i]);
		}
		*/
		/*if (t != NULL && t->mark_bits != NULL) {
			printf("  mark bits: %p\n", t->mark_bits);
			//if (t->mark_bits < 0x200000000) {
				for (int i = 0; i < real_words; i++) {
					if (GET_BIT32(t->mark_bits, i)) {
						printf("  - %d\n", i); //": %p\n", i, *(void **)(p[i]));
					}
				}
			//}
		}*/
		printf("block: %p\n", block);
		gc_debug_block(block);
		fflush(stdout);
	}
}

static const uchar *TSTR[] = {
	USTR("void"), USTR("i8"), USTR("i16"), USTR("i32"), USTR("i64"), USTR("f32"), USTR("f64"),
	USTR("bool"), USTR("bytes"), USTR("dynamic"), NULL, NULL,
	USTR("array"), USTR("type"), NULL, NULL, USTR("dynobj"),
	NULL, NULL, NULL, NULL, NULL
};
typedef struct tlist {
	hl_type *t;
	struct tlist *next;
} tlist;
typedef struct _stringitem {
	uchar *str;
	int size;
	int len;
	struct _stringitem *next;
} * stringitem;
struct hl_buffer {
	int totlen;
	int blen;
	stringitem data;
};
void hl_type_str_rec( hl_buffer *b, hl_type *t, tlist *parents );

void gc_debug_type(hl_type *t) {
	uchar buf_data[1024] = {0};
	struct _stringitem buf_item = {
		.str = buf_data,
		.size = 1024,
		.len = 0
	};
	struct hl_buffer buf = {
		.totlen = 0,
		.blen = 1024,
		.data = &buf_item
	};

	const uchar *c = TSTR[t->kind];
	hl_buffer *b;
	if (c != NULL) {
		uprintf(USTR("%s\n"), c);
	} else {
		hl_type_str_rec(&buf, t, NULL);
		uprintf(USTR("%s\n"), buf_data);
		buf_data[0] = 0;
		buf_item.len = 0;
		buf.totlen = 0;
	}
}

void gc_debug_verify_pool(const char *at) {
	gc_block_header_t *last, *cur;

	if (current_thread && current_thread->full_blocks) {
		last = NULL;
		cur = current_thread->full_blocks;
		GC_ASSERT(cur == &current_thread->sentinels[0]);
		while (cur != NULL) {
			GC_ASSERT(cur->prev == last);
			if (last != NULL && cur->next != NULL) {
				GC_ASSERT_M(cur->kind == GC_BLOCK_FULL, "%p (%s)", cur, at);
			}
			last = cur;
			cur = cur->next;
		}
		GC_ASSERT(last == &current_thread->sentinels[1]);
	}

	if (current_thread && current_thread->recyclable_blocks) {
		last = NULL;
		cur = current_thread->recyclable_blocks;
		GC_ASSERT(cur == &current_thread->sentinels[2]);
		while (cur != NULL) {
			GC_ASSERT(cur->prev == last);
			if (last != NULL && cur->next != NULL) {
				GC_ASSERT_M(cur->kind == GC_BLOCK_RECYCLED, "%p", cur);
			}
			last = cur;
			cur = cur->next;
		}
		GC_ASSERT(last == &current_thread->sentinels[3]);
	}

	last = NULL;
	cur = gc_pool;
	for (int i = 0; i < gc_pool_count; i++) {
		GC_ASSERT_M(cur != NULL, "found NULL (%d)", i);
		GC_ASSERT_M(cur >= gc_min_allocated && cur <= gc_max_allocated, "found unallocated (%p -> %p at %d)", last, cur, i);
		last = cur;
		cur = cur->next;
	}
	GC_ASSERT(cur == NULL);
}

void *gc_debug_track_id(int pagenum, int low) {
	return ID_TRACK(pagenum, low);
}

void gc_debug_thread(void) {
	hl_thread_info *t = current_thread;
	printf("info: %p\n", t);
	if (t != NULL) {
		printf("  thread_id:         %d\n", t->thread_id);
		printf("  gc_blocking:       %d\n", t->gc_blocking);
		printf("  stack_top:         %p\n", t->stack_top);
		printf("  stack_cur:         %p\n", t->stack_cur);
		printf("  lines_start:       %p\n", t->lines_start);
		printf("  lines_limit:       %p\n", t->lines_limit);
		printf("  sentinels:         %p\n", t->sentinels);
		printf("  lines_block:       %p\n", t->lines_block);
		printf("  full_blocks:       %p\n", t->full_blocks);
		printf("  recyclable_blocks: %p\n", t->recyclable_blocks);
	}
}

void gc_debug_interactive(void) {
	puts("gc interactive debugger (h for help)");
	bool run = true;
	while (run) {
		printf("> "); fflush(stdout);
		char cmd;
		scanf(" %c", &cmd);
		switch (cmd) {
			case 'h':
				puts("h                  help");
				puts("b <block>          debug block");
				puts("o <obj>            debug object");
				puts("O <pagenum> <low>  debug object (by ID_TRACK)");
				puts("s                  dump call stack");
				puts("t                  dump thread info");
				puts("T <type>           print type name");
				puts("q                  quit");
				break;
			case 'b': {
				gc_block_header_t *block;
				if (scanf("%lx", &block) && block != NULL) {
					gc_debug_block(block);
				}
			} break;
			case 'o': {
				gc_object_t *obj;
				if (scanf("%lx", &obj) && obj != NULL) {
					gc_debug_obj(obj);
				}
			} break;
			case 'O': {
				int pagenum;
				int low;
				if (scanf("%d %d", &pagenum, &low) && pagenum != 0 && low != 0) {
					gc_debug_obj(ID_TRACK(pagenum, low));
				}
			} break;
			case 's':
				hl_dump_stack();
				break;
			case 't':
				gc_debug_thread();
				break;
			case 'T': {
				hl_type *t;
				if (scanf("%lx", &t) && t != NULL) {
					gc_debug_type(t);
				}
			} break;
			case 'q': {
				run = false;
			} break;
			default: break;
		}
	}
}

#endif // GC_ENABLE_DEBUG

// TODO
void hl_gc_enable(bool a) {}
void hl_gc_profile(bool a) {}
void hl_gc_stats(double *a, double *b, double *c) {}
void hl_gc_dump_memory(vbyte *a) {}
int hl_gc_get_flags(void) { return 0; }
void hl_gc_set_flags(int a) {}
vdynamic *hl_debug_call(int a, vdynamic *b) { return NULL; }

DEFINE_PRIM(_VOID, gc_major, _NO_ARG);
DEFINE_PRIM(_VOID, gc_enable, _BOOL);
DEFINE_PRIM(_VOID, gc_profile, _BOOL);
DEFINE_PRIM(_VOID, gc_stats, _REF(_F64) _REF(_F64) _REF(_F64));
DEFINE_PRIM(_VOID, gc_dump_memory, _BYTES);
DEFINE_PRIM(_I32, gc_get_flags, _NO_ARG);
DEFINE_PRIM(_VOID, gc_set_flags, _I32);
DEFINE_PRIM(_DYN, debug_call, _I32 _DYN);
DEFINE_PRIM(_VOID, blocking, _BOOL);
