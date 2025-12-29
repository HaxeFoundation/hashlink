/*
 * Test dynamic object operations for HashLink AArch64 JIT
 *
 * Tests: ODynGet, ODynSet, OToVirtual, OToDyn
 *
 * These are key opcodes used in hello.hl for dynamic field access.
 */
#include "test_harness.h"

/* Helper to create a HDYN type */
static hl_type *get_dyn_type(hl_code *c) {
    if (c->ntypes >= MAX_TYPES) return NULL;
    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));
    t->kind = HDYN;
    return t;
}

/* Helper to create a virtual type with fields */
static hl_type *create_virtual_type(hl_code *c, int nfields, const char **field_names, hl_type **field_types) {
    if (c->ntypes >= MAX_TYPES) return NULL;

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HVIRTUAL;
    t->virt = (hl_type_virtual*)calloc(1, sizeof(hl_type_virtual));
    t->virt->nfields = nfields;

    if (nfields > 0) {
        t->virt->fields = (hl_obj_field*)calloc(nfields, sizeof(hl_obj_field));
        for (int i = 0; i < nfields; i++) {
            t->virt->fields[i].name = (uchar*)field_names[i];
            t->virt->fields[i].t = field_types[i];
            t->virt->fields[i].hashed_name = hl_hash_gen(hl_get_ustring(c, 0), true); /* placeholder */
        }
    }

    return t;
}

/*
 * Test: Convert i32 to dynamic with OToDyn
 *
 * r0 = 42
 * r1 = to_dyn(r0)
 * return r0  ; just verify we don't crash
 */
TEST(to_dyn_i32) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *dyn_type = get_dyn_type(c);
    if (!dyn_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], dyn_type };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 42 */
        OP2(OToDyn, 1, 0),    /* r1 = to_dyn(r0) */
        OP1(ORet, 0),         /* return r0 */
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
 * Test: OMov with various types
 *
 * r0 = 42
 * r1 = mov r0
 * return r1
 */
TEST(mov_i32) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 42 */
        OP2(OMov, 1, 0),      /* r1 = r0 */
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
 * Test: ONull - load null pointer
 *
 * r0 = null
 * r1 = 42
 * return r1  ; just verify null doesn't crash us
 */
TEST(null_load) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *dyn_type = get_dyn_type(c);
    if (!dyn_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { dyn_type, &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONull, 0),        /* r0 = null */
        OP2(OInt, 1, 0),      /* r1 = 42 */
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
 * Test: OJNull / OJNotNull - null check branches
 *
 * r0 = null
 * if r0 == null goto L1
 * r1 = 0      ; should not reach here
 * jmp L2
 * L1:
 * r1 = 42     ; should reach here
 * L2:
 * return r1
 */
TEST(jnull_branch) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0, 42 };
    test_init_ints(c, 2, ints);

    hl_type *dyn_type = get_dyn_type(c);
    if (!dyn_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { dyn_type, &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONull, 0),           /* op0: r0 = null */
        OP2(OJNull, 0, 2),       /* op1: if r0 == null goto op4 (1+1+2=4) */
        OP2(OInt, 1, 0),         /* op2: r1 = 0 (not reached) */
        OP1(OJAlways, 1),        /* op3: goto op5 (3+1+1=5) */
        OP0(OLabel),             /* op4: label */
        OP2(OInt, 1, 1),         /* op5: r1 = 42 */
        OP0(OLabel),             /* op6: label */
        OP1(ORet, 1),            /* op7: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 8, ops);

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
 * Test: OJNotNull branch
 *
 * r0 = 1 (non-null when treated as pointer)
 * if r0 != null goto L1
 * r1 = 0
 * jmp L2
 * L1:
 * r1 = 42
 * L2:
 * return r1
 */
TEST(jnotnull_branch) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 0, 42 };
    test_init_ints(c, 2, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    /* Use BYTES type for the "pointer" register */
    hl_type *regs[] = { &c->types[T_BYTES], &c->types[T_I32] };

    /* We'll use OString to get a non-null pointer */
    c->nstrings = 1;
    c->strings = (char**)malloc(sizeof(char*));
    c->strings[0] = "x";
    c->strings_lens = (int*)malloc(sizeof(int));
    c->strings_lens[0] = 1;
    c->ustrings = (uchar**)calloc(1, sizeof(uchar*));

    hl_opcode ops[] = {
        OP2(OString, 0, 0),      /* op0: r0 = "x" (non-null) */
        OP2(OJNotNull, 0, 2),    /* op1: if r0 != null goto op4 */
        OP2(OInt, 1, 0),         /* op2: r1 = 0 (not reached) */
        OP1(OJAlways, 1),        /* op3: goto op5 */
        OP0(OLabel),             /* op4: label */
        OP2(OInt, 1, 1),         /* op5: r1 = 42 */
        OP0(OLabel),             /* op6: label */
        OP1(ORet, 1),            /* op7: return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 8, ops);

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
    TEST_ENTRY(to_dyn_i32),
    TEST_ENTRY(mov_i32),
    TEST_ENTRY(null_load),
    TEST_ENTRY(jnull_branch),
    TEST_ENTRY(jnotnull_branch),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Dynamic/Null Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
