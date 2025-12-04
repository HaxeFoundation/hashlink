/*
 * Test 64-bit integer operations for HashLink AArch64 JIT
 *
 * Tests: i64 arithmetic with OAdd, OSub, OMul, OSDiv
 *
 * Note: OInt loads 32-bit values. For i64 registers, the value is sign-extended.
 */
#include "test_harness.h"

/*
 * Test: Return 64-bit constant (sign-extended from i32)
 */
TEST(return_i64_constant) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),   /* r0:i64 = 42 (sign-extended) */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Add 64-bit integers
 */
TEST(add_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OAdd, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Subtract 64-bit integers
 */
TEST(sub_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 100, 58 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OSub, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Multiply 64-bit integers
 */
TEST(mul_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 6, 7 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OMul, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Divide 64-bit integers
 */
TEST(sdiv_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 84, 2 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OSDiv, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Modulo 64-bit integers
 */
TEST(smod_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 142, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OSMod, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Negate 64-bit integer
 */
TEST(neg_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(ONeg, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Bitwise AND 64-bit
 */
TEST(and_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0xFF, 0x2A };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OAnd, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Bitwise OR 64-bit
 */
TEST(or_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0x20, 0x0A };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OOr, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Left shift 64-bit: 21 << 1 = 42
 */
TEST(shl_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 21, 1 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OShl, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Large shift - beyond 32 bits
 * 1 << 40 = 0x10000000000 (1099511627776)
 */
TEST(shl_i64_large) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 40 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OShl, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    long long expected = 1LL << 40;
    if (ret != expected) {
        fprintf(stderr, "    Expected %lld, got %lld\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Move i64 register
 */
TEST(mov_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64], &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OMov, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Increment i64
 */
TEST(incr_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 41 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP1(OIncr, 0),
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 3, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Decrement i64
 */
TEST(decr_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 43 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP1(ODecr, 0),
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 3, ops);

    int result;
    long long (*fn)(void) = (long long(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    long long ret = fn();
    if (ret != 42LL) {
        fprintf(stderr, "    Expected 42, got %lld\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(return_i64_constant),
    TEST_ENTRY(add_i64),
    TEST_ENTRY(sub_i64),
    TEST_ENTRY(mul_i64),
    TEST_ENTRY(sdiv_i64),
    TEST_ENTRY(smod_i64),
    TEST_ENTRY(neg_i64),
    TEST_ENTRY(and_i64),
    TEST_ENTRY(or_i64),
    TEST_ENTRY(shl_i64),
    TEST_ENTRY(shl_i64_large),
    TEST_ENTRY(mov_i64),
    TEST_ENTRY(incr_i64),
    TEST_ENTRY(decr_i64),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - 64-bit Integer Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
