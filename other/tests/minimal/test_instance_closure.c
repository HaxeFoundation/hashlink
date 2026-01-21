/*
 * Test instance and virtual closure operations for HashLink AArch64 JIT
 *
 * Tests: OInstanceClosure, OVirtualClosure, OCallClosure with captured values
 *
 * OInstanceClosure creates a closure that captures a value (typically 'this').
 * OVirtualClosure creates a closure from a virtual method lookup.
 */
#include "test_harness.h"

/*
 * Test: OInstanceClosure with captured i32 value
 *
 * fn0: (i32) -> i32  { return arg; }  // The captured value becomes the arg
 * fn1: () -> i32  {
 *   r0 = 42
 *   r1 = instance_closure(fn0, r0)   ; OInstanceClosure with captured value
 *   r2 = call_closure(r1)            ; OCallClosure with 0 explicit args
 *   return r2
 * }
 *
 * When called, the closure passes the captured value (42) as the first argument.
 */
TEST(instance_closure_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns its argument */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP1(ORet, 0),  /* return r0 (the captured value passed as arg) */
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_i32_i32;
        f->nregs = 1;
        f->nops = 1;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 1);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 1);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates instance closure and calls it */
    {
        /* r0 = captured value, r1 = closure, r2 = result */
        hl_type *regs[] = { &c->types[T_I32], fn_type_i32_i32, &c->types[T_I32] };

        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                /* r0 = 42 (captured value) */
            OP3(OInstanceClosure, 1, 0, 0), /* r1 = closure(fn0, r0) */
            {OCallClosure, 2, 1, 0, NULL},  /* r2 = call_closure(r1) with 0 explicit args */
            OP1(ORet, 2),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn_type_void_i32;
        f->nregs = 3;
        f->nops = 4;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 3);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 4);
        memcpy(f->ops, ops, sizeof(ops));
    }

    c->entrypoint = 1;

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
 * Test: OInstanceClosure with captured value and additional arguments
 *
 * fn0: (i32, i32) -> i32  { return arg0 + arg1; }
 * fn1: () -> i32  {
 *   r0 = 10                          ; value to capture
 *   r1 = instance_closure(fn0, r0)   ; closure captures 10
 *   r2 = 32
 *   r3 = call_closure(r1, r2)        ; calls fn0(10, 32) = 42
 *   return r3
 * }
 */
TEST(instance_closure_with_arg) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *two_args[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn_type_i32_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 2, two_args);
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* For the closure type: when called with 1 arg, passes captured + arg */
    hl_type *one_arg[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, one_arg);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns arg0 + arg1 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP3(OAdd, 2, 0, 1),  /* r2 = r0 + r1 */
            OP1(ORet, 2),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_i32_i32_i32;
        f->nregs = 3;
        f->nops = 2;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 3);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 2);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates instance closure and calls with additional arg */
    {
        /* r0 = captured value, r1 = closure, r2 = additional arg, r3 = result */
        hl_type *regs[] = { &c->types[T_I32], fn_type_i32_i32, &c->types[T_I32], &c->types[T_I32] };

        static int extra[] = { 2 };  /* r2 is the additional argument */
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                /* r0 = 10 (captured value) */
            OP3(OInstanceClosure, 1, 0, 0), /* r1 = closure(fn0, r0) */
            OP2(OInt, 2, 1),                /* r2 = 32 */
            {OCallClosure, 3, 1, 1, extra}, /* r3 = call_closure(r1, r2) -> fn0(10, 32) */
            OP1(ORet, 3),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn_type_void_i32;
        f->nregs = 4;
        f->nops = 5;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 4);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 5);
        memcpy(f->ops, ops, sizeof(ops));
    }

    c->entrypoint = 1;

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
 * Test: OInstanceClosure used in a loop pattern
 *
 * This tests that closures work correctly when called multiple times,
 * similar to how they're used in event handlers.
 *
 * fn0: (i32, i32) -> i32  { return arg0 + arg1; }
 * fn1: () -> i32  {
 *   r0 = 0                           ; accumulator
 *   r1 = instance_closure(fn0, r0)   ; closure captures accumulator reference
 *   // Call closure 3 times with different values
 *   r2 = 10
 *   r3 = call_closure(r1, r2)        ; 0 + 10 = 10
 *   r4 = 20
 *   r5 = call_closure(r1, r4)        ; 0 + 20 = 20
 *   r6 = r3 + r5                     ; 10 + 20 = 30
 *   return r6
 * }
 */
TEST(instance_closure_multiple_calls) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0, 10, 20 };
    test_init_ints(c, 3, ints);

    /* Function types */
    hl_type *two_args[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn_type_i32_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 2, two_args);
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *one_arg[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, one_arg);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns arg0 + arg1 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP3(OAdd, 2, 0, 1),
            OP1(ORet, 2),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_i32_i32_i32;
        f->nregs = 3;
        f->nops = 2;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 3);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 2);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates closure and calls it multiple times */
    {
        /*
         * r0 = captured base value (0)
         * r1 = closure
         * r2 = first arg (10)
         * r3 = first result
         * r4 = second arg (20)
         * r5 = second result
         * r6 = final sum
         */
        hl_type *regs[] = {
            &c->types[T_I32],    /* r0 */
            fn_type_i32_i32,     /* r1 */
            &c->types[T_I32],    /* r2 */
            &c->types[T_I32],    /* r3 */
            &c->types[T_I32],    /* r4 */
            &c->types[T_I32],    /* r5 */
            &c->types[T_I32],    /* r6 */
        };

        static int extra1[] = { 2 };
        static int extra2[] = { 4 };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                /* r0 = 0 */
            OP3(OInstanceClosure, 1, 0, 0), /* r1 = closure(fn0, r0) */
            OP2(OInt, 2, 1),                /* r2 = 10 */
            {OCallClosure, 3, 1, 1, extra1},/* r3 = closure(10) = 0 + 10 = 10 */
            OP2(OInt, 4, 2),                /* r4 = 20 */
            {OCallClosure, 5, 1, 1, extra2},/* r5 = closure(20) = 0 + 20 = 20 */
            OP3(OAdd, 6, 3, 5),             /* r6 = r3 + r5 = 30 */
            OP1(ORet, 6),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn_type_void_i32;
        f->nregs = 7;
        f->nops = 8;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 7);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 8);
        memcpy(f->ops, ops, sizeof(ops));
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 30) {
        fprintf(stderr, "    Expected 30, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OInstanceClosure with i64 captured value
 *
 * This tests that pointer-sized captured values work correctly.
 */
TEST(instance_closure_i64) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function types with i64 */
    hl_type *arg_types[] = { &c->types[T_I64] };
    hl_type *fn_type_i64_i64 = test_alloc_fun_type(c, &c->types[T_I64], 1, arg_types);
    hl_type *fn_type_void_i64 = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns its argument */
    {
        hl_type *regs[] = { &c->types[T_I64] };
        hl_opcode ops[] = {
            OP1(ORet, 0),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_i64_i64;
        f->nregs = 1;
        f->nops = 1;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 1);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 1);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates instance closure with i64 */
    {
        hl_type *regs[] = { &c->types[T_I64], fn_type_i64_i64, &c->types[T_I64] };

        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                /* r0 = 42 (will be i64) */
            OP3(OInstanceClosure, 1, 0, 0), /* r1 = closure(fn0, r0) */
            {OCallClosure, 2, 1, 0, NULL},  /* r2 = call_closure(r1) */
            OP1(ORet, 2),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn_type_void_i64;
        f->nregs = 3;
        f->nops = 4;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 3);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 4);
        memcpy(f->ops, ops, sizeof(ops));
    }

    c->entrypoint = 1;

    int result;
    int64_t (*fn)(void) = (int64_t(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(instance_closure_basic),
    TEST_ENTRY(instance_closure_with_arg),
    TEST_ENTRY(instance_closure_multiple_calls),
    TEST_ENTRY(instance_closure_i64),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Instance Closure Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
