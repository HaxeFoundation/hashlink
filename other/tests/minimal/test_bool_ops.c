/*
 * Test boolean operations for HashLink AArch64 JIT
 *
 * Tests: OBool, ONot
 */
#include "test_harness.h"

/*
 * Test: Return true
 */
TEST(return_true) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),   /* r0 = true (p2=1 means true) */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1 (true), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Return false
 */
TEST(return_false) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 0),   /* r0 = false (p2=0 means false) */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 0) {
        fprintf(stderr, "    Expected 0 (false), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: NOT true = false
 */
TEST(not_true) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),   /* r0 = true */
        OP2(ONot, 1, 0),    /* r1 = !r0 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 0) {
        fprintf(stderr, "    Expected 0 (false), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: NOT false = true
 */
TEST(not_false) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 0),   /* r0 = false */
        OP2(ONot, 1, 0),    /* r1 = !r0 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1 (true), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Double NOT: !!true = true
 */
TEST(double_not_true) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),   /* r0 = true */
        OP2(ONot, 1, 0),    /* r1 = !r0 = false */
        OP2(ONot, 2, 1),    /* r2 = !r1 = true */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1 (true), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: NOT on bool (false) -> true
 * Note: ONot is only valid for boolean operands (0 or 1).
 * Using OInt with 0 works because bool false is represented as 0.
 */
TEST(not_bool_false) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 0),   /* r0 = false (0) */
        OP2(ONot, 1, 0),    /* r1 = !r0 = true */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1 (true), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: NOT on bool (true) -> false
 */
TEST(not_bool_true_explicit) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),   /* r0 = true (1) */
        OP2(ONot, 1, 0),    /* r1 = !r0 = false */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 0) {
        fprintf(stderr, "    Expected 0 (false), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Move bool register
 */
TEST(mov_bool) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BOOL], 0, NULL);
    hl_type *regs[] = { &c->types[T_BOOL], &c->types[T_BOOL] };

    hl_opcode ops[] = {
        OP2(OBool, 0, 1),   /* r0 = true */
        OP2(OMov, 1, 0),    /* r1 = r0 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Expected 1 (true), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(return_true),
    TEST_ENTRY(return_false),
    TEST_ENTRY(not_true),
    TEST_ENTRY(not_false),
    TEST_ENTRY(double_not_true),
    TEST_ENTRY(not_bool_false),
    TEST_ENTRY(not_bool_true_explicit),
    TEST_ENTRY(mov_bool),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Boolean Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
