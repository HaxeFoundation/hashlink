#pragma once

#define BEGIN_TEST_CASE(name) \
	void test_ ## name (int *_total_assertions, int *_total_assertions_successful) { \
		puts("TEST: " # name); \
		gc_stats_t *gc_stats = gc_init(); \
		hl_register_thread(_total_assertions); \
		const char *_test_name = #name; \
		int _assertions = 0; \
		int _assertions_successful = 0; \
		do
#define END_TEST_CASE \
		while (0); \
		printf("%d / %d checks passed\n", _assertions_successful, _assertions); \
		hl_unregister_thread(); \
		gc_deinit(); \
	}

#define BEGIN_BENCH_CASE(name) \
	void bench_ ## name (int _bench_count) { \
		puts("BENCH: " # name); \
		gc_stats_t *gc_stats = gc_init(); \
		hl_register_thread(&gc_stats); \
		const char *_bench_name = #name; \
		double _bench_start = hires_timestamp(); \
		for (int _bench_num = 0; _bench_num < _bench_count; _bench_num++)
#define END_BENCH_CASE \
		double _bench_end = hires_timestamp(); \
		hl_unregister_thread(); \
		gc_deinit(); \
		printf("\x1B[38;5;33m[.] benchmark %s completed in %.02f seconds\n\x1B[0m", _bench_name, _bench_end - _bench_start); \
	}

#define ASSERT(cond) \
	_assertions++; \
	(*_total_assertions)++; \
	if (cond) { \
		_assertions_successful++; \
		(*_total_assertions_successful)++; \
		printf("\x1B[38;5;28m[.] assertion passed in %s (line %d): " #cond "\n\x1B[0m", _test_name, __LINE__); \
	} else { \
		printf("\x1B[38;5;160m[!] assertion failed in %s (line %d): " #cond "\n\x1B[0m", _test_name, __LINE__); \
		printf("%d / %d checks passed\n", _assertions_successful, _assertions); \
		hl_unregister_thread(); \
		gc_deinit(); \
		return; \
	}
