/*
 * Test unsigned operations for HashLink AArch64 JIT
 *
 * Tests: OUDiv, OUMod, OUShr
 *
 * These opcodes perform unsigned arithmetic:
 * OUDiv: unsigned division
 * OUMod: unsigned modulo
 * OUShr: unsigned (logical) right shift
 */
#include "test_harness.h"

/*
 * Test: OUDiv - unsigned division
 *
 * 100 / 3 = 33 (unsigned)
 */
TEST(udiv_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 100, 3 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = dividend */
        &c->types[T_I32],  /* r1 = divisor */
        &c->types[T_I32],  /* r2 = result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 100 */
        OP2(OInt, 1, 1),       /* r1 = 3 */
        OP3(OUDiv, 2, 0, 1),   /* r2 = r0 / r1 (unsigned) */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 33) {
        fprintf(stderr, "    Expected 33, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUDiv with large unsigned values
 *
 * When treating -1 as unsigned int32, it's 0xFFFFFFFF = 4294967295
 * 4294967295 / 2 = 2147483647 (unsigned division)
 *
 * With signed division, -1 / 2 = 0
 */
TEST(udiv_large_unsigned) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 2 };  /* -1 as unsigned is 0xFFFFFFFF */
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = -1 (0xFFFFFFFF as unsigned) */
        OP2(OInt, 1, 1),       /* r1 = 2 */
        OP3(OUDiv, 2, 0, 1),   /* r2 = 0xFFFFFFFF / 2 = 0x7FFFFFFF */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    unsigned int (*fn)(void) = (unsigned int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    unsigned int ret = fn();
    unsigned int expected = 0x7FFFFFFF;  /* 2147483647 */
    if (ret != expected) {
        fprintf(stderr, "    Expected %u, got %u\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUMod - unsigned modulo
 *
 * 100 % 3 = 1
 */
TEST(umod_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 100, 3 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 100 */
        OP2(OInt, 1, 1),       /* r1 = 3 */
        OP3(OUMod, 2, 0, 1),   /* r2 = r0 % r1 (unsigned) */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUMod with large unsigned values
 *
 * 0xFFFFFFFF % 7 = 4294967295 % 7 = 3
 */
TEST(umod_large_unsigned) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 7 };  /* -1 as unsigned is 0xFFFFFFFF */
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OUMod, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    unsigned int (*fn)(void) = (unsigned int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    unsigned int ret = fn();
    unsigned int expected = 0xFFFFFFFF % 7;  /* 4294967295 % 7 = 3 */
    if (ret != expected) {
        fprintf(stderr, "    Expected %u, got %u\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUShr - unsigned (logical) right shift
 *
 * 0xFF000000 >> 8 (logical) = 0x00FF0000
 *
 * Signed shift would sign-extend: 0xFFFF0000
 */
TEST(ushr_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { (int)0xFF000000, 8 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 0xFF000000 */
        OP2(OInt, 1, 1),       /* r1 = 8 */
        OP3(OUShr, 2, 0, 1),   /* r2 = r0 >>> r1 (logical shift) */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    unsigned int (*fn)(void) = (unsigned int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    unsigned int ret = fn();
    unsigned int expected = 0x00FF0000;
    if (ret != expected) {
        fprintf(stderr, "    Expected 0x%08X, got 0x%08X\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUShr vs OSShr - compare unsigned vs signed shift
 *
 * -1 (0xFFFFFFFF) >> 16:
 *   - Unsigned: 0x0000FFFF
 *   - Signed:   0xFFFFFFFF (sign-extended)
 */
TEST(ushr_vs_sshr) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 16 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = value */
        &c->types[T_I32],  /* r1 = shift amount */
        &c->types[T_I32],  /* r2 = unsigned result */
        &c->types[T_I32],  /* r3 = signed result */
        &c->types[T_I32],  /* r4 = difference */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = -1 */
        OP2(OInt, 1, 1),       /* r1 = 16 */
        OP3(OUShr, 2, 0, 1),   /* r2 = unsigned shift */
        OP3(OSShr, 3, 0, 1),   /* r3 = signed shift */
        OP3(OSub, 4, 2, 3),    /* r4 = r2 - r3 */
        OP1(ORet, 4),          /* return difference */
    };

    test_alloc_function(c, 0, fn_type, 5, regs, 6, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    /*
     * UShr: 0xFFFFFFFF >>> 16 = 0x0000FFFF = 65535
     * SShr: 0xFFFFFFFF >> 16 = 0xFFFFFFFF = -1
     * Difference: 65535 - (-1) = 65536
     */
    int expected = 65536;
    if (ret != expected) {
        fprintf(stderr, "    Expected %d, got %d\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUDiv and OUMod together - verify quotient * divisor + remainder = dividend
 */
TEST(udiv_umod_combined) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 12345, 67 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = dividend */
        &c->types[T_I32],  /* r1 = divisor */
        &c->types[T_I32],  /* r2 = quotient */
        &c->types[T_I32],  /* r3 = remainder */
        &c->types[T_I32],  /* r4 = quotient * divisor */
        &c->types[T_I32],  /* r5 = reconstructed dividend */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 12345 */
        OP2(OInt, 1, 1),       /* r1 = 67 */
        OP3(OUDiv, 2, 0, 1),   /* r2 = 12345 / 67 = 184 */
        OP3(OUMod, 3, 0, 1),   /* r3 = 12345 % 67 = 17 */
        OP3(OMul, 4, 2, 1),    /* r4 = 184 * 67 = 12328 */
        OP3(OAdd, 5, 4, 3),    /* r5 = 12328 + 17 = 12345 */
        OP1(ORet, 5),
    };

    test_alloc_function(c, 0, fn_type, 6, regs, 7, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 12345) {
        fprintf(stderr, "    Expected 12345, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(udiv_basic),
    TEST_ENTRY(udiv_large_unsigned),
    TEST_ENTRY(umod_basic),
    TEST_ENTRY(umod_large_unsigned),
    TEST_ENTRY(ushr_basic),
    TEST_ENTRY(ushr_vs_sshr),
    TEST_ENTRY(udiv_umod_combined),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Unsigned Operation Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
