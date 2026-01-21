/*
 * Test HVIRTUAL field access with different sizes for HashLink AArch64 JIT
 *
 * This tests the fix for a bug where OSetField/OField for HVIRTUAL objects
 * would always use 64-bit load/store instructions regardless of the actual
 * field size. This caused adjacent fields to be corrupted.
 *
 * The bug manifested when storing a 32-bit integer to a vfield - the 64-bit
 * store would zero out the adjacent field's memory.
 */
#include "test_harness.h"

/* Extended type indices for this test */
#define T_UI8   8
#define T_UI16  9

/* Initialize types including smaller integer types */
static void init_extended_types(hl_code *c) {
    test_init_base_types(c);

    /* Add HUI8 */
    c->types[T_UI8].kind = HUI8;

    /* Add HUI16 */
    c->types[T_UI16].kind = HUI16;

    c->ntypes = 10;
}

/* Helper to get type size (simplified version of hl_type_size) */
static int get_type_size(hl_type *t) {
    switch (t->kind) {
        case HUI8: case HBOOL: return 1;
        case HUI16: return 2;
        case HI32: case HF32: return 4;
        case HI64: case HF64: return 8;
        default: return sizeof(void*);  /* Pointers */
    }
}

/* Helper to calculate alignment padding */
static int pad_struct(int size, hl_type *t) {
    int align;
    switch (t->kind) {
        case HVOID: return 0;
        case HUI8: case HBOOL: align = 1; break;
        case HUI16: align = 2; break;
        case HI32: case HF32: align = 4; break;
        case HI64: case HF64: align = 8; break;
        default: align = sizeof(void*); break;  /* Pointers */
    }
    return (-size) & (align - 1);
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
        t->virt->indexes = (int*)calloc(nfields, sizeof(int));

        /* Calculate field layout (matching hl_init_virtual logic) */
        int vsize = sizeof(vvirtual) + sizeof(void*) * nfields;
        int size = vsize;

        for (int i = 0; i < nfields; i++) {
            char *name = (char*)malloc(16);
            sprintf(name, "field%d", i);
            t->virt->fields[i].name = (uchar*)name;
            t->virt->fields[i].t = field_types[i];
            t->virt->fields[i].hashed_name = i + 1000;  /* Unique hash */

            /* Add alignment padding */
            size += pad_struct(size, field_types[i]);
            t->virt->indexes[i] = size;
            size += get_type_size(field_types[i]);
        }

        t->virt->dataSize = size - vsize;
    }

    return t;
}

/*
 * Test: HVIRTUAL with adjacent i32 fields
 *
 * This tests the core bug: storing to one i32 field should not corrupt
 * the adjacent i32 field.
 *
 * struct { i32 a; i32 b; }
 *
 * r0 = new Virtual
 * r1 = 0xDEADBEEF
 * r2 = 0xCAFEBABE
 * set_field r0.field[0] = r1   (a = 0xDEADBEEF)
 * set_field r0.field[1] = r2   (b = 0xCAFEBABE)
 * r3 = get_field r0.field[0]   (read a - should still be 0xDEADBEEF)
 * return r3
 */
TEST(virtual_adjacent_i32_fields) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    init_extended_types(c);

    int ints[] = { (int)0xDEADBEEF, (int)0xCAFEBABE };
    test_init_ints(c, 2, ints);

    /* Create HVIRTUAL type with two i32 fields */
    hl_type *field_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *virt_type = create_virtual_type(c, 2, field_types);

    /* Function: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Registers: r0=virtual, r1=i32, r2=i32, r3=i32 */
    hl_type *regs[] = { virt_type, &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Virtual */
        OP2(OInt, 1, 0),           /* r1 = 0xDEADBEEF */
        OP2(OInt, 2, 1),           /* r2 = 0xCAFEBABE */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = r2 */
        OP3(OField, 3, 0, 0),      /* r3 = r0.field[0] */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 7, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != (int)0xDEADBEEF) {
        fprintf(stderr, "    Expected 0xDEADBEEF, got 0x%X\n", ret);
        fprintf(stderr, "    (Adjacent field store corrupted first field)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: HVIRTUAL with mixed size fields (i32 followed by pointer)
 *
 * This is the exact scenario from the bug report: an i32 field followed
 * by a pointer field. The i32 store with 64-bit instruction would zero
 * out the adjacent pointer.
 *
 * struct { i32 a; ptr b; }
 *
 * r0 = new Virtual (the struct)
 * r1 = 42
 * r2 = new Virtual (a non-null pointer to use as field value)
 * set_field r0.field[1] = r2   (b = pointer) - SET SECOND FIELD FIRST
 * set_field r0.field[0] = r1   (a = 42) - BUG: 64-bit store would zero b!
 * r3 = get_field r0.field[1]   (read b - should still be the pointer)
 * return r3
 */
TEST(virtual_i32_then_pointer) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    init_extended_types(c);

    int ints[] = { 42 };
    test_init_ints(c, 1, ints);

    /* Create a simple virtual type to use as a pointer value */
    hl_type *empty_field_types[] = { &c->types[T_I32] };
    hl_type *ptr_virt_type = create_virtual_type(c, 1, empty_field_types);

    /* Create HVIRTUAL type: { i32, virtual (pointer) } */
    hl_type *field_types[] = { &c->types[T_I32], ptr_virt_type };
    hl_type *virt_type = create_virtual_type(c, 2, field_types);

    /* Function: () -> virtual (pointer) */
    hl_type *fn_type = test_alloc_fun_type(c, ptr_virt_type, 0, NULL);

    /* Registers: r0=struct virtual, r1=i32, r2=ptr virtual, r3=ptr virtual */
    hl_type *regs[] = { virt_type, &c->types[T_I32], ptr_virt_type, ptr_virt_type };

    /*
     * We use ONew on r2 which has type ptr_virt_type (HVIRTUAL),
     * which can be new'd, giving us a non-null pointer.
     */
    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Virtual (the struct) */
        OP1(ONew, 2),              /* r2 = new Virtual (a non-null pointer value) */
        OP2(OInt, 1, 0),           /* r1 = 42 */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = r2 (set pointer FIRST) */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = r1 (BUG: would corrupt field[1]) */
        OP3(OField, 3, 0, 1),      /* r3 = r0.field[1] (read back pointer) */
        OP1(ORet, 3),              /* return r3 */
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 7, ops);

    int result;
    void *(*fn)(void) = (void*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    void *ret = fn();
    if (ret == NULL) {
        fprintf(stderr, "    Expected non-null pointer, got NULL\n");
        fprintf(stderr, "    (i32 store corrupted adjacent pointer field)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: HVIRTUAL with multiple i32 fields - verify no corruption
 *
 * struct { i32 a; i32 b; i32 c; i32 d; }
 *
 * Set all fields to different values, then read them all back.
 * Any corruption will show up as wrong values.
 */
TEST(virtual_multiple_i32_fields) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    init_extended_types(c);

    int ints[] = { 111, 222, 333, 444 };
    test_init_ints(c, 4, ints);

    /* Create HVIRTUAL type with four i32 fields */
    hl_type *field_types[] = { &c->types[T_I32], &c->types[T_I32],
                               &c->types[T_I32], &c->types[T_I32] };
    hl_type *virt_type = create_virtual_type(c, 4, field_types);

    /* Function: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Registers: r0=virtual, r1-r4=i32, r5=i32(result), r6=i32(temp) */
    hl_type *regs[] = { virt_type,
                        &c->types[T_I32], &c->types[T_I32],
                        &c->types[T_I32], &c->types[T_I32],
                        &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Virtual */
        OP2(OInt, 1, 0),           /* r1 = 111 */
        OP2(OInt, 2, 1),           /* r2 = 222 */
        OP2(OInt, 3, 2),           /* r3 = 333 */
        OP2(OInt, 4, 3),           /* r4 = 444 */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = 111 */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = 222 */
        OP3(OSetField, 0, 2, 3),   /* r0.field[2] = 333 */
        OP3(OSetField, 0, 3, 4),   /* r0.field[3] = 444 */
        /* Read back field[0] - should be 111 */
        OP3(OField, 5, 0, 0),      /* r5 = r0.field[0] */
        OP1(ORet, 5),
    };

    test_alloc_function(c, 0, fn_type, 7, regs, 11, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 111) {
        fprintf(stderr, "    Expected 111, got %d\n", ret);
        fprintf(stderr, "    (Field corruption detected)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Read back second field after setting first
 *
 * Same as above but read field[1] to verify it wasn't corrupted
 * by the field[0] store.
 */
TEST(virtual_read_second_field) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    init_extended_types(c);

    int ints[] = { 111, 222 };
    test_init_ints(c, 2, ints);

    /* Create HVIRTUAL type with two i32 fields */
    hl_type *field_types[] = { &c->types[T_I32], &c->types[T_I32] };
    hl_type *virt_type = create_virtual_type(c, 2, field_types);

    /* Function: () -> i32 */
    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    /* Registers: r0=virtual, r1=i32, r2=i32, r3=i32 */
    hl_type *regs[] = { virt_type, &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };

    hl_opcode ops[] = {
        OP1(ONew, 0),              /* r0 = new Virtual */
        OP2(OInt, 1, 0),           /* r1 = 111 */
        OP2(OInt, 2, 1),           /* r2 = 222 */
        OP3(OSetField, 0, 1, 2),   /* r0.field[1] = 222 (SET SECOND FIRST) */
        OP3(OSetField, 0, 0, 1),   /* r0.field[0] = 111 (this would corrupt field[1]) */
        OP3(OField, 3, 0, 1),      /* r3 = r0.field[1] (read back second field) */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 7, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 222) {
        fprintf(stderr, "    Expected 222, got %d\n", ret);
        fprintf(stderr, "    (field[0] store corrupted field[1] - the bug!)\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test registry */
static test_entry_t tests[] = {
    TEST_ENTRY(virtual_adjacent_i32_fields),
    TEST_ENTRY(virtual_i32_then_pointer),
    TEST_ENTRY(virtual_multiple_i32_fields),
    TEST_ENTRY(virtual_read_second_field),
};

/* Main test runner */
int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("HashLink AArch64 JIT - HVIRTUAL Field Size Tests\n");
    printf("Testing fix for 64-bit store corrupting adjacent fields\n\n");

    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
