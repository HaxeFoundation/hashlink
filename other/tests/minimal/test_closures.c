/*
 * Test closure operations for HashLink AArch64 JIT
 *
 * Tests: OStaticClosure, OCallClosure
 *
 * These are key opcodes used in hello.hl's main function.
 */
#include "test_harness.h"

/*
 * Test: Create a static closure and call it with no args
 *
 * fn0: () -> i32  { return 42; }
 * fn1: () -> i32  {
 *   r0 = static_closure(fn0)   ; OStaticClosure
 *   r1 = call_closure(r0)      ; OCallClosure with 0 args
 *   return r1
 * }
 */
TEST(static_closure_call0) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function type: () -> i32 */
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* We need a closure type for the register holding the closure */
    /* For now, use the function type directly */

    /* Pre-allocate function array */
    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns 42 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),   /* r0 = 42 */
            OP1(ORet, 0),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_void_i32;
        f->nregs = 1;
        f->nops = 2;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 1);
        f->regs[0] = &c->types[T_I32];
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 2);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates closure and calls it */
    {
        /* r0 = closure (pointer type), r1 = result */
        hl_type *regs[] = { fn_type_void_i32, &c->types[T_I32] };

        /* OCallClosure: p1=dst, p2=closure_reg, p3=nargs, extra=args */
        hl_opcode ops[] = {
            OP2(OStaticClosure, 0, 0),  /* r0 = closure pointing to fn0 */
            {OCallClosure, 1, 0, 0, NULL},  /* r1 = call_closure(r0) with 0 args */
            OP1(ORet, 1),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn_type_void_i32;
        f->nregs = 2;
        f->nops = 3;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 2);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 3);
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
 * Test: Static closure with one argument
 *
 * fn0: (i32) -> i32  { return arg + 10; }
 * fn1: () -> i32  {
 *   r0 = static_closure(fn0)
 *   r1 = 32
 *   r2 = call_closure(r0, r1)   ; 32 + 10 = 42
 *   return r2
 * }
 */
TEST(static_closure_call1) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns arg + 10 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 1, 0),       /* r1 = 10 */
            OP3(OAdd, 2, 0, 1),    /* r2 = r0 + r1 */
            OP1(ORet, 2),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn_type_i32_i32;
        f->nregs = 3;
        f->nops = 3;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 3);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 3);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: findex=1, creates closure and calls with arg */
    {
        hl_type *regs[] = { fn_type_i32_i32, &c->types[T_I32], &c->types[T_I32] };

        /* OCallClosure with 1 arg: extra[0] = arg register */
        static int extra[] = { 1 };  /* r1 is the argument */
        hl_opcode ops[] = {
            OP2(OStaticClosure, 0, 0),  /* r0 = closure pointing to fn0 */
            OP2(OInt, 1, 1),            /* r1 = 32 */
            {OCallClosure, 2, 0, 1, extra},  /* r2 = call_closure(r0, r1) */
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
 * Test: Static closure with two arguments
 *
 * fn0: (i32, i32) -> i32  { return arg0 + arg1; }
 * fn1: () -> i32  {
 *   r0 = static_closure(fn0)
 *   r1 = 10
 *   r2 = 32
 *   r3 = call_closure(r0, r1, r2)   ; 10 + 32 = 42
 *   return r3
 * }
 *
 * This matches the pattern used in hello.hl's F27.
 */
TEST(static_closure_call2) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn_type_i32_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 2, arg_types);
    hl_type *fn_type_void_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: findex=0, returns arg0 + arg1 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP3(OAdd, 2, 0, 1),    /* r2 = r0 + r1 */
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

    /* fn1: findex=1, creates closure and calls with 2 args */
    {
        hl_type *regs[] = { fn_type_i32_i32_i32, &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

        /* OCallClosure with 2 args: extra[0] = arg0 reg, extra[1] = arg1 reg */
        static int extra[] = { 1, 2 };  /* r1 and r2 are the arguments */
        hl_opcode ops[] = {
            OP2(OStaticClosure, 0, 0),  /* r0 = closure pointing to fn0 */
            OP2(OInt, 1, 0),            /* r1 = 10 */
            OP2(OInt, 2, 1),            /* r2 = 32 */
            {OCallClosure, 3, 0, 2, extra},  /* r3 = call_closure(r0, r1, r2) */
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

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(static_closure_call0),
    TEST_ENTRY(static_closure_call1),
    TEST_ENTRY(static_closure_call2),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Closure Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
