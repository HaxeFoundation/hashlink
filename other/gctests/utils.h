#pragma once

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

// HL type stubbing
#define TEST_TYPE(name, words, bits, fields) \
	hl_type hlt_ ## name; \
	hl_runtime_obj hltr_ ## name = { \
		.t = &hlt_ ## name, .nfields = words, .nproto = 0, .size = (1 + words) * sizeof(void *), \
		.nmethods = 0, .nbindings = 0, .hasPtr = true, .methods = NULL, \
		.fields_indexes = NULL, .bindings = NULL, .parent = NULL, .toStringFun = NULL, \
		.compareFun = NULL, .castFun = NULL, .getFieldFun = NULL, .nlookup = 0, .lookup = NULL }; \
	hl_type_obj hlto_ ## name = { .rt = &hltr_ ## name }; \
	unsigned int hltm_ ## name[(words + 31) / 32] = bits; \
	hl_type hlt_ ## name = { HOBJ, .obj = &hlto_ ## name, .mark_bits = hltm_ ## name }; \
	typedef struct { \
		hl_type *t; \
		struct fields; \
	} hlt_ ## name ## _t
