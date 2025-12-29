/*
 * Test exception operations for HashLink AArch64 JIT
 *
 * Tests: OThrow, ORethrow, OTrap, OEndTrap, OCatch
 *
 * Exception handling in HashLink uses setjmp/longjmp.
 * OTrap: set up exception handler (like try {)
 * OEndTrap: tear down exception handler (end of try block)
 * OThrow: throw an exception
 * ORethrow: rethrow current exception
 * OCatch: marks catch block (informational, no code generated)
 */
#include "test_harness.h"

/*
 * Test: OTrap and OEndTrap - basic try block without exception
 *
 * try {
 *   r0 = 42
 * }
 * return r0
 *
 * This tests that trap setup/teardown works without throwing.
 */
TEST(trap_no_exception) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = result */
        &c->types[T_VOID], /* r1 = exception (unused here) */
    };

    /*
     * Layout:
     * 0: OTrap r1, 3     ; setup trap, if exception goto +3 (catch block)
     * 1: OInt r0, $0     ; r0 = 42 (try body)
     * 2: OEndTrap        ; end try block
     * 3: ORet r0         ; return r0 (after try or from catch)
     *
     * Catch block would be at opcode 4 (1+3), but we don't have one.
     */
    hl_opcode ops[] = {
        OP2(OTrap, 1, 3),    /* trap -> catch at +3 */
        OP2(OInt, 0, 0),     /* r0 = 42 */
        OP1(OEndTrap, 1),    /* end trap */
        OP1(ORet, 0),
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
 * Test: OThrow - throw and catch exception
 *
 * try {
 *   throw 123
 *   r0 = 10  ; should not execute
 * } catch (e) {
 *   r0 = 42  ; should execute
 * }
 * return r0
 */
TEST(throw_catch_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 42 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* For throwing, we need a dynamic value.
     * We'll allocate a simple dynamic int and throw it. */
    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = result */
        &c->types[T_VOID], /* r1 = caught exception */
        &c->types[T_VOID], /* r2 = exception to throw */
    };

    /*
     * Layout:
     * 0: OTrap r1, 4       ; setup trap, if exception goto +4 (opcode 5)
     * 1: ONull r2          ; create null for throw (simplest throwable)
     * 2: OThrow r2         ; throw
     * 3: OInt r0, $0       ; r0 = 10 (should NOT execute)
     * 4: OEndTrap          ; end trap (won't reach if thrown)
     * 5: OCatch            ; catch marker
     * 6: OInt r0, $1       ; r0 = 42 (catch body)
     * 7: ORet r0
     */
    hl_opcode ops[] = {
        OP2(OTrap, 1, 5),    /* trap -> catch at op 5 (offset from next = 4) */
        OP1(ONull, 2),       /* r2 = null */
        OP1(OThrow, 2),      /* throw r2 */
        OP2(OInt, 0, 0),     /* r0 = 10 (unreachable) */
        OP1(OEndTrap, 1),    /* end trap (unreachable) */
        OP1(OCatch, 0),      /* catch marker */
        OP2(OInt, 0, 1),     /* r0 = 42 (catch body) */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 8, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42 (catch block), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Nested try blocks
 *
 * try {
 *   try {
 *     throw
 *   } catch {
 *     r0 = 10
 *   }
 *   r0 = r0 + 32  ; 10 + 32 = 42
 * } catch {
 *   r0 = 99  ; should not reach
 * }
 * return r0
 */
TEST(nested_trap) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32, 99 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = result */
        &c->types[T_VOID], /* r1 = outer exception */
        &c->types[T_VOID], /* r2 = inner exception */
        &c->types[T_VOID], /* r3 = throw value */
        &c->types[T_I32],  /* r4 = temp */
    };

    /*
     * Outer try: 0-11
     * Inner try: 1-6
     * Inner catch: 7-8
     * Continue outer (merge point): 9-11
     * Outer catch: 13-15
     *
     * Note: OLabel is required at merge points (op 9, op 16) because:
     * - Op 9 is reached via OJAlways from op 6 AND via fallthrough from op 8
     * - Op 16 is reached via OJAlways from op 12 AND via fallthrough from op 15
     * At runtime, spill_regs() before jumps puts values on stack,
     * but the generated code must use discard_regs() at labels to ensure
     * subsequent ops load from stack rather than assuming register bindings.
     */
    hl_opcode ops[] = {
        OP2(OTrap, 1, 13),   /* 0: outer trap -> catch at 14 (0+1+13) */
        OP2(OTrap, 2, 5),    /* 1: inner trap -> catch at 7 (1+1+5) */
        OP1(ONull, 3),       /* 2: r3 = null */
        OP1(OThrow, 3),      /* 3: throw */
        OP2(OInt, 0, 2),     /* 4: unreachable */
        OP1(OEndTrap, 2),    /* 5: end inner trap (unreachable) */
        OP2(OJAlways, 2, 0), /* 6: skip catch -> goto 9 (6+1+2) */
        OP1(OCatch, 0),      /* 7: inner catch marker */
        OP2(OInt, 0, 0),     /* 8: r0 = 10 */
        OP0(OLabel),         /* 9: merge point for op 6 jump and fallthrough */
        OP2(OInt, 4, 1),     /* 10: r4 = 32 */
        OP3(OAdd, 0, 0, 4),  /* 11: r0 = r0 + 32 = 42 */
        OP1(OEndTrap, 1),    /* 12: end outer trap */
        OP2(OJAlways, 2, 0), /* 13: skip outer catch -> goto 16 (13+1+2) */
        OP1(OCatch, 0),      /* 14: outer catch marker */
        OP2(OInt, 0, 2),     /* 15: r0 = 99 */
        OP0(OLabel),         /* 16: merge point for op 13 jump and fallthrough */
        OP1(ORet, 0),        /* 17: return */
    };

    test_alloc_function(c, 0, fn_type, 5, regs, 18, ops);

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
 * Test: OEndTrap without exception cleans up properly
 *
 * Multiple sequential try blocks that don't throw.
 */
TEST(multiple_traps_no_throw) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 12 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_VOID],
        &c->types[T_I32],
        &c->types[T_I32],
    };

    hl_opcode ops[] = {
        /* First try block */
        OP2(OTrap, 1, 3),    /* trap */
        OP2(OInt, 0, 0),     /* r0 = 10 */
        OP1(OEndTrap, 1),    /* end trap */
        /* Second try block */
        OP2(OTrap, 1, 3),    /* trap */
        OP2(OInt, 2, 1),     /* r2 = 20 */
        OP1(OEndTrap, 1),    /* end trap */
        /* Third try block */
        OP2(OTrap, 1, 3),    /* trap */
        OP2(OInt, 3, 2),     /* r3 = 12 */
        OP1(OEndTrap, 1),    /* end trap */
        /* Combine */
        OP3(OAdd, 0, 0, 2),  /* r0 = r0 + r2 = 30 */
        OP3(OAdd, 0, 0, 3),  /* r0 = r0 + r3 = 42 */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 12, ops);

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
    TEST_ENTRY(trap_no_exception),
    TEST_ENTRY(throw_catch_basic),
    TEST_ENTRY(nested_trap),
    TEST_ENTRY(multiple_traps_no_throw),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Exception Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
