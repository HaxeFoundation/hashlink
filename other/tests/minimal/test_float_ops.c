/*
 * Test floating-point operations for HashLink AArch64 JIT
 *
 * Tests: OFloat, OAdd/OSub/OMul/OSDiv (f64), ONeg, conversions
 */
#include "test_harness.h"
#include <math.h>

/* Helper to compare floats with epsilon */
static int float_eq(double a, double b) {
    double eps = 1e-9;
    return fabs(a - b) < eps;
}

/*
 * Test: Return constant float 3.14159
 */
TEST(return_float_constant) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Float pool */
    double floats[] = { 3.14159 };
    test_init_floats(c, 1, floats);

    /* Function type: () -> f64 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    /* Registers: r0:f64 */
    hl_type *regs[] = { &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),   /* r0 = floats[0] */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 3.14159)) {
        fprintf(stderr, "    Expected 3.14159, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Add floats: 1.5 + 2.5 = 4.0
 */
TEST(add_float_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 1.5, 2.5 };
    test_init_floats(c, 2, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OFloat, 1, 1),
        OP3(OAdd, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 4.0)) {
        fprintf(stderr, "    Expected 4.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Subtract floats: 10.5 - 6.5 = 4.0
 */
TEST(sub_float_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 10.5, 6.5 };
    test_init_floats(c, 2, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OFloat, 1, 1),
        OP3(OSub, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 4.0)) {
        fprintf(stderr, "    Expected 4.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Multiply floats: 2.0 * 3.5 = 7.0
 */
TEST(mul_float_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 2.0, 3.5 };
    test_init_floats(c, 2, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OFloat, 1, 1),
        OP3(OMul, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 7.0)) {
        fprintf(stderr, "    Expected 7.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Divide floats: 15.0 / 3.0 = 5.0
 */
TEST(div_float_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 15.0, 3.0 };
    test_init_floats(c, 2, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OFloat, 1, 1),
        OP3(OSDiv, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 5.0)) {
        fprintf(stderr, "    Expected 5.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Negate float: -(-3.5) = 3.5
 */
TEST(neg_float) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { -3.5 };
    test_init_floats(c, 1, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(ONeg, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 3.5)) {
        fprintf(stderr, "    Expected 3.5, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Move float register
 */
TEST(mov_float_register) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 2.718281828 };
    test_init_floats(c, 1, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OMov, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 2.718281828)) {
        fprintf(stderr, "    Expected 2.718281828, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Convert int to float (signed): 42 -> 42.0
 */
TEST(int_to_float_signed) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),       /* r0:i32 = 42 */
        OP2(OToSFloat, 1, 0),  /* r1:f64 = (f64)r0 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 42.0)) {
        fprintf(stderr, "    Expected 42.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Convert negative int to float: -42 -> -42.0
 */
TEST(neg_int_to_float) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_F64] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OToSFloat, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, -42.0)) {
        fprintf(stderr, "    Expected -42.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Convert float to int: 42.7 -> 42
 */
TEST(float_to_int) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 42.7 };
    test_init_floats(c, 1, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),   /* r0:f64 = 42.7 */
        OP2(OToInt, 1, 0),   /* r1:i32 = (i32)r0 */
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
 * Test: Convert negative float to int: -42.7 -> -42
 */
TEST(neg_float_to_int) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { -42.7 };
    test_init_floats(c, 1, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_F64], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OToInt, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != -42) {
        fprintf(stderr, "    Expected -42, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: f32 operations - load and return
 */
TEST(return_f32_constant) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* f32 is stored in floats pool as f64, converted on load */
    double floats[] = { 3.14159f };
    test_init_floats(c, 1, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F32], 0, NULL);
    hl_type *regs[] = { &c->types[T_F32] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    float (*fn)(void) = (float(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    float ret = fn();
    if (fabsf(ret - 3.14159f) > 1e-5f) {
        fprintf(stderr, "    Expected ~3.14159, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: f32 addition
 */
TEST(add_f32_constants) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    double floats[] = { 1.5f, 2.5f };
    test_init_floats(c, 2, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F32], 0, NULL);
    hl_type *regs[] = { &c->types[T_F32], &c->types[T_F32], &c->types[T_F32] };

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),
        OP2(OFloat, 1, 1),
        OP3(OAdd, 2, 0, 1),
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    float (*fn)(void) = (float(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    float ret = fn();
    if (fabsf(ret - 4.0f) > 1e-5f) {
        fprintf(stderr, "    Expected 4.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(return_float_constant),
    TEST_ENTRY(add_float_constants),
    TEST_ENTRY(sub_float_constants),
    TEST_ENTRY(mul_float_constants),
    TEST_ENTRY(div_float_constants),
    TEST_ENTRY(neg_float),
    TEST_ENTRY(mov_float_register),
    TEST_ENTRY(int_to_float_signed),
    TEST_ENTRY(neg_int_to_float),
    TEST_ENTRY(float_to_int),
    TEST_ENTRY(neg_float_to_int),
    TEST_ENTRY(return_f32_constant),
    TEST_ENTRY(add_f32_constants),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Floating Point Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
