/*
 * Test floating-point register pressure for HashLink AArch64 JIT
 *
 * This test verifies that the register allocator correctly handles
 * high FP register pressure by spilling to stack, without using
 * the callee-saved V8-V15 registers (which aren't saved in our prologue).
 *
 * We have 24 caller-saved FP registers (V0-V7, V16-V31).
 * If we use more than 24 float values simultaneously, the allocator
 * must spill some to stack.
 */
#include "test_harness.h"
#include <math.h>

/* Helper to compare floats with epsilon */
static int float_eq(double a, double b) {
    double eps = 1e-9;
    return fabs(a - b) < eps;
}

/*
 * Test: Sum of 10 floats
 * Uses moderate register pressure to verify basic allocation works.
 * r0-r9: float constants
 * r10: accumulator
 * Returns sum of 1.0 + 2.0 + ... + 10.0 = 55.0
 */
TEST(sum_10_floats) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Float pool: 1.0 through 10.0 */
    double floats[] = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0 };
    test_init_floats(c, 10, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    /* 11 registers: r0-r9 for constants, r10 for accumulator */
    hl_type *regs[11];
    for (int i = 0; i < 11; i++) regs[i] = &c->types[T_F64];

    hl_opcode ops[] = {
        OP2(OFloat, 0, 0),    /* r0 = 1.0 */
        OP2(OFloat, 1, 1),    /* r1 = 2.0 */
        OP2(OFloat, 2, 2),    /* r2 = 3.0 */
        OP2(OFloat, 3, 3),    /* r3 = 4.0 */
        OP2(OFloat, 4, 4),    /* r4 = 5.0 */
        OP2(OFloat, 5, 5),    /* r5 = 6.0 */
        OP2(OFloat, 6, 6),    /* r6 = 7.0 */
        OP2(OFloat, 7, 7),    /* r7 = 8.0 */
        OP2(OFloat, 8, 8),    /* r8 = 9.0 */
        OP2(OFloat, 9, 9),    /* r9 = 10.0 */
        OP3(OAdd, 10, 0, 1),  /* r10 = r0 + r1 = 3.0 */
        OP3(OAdd, 10, 10, 2), /* r10 = 3.0 + 3.0 = 6.0 */
        OP3(OAdd, 10, 10, 3), /* r10 = 6.0 + 4.0 = 10.0 */
        OP3(OAdd, 10, 10, 4), /* r10 = 10.0 + 5.0 = 15.0 */
        OP3(OAdd, 10, 10, 5), /* r10 = 15.0 + 6.0 = 21.0 */
        OP3(OAdd, 10, 10, 6), /* r10 = 21.0 + 7.0 = 28.0 */
        OP3(OAdd, 10, 10, 7), /* r10 = 28.0 + 8.0 = 36.0 */
        OP3(OAdd, 10, 10, 8), /* r10 = 36.0 + 9.0 = 45.0 */
        OP3(OAdd, 10, 10, 9), /* r10 = 45.0 + 10.0 = 55.0 */
        OP1(ORet, 10),
    };

    test_alloc_function(c, 0, fn_type, 11, regs, 20, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    if (!float_eq(ret, 55.0)) {
        fprintf(stderr, "    Expected 55.0, got %f\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Sum of 25 floats - forces register spilling
 * Uses 25 float values, which is more than the 24 available caller-saved
 * FP registers (V0-V7, V16-V31). This forces spilling to stack.
 * Returns sum of 1.0 + 2.0 + ... + 25.0 = 325.0
 */
TEST(sum_25_floats_spill) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Float pool: 1.0 through 25.0 */
    double floats[25];
    for (int i = 0; i < 25; i++) {
        floats[i] = (double)(i + 1);
    }
    test_init_floats(c, 25, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    /* 26 registers: r0-r24 for constants, r25 for accumulator */
    hl_type *regs[26];
    for (int i = 0; i < 26; i++) regs[i] = &c->types[T_F64];

    /* Build opcodes dynamically */
    hl_opcode ops[52];  /* 25 loads + 1 initial add + 23 adds + 1 ret = 50, plus some slack */
    int op_idx = 0;

    /* Load all 25 float constants */
    for (int i = 0; i < 25; i++) {
        ops[op_idx++] = (hl_opcode){ .op = OFloat, .p1 = i, .p2 = i };
    }

    /* Sum them: r25 = r0 + r1, then r25 = r25 + r2, etc. */
    ops[op_idx++] = (hl_opcode){ .op = OAdd, .p1 = 25, .p2 = 0, .p3 = 1 };
    for (int i = 2; i < 25; i++) {
        ops[op_idx++] = (hl_opcode){ .op = OAdd, .p1 = 25, .p2 = 25, .p3 = i };
    }

    ops[op_idx++] = (hl_opcode){ .op = ORet, .p1 = 25 };

    test_alloc_function(c, 0, fn_type, 26, regs, op_idx, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 325.0;  /* 1+2+...+25 = 25*26/2 = 325 */
    if (!float_eq(ret, expected)) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Complex expression with many live floats
 * Computes: (a*b + c*d + e*f + g*h) * (i*j + k*l + m*n + o*p)
 * This keeps many intermediate values live simultaneously.
 */
TEST(complex_expression_many_live) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* 16 input values: a=1, b=2, c=3, d=4, ... p=16 */
    double floats[16];
    for (int i = 0; i < 16; i++) {
        floats[i] = (double)(i + 1);
    }
    test_init_floats(c, 16, floats);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    /* 28 registers:
     * r0-r15: input values (a-p)
     * r16-r23: products (a*b, c*d, e*f, g*h, i*j, k*l, m*n, o*p)
     * r24-r25: partial sums (left and right)
     * r26-r27: more partial sums
     */
    hl_type *regs[28];
    for (int i = 0; i < 28; i++) regs[i] = &c->types[T_F64];

    hl_opcode ops[] = {
        /* Load 16 values */
        OP2(OFloat, 0, 0),   OP2(OFloat, 1, 1),   OP2(OFloat, 2, 2),   OP2(OFloat, 3, 3),
        OP2(OFloat, 4, 4),   OP2(OFloat, 5, 5),   OP2(OFloat, 6, 6),   OP2(OFloat, 7, 7),
        OP2(OFloat, 8, 8),   OP2(OFloat, 9, 9),   OP2(OFloat, 10, 10), OP2(OFloat, 11, 11),
        OP2(OFloat, 12, 12), OP2(OFloat, 13, 13), OP2(OFloat, 14, 14), OP2(OFloat, 15, 15),

        /* 8 products - all computed before any are consumed */
        OP3(OMul, 16, 0, 1),   /* r16 = a*b = 1*2 = 2 */
        OP3(OMul, 17, 2, 3),   /* r17 = c*d = 3*4 = 12 */
        OP3(OMul, 18, 4, 5),   /* r18 = e*f = 5*6 = 30 */
        OP3(OMul, 19, 6, 7),   /* r19 = g*h = 7*8 = 56 */
        OP3(OMul, 20, 8, 9),   /* r20 = i*j = 9*10 = 90 */
        OP3(OMul, 21, 10, 11), /* r21 = k*l = 11*12 = 132 */
        OP3(OMul, 22, 12, 13), /* r22 = m*n = 13*14 = 182 */
        OP3(OMul, 23, 14, 15), /* r23 = o*p = 15*16 = 240 */

        /* Left sum: (a*b + c*d + e*f + g*h) */
        OP3(OAdd, 24, 16, 17), /* r24 = 2 + 12 = 14 */
        OP3(OAdd, 25, 18, 19), /* r25 = 30 + 56 = 86 */
        OP3(OAdd, 24, 24, 25), /* r24 = 14 + 86 = 100 */

        /* Right sum: (i*j + k*l + m*n + o*p) */
        OP3(OAdd, 26, 20, 21), /* r26 = 90 + 132 = 222 */
        OP3(OAdd, 27, 22, 23), /* r27 = 182 + 240 = 422 */
        OP3(OAdd, 26, 26, 27), /* r26 = 222 + 422 = 644 */

        /* Final result: left * right */
        OP3(OMul, 24, 24, 26), /* r24 = 100 * 644 = 64400 */
        OP1(ORet, 24),
    };

    test_alloc_function(c, 0, fn_type, 28, regs, sizeof(ops)/sizeof(ops[0]), ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 64400.0;
    if (!float_eq(ret, expected)) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test registration
 */
static test_entry_t tests[] = {
    TEST_ENTRY(sum_10_floats),
    TEST_ENTRY(sum_25_floats_spill),
    TEST_ENTRY(complex_expression_many_live),
};

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    return run_tests(tests, sizeof(tests)/sizeof(tests[0]));
}
