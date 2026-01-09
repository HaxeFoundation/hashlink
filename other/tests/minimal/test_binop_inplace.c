/*
 * Test in-place binary operations followed by spill
 *
 * Tests the bug where in-place binops like r0 = r0 << r1 don't properly
 * update the register binding, causing the old (pre-operation) value to
 * be spilled instead of the new value.
 *
 * Bug scenario:
 *   1. r0 = 21
 *   2. r1 = 1
 *   3. r0 = r0 << r1   ; in-place shift, result should be 42
 *   4. call fn()       ; triggers spill_regs - BUG: spills old r0 (21) instead of new (42)
 *   5. return r0       ; BUG: returns 21 instead of 42
 */
#include "test_harness.h"

/* Helper to allocate multiple functions at once */
static void test_alloc_functions(hl_code *c, int count) {
    c->functions = (hl_function*)calloc(count, sizeof(hl_function));
    c->nfunctions = 0;
}

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
 * Test: In-place left shift followed by function call
 *
 * This is the minimal reproduction of the string concat bug where:
 *   OShl r5, r5, r6   ; in-place shift
 *   OCallN ...        ; triggers spill, but spills the OLD r5 value
 *
 * fn0: () -> i32  { return 0; }   ; dummy function to trigger spill
 * fn1: () -> i32  {               ; entry point
 *   r0 = 21
 *   r1 = 1
 *   r0 = r0 << r1    ; r0 should become 42
 *   call fn0()       ; triggers spill - bug causes old r0 (21) to be saved
 *   return r0        ; should return 42, but returns 21 if bug present
 * }
 */
TEST(shl_inplace_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 21, 1, 0 };
    test_init_ints(c, 3, ints);

    /* Function types */
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: findex=0, returns 0 (dummy to trigger spill) */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 2),   /* r0 = 0 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: findex=1, does in-place shift then calls fn0 (entry point) */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 21 */
            OP2(OInt, 1, 1),       /* r1 = 1 */
            OP3(OShl, 0, 0, 1),    /* r0 = r0 << r1  (in-place! dst == src) */
            OP2(OCall0, 2, 0),     /* r2 = call fn0() - triggers spill */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 1, fn_type_i32, 3, regs, 5, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: in-place shift value not properly spilled)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: In-place add followed by function call
 * Same bug pattern but with OAdd instead of OShl
 */
TEST(add_inplace_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 21, 0 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: returns 0 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 1),
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: r0 = r0 + r0 (21 + 21 = 42), then call, then return r0 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 21 */
            OP3(OAdd, 0, 0, 0),    /* r0 = r0 + r0 (in-place!) */
            OP2(OCall0, 1, 0),     /* r1 = call fn0() */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 1, fn_type_i32, 2, regs, 4, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: in-place add value not properly spilled)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: In-place multiply followed by function call
 */
TEST(mul_inplace_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 6, 7, 0 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: returns 0 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 2),
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: r0 = 6, r1 = 7, r0 = r0 * r1, call, return r0 */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 6 */
            OP2(OInt, 1, 1),       /* r1 = 7 */
            OP3(OMul, 0, 0, 1),    /* r0 = r0 * r1 (in-place!) */
            OP2(OCall0, 2, 0),     /* r2 = call fn0() */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 1, fn_type_i32, 3, regs, 5, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: in-place mul value not properly spilled)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Chain of in-place operations then call
 * This is closer to the real-world string concat scenario
 */
TEST(chain_inplace_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 1, 0 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: returns 0 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 2),
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: r0 = 10, r1 = 1, r0 = r0 << r1, r0 = r0 + r0, r0 = r0 + r1 + r1, call, return r0
     * 10 << 1 = 20, 20 + 20 = 40, 40 + 1 + 1 = 42
     */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 10 */
            OP2(OInt, 1, 1),       /* r1 = 1 */
            OP3(OShl, 0, 0, 1),    /* r0 = r0 << r1 = 20 */
            OP3(OAdd, 0, 0, 0),    /* r0 = r0 + r0 = 40 */
            OP3(OAdd, 0, 0, 1),    /* r0 = r0 + r1 = 41 */
            OP3(OAdd, 0, 0, 1),    /* r0 = r0 + r1 = 42 */
            OP2(OCall0, 2, 0),     /* r2 = call fn0() */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 1, fn_type_i32, 3, regs, 8, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: chain of in-place ops not properly spilled)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Simulates the actual string concat bug pattern more closely
 *
 * The real bug occurs in this sequence (from function 20):
 *   OField r5, r1, 1    ; r5 = load from object field
 *   OInt r6, 1          ; r6 = 1
 *   OShl r5, r5, r6     ; r5 = r5 << r6 (in-place)
 *   ... more ops ...
 *   OCallN              ; triggers spill - BUG: spills old r5
 *
 * The key is that r5 comes from OField (not OInt), so fetch() loads it
 * into a register. Then OShl does in-place shift, allocating a NEW
 * register for the result. But the old register still thinks it holds r5.
 *
 * We simulate this with OCall1 to pass a value, then shift it in-place.
 */
TEST(shl_inplace_arg_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 0 };
    test_init_ints(c, 2, ints);

    /* Function types */
    hl_type *arg_types[] = { &c->types[T_I32] };
    hl_type *fn_type_i32_i32 = test_alloc_fun_type(c, &c->types[T_I32], 1, arg_types);
    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 3);

    /* fn0: returns 21 (simulates loading a value like OField does) */
    {
        int fn0_ints[] = { 21 };
        /* We need to add these ints - but we'll use the global pool */
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),   /* r0 = ints[0] = 1... wait we need 21 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: returns 0 (dummy to trigger second spill) */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 1),   /* r0 = 0 */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn2: entry point
     *   r0 = call fn0()     ; r0 = 21 (value comes from call, like OField)
     *   r1 = 1
     *   r0 = r0 << r1       ; in-place shift, r0 should be 42
     *   r2 = call fn1()     ; triggers spill
     *   return r0           ; should be 42
     */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OCall0, 0, 0),     /* r0 = call fn0() = 21 */
            OP2(OInt, 1, 0),       /* r1 = 1 */
            OP3(OShl, 0, 0, 1),    /* r0 = r0 << r1 = 42 (in-place!) */
            OP2(OCall0, 2, 1),     /* r2 = call fn1() - triggers spill */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 2, fn_type_i32, 3, regs, 5, ops);
    }

    c->entrypoint = 2;

    /* Fix: fn0 needs to return 21. Let's update the ints pool */
    c->ints[0] = 21;  /* fn0 uses ints[0] */
    c->ints[1] = 1;   /* fn2 uses ints[1] for the shift amount... wait no */

    /* Actually let's redo the ints pool properly */
    free(c->ints);
    int new_ints[] = { 21, 1, 0 };  /* 21 for fn0, 1 for shift, 0 for fn1 */
    test_init_ints(c, 3, new_ints);

    /* Update fn0 to use ints[0]=21, fn1 to use ints[2]=0, fn2 r1 to use ints[1]=1 */
    c->functions[0].ops[0].p2 = 0;  /* fn0: OInt r0, ints[0]=21 */
    c->functions[1].ops[0].p2 = 2;  /* fn1: OInt r0, ints[2]=0 */
    c->functions[2].ops[1].p2 = 1;  /* fn2: OInt r1, ints[1]=1 */

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: value from call not properly spilled after in-place shift)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Multiple registers active, simulating the string concat pattern
 *
 * This more closely matches the real bug:
 *   r4 = get length1
 *   r5 = 1
 *   r4 = r4 << r5      ; first shift
 *   r5 = get length2   ; r5 REUSED for different value
 *   r6 = 1
 *   r5 = r5 << r6      ; second shift (in-place) - THIS IS WHERE BUG OCCURS
 *   r6 = r4 + r5       ; need both shifted values
 *   call(...)          ; spill - r5 gets wrong value
 */
TEST(string_concat_pattern) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* ints: 13, 1, 1 (two lengths, and 1 for shift) */
    int ints[] = { 13, 1, 1, 0 };
    test_init_ints(c, 4, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: returns 0 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 3),   /* r0 = 0 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: entry - simulates string concat length calculation
     *   r0 = 13          ; length1 (chars)
     *   r1 = 1           ; shift amount
     *   r0 = r0 << r1    ; length1 in bytes = 26
     *   r2 = 1           ; length2 (chars) - reusing pattern
     *   r3 = 1           ; shift amount
     *   r2 = r2 << r3    ; length2 in bytes = 2 (in-place!)
     *   r4 = r0 + r2     ; total = 28
     *   call fn0()       ; triggers spill
     *   return r4        ; should be 28
     */
    {
        hl_type *regs[] = {
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32]
        };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 13 */
            OP2(OInt, 1, 1),       /* r1 = 1 */
            OP3(OShl, 0, 0, 1),    /* r0 = r0 << r1 = 26 */
            OP2(OInt, 2, 1),       /* r2 = 1 */
            OP2(OInt, 3, 2),       /* r3 = 1 */
            OP3(OShl, 2, 2, 3),    /* r2 = r2 << r3 = 2 (in-place!) */
            OP3(OAdd, 4, 0, 2),    /* r4 = r0 + r2 = 28 */
            OP2(OCall0, 5, 0),     /* r5 = call fn0() - triggers spill */
            OP1(ORet, 4),          /* return r4 = 28 */
        };
        test_add_function(c, 1, fn_type_i32, 6, regs, 9, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 28) {
        fprintf(stderr, "    Expected 28, got %d\n", ret);
        fprintf(stderr, "    (Bug: in-place shift in multi-register scenario failed)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Force pd < pa scenario
 *
 * THE REAL BUG: The bug only manifests when the RESULT register (pd) has a
 * LOWER index than the SOURCE register (pa). In spill_regs(), registers are
 * processed from X0 to X17. If pd < pa:
 *   1. pd is spilled first (correct value stored)
 *   2. pa is spilled later (OLD value overwrites correct value!)
 *
 * To trigger this, we need:
 *   1. Allocate several low-numbered registers (X0, X1, X2, ...)
 *   2. Free a low register (X0)
 *   3. Load a value into a high register (X5 say)
 *   4. Do in-place operation - result goes to freed X0, source stays in X5
 *   5. Call function - spill_regs processes X0 first, then X5 overwrites!
 */
TEST(force_pd_less_than_pa) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0, 1, 2, 3, 4, 21, 1 };
    test_init_ints(c, 7, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 2);

    /* fn0: returns 0 */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),   /* r0 = 0 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: Entry point
     * Strategy: Allocate registers 0-4, then use r5 for the actual value.
     * When we do the in-place op on r5, the result register will be
     * allocated to a lower number (after we stop using r0-r4).
     *
     *   r0 = 0  (uses X0)
     *   r1 = 1  (uses X1)
     *   r2 = 2  (uses X2)
     *   r3 = 3  (uses X3)
     *   r4 = 4  (uses X4)
     *   r5 = 21 (uses X5)
     *   r6 = 1  (uses X6)
     *   ; Now we "forget" r0-r4 by doing operations that don't involve them
     *   ; (The registers will be evicted when new ones are needed)
     *   r5 = r5 << r6  ; In-place shift. pd might be X0-X4 if they get freed
     *   ; Actually, let's force it by having the MOV instruction cause eviction
     *   r0 = r5        ; This forces r5's value to r0
     *   call fn0()     ; Spill
     *   return r0      ; Should be 42
     */
    {
        hl_type *regs[] = {
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
            &c->types[T_I32], &c->types[T_I32]
        };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),       /* r0 = 0 - allocates X0 */
            OP2(OInt, 1, 1),       /* r1 = 1 - allocates X1 */
            OP2(OInt, 2, 2),       /* r2 = 2 - allocates X2 */
            OP2(OInt, 3, 3),       /* r3 = 3 - allocates X3 */
            OP2(OInt, 4, 4),       /* r4 = 4 - allocates X4 */
            OP2(OInt, 5, 5),       /* r5 = 21 - allocates X5 */
            OP2(OInt, 6, 6),       /* r6 = 1 - allocates X6 */
            OP3(OShl, 5, 5, 6),    /* r5 = r5 << r6 = 42 (in-place!) */
            OP2(OMov, 0, 5),       /* r0 = r5 (moves 42 to r0) */
            OP2(OCall0, 7, 0),     /* r7 = call fn0() - triggers spill */
            OP1(ORet, 0),          /* return r0 - should be 42 */
        };
        test_add_function(c, 1, fn_type_i32, 8, regs, 11, ops);
    }

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42, got %d\n", ret);
        fprintf(stderr, "    (Bug: pd < pa causes old value to overwrite new in spill_regs)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Direct reproduction of the hello.hl bug scenario
 *
 * In hello.hl, the bug occurred in function 20 (String.__add):
 *   - OField loads string length into r5 (ends up in X5)
 *   - OShl shifts r5 by 1 (to convert chars to bytes)
 *   - Result goes into a LOWER register (X2)
 *   - OCallN triggers spill
 *   - X2 is spilled first (correct value)
 *   - X5 is spilled later (OLD value overwrites!)
 *
 * We can't easily reproduce OField in our minimal test, but we CAN
 * reproduce the scenario by:
 *   1. Getting a value via function call (forces it into return register X0)
 *   2. Moving it to a higher-numbered vreg
 *   3. Doing in-place shift
 */
TEST(hello_hl_scenario) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 13, 1, 0 };
    test_init_ints(c, 3, ints);

    hl_type *fn_type_i32 = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    test_alloc_functions(c, 3);

    /* fn0: returns 13 (simulates OField loading string length) */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),   /* r0 = 13 */
            OP1(ORet, 0),
        };
        test_add_function(c, 0, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn1: returns 0 (dummy to trigger second spill) */
    {
        hl_type *regs[] = { &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OInt, 0, 2),   /* r0 = 0 */
            OP1(ORet, 0),
        };
        test_add_function(c, 1, fn_type_i32, 1, regs, 2, ops);
    }

    /* fn2: Entry point - simulates String.__add length calculation
     *   r0 = call fn0()   ; Get length (13), result in X0, then stored to r0
     *   r1 = 1            ; Shift amount
     *   r0 = r0 << r1     ; r0 = 13 << 1 = 26 (in-place!)
     *   r2 = call fn1()   ; Triggers spill - BUG: old r0 may overwrite new
     *   return r0         ; Should be 26
     */
    {
        hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
        hl_opcode ops[] = {
            OP2(OCall0, 0, 0),     /* r0 = call fn0() = 13 */
            OP2(OInt, 1, 1),       /* r1 = 1 */
            OP3(OShl, 0, 0, 1),    /* r0 = r0 << r1 = 26 (in-place!) */
            OP2(OCall0, 2, 1),     /* r2 = call fn1() - triggers spill */
            OP1(ORet, 0),          /* return r0 - should be 26 */
        };
        test_add_function(c, 2, fn_type_i32, 3, regs, 5, ops);
    }

    c->entrypoint = 2;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 26) {
        fprintf(stderr, "    Expected 26, got %d\n", ret);
        fprintf(stderr, "    (Bug: String.__add pattern - in-place shift corrupted by spill)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(shl_inplace_then_call),
    TEST_ENTRY(add_inplace_then_call),
    TEST_ENTRY(mul_inplace_then_call),
    TEST_ENTRY(chain_inplace_then_call),
    TEST_ENTRY(shl_inplace_arg_then_call),
    TEST_ENTRY(string_concat_pattern),
    TEST_ENTRY(force_pd_less_than_pa),
    TEST_ENTRY(hello_hl_scenario),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - In-place Binary Op + Spill Tests\n");
    printf("(Tests for register binding bug in op_binop)\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
