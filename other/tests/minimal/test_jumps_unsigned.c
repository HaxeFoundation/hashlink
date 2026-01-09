/*
 * Test unsigned jump operations for HashLink AArch64 JIT
 *
 * Tests: OJULt, OJUGte, OJNotLt, OJNotGte, OJSGt
 *
 * These opcodes perform unsigned comparisons and conditional jumps.
 * OJNotLt and OJNotGte are for NaN-aware float comparisons.
 */
#include "test_harness.h"

/*
 * Test: OJULt - unsigned less than
 *
 * Tests that -1 (0xFFFFFFFF) is NOT less than 1 when compared as unsigned.
 * With signed comparison, -1 < 1 would be true.
 */
TEST(jult_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 1, 10, 20 };  /* -1 as unsigned is 0xFFFFFFFF */
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = large unsigned (0xFFFFFFFF) */
        &c->types[T_I32],  /* r1 = small value (1) */
        &c->types[T_I32],  /* r2 = result */
    };

    /*
     * if (0xFFFFFFFF <u 1) goto true_branch
     * r2 = 10  ; false branch (correct - 0xFFFFFFFF is NOT < 1 unsigned)
     * goto end
     * true_branch:
     * r2 = 20  ; true branch (wrong)
     * end:
     * return r2
     *
     * OLabel is required at jump targets to discard stale register bindings.
     */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = -1 (0xFFFFFFFF), opcode 0 */
        OP2(OInt, 1, 1),       /* r1 = 1, opcode 1 */
        OP3(OJULt, 0, 1, 3),   /* if r0 <u r1 goto opcode 6, opcode 2 */
        OP2(OInt, 2, 2),       /* r2 = 10 (false branch), opcode 3 */
        OP2(OJAlways, 2, 0),   /* goto opcode 7, opcode 4 */
        OP0(OLabel),           /* true branch target, opcode 5 */
        OP2(OInt, 2, 3),       /* r2 = 20 (true branch), opcode 6 */
        OP0(OLabel),           /* end (merge point), opcode 7 */
        OP1(ORet, 2),          /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (false branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OJULt with small values - 1 <u 100 should be true
 */
TEST(jult_small_values) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 100, 10, 20 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 1, opcode 0 */
        OP2(OInt, 1, 1),       /* r1 = 100, opcode 1 */
        OP3(OJULt, 0, 1, 3),   /* if r0 <u r1 goto opcode 6, opcode 2 */
        OP2(OInt, 2, 3),       /* r2 = 20 (false branch), opcode 3 */
        OP2(OJAlways, 2, 0),   /* goto opcode 7, opcode 4 */
        OP0(OLabel),           /* true branch target, opcode 5 */
        OP2(OInt, 2, 2),       /* r2 = 10 (true branch), opcode 6 */
        OP0(OLabel),           /* end (merge point), opcode 7 */
        OP1(ORet, 2),          /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (true branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OJUGte - unsigned greater than or equal
 *
 * 0xFFFFFFFF >=u 1 should be true (unsigned)
 */
TEST(jugte_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 1, 10, 20 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = -1 (0xFFFFFFFF), opcode 0 */
        OP2(OInt, 1, 1),       /* r1 = 1, opcode 1 */
        OP3(OJUGte, 0, 1, 3),  /* if r0 >=u r1 goto opcode 6, opcode 2 */
        OP2(OInt, 2, 3),       /* r2 = 20 (false branch), opcode 3 */
        OP2(OJAlways, 2, 0),   /* goto opcode 7, opcode 4 */
        OP0(OLabel),           /* true branch target, opcode 5 */
        OP2(OInt, 2, 2),       /* r2 = 10 (true branch), opcode 6 */
        OP0(OLabel),           /* end (merge point), opcode 7 */
        OP1(ORet, 2),          /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (true branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OJSGt - signed greater than
 *
 * Tests signed comparison: 1 > -1 should be true
 */
TEST(jsgt_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, -1, 10, 20 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = 1, opcode 0 */
        OP2(OInt, 1, 1),       /* r1 = -1, opcode 1 */
        OP3(OJSGt, 0, 1, 3),   /* if r0 > r1 (signed) goto opcode 6, opcode 2 */
        OP2(OInt, 2, 3),       /* r2 = 20 (false branch), opcode 3 */
        OP2(OJAlways, 2, 0),   /* goto opcode 7, opcode 4 */
        OP0(OLabel),           /* true branch target, opcode 5 */
        OP2(OInt, 2, 2),       /* r2 = 10 (true branch), opcode 6 */
        OP0(OLabel),           /* end (merge point), opcode 7 */
        OP1(ORet, 2),          /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (true branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OJNotLt - "not less than" for NaN-aware float comparison
 *
 * For floats, NaN comparisons need special handling.
 * OJNotLt: jumps if !(a < b), which includes NaN cases.
 */
TEST(jnotlt_float) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 2.0, 1.0 };  /* 2.0 is not less than 1.0 */
    test_init_floats(c, 2, floats);

    int ints[] = { 10, 20 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_F64],
        &c->types[T_F64],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),      /* r0 = 2.0, opcode 0 */
        OP2(OFloat, 1, 1),      /* r1 = 1.0, opcode 1 */
        OP3(OJNotLt, 0, 1, 3),  /* if !(r0 < r1) goto opcode 6, opcode 2 */
        OP2(OInt, 2, 1),        /* r2 = 20 (false: r0 < r1), opcode 3 */
        OP2(OJAlways, 2, 0),    /* goto opcode 7, opcode 4 */
        OP0(OLabel),            /* true branch target, opcode 5 */
        OP2(OInt, 2, 0),        /* r2 = 10 (true: r0 >= r1 or NaN), opcode 6 */
        OP0(OLabel),            /* end (merge point), opcode 7 */
        OP1(ORet, 2),           /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    /* 2.0 is NOT less than 1.0, so we should take the true branch */
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (not-less-than branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OJNotGte - "not greater than or equal" for NaN-aware comparison
 */
TEST(jnotgte_float) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 1.0, 2.0 };  /* 1.0 is not >= 2.0 */
    test_init_floats(c, 2, floats);

    int ints[] = { 10, 20 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_F64],
        &c->types[T_F64],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),       /* r0 = 1.0, opcode 0 */
        OP2(OFloat, 1, 1),       /* r1 = 2.0, opcode 1 */
        OP3(OJNotGte, 0, 1, 3),  /* if !(r0 >= r1) goto opcode 6, opcode 2 */
        OP2(OInt, 2, 1),         /* r2 = 20 (false: r0 >= r1), opcode 3 */
        OP2(OJAlways, 2, 0),     /* goto opcode 7, opcode 4 */
        OP0(OLabel),             /* true branch target, opcode 5 */
        OP2(OInt, 2, 0),         /* r2 = 10 (true: r0 < r1 or NaN), opcode 6 */
        OP0(OLabel),             /* end (merge point), opcode 7 */
        OP1(ORet, 2),            /* opcode 8 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 9, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    /* 1.0 is NOT >= 2.0, so we should take the true branch */
    if (ret != 10) {
        fprintf(stderr, "    Expected 10 (not-gte branch), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Compare signed vs unsigned jump behavior
 *
 * -1 vs 1:
 *   Signed: -1 < 1 (true)
 *   Unsigned: 0xFFFFFFFF > 1 (true)
 */
TEST(signed_vs_unsigned) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1, 1, 0 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = -1 */
        &c->types[T_I32],  /* r1 = 1 */
        &c->types[T_I32],  /* r2 = signed result */
        &c->types[T_I32],  /* r3 = unsigned result */
        &c->types[T_I32],  /* r4 = combined */
    };

    /*
     * Test signed: -1 < 1 (true) -> r2 = 1
     * Test unsigned: -1 <u 1 (false) -> r3 = 0
     * Return r2 * 10 + r3 = 10
     *
     * Structure for each test:
     *   if (condition) goto set_value
     *   goto after_test
     *   OLabel (set_value target)
     *   set value = 1
     *   OLabel (after_test / merge point)
     */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0 = -1, opcode 0 */
        OP2(OInt, 1, 1),       /* r1 = 1, opcode 1 */
        OP2(OInt, 2, 2),       /* r2 = 0 (default), opcode 2 */
        OP2(OInt, 3, 2),       /* r3 = 0 (default), opcode 3 */
        /* Signed test: if -1 < 1 (true), set r2 = 1 */
        OP3(OJSLt, 0, 1, 2),   /* if r0 < r1 goto opcode 7 (set_r2), opcode 4 */
        OP2(OJAlways, 2, 0),   /* goto opcode 8 (after_signed), opcode 5 */
        OP0(OLabel),           /* set_r2 target, opcode 6 */
        OP2(OInt, 2, 1),       /* r2 = 1 (signed true), opcode 7 */
        OP0(OLabel),           /* after_signed, opcode 8 */
        /* Unsigned test: if -1 <u 1 (false), set r3 = 1 */
        OP3(OJULt, 0, 1, 2),   /* if r0 <u r1 goto opcode 12 (set_r3), opcode 9 */
        OP2(OJAlways, 2, 0),   /* goto opcode 13 (after_unsigned), opcode 10 */
        OP0(OLabel),           /* set_r3 target, opcode 11 */
        OP2(OInt, 3, 1),       /* r3 = 1 (unsigned true), opcode 12 */
        OP0(OLabel),           /* after_unsigned, opcode 13 */
        /* Combine: r4 = r2 * 10 + r3 */
        OP2(OInt, 4, 1),       /* r4 = 1 (will use as 10), opcode 14 */
        OP3(OAdd, 4, 4, 4),    /* r4 = 2, opcode 15 */
        OP3(OAdd, 4, 4, 4),    /* r4 = 4, opcode 16 */
        OP3(OAdd, 4, 4, 4),    /* r4 = 8, opcode 17 */
        OP3(OAdd, 4, 4, 1),    /* r4 = 9, opcode 18 */
        OP3(OAdd, 4, 4, 1),    /* r4 = 10, opcode 19 */
        OP3(OMul, 4, 2, 4),    /* r4 = r2 * 10, opcode 20 */
        OP3(OAdd, 4, 4, 3),    /* r4 = r2 * 10 + r3, opcode 21 */
        OP1(ORet, 4),          /* opcode 22 */
    };

    test_alloc_function(c, 0, fn_type, 5, regs, 23, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    /* Signed: -1 < 1 is TRUE, so r2 = 1
     * Unsigned: -1 <u 1 is FALSE (0xFFFFFFFF > 1), so r3 = 0
     * Result: 1 * 10 + 0 = 10
     */
    if (ret != 10) {
        fprintf(stderr, "    Expected 10, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(jult_basic),
    TEST_ENTRY(jult_small_values),
    TEST_ENTRY(jugte_basic),
    TEST_ENTRY(jsgt_basic),
    TEST_ENTRY(jnotlt_float),
    TEST_ENTRY(jnotgte_float),
    TEST_ENTRY(signed_vs_unsigned),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Unsigned Jump Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
