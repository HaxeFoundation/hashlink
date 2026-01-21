/*
 * Test native call result stored in object field
 *
 * This mimics the pattern in hello.hl that crashes:
 * 1. Call native function that returns a value
 * 2. Store result in object field
 * 3. Return object
 * 4. Read field from returned object
 * 5. Use the value
 */
#include "test_harness.h"

/* Native function that returns an integer */
static int native_get_value(void) {
    return 42;
}

/* Native function that returns a pointer */
static void *native_get_ptr(void) {
    static int data = 123;
    return &data;
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
            t->obj->fields[i].hashed_name = i;
        }
    }

    return t;
}

/*
 * Test: Call native, store in field, return object, read field
 *
 * This is a two-function test to match hello.hl's pattern:
 *
 * F0 (inner):
 *   r0 = new Obj
 *   r1 = call native_get_value()
 *   set_field r0.field[0] = r1
 *   return r0
 *
 * F1 (outer, entrypoint):
 *   r0 = call F0()
 *   r1 = get_field r0.field[0]
 *   return r1
 *
 * Expected: 42
 */
TEST(native_to_field_to_return) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create object type with one i32 field */
    hl_type *field_types[] = { &c->types[T_I32] };
    hl_type *obj_type = create_obj_type(c, "TestObj", 1, field_types);
    if (!obj_type) return TEST_FAIL;

    /* Native function type: () -> i32 */
    hl_type *native_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Add native function at findex 2 */
    test_add_native(c, 2, "test", "native_get_value", native_fn_type, native_get_value);

    /* F0 (inner function): () -> obj
     * r0 = new Obj
     * r1 = call native (findex 2)
     * set_field r0.field[0] = r1
     * return r0
     */
    hl_type *inner_fn_type = test_alloc_fun_type(c, obj_type, 0, NULL);
    hl_type *inner_regs[] = { obj_type, &c->types[T_I32] };
    hl_opcode inner_ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OCall0, 1, 2),         /* r1 = call native F2 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP1(ORet, 0),              /* return r0 */
    };
    test_alloc_function(c, 0, inner_fn_type, 2, inner_regs, 4, inner_ops);

    /* F1 (outer function, entrypoint): () -> i32
     * r0 = call F0()
     * r1 = get_field r0.field[0]
     * return r1
     */
    hl_type *outer_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *outer_regs[] = { obj_type, &c->types[T_I32] };
    hl_opcode outer_ops[] = {
        OP2(OCall0, 0, 0),         /* r0 = call F0 */
        OP3(OField, 1, 0, 0),      /* r1 = r0.field[0] */
        OP1(ORet, 1),              /* return r1 */
    };
    test_alloc_function(c, 1, outer_fn_type, 2, outer_regs, 3, outer_ops);

    /* Set entrypoint to F1 */
    c->entrypoint = 1;

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
 * Test: Same pattern but with pointer type (like array)
 *
 * F0 (inner):
 *   r0 = new Obj
 *   r1 = call native_get_ptr()
 *   set_field r0.field[0] = r1
 *   return r0
 *
 * F1 (outer):
 *   r0 = call F0()
 *   r1 = get_field r0.field[0]
 *   return r1
 */
TEST(native_ptr_to_field_to_return) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Create object type with one bytes (pointer) field */
    hl_type *field_types[] = { &c->types[T_BYTES] };
    hl_type *obj_type = create_obj_type(c, "TestObjPtr", 1, field_types);
    if (!obj_type) return TEST_FAIL;

    /* Native function type: () -> bytes */
    hl_type *native_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);

    /* Add native function at findex 2 */
    test_add_native(c, 2, "test", "native_get_ptr", native_fn_type, native_get_ptr);

    /* F0 (inner function): () -> obj */
    hl_type *inner_fn_type = test_alloc_fun_type(c, obj_type, 0, NULL);
    hl_type *inner_regs[] = { obj_type, &c->types[T_BYTES] };
    hl_opcode inner_ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OCall0, 1, 2),         /* r1 = call native F2 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP1(ORet, 0),              /* return r0 */
    };
    test_alloc_function(c, 0, inner_fn_type, 2, inner_regs, 4, inner_ops);

    /* F1 (outer function): () -> bytes */
    hl_type *outer_fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);
    hl_type *outer_regs[] = { obj_type, &c->types[T_BYTES] };
    hl_opcode outer_ops[] = {
        OP2(OCall0, 0, 0),         /* r0 = call F0 */
        OP3(OField, 1, 0, 0),      /* r1 = r0.field[0] */
        OP1(ORet, 1),              /* return r1 */
    };
    test_alloc_function(c, 1, outer_fn_type, 2, outer_regs, 3, outer_ops);

    c->entrypoint = 1;

    int result;
    void *(*fn)(void) = (void*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    void *ret = fn();
    static int expected_data = 123;
    if (ret != &expected_data) {
        /* The native returns a pointer to its static - compare values */
        if (ret == NULL) {
            fprintf(stderr, "    Got NULL pointer\n");
            return TEST_FAIL;
        }
        int got = *(int*)ret;
        if (got != 123) {
            fprintf(stderr, "    Expected ptr to 123, got ptr to %d\n", got);
            return TEST_FAIL;
        }
    }

    return TEST_PASS;
}

/*
 * Test: Multiple fields set from native calls
 *
 * This more closely matches F295 which sets multiple fields
 */
TEST(native_multiple_fields) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 100 };
    test_init_ints(c, 1, ints);

    /* Create object type with 3 fields */
    hl_type *field_types[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_BYTES] };
    hl_type *obj_type = create_obj_type(c, "TestObj3", 3, field_types);
    if (!obj_type) return TEST_FAIL;

    /* Native function types */
    hl_type *native_int_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *native_ptr_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);

    /* Add native functions at findex 2 and 3 */
    test_add_native(c, 2, "test", "native_get_value", native_int_type, native_get_value);
    test_add_native(c, 3, "test", "native_get_ptr", native_ptr_type, native_get_ptr);

    /* F0 (inner): () -> obj
     * r0 = new Obj
     * r1 = 100
     * set_field r0.field[0] = r1
     * r2 = call native_get_value()
     * set_field r0.field[1] = r2
     * r3 = call native_get_ptr()
     * set_field r0.field[2] = r3
     * return r0
     */
    hl_type *inner_fn_type = test_alloc_fun_type(c, obj_type, 0, NULL);
    hl_type *inner_regs[] = { obj_type, &c->types[T_I32], &c->types[T_I32], &c->types[T_BYTES] };
    hl_opcode inner_ops[] = {
        OP1(ONew, 0),              /* r0 = new Obj */
        OP2(OInt, 1, 0),           /* r1 = 100 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP2(OCall0, 2, 2),         /* r2 = call native F2 (returns 42) */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = r2 */
        OP2(OCall0, 3, 3),         /* r3 = call native F3 (returns ptr) */
        OP3(OSetField, 0, 2, 3),   /* r0.field[2] = r3 */
        OP1(ORet, 0),              /* return r0 */
    };
    test_alloc_function(c, 0, inner_fn_type, 4, inner_regs, 8, inner_ops);

    /* F1 (outer): () -> i32
     * r0 = call F0()
     * r1 = get_field r0.field[0]   ; should be 100
     * r2 = get_field r0.field[1]   ; should be 42
     * r3 = r1 + r2                 ; should be 142
     * r4 = get_field r0.field[2]   ; should be ptr
     * null_check r4                ; ptr should not be null
     * return r3
     */
    hl_type *outer_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *outer_regs[] = { obj_type, &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_BYTES] };
    hl_opcode outer_ops[] = {
        OP2(OCall0, 0, 0),         /* r0 = call F0 */
        OP3(OField, 1, 0, 0),      /* r1 = r0.field[0] */
        OP3(OField, 2, 0, 1),      /* r2 = r0.field[1] */
        OP3(OAdd, 3, 1, 2),        /* r3 = r1 + r2 */
        OP3(OField, 4, 0, 2),      /* r4 = r0.field[2] */
        OP1(ONullCheck, 4),        /* null_check r4 */
        OP1(ORet, 3),              /* return r3 */
    };
    test_alloc_function(c, 1, outer_fn_type, 5, outer_regs, 7, outer_ops);

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 142) {
        fprintf(stderr, "    Expected 142 (100+42), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OCall2 with arguments passed to inner function
 *
 * This matches hello.hl's pattern more closely:
 * - Entrypoint uses OCall2 to call F295 with 2 type args
 * - F295 uses OCall1 to call native with one of those args
 *
 * F0 (inner): (i32 a, i32 b) -> obj
 *   r2 = new Obj
 *   r3 = call native_get_value()  ; returns 42
 *   r4 = a + b + r3
 *   set_field r2.field[0] = r4
 *   return r2
 *
 * F1 (outer): () -> i32
 *   r0 = 10
 *   r1 = 20
 *   r2 = call F0(r0, r1)          ; OCall2
 *   r3 = get_field r2.field[0]    ; should be 10+20+42=72
 *   return r3
 */
TEST(ocall2_with_native_in_callee) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20 };
    test_init_ints(c, 2, ints);

    /* Create object type with one i32 field */
    hl_type *field_types[] = { &c->types[T_I32] };
    hl_type *obj_type = create_obj_type(c, "TestObj", 1, field_types);
    if (!obj_type) return TEST_FAIL;

    /* Native function type: () -> i32 */
    hl_type *native_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Add native function at findex 2 */
    test_add_native(c, 2, "test", "native_get_value", native_fn_type, native_get_value);

    /* F0 (inner): (i32, i32) -> obj
     * r0 = arg a (i32)
     * r1 = arg b (i32)
     * r2 = new Obj
     * r3 = call native F2 (returns 42)
     * r4 = a + b
     * r5 = r4 + r3
     * set_field r2.field[0] = r5
     * return r2
     */
    hl_type *inner_arg_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *inner_fn_type = test_alloc_fun_type(c, obj_type, 2, inner_arg_types);
    hl_type *inner_regs[] = {
        &c->types[T_I32], &c->types[T_I32],  /* r0, r1 = args */
        obj_type, &c->types[T_I32],          /* r2 = obj, r3 = native result */
        &c->types[T_I32], &c->types[T_I32]   /* r4, r5 = temps */
    };
    hl_opcode inner_ops[] = {
        OP1(ONew, 2),              /* r2 = new Obj */
        OP2(OCall0, 3, 2),         /* r3 = call native F2 (returns 42) */
        OP3(OAdd, 4, 0, 1),        /* r4 = r0 + r1 */
        OP3(OAdd, 5, 4, 3),        /* r5 = r4 + r3 */
        OP3(OSetField, 2, 0, 5),   /* r2.field[0] = r5 */
        OP1(ORet, 2),              /* return r2 */
    };
    test_alloc_function(c, 0, inner_fn_type, 6, inner_regs, 6, inner_ops);

    /* F1 (outer): () -> i32
     * r0 = 10
     * r1 = 20
     * r2 = call F0(r0, r1)  ; OCall2
     * r3 = get_field r2.field[0]
     * return r3
     */
    hl_type *outer_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *outer_regs[] = {
        &c->types[T_I32], &c->types[T_I32],  /* r0, r1 = args to pass */
        obj_type, &c->types[T_I32]           /* r2 = result obj, r3 = field value */
    };
    hl_opcode outer_ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 10 */
        OP2(OInt, 1, 1),           /* r1 = 20 */
        OP4_CALL2(OCall2, 2, 0, 0, 1),  /* r2 = call F0(r0, r1) */
        OP3(OField, 3, 2, 0),      /* r3 = r2.field[0] */
        OP1(ORet, 3),              /* return r3 */
    };
    test_alloc_function(c, 1, outer_fn_type, 4, outer_regs, 5, outer_ops);

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 72) {  /* 10 + 20 + 42 = 72 */
        fprintf(stderr, "    Expected 72 (10+20+42), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OCall1 passing argument to native
 *
 * F0 (inner): (i32 x) -> i32
 *   r1 = call native_add_ten(r0)
 *   return r1
 *
 * F1 (outer): () -> i32
 *   r0 = 32
 *   r1 = call F0(r0)  ; OCall1
 *   return r1         ; should be 42
 */
static int native_add_ten(int x) {
    return x + 10;
}

TEST(ocall1_arg_to_native) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 32 };
    test_init_ints(c, 1, ints);

    /* Native function type: (i32) -> i32 */
    hl_type *native_arg_types[] = { &c->types[T_I32] };
    hl_type *native_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, native_arg_types);

    /* Add native function at findex 2 */
    test_add_native(c, 2, "test", "native_add_ten", native_fn_type, native_add_ten);

    /* F0 (inner): (i32) -> i32
     * r0 = arg x
     * r1 = call native F2(r0)
     * return r1
     */
    hl_type *inner_arg_types[] = { &c->types[T_I32] };
    hl_type *inner_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 1, inner_arg_types);
    hl_type *inner_regs[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_opcode inner_ops[] = {
        OP3(OCall1, 1, 2, 0),      /* r1 = call F2(r0) */
        OP1(ORet, 1),              /* return r1 */
    };
    test_alloc_function(c, 0, inner_fn_type, 2, inner_regs, 2, inner_ops);

    /* F1 (outer): () -> i32
     * r0 = 32
     * r1 = call F0(r0)  ; OCall1
     * return r1
     */
    hl_type *outer_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);
    hl_type *outer_regs[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_opcode outer_ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 32 */
        OP3(OCall1, 1, 0, 0),      /* r1 = call F0(r0) */
        OP1(ORet, 1),              /* return r1 */
    };
    test_alloc_function(c, 1, outer_fn_type, 2, outer_regs, 3, outer_ops);

    c->entrypoint = 1;

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 42) {
        fprintf(stderr, "    Expected 42 (32+10), got %d\n", ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(native_to_field_to_return),
    TEST_ENTRY(native_ptr_to_field_to_return),
    TEST_ENTRY(native_multiple_fields),
    TEST_ENTRY(ocall2_with_native_in_callee),
    TEST_ENTRY(ocall1_arg_to_native),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Native->Field Pattern Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
