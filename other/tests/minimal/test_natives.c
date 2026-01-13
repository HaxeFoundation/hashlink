/*
 * Test native function calls for HashLink AArch64 JIT
 *
 * Tests calling C functions from JIT code
 */
#include "test_harness.h"

/* Simple native functions for testing */
static int native_return_42(void) {
    return 42;
}

static int native_add(int a, int b) {
    return a + b;
}

static int native_add3(int a, int b, int c) {
    return a + b + c;
}

static int g_side_effect = 0;

static void native_set_global(int val) {
    g_side_effect = val;
}

static int native_get_global(void) {
    return g_side_effect;
}

/*
 * Test: Call native function with no args
 *
 * op0: call0 r0, native_return_42
 * op1: ret r0
 */
TEST(native_call0) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Native at findex 1, our function at findex 0 */
    hl_type *native_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    test_add_native(c, 1, "test", "return_42", native_type, native_return_42);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OCall0, 0, 1),    /* op0: r0 = call native findex=1 */
        OP1(ORet, 0),         /* op1: return r0 */
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
 * Test: Call native function with 2 args
 *
 * op0: int r0, 0       ; r0 = 10
 * op1: int r1, 1       ; r1 = 32
 * op2: call2 r2, native_add, r0, r1
 * op3: ret r2          ; return 42
 */
TEST(native_call2) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Native at findex 1 */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *native_type = test_alloc_fun_type(c, &c->types[T_I32], 2, arg_types);
    test_add_native(c, 1, "test", "add", native_type, native_add);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* op0: r0 = 10 */
        OP2(OInt, 1, 1),           /* op1: r1 = 32 */
        OP4_CALL2(OCall2, 2, 1, 0, 1),  /* op2: r2 = call native(r0, r1) */
        OP1(ORet, 2),              /* op3: return r2 */
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
 * Test: Call native function with 3 args (uses OCall3)
 *
 * op0: int r0, 0       ; r0 = 10
 * op1: int r1, 1       ; r1 = 20
 * op2: int r2, 2       ; r2 = 12
 * op3: call3 r3, native_add3, r0, r1, r2
 * op4: ret r3          ; return 42
 */
TEST(native_call3) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 12 };
    test_init_ints(c, 3, ints);

    /* Native at findex 1 */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
    hl_type *native_type = test_alloc_fun_type(c, &c->types[T_I32], 3, arg_types);
    test_add_native(c, 1, "test", "add3", native_type, native_add3);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = {
        &c->types[T_I32], &c->types[T_I32],
        &c->types[T_I32], &c->types[T_I32]
    };

    /* OCall3: p1=dst, p2=findex, p3=arg0, extra[0]=arg1, extra[1]=arg2 */
    int extra[] = { 1, 2 };
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* op0: r0 = 10 */
        OP2(OInt, 1, 1),           /* op1: r1 = 20 */
        OP2(OInt, 2, 2),           /* op2: r2 = 12 */
        {OCall3, 3, 1, 0, extra},  /* op3: r3 = call native(r0, r1, r2) */
        OP1(ORet, 3),              /* op4: return r3 */
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 5, ops);

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
 * Test: Call void native function (side effect)
 *
 * op0: int r0, 0       ; r0 = 99
 * op1: call1 r1, native_set_global, r0
 * op2: call0 r2, native_get_global
 * op3: ret r2          ; return 99
 */
TEST(native_void_call) {
    test_init_runtime();

    g_side_effect = 0;  /* Reset */

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 99 };
    test_init_ints(c, 1, ints);

    /* Two natives */
    hl_type *set_args[] = { &c->types[T_I32] };
    hl_type *set_type = test_alloc_fun_type(c, &c->types[T_VOID], 1, set_args);
    test_add_native(c, 1, "test", "set_global", set_type, native_set_global);

    hl_type *get_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    test_add_native(c, 2, "test", "get_global", get_type, native_get_global);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_VOID], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* op0: r0 = 99 */
        OP3(OCall1, 1, 1, 0),  /* op1: call set_global(r0) */
        OP2(OCall0, 2, 2),     /* op2: r2 = call get_global() */
        OP1(ORet, 2),          /* op3: return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 99) {
        fprintf(stderr, "    Expected 99, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(native_call0),
    TEST_ENTRY(native_call2),
    TEST_ENTRY(native_call3),
    TEST_ENTRY(native_void_call),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Native Function Call Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
