
typedef unsigned short fl_cursor;

typedef struct {
	fl_cursor pos;
	fl_cursor count;
} gc_fl;

typedef struct _gc_freelist {
	int current;
	int count;
	int size_bits;
	gc_fl *data;
} gc_freelist;

#define SIZES_PADDING 8

typedef struct {
	int block_size;
	unsigned char size_bits;
	unsigned char need_flush;
	short first_block;
	int max_blocks;
	// mutable
	gc_freelist free;
	unsigned char *sizes;
	char sizes_ref[SIZES_PADDING];
#	ifdef HL_THREADS
	int tlocal_owner; // thread_id of the thread that owns this page for fast-path allocation, 0 = unowned
#	endif
} gc_allocator_page_data;

