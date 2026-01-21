/*
 * Test function call operations for HashLink AArch64 JIT
 *
 * Tests: OCall0, OCall1, OCall2, OCall3
 *
 * These tests require multiple functions in the hl_code structure.
 */
#include "test_harness.h"
#include <math.h>

/* Helper to allocate multiple functions at once */
static void test_alloc_functions(hl_code *c, int count) {
    c->functions = (hl_function*)calloc(count, sizeof(hl_function));
    c->nfunctions = 0;  /* Will be incremented as we add */
}

/* Add a function to existing array */
static hl_function *test_add_function(hl_code *c, int findex, hl_type *type,
                                      int nregs, hl_type **regs,
                                      int nops, hl_opcode *ops) {
    hl_function *f = &c->functions[c->nfunctions++];
    f->findex = findex;
    f->type = type;
    f->nregs = nregs;
    f->nops = nops;

    f->regs = (hl_type**)malloc(sizeof(hl_type*) * nregs);
    memcpy(f->regs, regs, sizeof(hl_type*) * nregs);

    f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * nops);
    memcpy(f->ops, ops, sizeof(hl_opcode) * nops);

    f->debug = NULL;
    f->obj = NULL;
    f->field.ref = NULL;
    f->ref = 0;

    return f;
}

/*
 * Test: Call function with 0 arguments
 *
 * fn0: () -> i32  { return 42; }
 * fn1: () -> i32  { return call0(fn0); }  <- entry point
 */
TEST(call0_simple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function types */
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Pre-allocate function array */
    test_alloc_functions(c, 2);

    /* fn0: findex=0, returns 42 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),   /* r0 = 42 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: findex=1, calls fn0 (entry point) */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OCall0, 0, 0),  /* r0 = call fn0() */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 1, regs, 2, ops);
    }

    c->entrypoint = 1;  /* fn1 is entry */

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
 * Test: Call function with 1 argument
 *
 * fn0: (i32) -> i32  { return arg + 10; }
 * fn1: () -> i32     { return call1(fn0, 32); }  <- entry point
 */
TEST(call1_simple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: findex=0, returns arg0 + 10 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            /* r0 = first argument (passed in) */
            OP2(OInt, 1, 0),       /* r1 = 10 */
            OP3(OAdd, 2, 0, 1),    /* r2 = r0 + r1 */
            OP1(ORet, 2),
        };
        test_add_function(c, 0, fn_type_i32_i32, 3, regs, 3, ops);
    }

    /* fn1: findex=1, calls fn0(32) */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 1),        /* r1 = 32 */
            OP3(OCall1, 0, 0, 1),   /* r0 = call fn0(r1) */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 2, regs, 3, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {  /* 32 + 10 = 42 */
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Call function with 2 arguments
 *
 * fn0: (i32, i32) -> i32  { return a + b; }
 * fn1: () -> i32          { return call2(fn0, 10, 32); }
 */
TEST(call2_simple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn_type_i32_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 2, arg_types);
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: findex=0, returns arg0 + arg1 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            /* r0 = arg0, r1 = arg1 */
            OP3(OAdd, 2, 0, 1),    /* r2 = r0 + r1 */
            OP1(ORet, 2),
        };
        test_add_function(c, 0, fn_type_i32_i32_i32, 3, regs, 2, ops);
    }

    /* fn1: findex=1, calls fn0(10, 32) */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 0),             /* r1 = 10 */
            OP2(OInt, 2, 1),             /* r2 = 32 */
            OP4_CALL2(OCall2, 0, 0, 1, 2),  /* r0 = call fn0(r1, r2) */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 3, regs, 4, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {  /* 10 + 32 = 42 */
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Nested calls
 *
 * fn0: (i32) -> i32  { return arg * 2; }
 * fn1: (i32) -> i32  { return call1(fn0, arg) + 1; }
 * fn2: () -> i32     { return call1(fn1, 20); }  <- entry (20*2+1 = 41, need 42)
 *
 * Actually: 21 * 2 = 42
 */
TEST(nested_calls) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 2, 21 };
    test_init_ints(c, 2, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 3);

    /* fn0: findex=0, returns arg * 2 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 0),       /* r1 = 2 */
            OP3(OMul, 2, 0, 1),    /* r2 = r0 * 2 */
            OP1(ORet, 2),
        };
        test_add_function(c, 0, fn_type_i32_i32, 3, regs, 3, ops);
    }

    /* fn1: findex=1, returns call0(fn0, arg) */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP3(OCall1, 1, 0, 0),  /* r1 = call fn0(r0) */
            OP1(ORet, 1),
        };
        test_add_function(c, 1, fn_type_i32_i32, 2, regs, 2, ops);
    }

    /* fn2: findex=2, entry point */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 1),        /* r1 = 21 */
            OP3(OCall1, 0, 1, 1),   /* r0 = call fn1(r1) */
            OP1(ORet, 0),
        };
        test_add_function(c, 2, fn_type_i32, 2, regs, 3, ops);
    }

    c->entrypoint = 2;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {  /* 21 * 2 = 42 */
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Recursive call (factorial)
 *
 * fn0: (i32) -> i32 {
 *   if (n <= 1) return 1;
 *   return n * call1(fn0, n-1);
 * }
 * fn1: () -> i32 { return call1(fn0, 5); }  <- 5! = 120
 *
 * Note: We want result 42, so let's compute something else
 * Let's do: sum from 1 to n recursively
 * sum(n) = n + sum(n-1), sum(0) = 0
 * sum(8) = 8+7+6+5+4+3+2+1 = 36... not 42
 * sum(9) = 45...
 *
 * Let's just verify 5! = 120 works, and accept that as the test value
 */
TEST(recursive_factorial) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 5 };
    test_init_ints(c, 2, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: findex=0, factorial(n)
     * r0 = n
     * r1 = 1 (constant)
     * r2 = temp
     * r3 = n-1
     * r4 = result of recursive call
     */
    {
        hl_type *regs[] = {
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
        };
        hl_opcode ops[] = {
            OP2(OInt, 1, 0),        /* op0: r1 = 1 */
            OP3(OJSLte, 0, 1, 2),   /* op1: if n <= 1 goto op3 */
            OP1(OJAlways, 3),       /* op2: else goto op6 (skip return 1) */
            /* return 1 path */
            OP0(OLabel),            /* op3: label */
            OP1(ORet, 1),           /* op4: return 1 */
            /* recursive path */
            OP0(OLabel),            /* op5: label */
            OP3(OSub, 3, 0, 1),     /* op6: r3 = n - 1 */
            OP3(OCall1, 4, 0, 3),   /* op7: r4 = factorial(n-1) */
            OP3(OMul, 2, 0, 4),     /* op8: r2 = n * r4 */
            OP1(ORet, 2),           /* op9: return r2 */
        };
        test_add_function(c, 0, fn_type_i32_i32, 5, regs, 10, ops);
    }

    /* fn1: findex=1, entry point */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 1),        /* r1 = 5 */
            OP3(OCall1, 0, 0, 1),   /* r0 = factorial(5) */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 2, regs, 3, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 120) {  /* 5! = 120 */
        fprintf(stderr, "    Expected 120, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Call with float argument
 */
TEST(call1_float) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 10.5, 31.5 };
    test_init_floats(c, 2, floats);

    hl_type *arg_types[] = { &c->types[T_F64] };
    hl_type *fn_type_f64_f64 = test_alloc_fun_type(c, &c->types[T_F64], 1, arg_types);
    hl_type *fn_type_f64 = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: findex=0, returns arg + 10.5 */
    {
        hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64], &c->types[T_F64] };
        hl_opcode ops[] = {
            OP2(OFloat, 1, 0),     /* r1 = 10.5 */
            OP3(OAdd, 2, 0, 1),    /* r2 = r0 + r1 */
            OP1(ORet, 2),
        };
        test_add_function(c, 0, fn_type_f64_f64, 3, regs, 3, ops);
    }

    /* fn1: findex=1, calls fn0(31.5) */
    {
        hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64] };
        hl_opcode ops[] = {
            OP2(OFloat, 1, 1),      /* r1 = 31.5 */
            OP3(OCall1, 0, 0, 1),   /* r0 = call fn0(r1) */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_f64, 2, regs, 3, ops);
    }

    c->entrypoint = 1;

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (fabs(ret - 42.0) > 1e-9) {  /* 31.5 + 10.5 = 42.0 */
        fprintf(stderr, "    Expected 42.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(call0_simple),
    TEST_ENTRY(call1_simple),
    TEST_ENTRY(call2_simple),
    TEST_ENTRY(nested_calls),
    TEST_ENTRY(recursive_factorial),
    TEST_ENTRY(call1_float),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Function Call Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
