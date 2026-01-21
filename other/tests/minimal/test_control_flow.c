/*
 * Test control flow operations for HashLink AArch64 JIT
 *
 * Tests: OLabel, OJAlways, OJTrue, OJFalse, OJSLt, OJSGte, OJEq, OJNotEq
 *
 * Jump offset semantics: target = (currentOpIndex + 1) + offset
 * Example: at op1 with offset=1 -> target = (1+1)+1 = 3
 */
#include "test_harness.h"

/*
 * Test: Unconditional jump - skip one instruction
 *
 * op0: int r0, 0       ; r0 = 42
 * op1: jalways +1      ; jump to op3 (target = 2+1 = 3)
 * op2: int r0, 1       ; r0 = 100 (SKIPPED)
 * op3: ret r0          ; return 42
 */
TEST(jump_always_skip) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 42 */
        OP1(OJAlways, 1),     /* op1: jump to op3 (target = 2+1 = 3) */
        OP2(OInt, 0, 1),      /* op2: r0 = 100 (skipped) */
        OP1(ORet, 0),         /* op3: return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 4, ops);

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
 * Test: Jump if true (taken)
 *
 * op0: bool r0, 1      ; r0 = true
 * op1: int r1, 0       ; r1 = 42
 * op2: jtrue r0, +1    ; if r0 goto op4 (target = 3+1 = 4)
 * op3: int r1, 1       ; r1 = 100 (skipped)
 * op4: ret r1          ; return 42
 */
TEST(jump_true_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),     /* op0: r0 = true */
        OP2(OInt, 1, 0),      /* op1: r1 = 42 */
        OP2(OJTrue, 0, 1),    /* op2: if r0 goto op4 (target = 3+1 = 4) */
        OP2(OInt, 1, 1),      /* op3: r1 = 100 (skipped) */
        OP1(ORet, 1),         /* op4: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

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
 * Test: Jump if true (not taken)
 */
TEST(jump_true_not_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 0),     /* op0: r0 = false */
        OP2(OInt, 1, 0),      /* op1: r1 = 42 */
        OP2(OJTrue, 0, 1),    /* op2: if r0 goto op4 (not taken) */
        OP2(OInt, 1, 1),      /* op3: r1 = 100 */
        OP1(ORet, 1),         /* op4: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 100) {
        fprintf(stderr, "    Expected 100, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Jump if false (taken)
 */
TEST(jump_false_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 0),     /* op0: r0 = false */
        OP2(OInt, 1, 0),      /* op1: r1 = 42 */
        OP2(OJFalse, 0, 1),   /* op2: if !r0 goto op4 (target = 3+1 = 4) */
        OP2(OInt, 1, 1),      /* op3: r1 = 100 (skipped) */
        OP1(ORet, 1),         /* op4: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

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
 * Test: Jump if false (not taken)
 */
TEST(jump_false_not_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),     /* op0: r0 = true */
        OP2(OInt, 1, 0),      /* op1: r1 = 42 */
        OP2(OJFalse, 0, 1),   /* op2: if !r0 goto op4 (not taken) */
        OP2(OInt, 1, 1),      /* op3: r1 = 100 */
        OP1(ORet, 1),         /* op4: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 100) {
        fprintf(stderr, "    Expected 100, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Jump if signed less than (taken): 5 < 10
 */
TEST(jump_slt_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 5, 10, 42, 100 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 5 */
        OP2(OInt, 1, 1),      /* op1: r1 = 10 */
        OP2(OInt, 2, 2),      /* op2: r2 = 42 */
        OP3(OJSLt, 0, 1, 1),  /* op3: if r0 < r1 goto op5 (target = 4+1 = 5) */
        OP2(OInt, 2, 3),      /* op4: r2 = 100 (skipped) */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

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
 * Test: Jump if signed less than (not taken): 10 < 5
 */
TEST(jump_slt_not_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 5, 42, 100 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 10 */
        OP2(OInt, 1, 1),      /* op1: r1 = 5 */
        OP2(OInt, 2, 2),      /* op2: r2 = 42 */
        OP3(OJSLt, 0, 1, 1),  /* op3: if r0 < r1 goto op5 (not taken) */
        OP2(OInt, 2, 3),      /* op4: r2 = 100 */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 100) {
        fprintf(stderr, "    Expected 100, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Jump if signed greater-or-equal (taken): 10 >= 5
 */
TEST(jump_sgte_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 5, 42, 100 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 10 */
        OP2(OInt, 1, 1),      /* op1: r1 = 5 */
        OP2(OInt, 2, 2),      /* op2: r2 = 42 */
        OP3(OJSGte, 0, 1, 1), /* op3: if r0 >= r1 goto op5 (target = 4+1 = 5) */
        OP2(OInt, 2, 3),      /* op4: r2 = 100 (skipped) */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

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
 * Test: Jump if equal (taken): 42 == 42
 */
TEST(jump_eq_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 42 */
        OP2(OInt, 1, 0),      /* op1: r1 = 42 */
        OP2(OInt, 2, 0),      /* op2: r2 = 42 */
        OP3(OJEq, 0, 1, 1),   /* op3: if r0 == r1 goto op5 */
        OP2(OInt, 2, 1),      /* op4: r2 = 100 (skipped) */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

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
 * Test: Jump if equal (not taken): 42 == 100
 */
TEST(jump_eq_not_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = 42 */
        OP2(OInt, 1, 1),      /* op1: r1 = 100 */
        OP2(OInt, 2, 0),      /* op2: r2 = 42 */
        OP3(OJEq, 0, 1, 1),   /* op3: if r0 == r1 goto op5 (not taken) */
        OP2(OInt, 2, 1),      /* op4: r2 = 100 */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 100) {
        fprintf(stderr, "    Expected 100, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Jump if not equal (taken): 42 != 100
 */
TEST(jump_neq_taken) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 100 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* op0: r0 = 42 */
        OP2(OInt, 1, 1),       /* op1: r1 = 100 */
        OP2(OInt, 2, 0),       /* op2: r2 = 42 */
        OP3(OJNotEq, 0, 1, 1), /* op3: if r0 != r1 goto op5 */
        OP2(OInt, 2, 1),       /* op4: r2 = 100 (skipped) */
        OP1(ORet, 2),          /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

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
 * Test: Simple loop - sum 1 to 5 = 15
 *
 * r0 = counter (starts at 1)
 * r1 = sum (starts at 0)
 * r2 = limit (5)
 *
 * loop:
 *   sum += counter
 *   counter++
 *   if counter <= limit goto loop
 * return sum
 */
TEST(simple_loop_sum) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 0, 5 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = {
        &c->types[T_I32],  /* r0: counter */
        &c->types[T_I32],  /* r1: sum */
        &c->types[T_I32],  /* r2: limit */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* op0: r0 = 1 (counter) */
        OP2(OInt, 1, 1),       /* op1: r1 = 0 (sum) */
        OP2(OInt, 2, 2),       /* op2: r2 = 5 (limit) */
        /* loop body starts at op3 */
        OP0(OLabel),           /* op3: loop target */
        OP3(OAdd, 1, 1, 0),    /* op4: sum += counter */
        OP1(OIncr, 0),         /* op5: counter++ */
        OP3(OJSLte, 0, 2, -4), /* op6: if counter <= limit goto op3 (target = 7-4 = 3) */
        OP1(ORet, 1),          /* op7: return sum */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 8, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 15) {  /* 1+2+3+4+5 = 15 */
        fprintf(stderr, "    Expected 15, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Signed comparison with negative numbers: -5 < 5
 */
TEST(jump_slt_negative) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -5, 5, 42, 100 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* op0: r0 = -5 */
        OP2(OInt, 1, 1),      /* op1: r1 = 5 */
        OP2(OInt, 2, 2),      /* op2: r2 = 42 */
        OP3(OJSLt, 0, 1, 1),  /* op3: if r0 < r1 goto op5 (target = 4+1 = 5) */
        OP2(OInt, 2, 3),      /* op4: r2 = 100 (skipped) */
        OP1(ORet, 2),         /* op5: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 6, ops);

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
    TEST_ENTRY(jump_always_skip),
    TEST_ENTRY(jump_true_taken),
    TEST_ENTRY(jump_true_not_taken),
    TEST_ENTRY(jump_false_taken),
    TEST_ENTRY(jump_false_not_taken),
    TEST_ENTRY(jump_slt_taken),
    TEST_ENTRY(jump_slt_not_taken),
    TEST_ENTRY(jump_sgte_taken),
    TEST_ENTRY(jump_eq_taken),
    TEST_ENTRY(jump_eq_not_taken),
    TEST_ENTRY(jump_neq_taken),
    TEST_ENTRY(simple_loop_sum),
    TEST_ENTRY(jump_slt_negative),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Control Flow Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
