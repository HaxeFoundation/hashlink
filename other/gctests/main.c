#include <stdio.h>
#include <stddef.h>
#include "hl.h"
#include "gc.h"
#include "utils.h"
#include "suite.h"

// types

TEST_TYPE(one_ptr, 1, {1}, {
	void *ptr;
});
TEST_TYPE(some_ptr, 10, {1 | 4 | 64 | 128 | 512}, {
	void *ptr_0;
	void *data_1;
	void *ptr_2;
	void *data_3;
	void *data_4;
	void *data_5;
	void *ptr_6;
	void *ptr_7;
	void *data_8;
	void *ptr_9;
});
TEST_TYPE(medium_one_ptr, 256, {1}, {
	void *ptr;
	void *data[255];
});
TEST_TYPE(big_no_ptr, 10000, {0}, {
	int _dummy;
});

// test cases

BEGIN_TEST_CASE(sanity) {
	ASSERT(sizeof(hl_type *) == 8);
	ASSERT(sizeof(gc_page_header_t) == 32);
	ASSERT(sizeof(gc_metadata_t) == 1);
	ASSERT(sizeof(gc_metadata_ext_t) == 2);
	ASSERT(sizeof(gc_block_header_t) == GC_BLOCK_SIZE);
	ASSERT(offsetof(gc_block_header_t, lines) == 64 * GC_LINE_SIZE);

	ASSERT(GC_PAGE_BLOCK(0x686178650B400000ul, 0) == (gc_block_header_t *)0x686178650B400000ul);
	ASSERT(GC_PAGE_BLOCK(0x686178650B400000ul, 13) == (gc_block_header_t *)0x686178650B4D0000ul);
	ASSERT(GC_PAGE_BLOCK(0x686178650B400000ul, 63) == (gc_block_header_t *)0x686178650B7F0000ul);

	ASSERT(GC_BLOCK_ID(0x686178650B400000ul) == 0);
	ASSERT(GC_BLOCK_ID(0x686178650B4D0000ul) == 13);
	ASSERT(GC_BLOCK_ID(0x686178650B7F0000ul) == 63);

	ASSERT(GC_BLOCK_PAGE(0x686178650B400000ul) == (gc_page_header_t *)0x686178650B400000ul);
	ASSERT(GC_BLOCK_PAGE(0x686178650B400123ul) == (gc_page_header_t *)0x686178650B400000ul);
	ASSERT(GC_BLOCK_PAGE(0x686178650B4D0000ul) == (gc_page_header_t *)0x686178650B400000ul);
	ASSERT(GC_BLOCK_PAGE(0x686178650B7F0000ul) == (gc_page_header_t *)0x686178650B400000ul);

	ASSERT(GC_LINE_BLOCK(0x686178650B4D0000ul) == (gc_block_header_t *)0x686178650B4D0000ul);
	ASSERT(GC_LINE_BLOCK(0x686178650B4D0080ul) == (gc_block_header_t *)0x686178650B4D0000ul);
	ASSERT(GC_LINE_BLOCK(0x686178650B4D2000ul) == (gc_block_header_t *)0x686178650B4D0000ul);
	ASSERT(GC_LINE_BLOCK(0x686178650B4D2123ul) == (gc_block_header_t *)0x686178650B4D0000ul);
	ASSERT(GC_LINE_BLOCK(0x686178650B4DFF80ul) == (gc_block_header_t *)0x686178650B4D0000ul);

	ASSERT(GC_LINE_ID(0x686178650B4D2000ul) == 0);
	ASSERT(GC_LINE_ID(0x686178650B4D2123ul) == 2);
	ASSERT(GC_LINE_ID(0x686178650B4DFF80ul) == 447);

	ASSERT(GC_METADATA(0x686178650B4D2000ul) == (gc_metadata_t *)0x686178650B4D0200ul);
	ASSERT(GC_METADATA(0x686178650B4D2010ul) == (gc_metadata_t *)0x686178650B4D0202ul);
	ASSERT(GC_METADATA(0x686178650B4D2018ul) == (gc_metadata_t *)0x686178650B4D0203ul);
	ASSERT(GC_METADATA(0x686178650B4D2128ul) == (gc_metadata_t *)0x686178650B4D0225ul);
	ASSERT(GC_METADATA(0x686178650B4DFFF0ul) == (gc_metadata_t *)0x686178650B4D1DFEul);
} END_TEST_CASE

// a cycle of one object
BEGIN_TEST_CASE(self_cycle) {
	hlt_one_ptr_t *obj = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	obj->ptr = obj;

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 1);

	printf("%p\n", obj);
} END_TEST_CASE

// a cycle of three objects
BEGIN_TEST_CASE(tiny_cycle) {
	hlt_one_ptr_t *obj_1 = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hlt_one_ptr_t *obj_2 = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hlt_one_ptr_t *obj_3 = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);

	obj_1->ptr = obj_2;
	obj_2->ptr = obj_3;
	obj_3->ptr = obj_1;

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 3);
	ASSERT(gc_stats->total_pages == 1);
	hl_gc_major();
	ASSERT(gc_stats->live_objects == 3);
} END_TEST_CASE

// a cycle spanning more than one block
BEGIN_TEST_CASE(block_cycle) {
	hlt_one_ptr_t *first = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hlt_one_ptr_t *last = first;

	for (int i = 0; i < 5000; i++) {
		hlt_one_ptr_t *next = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
		last->ptr = next;
		last = next;
	}
	last->ptr = first;

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 5001);
} END_TEST_CASE

// a cycle in objects with many pointers
BEGIN_TEST_CASE(complex_cycle) {
	hlt_some_ptr_t *objs[100];
	for (int i = 0; i < 100; i++) {
		objs[i] = (hlt_some_ptr_t *)hl_alloc_obj(&hlt_some_ptr);
	}

	for (int i = 0; i < 100; i++) {
		objs[i]->ptr_0 = objs[(i + 1 + 0) % 100];
		objs[i]->ptr_2 = objs[(i + 1 + 2) % 100];
		objs[i]->ptr_6 = objs[(i + 1 + 6) % 100];
		objs[i]->ptr_7 = objs[(i + 1 + 7) % 100];
		objs[i]->ptr_9 = objs[(i + 1 + 9) % 100];
	}

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 100);
} END_TEST_CASE

// allocate small objects and fit a medium object
BEGIN_TEST_CASE(medium_object_recycle) {
	// start with small objects
	//   BLOCK 1                               |
	//   s > s > s > s > ... > s > s > ... > s |
	// create gap large enough for a medium object
	//   BLOCK 1                               |
	//   s > s > s   -   ...   -   s > ... > s |
	//            \________________^
	// allocate a full block of dummy objects
	//   BLOCK 1                               | BLOCK 2             |
	//   s > s > s   -   ...   -   s > ... > s | s   s   s   s   ... |
	//            \________________^
	// allocate a small object, forces recycle of block 1
	//   BLOCK 1                               |
	//   s > s > s   s   ...   -   s > ... > s |
	//            \________________^
	// allocate a medium object, should fit in block 1
	//   BLOCK 1                               |
	//   s > s > s > s > [m .....] s > ... > s |
	//                    \________^

	// (448 lines * 128 bytes per line) / 16 bytes per object = 3584
	#define OBJ_COUNT 3584

	hlt_one_ptr_t *objs[OBJ_COUNT];
	for (int i = 0; i < OBJ_COUNT; i++) {
		objs[i] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	}

	for (int i = 0; i < OBJ_COUNT - 1; i++) {
		objs[i]->ptr = objs[(i + 1) % OBJ_COUNT];
	}

	hl_gc_major();
	ASSERT(gc_stats->live_objects == OBJ_COUNT);

	// a gap of 129 small objects is the minimum to fit the medium one
	ASSERT(sizeof(hlt_one_ptr_t) == 16);
	ASSERT(sizeof(hlt_medium_one_ptr_t) == 2056);
	// ceil(2056 / 16) = 129
	// add 25 more to allow for:
	//  - worst-case conservatively marked lines on the "left"
	//  - worst-case occupied line on the "right"
	int gap = 129 + 25;
	objs[1337]->ptr = objs[1338 + gap];
	for (int i = 1338; i < 1338 + gap; i++) {
		objs[i] = NULL;
	}
	hl_gc_major();
	ASSERT(gc_stats->live_objects == OBJ_COUNT - gap);

	// exhaust full block to force recycle of first block
	for (int i = 0; i < OBJ_COUNT; i++) {
		hl_alloc_obj(&hlt_one_ptr);
	}

	hl_gc_major();
	ASSERT(gc_stats->live_objects == OBJ_COUNT - gap);

	objs[1338] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hlt_medium_one_ptr_t *med = (hlt_medium_one_ptr_t *)hl_alloc_obj(&hlt_medium_one_ptr);
	objs[1337]->ptr = objs[1338];
	objs[1338]->ptr = med;
	med->ptr = objs[1338 + gap];
	hl_gc_major();
	ASSERT(gc_stats->live_objects == OBJ_COUNT - gap + 2);
	ASSERT(gc_get_block(objs[0]) == gc_get_block(objs[1338]));
	ASSERT(gc_get_block(objs[0]) == gc_get_block(med));
} END_TEST_CASE

BEGIN_TEST_CASE(big_object) {
	hlt_one_ptr_t *obj = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);

	hlt_big_no_ptr_t *big = (hlt_big_no_ptr_t *)hl_alloc_obj(&hlt_big_no_ptr);
	obj->ptr = big;

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 2);
	ASSERT(gc_stats->total_pages == 2);

	printf("%p %p\n", obj, &obj); // force stack variable for obj
	printf("%p %p\n", big, &big); // force stack variable for big
} END_TEST_CASE

BEGIN_TEST_CASE(simple_array) {
	varray *arr = hl_alloc_array(&hlt_one_ptr, 20);

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 1);

	for (int i = 0; i < 20; i++) {
		hlt_one_ptr_t *obj = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
		hl_aptr(arr, hlt_one_ptr_t *)[i] = obj;
		obj = NULL;
	}

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 21);

	for (int i = 0; i < 20; i++) {
		hlt_one_ptr_t *obj = hl_aptr(arr, hlt_one_ptr_t *)[i];
		obj->ptr = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
		obj = NULL;
	}

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 41);
} END_TEST_CASE

BEGIN_TEST_CASE(big_array) {
	varray *arr = hl_alloc_array(&hlt_one_ptr, 15000);
	//hl_add_root((void *)&arr);

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 1);

	hl_aptr(arr, hlt_one_ptr_t *)[0] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hl_aptr(arr, hlt_one_ptr_t *)[0] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hl_aptr(arr, hlt_one_ptr_t *)[0] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hl_aptr(arr, hlt_one_ptr_t *)[0] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);
	hl_aptr(arr, hlt_one_ptr_t *)[14999] = (hlt_one_ptr_t *)hl_alloc_obj(&hlt_one_ptr);

	hl_gc_major();
	ASSERT(gc_stats->live_objects == 3);

	printf("%p %p\n", arr, &arr); // force stack variable for arr
} END_TEST_CASE

// benchmark cases

void bench_mandelbrot(int);

// test runner

int main(int argc, char **argv) {
	int assertions = 0;
	int assertions_successful = 0;
	test_sanity(&assertions, &assertions_successful);
	test_self_cycle(&assertions, &assertions_successful);
	test_tiny_cycle(&assertions, &assertions_successful);
	test_block_cycle(&assertions, &assertions_successful);
	test_complex_cycle(&assertions, &assertions_successful);
	test_medium_object_recycle(&assertions, &assertions_successful);
	test_big_object(&assertions, &assertions_successful);
	test_simple_array(&assertions, &assertions_successful);
	test_big_array(&assertions, &assertions_successful);
	puts("---");
	puts("TOTAL:");
	printf("  %d / %d checks passed\n", assertions_successful, assertions);

	if (assertions_successful != assertions)
		return 1;

	bench_mandelbrot(1);
}
