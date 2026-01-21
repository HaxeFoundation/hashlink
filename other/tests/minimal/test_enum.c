/*
 * Test enum operations for HashLink AArch64 JIT
 *
 * Tests: OEnumAlloc, OEnumField, OSetEnumField, OEnumIndex, OMakeEnum
 */
#include "test_harness.h"

/*
 * Helper to create an enum type with a single construct that has pointer fields.
 * This is similar to how Option<T> or similar sum types work.
 *
 * Construct 0: has `nfields` pointer-sized fields at 8-byte offsets starting at offset 8
 * (offset 0 is typically the enum tag/index)
 */
static hl_type *create_enum_type(hl_code *c, const char *name, int nfields) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HENUM;
    t->tenum = (hl_type_enum*)calloc(1, sizeof(hl_type_enum));
    t->tenum->name = (const uchar*)name;
    t->tenum->nconstructs = 1;
    t->tenum->constructs = (hl_enum_construct*)calloc(1, sizeof(hl_enum_construct));

    hl_enum_construct *cons = &t->tenum->constructs[0];
    cons->name = (const uchar*)"Cons";
    cons->nparams = nfields;
    cons->hasptr = true;

    if (nfields > 0) {
        cons->params = (hl_type**)calloc(nfields, sizeof(hl_type*));
        cons->offsets = (int*)calloc(nfields, sizeof(int));
        for (int i = 0; i < nfields; i++) {
            cons->params[i] = &c->types[T_I64];  /* Use i64/pointer type */
            cons->offsets[i] = 8 + i * 8;  /* Fields start at offset 8 (after tag) */
        }
    }

    /* Size = 8 (tag) + nfields * 8 */
    cons->size = 8 + nfields * 8;

    return t;
}

/*
 * Test: OEnumField - extract a field from an enum, then use it
 *
 * This test specifically targets the bug where OEnumField doesn't clear
 * the destination register binding, causing stale values to be used.
 *
 * The pattern is:
 *   r1 = alloc_enum          ; allocate enum
 *   set_enum_field r1, 0, r0 ; store a value (42) into field 0
 *   r2 = enum_field r1, 0    ; extract field 0 -> should be 42
 *   return r2                ; return extracted value
 *
 * If the register binding bug exists, r2 might return garbage instead of 42.
 */
TEST(enum_field_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create enum type with 1 field */
    hl_type *enum_t = create_enum_type(c, "TestEnum", 1);
    if (!enum_t) return TEST_FAIL;

    /* Function: () -> i64 */
    hl_type *ret_type = &c->types[T_I64];
    hl_type *fn_type = test_alloc_fun_type(c, ret_type, 0, NULL);

    /* Registers:
     * r0: i64 (temp for value 42)
     * r1: enum (the allocated enum)
     * r2: i64 (extracted field value)
     */
    hl_type *regs[] = { &c->types[T_I64], enum_t, &c->types[T_I64] };

    /* Integer constants */
    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Opcodes */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),              /* r0 = 42 */
        OP2(OEnumAlloc, 1, 0),        /* r1 = alloc enum (construct 0) */
        OP3(OSetEnumField, 1, 0, 0),  /* r1.field[0] = r0 (42) */
        { OEnumField, 2, 1, 0, (int*)(intptr_t)0 }, /* r2 = r1.field[0] (extra=0) */
        OP1(ORet, 2),                 /* return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 5, ops);

    int result;
    int64_t (*func)(void) = (int64_t (*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = func();
    if (ret != 42) {
        printf("\n    Expected 42, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OEnumField with multiple fields and uses
 *
 * This test more closely matches the uvsample crash pattern:
 *   - Multiple OEnumField extractions
 *   - The extracted values are then used as function arguments
 *
 * Pattern:
 *   r0 = 100
 *   r1 = 200
 *   r2 = alloc_enum
 *   set_enum_field r2, 0, r0  ; field 0 = 100
 *   set_enum_field r2, 1, r1  ; field 1 = 200
 *   r3 = enum_field r2, 0     ; r3 = 100
 *   r4 = enum_field r2, 1     ; r4 = 200
 *   r5 = r3 + r4              ; r5 = 300
 *   return r5
 */
TEST(enum_field_multiple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create enum type with 2 fields */
    hl_type *enum_t = create_enum_type(c, "TestEnum2", 2);
    if (!enum_t) return TEST_FAIL;

    /* Function: () -> i64 */
    hl_type *ret_type = &c->types[T_I64];
    hl_type *fn_type = test_alloc_fun_type(c, ret_type, 0, NULL);

    /* Registers:
     * r0: i64 (value 100)
     * r1: i64 (value 200)
     * r2: enum
     * r3: i64 (extracted field 0)
     * r4: i64 (extracted field 1)
     * r5: i64 (sum)
     */
    hl_type *regs[] = {
        &c->types[T_I64], &c->types[T_I64], enum_t,
        &c->types[T_I64], &c->types[T_I64], &c->types[T_I64]
    };

    /* Integer constants */
    int ints[] = { 100, 200 };
    test_init_ints(c, 2, ints);

    /* Opcodes */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),              /* r0 = 100 */
        OP2(OInt, 1, 1),              /* r1 = 200 */
        OP2(OEnumAlloc, 2, 0),        /* r2 = alloc enum */
        OP3(OSetEnumField, 2, 0, 0),  /* r2.field[0] = r0 */
        OP3(OSetEnumField, 2, 1, 1),  /* r2.field[1] = r1 */
        { OEnumField, 3, 2, 0, (int*)(intptr_t)0 }, /* r3 = r2.field[0] */
        { OEnumField, 4, 2, 0, (int*)(intptr_t)1 }, /* r4 = r2.field[1] */
        OP3(OAdd, 5, 3, 4),           /* r5 = r3 + r4 */
        OP1(ORet, 5),                 /* return r5 */
    };

    test_alloc_function(c, 0, fn_type, 6, regs, 9, ops);

    int result;
    int64_t (*func)(void) = (int64_t (*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = func();
    if (ret != 300) {
        printf("\n    Expected 300, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OEnumField followed by function call
 *
 * This is the exact pattern that causes the uvsample crash:
 *   - Extract a field from enum
 *   - Pass it as argument to a function call
 *
 * If dst register binding isn't cleared, the call might use a stale value.
 *
 * Pattern:
 *   r0 = 42
 *   r1 = alloc_enum
 *   set_enum_field r1, 0, r0
 *   r2 = enum_field r1, 0    ; extract 42
 *   r3 = call identity(r2)   ; call function with extracted value
 *   return r3
 */

/* Native identity function for testing */
static int64_t native_identity(int64_t x) {
    return x;
}

TEST(enum_field_then_call) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create enum type with 1 field */
    hl_type *enum_t = create_enum_type(c, "TestEnum3", 1);
    if (!enum_t) return TEST_FAIL;

    /* Native function type: (i64) -> i64 */
    hl_type *i64_t = &c->types[T_I64];
    hl_type *native_args[] = { i64_t };
    hl_type *native_fn_type = test_alloc_fun_type(c, i64_t, 1, native_args);

    /* Add native function at findex 1 */
    test_add_native(c, 1, "test", "identity", native_fn_type, (void*)native_identity);

    /* Main function type: () -> i64 */
    hl_type *fn_type = test_alloc_fun_type(c, i64_t, 0, NULL);

    /* Registers:
     * r0: i64 (value 42)
     * r1: enum
     * r2: i64 (extracted field)
     * r3: i64 (call result)
     */
    hl_type *regs[] = { i64_t, enum_t, i64_t, i64_t };

    /* Integer constants */
    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Opcodes */
    hl_opcode ops[] = {
        OP2(OInt, 0, 0),              /* r0 = 42 */
        OP2(OEnumAlloc, 1, 0),        /* r1 = alloc enum */
        OP3(OSetEnumField, 1, 0, 0),  /* r1.field[0] = r0 */
        { OEnumField, 2, 1, 0, (int*)(intptr_t)0 }, /* r2 = r1.field[0] */
        OP3(OCall1, 3, 1, 2),         /* r3 = call F1(r2) - native identity */
        OP1(ORet, 3),                 /* return r3 */
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 6, ops);

    int result;
    int64_t (*func)(void) = (int64_t (*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = func();
    if (ret != 42) {
        printf("\n    Expected 42, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OEnumIndex - get the construct index of an enum value
 */
TEST(enum_index) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create enum type */
    hl_type *enum_t = create_enum_type(c, "TestEnum4", 1);
    if (!enum_t) return TEST_FAIL;

    /* Function: () -> i32 */
    hl_type *ret_type = &c->types[T_I32];
    hl_type *fn_type = test_alloc_fun_type(c, ret_type, 0, NULL);

    /* Registers:
     * r0: enum
     * r1: i32 (index result)
     */
    hl_type *regs[] = { enum_t, &c->types[T_I32] };

    /* Opcodes */
    hl_opcode ops[] = {
        OP2(OEnumAlloc, 0, 0),  /* r0 = alloc enum (construct 0) */
        OP2(OEnumIndex, 1, 0),  /* r1 = index of r0 (should be 0) */
        OP1(ORet, 1),           /* return r1 */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    int (*func)(void) = (int (*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = func();
    if (ret != 0) {
        printf("\n    Expected 0, got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test registry */
int main(int argc, char **argv) {
    test_entry_t tests[] = {
        TEST_ENTRY(enum_field_basic),
        TEST_ENTRY(enum_field_multiple),
        TEST_ENTRY(enum_field_then_call),
        TEST_ENTRY(enum_index),
    };

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
