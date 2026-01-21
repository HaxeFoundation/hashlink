/*
 * Test memory operations for HashLink AArch64 JIT
 *
 * Tests: OGetI8, OGetI16, OGetMem, OSetI8, OSetI16, OSetMem
 *
 * These opcodes access memory at (base + offset) where offset is a register value.
 * OGetI8/OGetI16/OGetMem: dst = *(type*)(base + offset)
 * OSetI8/OSetI16/OSetMem: *(type*)(base + offset) = value
 */
#include "test_harness.h"

/* Native function to allocate test buffer */
static void *alloc_test_buffer(int size) {
    void *buf = malloc(size);
    memset(buf, 0, size);
    return buf;
}

/* Native function to free test buffer */
static void free_test_buffer(void *buf) {
    free(buf);
}

/*
 * Test: OSetI8 and OGetI8 - write and read byte values
 *
 * alloc buffer
 * set_i8(buffer, 0, 0x42)
 * set_i8(buffer, 1, 0x37)
 * r0 = get_i8(buffer, 0)  ; should be 0x42 = 66
 * r1 = get_i8(buffer, 1)  ; should be 0x37 = 55
 * r2 = r0 + r1            ; 66 + 55 = 121
 * return r2
 */
TEST(mem_i8_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 0, 1, 0x42, 0x37 };  /* size, offset0, offset1, val0, val1 */
    test_init_ints(c, 5, ints);

    /* Native: alloc_test_buffer(size) -> bytes */
    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    /* Function type: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /*
     * Registers:
     * r0: size (64)
     * r1: buffer (bytes)
     * r2: offset0 (0)
     * r3: offset1 (1)
     * r4: val0 (0x42)
     * r5: val1 (0x37)
     * r6: read val0
     * r7: read val1
     * r8: result
     */
    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2 = offset0 */
        &c->types[T_I32],    /* r3 = offset1 */
        &c->types[T_I32],    /* r4 = val0 */
        &c->types[T_I32],    /* r5 = val1 */
        &c->types[T_I32],    /* r6 = read val0 */
        &c->types[T_I32],    /* r7 = read val1 */
        &c->types[T_I32],    /* r8 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 0 (offset) */
        OP2(OInt, 3, 2),           /* r3 = 1 (offset) */
        OP2(OInt, 4, 3),           /* r4 = 0x42 */
        OP2(OInt, 5, 4),           /* r5 = 0x37 */
        OP3(OSetI8, 1, 2, 4),      /* *(i8*)(r1 + r2) = r4 */
        OP3(OSetI8, 1, 3, 5),      /* *(i8*)(r1 + r3) = r5 */
        OP3(OGetI8, 6, 1, 2),      /* r6 = *(i8*)(r1 + r2) */
        OP3(OGetI8, 7, 1, 3),      /* r7 = *(i8*)(r1 + r3) */
        OP3(OAdd, 8, 6, 7),        /* r8 = r6 + r7 */
        OP1(ORet, 8),
    };

    test_alloc_function(c, 0, fn_type, 9, regs, 12, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    int expected = 0x42 + 0x37;  /* 66 + 55 = 121 */
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetI16 and OGetI16 - write and read 16-bit values
 */
TEST(mem_i16_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 0, 2, 0x1234, 0x5678 };
    test_init_ints(c, 5, ints);

    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2 = offset0 */
        &c->types[T_I32],    /* r3 = offset1 */
        &c->types[T_I32],    /* r4 = val0 */
        &c->types[T_I32],    /* r5 = val1 */
        &c->types[T_I32],    /* r6 = read val0 */
        &c->types[T_I32],    /* r7 = read val1 */
        &c->types[T_I32],    /* r8 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 0 (offset) */
        OP2(OInt, 3, 2),           /* r3 = 2 (offset for second i16) */
        OP2(OInt, 4, 3),           /* r4 = 0x1234 */
        OP2(OInt, 5, 4),           /* r5 = 0x5678 */
        OP3(OSetI16, 1, 2, 4),     /* *(i16*)(r1 + r2) = r4 */
        OP3(OSetI16, 1, 3, 5),     /* *(i16*)(r1 + r3) = r5 */
        OP3(OGetI16, 6, 1, 2),     /* r6 = *(i16*)(r1 + r2) */
        OP3(OGetI16, 7, 1, 3),     /* r7 = *(i16*)(r1 + r3) */
        OP3(OAdd, 8, 6, 7),        /* r8 = r6 + r7 */
        OP1(ORet, 8),
    };

    test_alloc_function(c, 0, fn_type, 9, regs, 12, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    int expected = 0x1234 + 0x5678;  /* 4660 + 22136 = 26796 */
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetMem and OGetMem - write and read 32-bit values (i32)
 */
TEST(mem_i32_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 0, 4, 100, 200 };
    test_init_ints(c, 5, ints);

    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2 = offset0 */
        &c->types[T_I32],    /* r3 = offset1 */
        &c->types[T_I32],    /* r4 = val0 */
        &c->types[T_I32],    /* r5 = val1 */
        &c->types[T_I32],    /* r6 = read val0 */
        &c->types[T_I32],    /* r7 = read val1 */
        &c->types[T_I32],    /* r8 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 0 (offset) */
        OP2(OInt, 3, 2),           /* r3 = 4 (offset for second i32) */
        OP2(OInt, 4, 3),           /* r4 = 100 */
        OP2(OInt, 5, 4),           /* r5 = 200 */
        OP3(OSetMem, 1, 2, 4),     /* *(i32*)(r1 + r2) = r4 */
        OP3(OSetMem, 1, 3, 5),     /* *(i32*)(r1 + r3) = r5 */
        OP3(OGetMem, 6, 1, 2),     /* r6 = *(i32*)(r1 + r2) */
        OP3(OGetMem, 7, 1, 3),     /* r7 = *(i32*)(r1 + r3) */
        OP3(OAdd, 8, 6, 7),        /* r8 = r6 + r7 */
        OP1(ORet, 8),
    };

    test_alloc_function(c, 0, fn_type, 9, regs, 12, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    int expected = 100 + 200;
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSetMem and OGetMem with i64 values
 */
TEST(mem_i64_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 0, 8, 1000, 2000 };
    test_init_ints(c, 5, ints);

    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2 = offset0 */
        &c->types[T_I32],    /* r3 = offset1 */
        &c->types[T_I64],    /* r4 = val0 */
        &c->types[T_I64],    /* r5 = val1 */
        &c->types[T_I64],    /* r6 = read val0 */
        &c->types[T_I64],    /* r7 = read val1 */
        &c->types[T_I64],    /* r8 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 0 (offset) */
        OP2(OInt, 3, 2),           /* r3 = 8 (offset for second i64) */
        OP2(OInt, 4, 3),           /* r4 = 1000 (as i64) */
        OP2(OInt, 5, 4),           /* r5 = 2000 (as i64) */
        OP3(OSetMem, 1, 2, 4),     /* *(i64*)(r1 + r2) = r4 */
        OP3(OSetMem, 1, 3, 5),     /* *(i64*)(r1 + r3) = r5 */
        OP3(OGetMem, 6, 1, 2),     /* r6 = *(i64*)(r1 + r2) */
        OP3(OGetMem, 7, 1, 3),     /* r7 = *(i64*)(r1 + r3) */
        OP3(OAdd, 8, 6, 7),        /* r8 = r6 + r7 */
        OP1(ORet, 8),
    };

    test_alloc_function(c, 0, fn_type, 9, regs, 12, ops);

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
 * Test: OSetMem and OGetMem with f64 values
 */
TEST(mem_f64_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 0, 8 };
    test_init_ints(c, 3, ints);

    double floats[] = { 1.5, 2.5 };
    test_init_floats(c, 2, floats);

    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2 = offset0 */
        &c->types[T_I32],    /* r3 = offset1 */
        &c->types[T_F64],    /* r4 = val0 */
        &c->types[T_F64],    /* r5 = val1 */
        &c->types[T_F64],    /* r6 = read val0 */
        &c->types[T_F64],    /* r7 = read val1 */
        &c->types[T_F64],    /* r8 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 0 (offset) */
        OP2(OInt, 3, 2),           /* r3 = 8 (offset for second f64) */
        OP2(OFloat, 4, 0),         /* r4 = 1.5 */
        OP2(OFloat, 5, 1),         /* r5 = 2.5 */
        OP3(OSetMem, 1, 2, 4),     /* *(f64*)(r1 + r2) = r4 */
        OP3(OSetMem, 1, 3, 5),     /* *(f64*)(r1 + r3) = r5 */
        OP3(OGetMem, 6, 1, 2),     /* r6 = *(f64*)(r1 + r2) */
        OP3(OGetMem, 7, 1, 3),     /* r7 = *(f64*)(r1 + r3) */
        OP3(OAdd, 8, 6, 7),        /* r8 = r6 + r7 */
        OP1(ORet, 8),
    };

    test_alloc_function(c, 0, fn_type, 9, regs, 12, ops);

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
 * Test: Non-zero base offset
 *
 * Tests accessing memory at non-aligned offsets
 */
TEST(mem_nonzero_offset) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 64, 10, 11, 12, 13, 1, 2, 3, 4 };
    test_init_ints(c, 9, ints);

    hl_type *alloc_args[] = { &c->types[T_I32] };
    hl_type *alloc_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, alloc_args);
    test_add_native(c, 1, "test", "alloc_buffer", alloc_fn_type, (void*)alloc_test_buffer);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],    /* r0 = size */
        &c->types[T_BYTES],  /* r1 = buffer */
        &c->types[T_I32],    /* r2-r5 = offsets */
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],    /* r6-r9 = values */
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],    /* r10 = sum */
        &c->types[T_I32],    /* r11-r14 = read values */
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 64 (size) */
        OP3(OCall1, 1, 1, 0),    /* r1 = alloc_buffer(r0) */
        OP2(OInt, 2, 1),           /* r2 = 10 */
        OP2(OInt, 3, 2),           /* r3 = 11 */
        OP2(OInt, 4, 3),           /* r4 = 12 */
        OP2(OInt, 5, 4),           /* r5 = 13 */
        OP2(OInt, 6, 5),           /* r6 = 1 */
        OP2(OInt, 7, 6),           /* r7 = 2 */
        OP2(OInt, 8, 7),           /* r8 = 3 */
        OP2(OInt, 9, 8),           /* r9 = 4 */
        OP3(OSetI8, 1, 2, 6),      /* buf[10] = 1 */
        OP3(OSetI8, 1, 3, 7),      /* buf[11] = 2 */
        OP3(OSetI8, 1, 4, 8),      /* buf[12] = 3 */
        OP3(OSetI8, 1, 5, 9),      /* buf[13] = 4 */
        OP3(OGetI8, 11, 1, 2),     /* r11 = buf[10] */
        OP3(OGetI8, 12, 1, 3),     /* r12 = buf[11] */
        OP3(OGetI8, 13, 1, 4),     /* r13 = buf[12] */
        OP3(OGetI8, 14, 1, 5),     /* r14 = buf[13] */
        OP3(OAdd, 10, 11, 12),     /* r10 = r11 + r12 */
        OP3(OAdd, 10, 10, 13),     /* r10 = r10 + r13 */
        OP3(OAdd, 10, 10, 14),     /* r10 = r10 + r14 */
        OP1(ORet, 10),
    };

    test_alloc_function(c, 0, fn_type, 15, regs, 22, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    int expected = 1 + 2 + 3 + 4;  /* 10 */
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(mem_i8_basic),
    TEST_ENTRY(mem_i16_basic),
    TEST_ENTRY(mem_i32_basic),
    TEST_ENTRY(mem_i64_basic),
    TEST_ENTRY(mem_f64_basic),
    TEST_ENTRY(mem_nonzero_offset),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Memory Operation Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
