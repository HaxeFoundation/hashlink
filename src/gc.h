#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/mman.h>

// OS page management ----------------------------------------------

// allocates a page-aligned region of memory from the OS
static void *gc_alloc_os_memory(int size);
// returns a region of memory back to the OS
static void gc_free_os_memory(void *ptr, int size);

// GC configuration -------------------------------------------------

//   4 MiB pages
//  64 KiB blocks (64 blocks per page)
// 128   B lines (8KiB header + 448 lines per block)
#define GC_PAGE_SIZE  (1 << 22)
#define GC_BLOCK_SIZE (1 << 16)
#define GC_LINE_SIZE  (1 << 7)

#define GC_BLOCKS_PER_PAGE 64
#define GC_LINES_PER_BLOCK 448

// up to 8184 (~25% of block size) for medium objects
#define GC_MEDIUM_SIZE ((1 << 13) - 8)

// TODO: check collisions
#define GC_HASH(ptr) (((int_val)(ptr) >> 22) ^ (((int_val)(ptr) >> 22) << 5))

#define GC_STATIC

// GC data structures -----------------------------------------------

typedef enum {
	GC_PAGE_NORMAL = 1,
	GC_PAGE_HUGE
} gc_page_kind_t;

typedef struct gc_page_header_s {
	struct gc_page_header_s *next_page; // next page in allocation order
	struct gc_page_header_s *next_page_bucket; // next page in page hash bucket
	unsigned char block_bmp[8]; // used blocks (for sweeping)
	int size;
	unsigned char free_blocks; // TODO: not really used
	unsigned char kind;
} gc_page_header_t;

typedef struct {
	gc_page_header_t **buckets;
	int bucket_count;
	int total_pages;
} gc_page_hash_t;

typedef struct {
	unsigned char data[GC_LINE_SIZE];
} gc_line_t;

typedef enum {
	// free: next refers to next free block (SLL)
	GC_BLOCK_FREE = 1,
	// new: no pointers
	GC_BLOCK_NEW,
	// full: prev/next refer to blocks in thread's full_blocks (DLL)
	GC_BLOCK_FULL,
	// recycled: prev/next refer to blocks in thread's recyclable_blocks (DLL)
	GC_BLOCK_RECYCLED,
	// TODO: zombie blocks
	GC_BLOCK_ZOMBIE,
	_force_int = 0x7FFFFFFF
} gc_block_type_t;

typedef union {
	struct {
		// set to mark polarity during mark phase
		uint8_t marked : 1;
		// whether the object spans multiple lines
		uint8_t medium_sized : 1;
		// whether the object is on a page of its own
		// uint8_t huge_sized : 1;
		// when set, object cannot be moved
		// uint8_t pinned : 1;
		// whether all words of the object are potentially pointers
		// (set for dynamics and pointer arrays)
		uint8_t dynamic_mark : 1;
		// when set, no pointers are traced in the object
		uint8_t no_ptr : 1;
		// number of words (sizeof(void *))
		uint8_t words : 4;
	};
	uint8_t flags;
} gc_metadata_t;

typedef union {
	struct {
		gc_metadata_t base;
		uint8_t words_ext;
	};
	uint16_t flags;
} gc_metadata_ext_t;

typedef struct gc_block_header_s {
	gc_page_header_t page_header; // only set in the first block of page
	struct gc_block_header_s *prev; // meaning of prev/next depends on kind
	struct gc_block_header_s *next;
	gc_block_type_t kind;
	int owner_thread;
	int huge_sized;
	unsigned char _pad[4];
	unsigned char line_marks[GC_LINES_PER_BLOCK];
	gc_metadata_t metadata[GC_LINES_PER_BLOCK * 16];
	unsigned char _pad2[512];
	gc_line_t lines[GC_LINES_PER_BLOCK];
} gc_block_header_t;

typedef struct gc_block_dummy_s {
	unsigned char _pad[32];
	gc_block_header_t *prev; // must align with gc_block_header_t
	gc_block_header_t *next;
} gc_block_dummy_t;

typedef struct {
	// bool gc_blocking;
	void *lines_start;
	void *lines_limit;
	gc_block_dummy_t *sentinels;
	gc_block_header_t *lines_block; // active block for new allocations
	gc_block_header_t *full_blocks; // DLL of all full blocks, may become recycled
	gc_block_header_t *recyclable_blocks; // DLL of recyclable blocks
} gc_thread_info_t;

typedef struct {
	hl_type *t;
} gc_object_t;

typedef struct {
	gc_object_t header;
	void *data[];
} gc_object_hl_t;

typedef struct {
	gc_object_t header;
	hl_type *et;
	int size;
	int _pad;
	void *elements[];
} gc_array_t;

typedef struct {
	void *object;
	void *reference;
} gc_mark_stack_entry_t;

typedef struct {
	gc_mark_stack_entry_t *data;
	int pos;
	int capacity;
} gc_mark_stack_t;

typedef struct {
	unsigned long live_objects;
	unsigned long live_blocks;
	unsigned long total_memory;
	unsigned long cycles;
	int total_pages;
} gc_stats_t;

typedef struct {
	unsigned long memory_limit;
	bool debug_fatal;
	bool debug_os;
	bool debug_page;
	bool debug_alloc;
	bool debug_mark;
	bool debug_sweep;
	bool debug_other;
	bool debug_dump;
	FILE *dump_file;
} gc_config_t;

// GC page management ----------------------------------------------

// allocates and initialises a GC page
GC_STATIC gc_page_header_t *gc_alloc_page_memory(void);
// frees a GC page
GC_STATIC void gc_free_page_memory(gc_page_header_t *header);

// gets the `block`th block in `page`
#define GC_PAGE_BLOCK(page, block) ((gc_block_header_t *)((char *)(page) + (block) * GC_BLOCK_SIZE))
// gets the page to which `block` belongs
#define GC_BLOCK_PAGE(block) ((gc_page_header_t *)((int_val)(block) & (int_val)(~(GC_PAGE_SIZE - 1))))
// gets the block to which `line` belongs
#define GC_LINE_BLOCK(line) ((gc_block_header_t *)((int_val)(line) & (int_val)(~(GC_BLOCK_SIZE - 1))))
// gets the index of `block` in its page
#define GC_BLOCK_ID(block) ((int)(((int_val)(block) - (int_val)GC_BLOCK_PAGE(block)) / GC_BLOCK_SIZE))
// gets the line index of `ptr` in its block
#define GC_LINE_ID(ptr) ((int)((int_val)(ptr) - (int_val)GC_LINE_BLOCK(ptr)) / GC_LINE_SIZE - 64)

#define GC_METADATA(obj) (&(GC_LINE_BLOCK(obj)->metadata[((int_val)(obj) - (int_val)GC_LINE_BLOCK(obj)) / 8 - 1024]))

// roots -----------------------------------------------------------

HL_PRIM void hl_add_root(void **);
HL_PRIM void hl_remove_root(void **);

// thread-local allocator <-> global GC
GC_STATIC gc_block_header_t *gc_pop_block(void); // size fixed to 4 MiB
GC_STATIC void gc_push_block(gc_block_header_t *);
GC_STATIC void *gc_pop_huge(int size);
GC_STATIC void gc_push_huge(void *);

// thread setup
HL_API int hl_thread_id(void);
HL_API void hl_register_thread(void *stack_top);
HL_API void hl_unregister_thread(void);

// thread <-> thread-local allocator

HL_API void *hl_gc_alloc_gen(hl_type *t, int size, int flags);

HL_API void hl_gc_major(void);

// GC internals
GC_STATIC void gc_stop_world(bool);

GC_STATIC void gc_mark(void);
GC_STATIC void gc_sweep(void); // or collect, compact, copy, etc

// marking and sweeping
GC_STATIC gc_block_header_t *gc_get_block(void *); // NULL if not a GC pointer

// GC debug

#if GC_ENABLE_DEBUG
#	define GC_DEBUG(stream, f, ...) do { if (gc_config.debug_ ## stream) printf("[" #stream "] " f "\n", ##__VA_ARGS__); } while (0)
#	define GC_DEBUG_DUMP_S(id) do { if (gc_config.debug_dump) { fwrite((id), 1, strlen(id) + 1, gc_config.dump_file); fflush(gc_config.dump_file); } } while (0)
#	define GC_DEBUG_DUMP_R(arg) do { if (gc_config.debug_dump) { fwrite(&(arg), 1, sizeof(arg), gc_config.dump_file); fflush(gc_config.dump_file); } } while (0)
#	define GC_DEBUG_DUMP0(id) do { GC_DEBUG_DUMP_S(id); int s = 0; GC_DEBUG_DUMP_R(s); } while (0)
#	define GC_DEBUG_DUMP1(id, arg1) do { GC_DEBUG_DUMP_S(id); int s = sizeof(arg1); GC_DEBUG_DUMP_R(s); GC_DEBUG_DUMP_R(arg1); } while (0)
#	define GC_DEBUG_DUMP2(id, arg1, arg2) do { GC_DEBUG_DUMP_S(id); int s = sizeof(arg1) + sizeof(arg2); GC_DEBUG_DUMP_R(s); GC_DEBUG_DUMP_R(arg1); GC_DEBUG_DUMP_R(arg2); } while (0)
#	define GC_DEBUG_DUMP3(id, arg1, arg2, arg3) do { GC_DEBUG_DUMP_S(id); int s = sizeof(arg1) + sizeof(arg2) + sizeof(arg3); GC_DEBUG_DUMP_R(s); GC_DEBUG_DUMP_R(arg1); GC_DEBUG_DUMP_R(arg2); GC_DEBUG_DUMP_R(arg3); } while (0)
#else
#	define GC_DEBUG(...)
#	define GC_DEBUG_DUMP_S(...)
#	define GC_DEBUG_DUMP_R(...)
#	define GC_DEBUG_DUMP0(...)
#	define GC_DEBUG_DUMP1(...)
#	define GC_DEBUG_DUMP2(...)
#	define GC_DEBUG_DUMP3(...)
#endif
#define GC_FATAL(msg) do { puts(msg); __builtin_trap(); } while (0)

/*
GC_STATIC void gc_debug_block(gc_block_header_t *block);
GC_STATIC void gc_debug_general(void);
GC_STATIC void gc_dump(const char *);
*/

// GC initialisation

GC_STATIC gc_stats_t *gc_init(void);
GC_STATIC void gc_deinit(void);
