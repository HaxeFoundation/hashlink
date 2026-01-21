/*
 * Test reference operations for HashLink AArch64 JIT
 *
 * Tests: ORef, OUnref, OSetref, ORefData, ORefOffset
 *
 * ORef: creates a reference (pointer) to a stack variable
 * OUnref: dereferences a reference
 * OSetref: assigns through a reference
 * ORefData: gets pointer to array/bytes data
 * ORefOffset: offsets a reference by index * element_size
 */
#include "test_harness.h"

/* Helper to create a reference type */
static hl_type *create_ref_type(hl_code *c, hl_type *elem_type) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HREF;
    t->tparam = elem_type;

    return t;
}

/* Helper to create an array type */
static hl_type *create_array_type(hl_code *c, hl_type *elem_type) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HARRAY;
    t->tparam = elem_type;

    return t;
}

/*
 * Test: ORef and OUnref basic - create reference and dereference
 *
 * r0 = 42
 * r1 = ref(r0)   ; r1 = &r0
 * r2 = unref(r1) ; r2 = *r1 = 42
 * return r2
 */
TEST(ref_unref_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *ref_i32 = create_ref_type(c, &c->types[T_I32]);
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = value */
        ref_i32,           /* r1 = reference */
        &c->types[T_I32],  /* r2 = dereferenced value */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),    /* r0 = 42 */
        OP2(ORef, 1, 0),    /* r1 = &r0 */
        OP2(OUnref, 2, 1),  /* r2 = *r1 */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetref - modify value through reference
 *
 * r0 = 10
 * r1 = ref(r0)   ; r1 = &r0
 * r2 = 42
 * setref(r1, r2) ; *r1 = 42, so r0 = 42
 * return r0      ; should be 42
 */
TEST(setref_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 42 };
    test_init_ints(c, 2, ints);

    hl_type *ref_i32 = create_ref_type(c, &c->types[T_I32]);
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = value */
        ref_i32,           /* r1 = reference */
        &c->types[T_I32],  /* r2 = new value */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 10 */
        OP2(ORef, 1, 0),       /* r1 = &r0 */
        OP2(OInt, 2, 1),       /* r2 = 42 */
        OP2(OSetref, 1, 2),    /* *r1 = r2 */
        OP1(ORet, 0),          /* return r0 (should be 42 now) */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 5, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ORef/OUnref with i64
 */
TEST(ref_unref_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 12345 };
    test_init_ints(c, 1, ints);

    hl_type *ref_i64 = create_ref_type(c, &c->types[T_I64]);
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I64],  /* r0 = value */
        ref_i64,           /* r1 = reference */
        &c->types[T_I64],  /* r2 = dereferenced value */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),    /* r0 = 12345 */
        OP2(ORef, 1, 0),    /* r1 = &r0 */
        OP2(OUnref, 2, 1),  /* r2 = *r1 */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int64_t (*fn)(void) = (int64_t(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = fn();
    if (ret != 12345) {
        fprintf(stderr, "    Expected 12345, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ORef/OUnref with f64
 */
TEST(ref_unref_f64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 3.14159 };
    test_init_floats(c, 1, floats);

    hl_type *ref_f64 = create_ref_type(c, &c->types[T_F64]);
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_F64],  /* r0 = value */
        ref_f64,           /* r1 = reference */
        &c->types[T_F64],  /* r2 = dereferenced value */
    };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),  /* r0 = 3.14159 */
        OP2(ORef, 1, 0),    /* r1 = &r0 */
        OP2(OUnref, 2, 1),  /* r2 = *r1 */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 3.14159;
    double diff = ret - expected;
    if (diff < 0) diff = -diff;
    if (diff > 0.00001) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ORefData with array - get pointer to array data
 *
 * array = alloc_array(i32, 3)
 * array[0] = 10
 * array[1] = 20
 * array[2] = 12
 * ptr = ref_data(array)  ; get pointer to element data
 * val = *ptr             ; read first element via pointer
 * return val             ; should be 10
 */
TEST(ref_data_array) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 3, 0, 1, 2, 10, 20, 12 };
    test_init_ints(c, 7, ints);

    hl_type *array_i32 = create_array_type(c, &c->types[T_I32]);
    hl_type *ref_i32 = create_ref_type(c, &c->types[T_I32]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i32, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i32,           /* r2 = array */
        &c->types[T_I32],    /* r3 = idx 0 */
        &c->types[T_I32],    /* r4 = idx 1 */
        &c->types[T_I32],    /* r5 = idx 2 */
        &c->types[T_I32],    /* r6 = val 10 */
        &c->types[T_I32],    /* r7 = val 20 */
        &c->types[T_I32],    /* r8 = val 12 */
        ref_i32,             /* r9 = ptr to data */
        &c->types[T_I32],    /* r10 = read value */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),             /* r0 = type for i32 */
        OP2(OInt, 1, 0),                  /* r1 = 3 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 1 */
        OP2(OInt, 5, 3),                  /* r5 = 2 */
        OP2(OInt, 6, 4),                  /* r6 = 10 */
        OP2(OInt, 7, 5),                  /* r7 = 20 */
        OP2(OInt, 8, 6),                  /* r8 = 12 */
        OP3(OSetArray, 2, 3, 6),          /* array[0] = 10 */
        OP3(OSetArray, 2, 4, 7),          /* array[1] = 20 */
        OP3(OSetArray, 2, 5, 8),          /* array[2] = 12 */
        OP2(ORefData, 9, 2),              /* r9 = ptr to array data */
        OP2(OUnref, 10, 9),               /* r10 = *r9 = first element */
        OP1(ORet, 10),
    };

    test_alloc_function(c, 0, fn_type, 11, regs, 15, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 10) {
        fprintf(stderr, "    Expected 10, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ORefOffset - offset a pointer to access array elements
 *
 * array = alloc_array(i32, 3)
 * array[0] = 10
 * array[1] = 20
 * array[2] = 12
 * ptr = ref_data(array)     ; get pointer to element data
 * ptr2 = ref_offset(ptr, 2) ; ptr to array[2]
 * val = *ptr2               ; read third element
 * return val                ; should be 12
 */
TEST(ref_offset_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 3, 0, 1, 2, 10, 20, 12 };
    test_init_ints(c, 7, ints);

    hl_type *array_i32 = create_array_type(c, &c->types[T_I32]);
    hl_type *ref_i32 = create_ref_type(c, &c->types[T_I32]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i32, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i32,           /* r2 = array */
        &c->types[T_I32],    /* r3-r5 = indices */
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],    /* r6-r8 = values */
        &c->types[T_I32],
        &c->types[T_I32],
        ref_i32,             /* r9 = ptr to data */
        ref_i32,             /* r10 = offset ptr */
        &c->types[T_I32],    /* r11 = read value */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),             /* r0 = type for i32 */
        OP2(OInt, 1, 0),                  /* r1 = 3 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 1 */
        OP2(OInt, 5, 3),                  /* r5 = 2 */
        OP2(OInt, 6, 4),                  /* r6 = 10 */
        OP2(OInt, 7, 5),                  /* r7 = 20 */
        OP2(OInt, 8, 6),                  /* r8 = 12 */
        OP3(OSetArray, 2, 3, 6),          /* array[0] = 10 */
        OP3(OSetArray, 2, 4, 7),          /* array[1] = 20 */
        OP3(OSetArray, 2, 5, 8),          /* array[2] = 12 */
        OP2(ORefData, 9, 2),              /* r9 = ptr to array data */
        OP3(ORefOffset, 10, 9, 5),        /* r10 = r9 + 2 * sizeof(i32) */
        OP2(OUnref, 11, 10),              /* r11 = *r10 = array[2] */
        OP1(ORet, 11),
    };

    test_alloc_function(c, 0, fn_type, 12, regs, 16, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 12) {
        fprintf(stderr, "    Expected 12, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ORefOffset with i64 elements - larger element size
 */
TEST(ref_offset_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 3, 0, 1, 2, 100, 200, 300 };
    test_init_ints(c, 7, ints);

    hl_type *array_i64 = create_array_type(c, &c->types[T_I64]);
    hl_type *ref_i64 = create_ref_type(c, &c->types[T_I64]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i64, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i64,           /* r2 = array */
        &c->types[T_I32],    /* r3-r5 = indices */
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I64],    /* r6-r8 = values */
        &c->types[T_I64],
        &c->types[T_I64],
        ref_i64,             /* r9 = ptr to data */
        ref_i64,             /* r10 = offset ptr */
        &c->types[T_I64],    /* r11 = read value */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I64),             /* r0 = type for i64 */
        OP2(OInt, 1, 0),                  /* r1 = 3 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 1 */
        OP2(OInt, 5, 3),                  /* r5 = 2 */
        OP2(OInt, 6, 4),                  /* r6 = 100 */
        OP2(OInt, 7, 5),                  /* r7 = 200 */
        OP2(OInt, 8, 6),                  /* r8 = 300 */
        OP3(OSetArray, 2, 3, 6),          /* array[0] = 100 */
        OP3(OSetArray, 2, 4, 7),          /* array[1] = 200 */
        OP3(OSetArray, 2, 5, 8),          /* array[2] = 300 */
        OP2(ORefData, 9, 2),              /* r9 = ptr to array data */
        OP3(ORefOffset, 10, 9, 4),        /* r10 = r9 + 1 * sizeof(i64) */
        OP2(OUnref, 11, 10),              /* r11 = *r10 = array[1] */
        OP1(ORet, 11),
    };

    test_alloc_function(c, 0, fn_type, 12, regs, 16, ops);

    int result;
    int64_t (*fn)(void) = (int64_t(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = fn();
    if (ret != 200) {
        fprintf(stderr, "    Expected 200, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(ref_unref_basic),
    TEST_ENTRY(setref_basic),
    TEST_ENTRY(ref_unref_i64),
    TEST_ENTRY(ref_unref_f64),
    TEST_ENTRY(ref_data_array),
    TEST_ENTRY(ref_offset_basic),
    TEST_ENTRY(ref_offset_i64),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Reference Operation Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
