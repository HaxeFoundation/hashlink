/*
 * Test C-to-HL callback mechanism for HashLink AArch64 JIT
 *
 * Tests the callback_c2hl and jit_c2hl trampoline by:
 * 1. JIT compiling a function with arguments
 * 2. Calling it through hl_dyn_call (which uses callback_c2hl)
 *
 * This exercises the path: hl_dyn_call -> hl_call_method -> callback_c2hl -> jit_c2hl -> JIT code
 */
#include "test_harness.h"

/* hl_dyn_call declaration from hl.h */
extern vdynamic *hl_dyn_call(vclosure *c, vdynamic **args, int nargs);
extern vdynamic *hl_alloc_dynamic(hl_type *t);

/*
 * Test: Simple function call through callback (no arguments)
 *
 * JIT function: () -> i32 { return 42 }
 * Call through hl_dyn_call and verify result
 */
TEST(callback_no_args) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Function type: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),    /* r0 = 42 */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure for the function */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, NULL, 0);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.i != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret->v.i);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Function with one i32 argument
 *
 * JIT function: (i32 x) -> i32 { return x + 10 }
 */
TEST(callback_one_int_arg) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10 };
    test_init_ints(c, 1, ints);

    /* Function type: (i32) -> i32 */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    /* r0 = arg (i32), r1 = result (i32), r2 = const 10 */
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 2, 0),        /* r2 = 10 */
        OP3(OAdd, 1, 0, 2),     /* r1 = r0 + r2 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 3, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create argument: i32 value = 32 */
    vdynamic arg_val;
    arg_val.t = &c->types[T_I32];
    arg_val.v.i = 32;
    vdynamic *args[] = { &arg_val };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 1);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    /* Expected: 32 + 10 = 42 */
    if (ret->v.i != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret->v.i);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Function with two i32 arguments
 *
 * JIT function: (i32 a, i32 b) -> i32 { return a + b }
 */
TEST(callback_two_int_args) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (i32, i32) -> i32 */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 2, arg_types);

    /* r0 = arg0, r1 = arg1, r2 = result */
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP3(OAdd, 2, 0, 1),     /* r2 = r0 + r1 */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 2, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create arguments: 10 + 32 = 42 */
    vdynamic arg0, arg1;
    arg0.t = &c->types[T_I32];
    arg0.v.i = 10;
    arg1.t = &c->types[T_I32];
    arg1.v.i = 32;
    vdynamic *args[] = { &arg0, &arg1 };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 2);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.i != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret->v.i);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Function with i64 argument
 *
 * JIT function: (i64 x) -> i64 { return x }
 */
TEST(callback_i64_arg) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (i64) -> i64 */
    hl_type *arg_types[] = { &c->types[T_I64] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 1, arg_types);

    /* r0 = arg (i64) */
    hl_type *regs[] = { &c->types[T_I64] };

    hl_opcode ops[] = {
        OP1(ORet, 0),           /* return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 1, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create argument: i64 value = 0x123456789ABCDEF0 */
    vdynamic arg_val;
    arg_val.t = &c->types[T_I64];
    arg_val.v.i64 = 0x123456789ABCDEF0LL;
    vdynamic *args[] = { &arg_val };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 1);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.i64 != 0x123456789ABCDEF0LL) {
        fprintf(stderr, "    Expected 0x123456789ABCDEF0, got 0x%llx\n",
                (unsigned long long)ret->v.i64);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Function with f64 argument
 *
 * JIT function: (f64 x) -> f64 { return x }
 */
TEST(callback_f64_arg) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (f64) -> f64 */
    hl_type *arg_types[] = { &c->types[T_F64] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 1, arg_types);

    /* r0 = arg (f64) */
    hl_type *regs[] = { &c->types[T_F64] };

    hl_opcode ops[] = {
        OP1(ORet, 0),           /* return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 1, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create argument: f64 value = 3.14159 */
    vdynamic arg_val;
    arg_val.t = &c->types[T_F64];
    arg_val.v.d = 3.14159;
    vdynamic *args[] = { &arg_val };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 1);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    double diff = ret->v.d - 3.14159;
    if (diff < -0.00001 || diff > 0.00001) {
        fprintf(stderr, "    Expected 3.14159, got %f\n", ret->v.d);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Mixed int and float arguments
 *
 * JIT function: (i32 a, f64 b, i32 c) -> i32 { return a + c }
 * Tests that arguments are marshaled to correct registers
 */
TEST(callback_mixed_args) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (i32, f64, i32) -> i32 */
    hl_type *arg_types[] = { &c->types[T_I32], &c->types[T_F64], &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 3, arg_types);

    /* r0 = a (i32), r1 = b (f64), r2 = c (i32), r3 = result (i32) */
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_F64], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP3(OAdd, 3, 0, 2),     /* r3 = r0 + r2 */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 2, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create arguments: a=10, b=99.9, c=32 -> result = 42 */
    vdynamic arg0, arg1, arg2;
    arg0.t = &c->types[T_I32];
    arg0.v.i = 10;
    arg1.t = &c->types[T_F64];
    arg1.v.d = 99.9;
    arg2.t = &c->types[T_I32];
    arg2.v.i = 32;
    vdynamic *args[] = { &arg0, &arg1, &arg2 };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 3);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.i != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret->v.i);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Many arguments (stress test register allocation)
 *
 * JIT function: (i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) -> i32
 *               { return a + b + c + d + e + f }
 */
TEST(callback_many_int_args) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (i32, i32, i32, i32, i32, i32) -> i32 */
    hl_type *arg_types[] = {
        &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
        &c->types[T_I32], &c->types[T_I32], &c->types[T_I32]
    };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 6, arg_types);

    /* r0-r5 = args, r6 = temp, r7 = result */
    hl_type *regs[] = {
        &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
        &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
        &c->types[T_I32], &c->types[T_I32]
    };

    hl_opcode ops[] = {
        OP3(OAdd, 6, 0, 1),     /* r6 = r0 + r1 */
        OP3(OAdd, 6, 6, 2),     /* r6 = r6 + r2 */
        OP3(OAdd, 6, 6, 3),     /* r6 = r6 + r3 */
        OP3(OAdd, 6, 6, 4),     /* r6 = r6 + r4 */
        OP3(OAdd, 7, 6, 5),     /* r7 = r6 + r5 */
        OP1(ORet, 7),
    };

    test_alloc_function(c, 0, fn_type, 8, regs, 6, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create arguments: 1 + 2 + 3 + 4 + 5 + 27 = 42 */
    vdynamic arg0, arg1, arg2, arg3, arg4, arg5;
    arg0.t = &c->types[T_I32]; arg0.v.i = 1;
    arg1.t = &c->types[T_I32]; arg1.v.i = 2;
    arg2.t = &c->types[T_I32]; arg2.v.i = 3;
    arg3.t = &c->types[T_I32]; arg3.v.i = 4;
    arg4.t = &c->types[T_I32]; arg4.v.i = 5;
    arg5.t = &c->types[T_I32]; arg5.v.i = 27;
    vdynamic *args[] = { &arg0, &arg1, &arg2, &arg3, &arg4, &arg5 };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 6);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.i != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret->v.i);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Pointer argument (bytes)
 *
 * JIT function: (bytes ptr) -> bytes { return ptr }
 */
TEST(callback_ptr_arg) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Function type: (bytes) -> bytes */
    hl_type *arg_types[] = { &c->types[T_BYTES] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 1, arg_types);

    /* r0 = arg (bytes) */
    hl_type *regs[] = { &c->types[T_BYTES] };

    hl_opcode ops[] = {
        OP1(ORet, 0),           /* return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 1, ops);

    int result;
    void *fn_ptr = test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Create a closure */
    vclosure cl;
    memset(&cl, 0, sizeof(cl));
    cl.t = fn_type;
    cl.fun = fn_ptr;
    cl.hasValue = 0;

    /* Create argument: pointer to a test value */
    static char test_data[] = "hello";
    vdynamic arg_val;
    arg_val.t = &c->types[T_BYTES];
    arg_val.v.ptr = test_data;
    vdynamic *args[] = { &arg_val };

    /* Call through hl_dyn_call */
    vdynamic *ret = hl_dyn_call(&cl, args, 1);

    if (ret == NULL) {
        fprintf(stderr, "    hl_dyn_call returned NULL\n");
        return TEST_FAIL;
    }

    if (ret->v.ptr != test_data) {
        fprintf(stderr, "    Expected %p, got %p\n", (void*)test_data, ret->v.ptr);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(callback_no_args),
    TEST_ENTRY(callback_one_int_arg),
    TEST_ENTRY(callback_two_int_args),
    TEST_ENTRY(callback_i64_arg),
    TEST_ENTRY(callback_f64_arg),
    TEST_ENTRY(callback_mixed_args),
    TEST_ENTRY(callback_many_int_args),
    TEST_ENTRY(callback_ptr_arg),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - C-to-HL Callback Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
