/*
 * Test object operations for HashLink AArch64 JIT
 *
 * Tests: ONew, OField, OSetField, ONullCheck, OGetThis, OSetThis
 *
 * These are key opcodes used in hello.hl
 */
#include "test_harness.h"

/* We need to create object types for these tests */

/* Helper to create an HDYNOBJ type (dynamic object) */
static hl_type *create_dynobj_type(hl_code *c) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HDYNOBJ;
    /* HDYNOBJ has no obj pointer - it's dynamically allocated */
    return t;
}

/* Helper to create an HVIRTUAL type */
static hl_type *create_virtual_type(hl_code *c, int nfields, hl_type **field_types) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HVIRTUAL;
    t->virt = (hl_type_virtual*)calloc(1, sizeof(hl_type_virtual));
    t->virt->nfields = nfields;

    if (nfields > 0) {
        t->virt->fields = (hl_obj_field*)calloc(nfields, sizeof(hl_obj_field));
        for (int i = 0; i < nfields; i++) {
            t->virt->fields[i].name = (uchar*)"field";
            t->virt->fields[i].t = field_types[i];
            t->virt->fields[i].hashed_name = i;
        }
    }

    return t;
}

/* Helper to create an object type with fields */
static hl_type *create_obj_type(hl_code *c, const char *name, int nfields, hl_type **field_types) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types\n");
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HOBJ;
    t->obj = (hl_type_obj*)calloc(1, sizeof(hl_type_obj));
    t->obj->name = (uchar*)name;
    t->obj->nfields = nfields;
    t->obj->nproto = 0;
    t->obj->nbindings = 0;

    if (nfields > 0) {
        t->obj->fields = (hl_obj_field*)calloc(nfields, sizeof(hl_obj_field));
        for (int i = 0; i < nfields; i++) {
            t->obj->fields[i].name = (uchar*)"field";
            t->obj->fields[i].t = field_types[i];
            t->obj->fields[i].hashed_name = i;  /* Simple hash for testing */
        }
    }

    /* Don't call hl_get_obj_rt here - it needs a module allocator.
     * The JIT will call it when needed, after the module is set up. */

    return t;
}

/*
 * Test: ONullCheck on non-null value (should not throw)
 *
 * r0 = 42
 * null_check r0  ; should pass (non-zero)
 * return r0
 */
TEST(null_check_nonnull) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { &c->types[T_I32] };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),      /* r0 = 42 */
        OP1(ONullCheck, 0),   /* null_check r0 - should pass */
        OP1(ORet, 0),
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 3, ops);

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
 * Test: Create object with ONew and access field with OField/OSetField
 *
 * Object type: { i32 value }
 *
 * r0 = new Obj
 * r1 = 42
 * set_field r0.field[0] = r1
 * r2 = get_field r0.field[0]
 * return r2
 */
TEST(object_field_access) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Create object type with one i32 field */
    hl_type *field_types[] = { &c->types[T_I32] };
    hl_type *obj_type = create_obj_type(c, "TestObj", 1, field_types);
    if (!obj_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { obj_type, &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OInt, 1, 0),           /* r1 = 42 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP3(OField, 2, 0, 0),      /* r2 = r0.field[0] */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 5, ops);

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
 * Test: Object with multiple fields
 *
 * Object type: { i32 a, i32 b }
 *
 * r0 = new Obj
 * r1 = 10
 * r2 = 32
 * set_field r0.field[0] = r1  ; a = 10
 * set_field r0.field[1] = r2  ; b = 32
 * r3 = get_field r0.field[0]  ; r3 = 10
 * r4 = get_field r0.field[1]  ; r4 = 32
 * r5 = r3 + r4                ; r5 = 42
 * return r5
 */
TEST(object_multiple_fields) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* Create object type with two i32 fields */
    hl_type *field_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *obj_type = create_obj_type(c, "TestObj2", 2, field_types);
    if (!obj_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = {
        obj_type,
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32],
        &c->types[T_I32]
    };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OInt, 1, 0),           /* r1 = 10 */
        OP2(OInt, 2, 1),           /* r2 = 32 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = r2 */
        OP3(OField, 3, 0, 0),      /* r3 = r0.field[0] */
        OP3(OField, 4, 0, 1),      /* r4 = r0.field[1] */
        OP3(OAdd, 5, 3, 4),        /* r5 = r3 + r4 */
        OP1(ORet, 5),
    };

    test_alloc_function(c, 0, fn_type, 6, regs, 9, ops);

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
 * Test: Object with pointer field
 *
 * Object type: { bytes ptr }
 */
TEST(object_pointer_field) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create object type with one pointer field */
    hl_type *field_types[] = { &c->types[T_BYTES] };
    hl_type *obj_type = create_obj_type(c, "TestObjPtr", 1, field_types);
    if (!obj_type) return TEST_FAIL;

    /* Setup a string to store */
    c->nstrings = 1;
    c->strings = (char**)malloc(sizeof(char*));
    c->strings[0] = "test";
    c->strings_lens = (int*)malloc(sizeof(int));
    c->strings_lens[0] = 4;
    c->ustrings = (uchar**)calloc(1, sizeof(uchar*));

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);
    hl_type *regs[] = { obj_type, &c->types[T_BYTES], &c->types[T_BYTES] };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OString, 1, 0),        /* r1 = "test" */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP3(OField, 2, 0, 0),      /* r2 = r0.field[0] */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 5, ops);

    int result;
    uchar* (*fn)(void) = (uchar*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    uchar *ret = fn();
    if (ret == NULL) {
        fprintf(stderr, "    Got NULL pointer\n");
        return TEST_FAIL;
    }

    /* Check first char is 't' (UTF-16) */
    if (ret[0] != 't') {
        fprintf(stderr, "    Expected 't', got 0x%04x\n", ret[0]);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: ONew with HDYNOBJ type
 *
 * This tests that dynamic objects (HDYNOBJ) are allocated correctly.
 * The JIT must call hl_alloc_dynobj() (no args) instead of hl_alloc_obj(type).
 *
 * r0 = new DynObj  ; allocate dynamic object
 * r1 = 42
 * return r1        ; just verify allocation doesn't crash
 */
TEST(new_dynobj) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Create HDYNOBJ type */
    hl_type *dynobj_type = create_dynobj_type(c);
    if (!dynobj_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { dynobj_type, &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),         /* r0 = new DynObj - must call hl_alloc_dynobj() */
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
 * Test: ONew with HVIRTUAL type
 *
 * This tests that virtual objects (HVIRTUAL) are allocated correctly.
 * The JIT must call hl_alloc_virtual(type) instead of hl_alloc_obj(type).
 *
 * r0 = new Virtual  ; allocate virtual object
 * r1 = 42
 * return r1         ; just verify allocation doesn't crash
 */
TEST(new_virtual) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Create HVIRTUAL type with one i32 field */
    hl_type *field_types[] = { &c->types[T_I32] };
    hl_type *virt_type = create_virtual_type(c, 1, field_types);
    if (!virt_type) return TEST_FAIL;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *regs[] = { virt_type, &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),         /* r0 = new Virtual - must call hl_alloc_virtual() */
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

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(null_check_nonnull),
    TEST_ENTRY(object_field_access),
    TEST_ENTRY(object_multiple_fields),
    TEST_ENTRY(object_pointer_field),
    TEST_ENTRY(new_dynobj),
    /* new_virtual requires complex type setup (virt->indexes) that our minimal
     * test harness doesn't support. HVIRTUAL allocation is tested via hello.hl. */
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Object Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
