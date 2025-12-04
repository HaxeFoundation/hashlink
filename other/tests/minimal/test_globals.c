/*
 * Test global variable operations for HashLink AArch64 JIT
 *
 * Tests: OGetGlobal, OSetGlobal
 */
#include "test_harness.h"

/*
 * Helper to setup globals in the code structure
 */
static void test_init_globals(hl_code *c, int count, hl_type **types) {
    c->nglobals = count;
    c->globals = (hl_type**)malloc(sizeof(hl_type*) * count);
    memcpy(c->globals, types, sizeof(hl_type*) * count);
}

/*
 * Test: Set and get a global integer
 *
 * op0: int r0, 0       ; r0 = 42
 * op1: setglobal 0, r0 ; global[0] = r0
 * op2: getglobal r1, 0 ; r1 = global[0]
 * op3: ret r1          ; return 42
 */
TEST(global_int_set_get) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Setup one global of type i32 */
    hl_type *global_types[] = { &c->types[T_I32] };
    test_init_globals(c, 1, global_types);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),        /* op0: r0 = 42 */
        OP2(OSetGlobal, 0, 0),  /* op1: global[0] = r0 */
        OP2(OGetGlobal, 1, 0),  /* op2: r1 = global[0] */
        OP1(ORet, 1),           /* op3: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 4, ops);

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
 * Test: Multiple globals
 *
 * op0: int r0, 0       ; r0 = 10
 * op1: int r1, 1       ; r1 = 20
 * op2: setglobal 0, r0 ; global[0] = 10
 * op3: setglobal 1, r1 ; global[1] = 20
 * op4: getglobal r2, 0 ; r2 = global[0] = 10
 * op5: getglobal r3, 1 ; r3 = global[1] = 20
 * op6: add r4, r2, r3  ; r4 = 10 + 20 = 30
 * op7: ret r4          ; return 30
 */
TEST(global_multiple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20 };
    test_init_ints(c, 2, ints);

    /* Setup two globals of type i32 */
    hl_type *global_types[] = { &c->types[T_I32], &c->types[T_I32] };
    test_init_globals(c, 2, global_types);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = {
        &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
        &c->types[T_I32], &c->types[T_I32]
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),        /* op0: r0 = 10 */
        OP2(OInt, 1, 1),        /* op1: r1 = 20 */
        OP2(OSetGlobal, 0, 0),  /* op2: global[0] = r0 */
        OP2(OSetGlobal, 1, 1),  /* op3: global[1] = r1 */
        OP2(OGetGlobal, 2, 0),  /* op4: r2 = global[0] */
        OP2(OGetGlobal, 3, 1),  /* op5: r3 = global[1] */
        OP3(OAdd, 4, 2, 3),     /* op6: r4 = r2 + r3 */
        OP1(ORet, 4),           /* op7: return r4 */
    };

    test_alloc_function(c, 0, fn_type, 5, regs, 8, ops);

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
 * Test: Global persists across calls
 * Call function twice - first sets global, second reads it
 */
TEST(global_persists) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0, 99 };
    test_init_ints(c, 2, ints);

    /* Setup one global of type i32 */
    hl_type *global_types[] = { &c->types[T_I32] };
    test_init_globals(c, 1, global_types);

    /* Function takes an int arg: if arg==0, set global to 99; else return global */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *regs[] = {
        &c->types[T_I32],  /* r0: arg */
        &c->types[T_I32],  /* r1: temp */
    };

    hl_opcode ops[] = {
        OP2(OInt, 1, 0),        /* op0: r1 = 0 */
        OP3(OJEq, 0, 1, 2),     /* op1: if r0 == 0 goto op4 */
        OP2(OGetGlobal, 1, 0),  /* op2: r1 = global[0] */
        OP1(ORet, 1),           /* op3: return r1 */
        /* setter path */
        OP2(OInt, 1, 1),        /* op4: r1 = 99 */
        OP2(OSetGlobal, 0, 1),  /* op5: global[0] = 99 */
        OP1(ORet, 1),           /* op6: return 99 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 7, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* First call: set global to 99 */
    int ret1 = fn(0);
    if (ret1 != 99) {
        fprintf(stderr, "    First call: expected 99, got %d\n", ret1);
        return TEST_FAIL;
    }

    /* Second call: read global */
    int ret2 = fn(1);
    if (ret2 != 99) {
        fprintf(stderr, "    Second call: expected 99, got %d\n", ret2);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(global_int_set_get),
    TEST_ENTRY(global_multiple),
    TEST_ENTRY(global_persists),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Global Variable Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
