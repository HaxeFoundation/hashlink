/*
 * Test array operations for HashLink AArch64 JIT
 *
 * Tests: OGetArray, OSetArray, OArraySize
 *
 * OGetArray: dst = array[index]
 * OSetArray: array[index] = value
 * OArraySize: dst = array.length
 */
#include "test_harness.h"

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
 * Test: OSetArray and OGetArray with i32 elements
 *
 * array = alloc_array(i32, 3)
 * array[0] = 10
 * array[1] = 20
 * array[2] = 12
 * r0 = array[0] + array[1] + array[2]  ; 10 + 20 + 12 = 42
 * return r0
 */
TEST(array_i32_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 3, 0, 1, 2, 10, 20, 12 };
    test_init_ints(c, 7, ints);

    /* Create array type: Array<i32> */
    hl_type *array_i32 = create_array_type(c, &c->types[T_I32]);

    /* Native: hl_alloc_array(type, size) -> array */
    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };  /* type pointer */
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i32, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    /* Function type: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /*
     * Registers:
     * r0: type pointer (for alloc)
     * r1: size (3)
     * r2: array
     * r3-r5: indices (0, 1, 2)
     * r6-r8: values (10, 20, 12)
     * r9-r11: read values
     * r12: sum
     */
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
        &c->types[T_I32],    /* r9 = read[0] */
        &c->types[T_I32],    /* r10 = read[1] */
        &c->types[T_I32],    /* r11 = read[2] */
        &c->types[T_I32],    /* r12 = sum */
    };

    /* OType loads type at given index into register */
    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),         /* r0 = type for i32 */
        OP2(OInt, 1, 0),              /* r1 = 3 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),              /* r3 = 0 */
        OP2(OInt, 4, 2),              /* r4 = 1 */
        OP2(OInt, 5, 3),              /* r5 = 2 */
        OP2(OInt, 6, 4),              /* r6 = 10 */
        OP2(OInt, 7, 5),              /* r7 = 20 */
        OP2(OInt, 8, 6),              /* r8 = 12 */
        OP3(OSetArray, 2, 3, 6),      /* array[0] = 10 */
        OP3(OSetArray, 2, 4, 7),      /* array[1] = 20 */
        OP3(OSetArray, 2, 5, 8),      /* array[2] = 12 */
        OP3(OGetArray, 9, 2, 3),      /* r9 = array[0] */
        OP3(OGetArray, 10, 2, 4),     /* r10 = array[1] */
        OP3(OGetArray, 11, 2, 5),     /* r11 = array[2] */
        OP3(OAdd, 12, 9, 10),         /* r12 = r9 + r10 */
        OP3(OAdd, 12, 12, 11),        /* r12 = r12 + r11 */
        OP1(ORet, 12),
    };

    test_alloc_function(c, 0, fn_type, 13, regs, 18, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    int expected = 10 + 20 + 12;
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OArraySize
 *
 * array = alloc_array(i32, 5)
 * return array_size(array)  ; should be 5
 */
TEST(array_size) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 5 };
    test_init_ints(c, 1, ints);

    hl_type *array_i32 = create_array_type(c, &c->types[T_I32]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i32, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i32,           /* r2 = array */
        &c->types[T_I32],    /* r3 = array_size */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),             /* r0 = type for i32 */
        OP2(OInt, 1, 0),                  /* r1 = 5 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OArraySize, 3, 2),            /* r3 = array_size(r2) */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 5, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 5) {
        fprintf(stderr, "    Expected 5, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetArray and OGetArray with i64 elements
 */
TEST(array_i64_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 2, 0, 1, 1000, 2000 };
    test_init_ints(c, 5, ints);

    hl_type *array_i64 = create_array_type(c, &c->types[T_I64]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i64, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i64,           /* r2 = array */
        &c->types[T_I32],    /* r3 = idx 0 */
        &c->types[T_I32],    /* r4 = idx 1 */
        &c->types[T_I64],    /* r5 = val 1000 */
        &c->types[T_I64],    /* r6 = val 2000 */
        &c->types[T_I64],    /* r7 = read[0] */
        &c->types[T_I64],    /* r8 = read[1] */
        &c->types[T_I64],    /* r9 = sum */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I64),             /* r0 = type for i64 */
        OP2(OInt, 1, 0),                  /* r1 = 2 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 1 */
        OP2(OInt, 5, 3),                  /* r5 = 1000 */
        OP2(OInt, 6, 4),                  /* r6 = 2000 */
        OP3(OSetArray, 2, 3, 5),          /* array[0] = 1000 */
        OP3(OSetArray, 2, 4, 6),          /* array[1] = 2000 */
        OP3(OGetArray, 7, 2, 3),          /* r7 = array[0] */
        OP3(OGetArray, 8, 2, 4),          /* r8 = array[1] */
        OP3(OAdd, 9, 7, 8),               /* r9 = r7 + r8 */
        OP1(ORet, 9),
    };

    test_alloc_function(c, 0, fn_type, 10, regs, 13, ops);

    int result;
    int64_t (*fn)(void) = (int64_t(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = fn();
    int64_t expected = 1000 + 2000;
    if (ret != expected) {
        fprintf(stderr, "    Expected %ld, got %ld\n", (long)expected, (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetArray and OGetArray with f64 elements
 */
TEST(array_f64_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 2, 0, 1 };
    test_init_ints(c, 3, ints);

    double floats[] = { 1.5, 2.5 };
    test_init_floats(c, 2, floats);

    hl_type *array_f64 = create_array_type(c, &c->types[T_F64]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_f64, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_f64,           /* r2 = array */
        &c->types[T_I32],    /* r3 = idx 0 */
        &c->types[T_I32],    /* r4 = idx 1 */
        &c->types[T_F64],    /* r5 = val 1.5 */
        &c->types[T_F64],    /* r6 = val 2.5 */
        &c->types[T_F64],    /* r7 = read[0] */
        &c->types[T_F64],    /* r8 = read[1] */
        &c->types[T_F64],    /* r9 = sum */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_F64),             /* r0 = type for f64 */
        OP2(OInt, 1, 0),                  /* r1 = 2 (size) */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 1 */
        OP2(OFloat, 5, 0),                /* r5 = 1.5 */
        OP2(OFloat, 6, 1),                /* r6 = 2.5 */
        OP3(OSetArray, 2, 3, 5),          /* array[0] = 1.5 */
        OP3(OSetArray, 2, 4, 6),          /* array[1] = 2.5 */
        OP3(OGetArray, 7, 2, 3),          /* r7 = array[0] */
        OP3(OGetArray, 8, 2, 4),          /* r8 = array[1] */
        OP3(OAdd, 9, 7, 8),               /* r9 = r7 + r8 */
        OP1(ORet, 9),
    };

    test_alloc_function(c, 0, fn_type, 10, regs, 13, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 1.5 + 2.5;
    double diff = ret - expected;
    if (diff < 0) diff = -diff;
    if (diff > 0.0001) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Array with dynamic indices (not compile-time constants)
 */
TEST(array_dynamic_index) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 0, 42, 1 };  /* size, idx0, value, idx_offset */
    test_init_ints(c, 4, ints);

    hl_type *array_i32 = create_array_type(c, &c->types[T_I32]);

    hl_type *alloc_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, array_i32, 2, alloc_args);
    test_add_native(c, 1, "std", "alloc_array", alloc_fn_type, (void*)hl_alloc_array);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],   /* r0 = type pointer */
        &c->types[T_I32],    /* r1 = size */
        array_i32,           /* r2 = array */
        &c->types[T_I32],    /* r3 = idx (computed) */
        &c->types[T_I32],    /* r4 = value */
        &c->types[T_I32],    /* r5 = idx_offset */
        &c->types[T_I32],    /* r6 = computed idx */
        &c->types[T_I32],    /* r7 = read value */
    };

    /* Store at index 0, then compute index 0+1-1=0 to read back */
    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),             /* r0 = type */
        OP2(OInt, 1, 0),                  /* r1 = 10 */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* r2 = alloc_array(r0, r1) */
        OP2(OInt, 3, 1),                  /* r3 = 0 */
        OP2(OInt, 4, 2),                  /* r4 = 42 */
        OP3(OSetArray, 2, 3, 4),          /* array[0] = 42 */
        OP2(OInt, 5, 3),                  /* r5 = 1 */
        OP3(OAdd, 6, 3, 5),               /* r6 = r3 + r5 = 1 */
        OP3(OSub, 6, 6, 5),               /* r6 = r6 - r5 = 0 */
        OP3(OGetArray, 7, 2, 6),          /* r7 = array[r6] = array[0] */
        OP1(ORet, 7),
    };

    test_alloc_function(c, 0, fn_type, 8, regs, 11, ops);

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

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(array_i32_basic),
    TEST_ENTRY(array_size),
    TEST_ENTRY(array_i64_basic),
    TEST_ENTRY(array_f64_basic),
    TEST_ENTRY(array_dynamic_index),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Array Operation Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
