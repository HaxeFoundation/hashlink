/*
 * Test switch operations for HashLink AArch64 JIT
 *
 * Tests: OSwitch
 *
 * OSwitch: switch(value) { case 0: ..., case 1: ..., ... }
 * Parameters:
 *   p1 = register containing value to switch on
 *   p2 = number of cases
 *   extra[i] = jump offset for case i (relative to opcode after switch)
 */
#include "test_harness.h"

/*
 * Test: OSwitch with 3 cases
 *
 * switch(value) {
 *   case 0: return 10;
 *   case 1: return 20;
 *   case 2: return 30;
 *   default: return 0;
 * }
 */
TEST(switch_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 30, 0 };  /* return values for each case */
    test_init_ints(c, 4, ints);

    /* Function type: (i32) -> i32 */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = input value */
        &c->types[T_I32],  /* r1 = return value */
    };

    /*
     * Opcodes layout (default case must be immediately after switch):
     * 0: OSwitch r0, 3, [2, 4, 6]  ; switch on r0 with 3 cases
     * 1: OInt r1, $3               ; default: r1 = 0 (fall through lands here)
     * 2: ORet r1
     * 3: OInt r1, $0               ; case 0: r1 = 10 (offset 2 -> opcode 3)
     * 4: ORet r1
     * 5: OInt r1, $1               ; case 1: r1 = 20 (offset 4 -> opcode 5)
     * 6: ORet r1
     * 7: OInt r1, $2               ; case 2: r1 = 30 (offset 6 -> opcode 7)
     * 8: ORet r1
     *
     * Jump offsets from opcode 1 (after switch):
     *   case 0: offset 2 -> opcode 3
     *   case 1: offset 4 -> opcode 5
     *   case 2: offset 6 -> opcode 7
     */
    static int switch_offsets[] = { 2, 4, 6 };
    hl_opcode ops[] = {
        { OSwitch, 0, 3, 0, switch_offsets },  /* switch r0, 3 cases */
        OP2(OInt, 1, 3),                       /* default: r1 = 0 */
        OP1(ORet, 1),
        OP2(OInt, 1, 0),                       /* case 0: r1 = 10 */
        OP1(ORet, 1),
        OP2(OInt, 1, 1),                       /* case 1: r1 = 20 */
        OP1(ORet, 1),
        OP2(OInt, 1, 2),                       /* case 2: r1 = 30 */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 9, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Test all cases */
    int test_cases[][2] = {
        { 0, 10 },
        { 1, 20 },
        { 2, 30 },
        { 3, 0 },   /* default */
        { 100, 0 }, /* default */
        { -1, 0 },  /* default */
    };

    for (int i = 0; i < 6; i++) {
        int input = test_cases[i][0];
        int expected = test_cases[i][1];
        int got = fn(input);
        if (got != expected) {
            fprintf(stderr, "    switch(%d): expected %d, got %d\n", input, expected, got);
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}

/*
 * Test: OSwitch with single case
 */
TEST(switch_single_case) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 0 };
    test_init_ints(c, 2, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
    };

    /* Default immediately after switch, case 0 at offset 2 */
    static int switch_offsets[] = { 2 };
    hl_opcode ops[] = {
        { OSwitch, 0, 1, 0, switch_offsets },  /* switch r0, 1 case */
        OP2(OInt, 1, 1),                       /* default: r1 = 0 (fall through) */
        OP1(ORet, 1),
        OP2(OInt, 1, 0),                       /* case 0: r1 = 42 (offset 2) */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    if (fn(0) != 42) {
        fprintf(stderr, "    switch(0): expected 42, got %d\n", fn(0));
        return TEST_FAIL;
    }

    if (fn(1) != 0) {
        fprintf(stderr, "    switch(1): expected 0 (default), got %d\n", fn(1));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSwitch where all cases jump to same target
 */
TEST(switch_same_target) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42, 0 };
    test_init_ints(c, 2, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
    };

    /* Default at opcode 1, all 3 cases jump to offset 2 (opcode 3) */
    static int switch_offsets[] = { 2, 2, 2 };
    hl_opcode ops[] = {
        { OSwitch, 0, 3, 0, switch_offsets },  /* switch r0, 3 cases all going to same place */
        OP2(OInt, 1, 1),                       /* default: r1 = 0 (fall through) */
        OP1(ORet, 1),
        OP2(OInt, 1, 0),                       /* case 0,1,2: r1 = 42 (offset 2) */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 5, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* Cases 0, 1, 2 should all return 42 */
    for (int i = 0; i < 3; i++) {
        if (fn(i) != 42) {
            fprintf(stderr, "    switch(%d): expected 42, got %d\n", i, fn(i));
            return TEST_FAIL;
        }
    }

    /* Anything else is default (0) */
    if (fn(5) != 0) {
        fprintf(stderr, "    switch(5): expected 0, got %d\n", fn(5));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSwitch with fallthrough pattern (consecutive cases)
 *
 * switch(value) {
 *   case 0:
 *   case 1:
 *     return 10;  // cases 0 and 1 both return 10
 *   case 2:
 *     return 20;
 *   default:
 *     return 0;
 * }
 */
TEST(switch_fallthrough) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 0 };
    test_init_ints(c, 3, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_I32],
    };

    /* Default at opcode 1, case 0,1 at offset 2, case 2 at offset 4 */
    static int switch_offsets[] = { 2, 2, 4 };
    hl_opcode ops[] = {
        { OSwitch, 0, 3, 0, switch_offsets },
        OP2(OInt, 1, 2),                       /* default: r1 = 0 (fall through) */
        OP1(ORet, 1),
        OP2(OInt, 1, 0),                       /* case 0,1: r1 = 10 (offset 2) */
        OP1(ORet, 1),
        OP2(OInt, 1, 1),                       /* case 2: r1 = 20 (offset 4) */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 7, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    if (fn(0) != 10 || fn(1) != 10) {
        fprintf(stderr, "    Cases 0,1 should return 10\n");
        return TEST_FAIL;
    }

    if (fn(2) != 20) {
        fprintf(stderr, "    Case 2 should return 20, got %d\n", fn(2));
        return TEST_FAIL;
    }

    if (fn(3) != 0) {
        fprintf(stderr, "    Default should return 0, got %d\n", fn(3));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OSwitch with computation after switch
 *
 * This tests that control flow properly resumes after switch.
 * OLabel opcodes are required at jump targets to discard register bindings.
 */
TEST(switch_with_computation) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 100 };
    test_init_ints(c, 3, ints);

    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = input */
        &c->types[T_I32],  /* r1 = multiplier from switch */
        &c->types[T_I32],  /* r2 = base value */
        &c->types[T_I32],  /* r3 = result */
    };

    /*
     * Default after switch, cases jump past default to their handlers.
     * Then all paths converge to common continuation.
     * OLabel is required at each jump target.
     *
     * 0: OSwitch r0, 2, [2, 5]  ; case 0->op3, case 1->op6
     * 1: OInt r1, 100           ; default: r1 = 100
     * 2: OJAlways 5             ; default jumps to op 8 (continuation)
     * 3: OLabel                 ; case 0 target
     * 4: OInt r1, 10            ; case 0: r1 = 10
     * 5: OJAlways 2             ; case 0 jumps to op 8
     * 6: OLabel                 ; case 1 target
     * 7: OInt r1, 20            ; case 1: r1 = 20, falls through
     * 8: OLabel                 ; continuation (merge point)
     * 9: OInt r2, 100           ; r2 = 100
     * 10: OAdd r3, r1, r2
     * 11: ORet r3
     */
    static int switch_offsets[] = { 2, 5 };
    hl_opcode ops[] = {
        { OSwitch, 0, 2, 0, switch_offsets },
        OP2(OInt, 1, 2),                       /* default: r1 = 100 */
        OP2(OJAlways, 5, 0),                   /* default jumps to continuation (op 8) */
        OP0(OLabel),                           /* case 0 target */
        OP2(OInt, 1, 0),                       /* case 0: r1 = 10 */
        OP2(OJAlways, 2, 0),                   /* case 0 jumps to continuation (op 8) */
        OP0(OLabel),                           /* case 1 target */
        OP2(OInt, 1, 1),                       /* case 1: r1 = 20, falls through */
        OP0(OLabel),                           /* continuation (merge point) */
        OP2(OInt, 2, 2),                       /* r2 = 100 */
        OP3(OAdd, 3, 1, 2),                    /* r3 = r1 + r2 */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 12, ops);

    int result;
    int (*fn)(int) = (int(*)(int))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    /* case 0: 10 + 100 = 110 */
    if (fn(0) != 110) {
        fprintf(stderr, "    switch(0): expected 110, got %d\n", fn(0));
        return TEST_FAIL;
    }

    /* case 1: 20 + 100 = 120 */
    if (fn(1) != 120) {
        fprintf(stderr, "    switch(1): expected 120, got %d\n", fn(1));
        return TEST_FAIL;
    }

    /* default: 100 + 100 = 200 */
    if (fn(5) != 200) {
        fprintf(stderr, "    switch(5) default: expected 200, got %d\n", fn(5));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(switch_basic),
    TEST_ENTRY(switch_single_case),
    TEST_ENTRY(switch_same_target),
    TEST_ENTRY(switch_fallthrough),
    TEST_ENTRY(switch_with_computation),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Switch Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
