/*
 * Test integer operations for HashLink AArch64 JIT
 *
 * Tests: OInt, OMov, OAdd, OSub, OMul, ORet
 */
#include "test_harness.h"

/*
 * Test: Return constant integer 42
 *
 * function test() -> i32:
 *   r0 = 42
 *   ret r0
 */
TEST(return_int_constant) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Integer pool: [42] */
    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function type: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Registers: r0:i32 */
    hl_type *regs[] = { &c->types[T_I32] };

    /* Opcodes:
     *   OInt r0, $0    ; r0 = ints[0] = 42
     *   ORet r0        ; return r0
     */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),   /* r0 = ints[0] */
        OP1(ORet, 0),      /* return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

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
 * Test: Add two constants: 10 + 32 = 42
 *
 * function test() -> i32:
 *   r0 = 10
 *   r1 = 32
 *   r2 = r0 + r1
 *   ret r2
 */
TEST(add_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Integer pool: [10, 32] */
    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function type: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Registers: r0:i32, r1:i32, r2:i32 */
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    /* Opcodes:
     *   OInt r0, $0    ; r0 = 10
     *   OInt r1, $1    ; r1 = 32
     *   OAdd r2, r0, r1  ; r2 = r0 + r1
     *   ORet r2        ; return r2
     */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = ints[0] = 10 */
        OP2(OInt, 1, 1),      /* r1 = ints[1] = 32 */
        OP3(OAdd, 2, 0, 1),   /* r2 = r0 + r1 */
        OP1(ORet, 2),         /* return r2 */
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
 * Test: Subtract: 100 - 58 = 42
 */
TEST(sub_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 100, 58 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 100 */
        OP2(OInt, 1, 1),      /* r1 = 58 */
        OP3(OSub, 2, 0, 1),   /* r2 = r0 - r1 */
        OP1(ORet, 2),         /* return r2 */
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
 * Test: Multiply: 6 * 7 = 42
 */
TEST(mul_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 6, 7 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 6 */
        OP2(OInt, 1, 1),      /* r1 = 7 */
        OP3(OMul, 2, 0, 1),   /* r2 = r0 * r1 */
        OP1(ORet, 2),         /* return r2 */
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
 * Test: Move register: r1 = r0
 */
TEST(mov_register) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 42 */
        OP2(OMov, 1, 0),      /* r1 = r0 */
        OP1(ORet, 1),         /* return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

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
 * Test: Signed division: 84 / 2 = 42
 */
TEST(sdiv_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 84, 2 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 84 */
        OP2(OInt, 1, 1),       /* r1 = 2 */
        OP3(OSDiv, 2, 0, 1),   /* r2 = r0 / r1 */
        OP1(ORet, 2),          /* return r2 */
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
 * Test: Signed modulo: 142 % 100 = 42
 */
TEST(smod_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 142, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 142 */
        OP2(OInt, 1, 1),       /* r1 = 100 */
        OP3(OSMod, 2, 0, 1),   /* r2 = r0 % r1 */
        OP1(ORet, 2),          /* return r2 */
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
 * Test: Bitwise AND: 0xFF & 0x2A = 42
 */
TEST(and_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0xFF, 0x2A };  /* 255 & 42 = 42 */
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 0xFF */
        OP2(OInt, 1, 1),      /* r1 = 0x2A */
        OP3(OAnd, 2, 0, 1),   /* r2 = r0 & r1 */
        OP1(ORet, 2),         /* return r2 */
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
 * Test: Bitwise OR: 0x20 | 0x0A = 42
 */
TEST(or_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0x20, 0x0A };  /* 32 | 10 = 42 */
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OOr, 2, 0, 1),
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
 * Test: Bitwise XOR: 0x55 ^ 0x7F = 42
 */
TEST(xor_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0x55, 0x7F };  /* 85 ^ 127 = 42 */
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OXor, 2, 0, 1),
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
 * Test: Left shift: 21 << 1 = 42
 */
TEST(shl_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 21, 1 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OShl, 2, 0, 1),
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
 * Test: Signed right shift: 168 >> 2 = 42
 */
TEST(sshr_int_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 168, 2 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OInt, 1, 1),
        OP3(OSShr, 2, 0, 1),
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
 * Test: Negate: -(-42) = 42
 */
TEST(neg_int) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = -42 */
        OP2(ONeg, 1, 0),      /* r1 = -r0 = 42 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

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
 * Test: Increment: 41 + 1 = 42
 */
TEST(incr_int) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 41 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 41 */
        OP1(OIncr, 0),        /* r0++ */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 3, ops);

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
 * Test: Decrement: 43 - 1 = 42
 */
TEST(decr_int) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 43 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 43 */
        OP1(ODecr, 0),        /* r0-- */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 3, ops);

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
    TEST_ENTRY(return_int_constant),
    TEST_ENTRY(add_int_constants),
    TEST_ENTRY(sub_int_constants),
    TEST_ENTRY(mul_int_constants),
    TEST_ENTRY(mov_register),
    TEST_ENTRY(sdiv_int_constants),
    TEST_ENTRY(smod_int_constants),
    TEST_ENTRY(and_int_constants),
    TEST_ENTRY(or_int_constants),
    TEST_ENTRY(xor_int_constants),
    TEST_ENTRY(shl_int_constants),
    TEST_ENTRY(sshr_int_constants),
    TEST_ENTRY(neg_int),
    TEST_ENTRY(incr_int),
    TEST_ENTRY(decr_int),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Integer Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
