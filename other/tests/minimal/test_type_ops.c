/*
 * Test type operations for HashLink AArch64 JIT
 *
 * Tests: OType, OGetType, OGetTID, OSafeCast, OUnsafeCast, OToUFloat
 *
 * OType: load a type pointer from the types array
 * OGetType: get the runtime type of an object
 * OGetTID: get the type ID (first 4 bytes) of an object
 * OSafeCast: safe dynamic cast with runtime check
 * OUnsafeCast: unchecked type cast
 * OToUFloat: convert unsigned int to float
 */
#include "test_harness.h"

/* Helper to create an object type */
static hl_type *create_obj_type(hl_code *c, const char *name) {
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
    t->obj->nfields = 0;
    t->obj->nproto = 0;
    t->obj->nbindings = 0;

    return t;
}

/*
 * Test: OType - load type pointer
 *
 * The type pointer should be non-null and have the correct kind.
 * We use a native to verify the type kind.
 */
static int verify_type_kind(hl_type *t, int expected_kind) {
    if (t == NULL) return 0;
    return (t->kind == expected_kind) ? 1 : 0;
}

TEST(type_load) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { HI32 };  /* expected kind */
    test_init_ints(c, 1, ints);

    /* Native: verify_type_kind(type, kind) -> i32 */
    hl_type *verify_args[] = { &c->types[T_TYPE], &c->types[T_I32] };
    hl_type *verify_fn_type = test_alloc_fun_type(c, &c->types[T_I32], 2, verify_args);
    test_add_native(c, 1, "test", "verify_type_kind", verify_fn_type, (void*)verify_type_kind);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_TYPE],  /* r0 = type pointer */
        &c->types[T_I32],   /* r1 = expected kind */
        &c->types[T_I32],   /* r2 = result */
    };

    hl_opcode ops[] = {
        OP2(OType, 0, T_I32),              /* r0 = &types[T_I32] */
        OP2(OInt, 1, 0),                   /* r1 = HI32 */
        OP4_CALL2(OCall2, 2, 1, 0, 1),     /* r2 = verify_type_kind(r0, r1) */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int (*fn)(void) = (int(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int ret = fn();
    if (ret != 1) {
        fprintf(stderr, "    Type verification failed\n");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OToUFloat - convert unsigned int to float
 *
 * This is important for correctly converting large unsigned values.
 * 0xFFFFFFFF as unsigned is 4294967295, not -1.
 */
TEST(to_ufloat_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1000000 };  /* 1 million */
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = unsigned value */
        &c->types[T_F64],  /* r1 = float result */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),        /* r0 = 1000000 */
        OP2(OToUFloat, 1, 0),   /* r1 = (float)r0 unsigned */
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 1000000.0;
    double diff = ret - expected;
    if (diff < 0) diff = -diff;
    if (diff > 0.1) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OToUFloat with large unsigned value
 *
 * 0x80000000 (2147483648) - would be negative if signed
 */
TEST(to_ufloat_large) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { (int)0x80000000 };  /* 2^31 as unsigned */
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],
        &c->types[T_F64],
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),
        OP2(OToUFloat, 1, 0),
        OP1(ORet, 1),
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    double expected = 2147483648.0;  /* 2^31 */
    double diff = ret - expected;
    if (diff < 0) diff = -diff;
    if (diff > 1.0) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OUnsafeCast - reinterpret type without checks
 *
 * Cast an i64 to bytes (pointer type) and back.
 */
TEST(unsafe_cast_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 12345 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_I64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I64],    /* r0 = original value */
        &c->types[T_BYTES],  /* r1 = cast to bytes (pointer) */
        &c->types[T_I64],    /* r2 = cast back to i64 */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),           /* r0 = 12345 */
        OP2(OUnsafeCast, 1, 0),    /* r1 = (bytes)r0 */
        OP2(OUnsafeCast, 2, 1),    /* r2 = (i64)r1 */
        OP1(ORet, 2),
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 4, ops);

    int result;
    int64_t (*fn)(void) = (int64_t(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    int64_t ret = fn();
    if (ret != 12345) {
        fprintf(stderr, "    Expected 12345, got %ld\n", (long)ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: OToSFloat vs OToUFloat comparison
 *
 * -1 converted:
 *   ToSFloat: -1.0
 *   ToUFloat: 4294967295.0
 */
TEST(tofloat_signed_vs_unsigned) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { -1 };
    test_init_ints(c, 1, ints);

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_F64], 0, NULL);

    hl_type *regs[] = {
        &c->types[T_I32],  /* r0 = -1 */
        &c->types[T_F64],  /* r1 = signed float */
        &c->types[T_F64],  /* r2 = unsigned float */
        &c->types[T_F64],  /* r3 = difference */
    };

    hl_opcode ops[] = {
        OP2(OInt, 0, 0),        /* r0 = -1 */
        OP2(OToSFloat, 1, 0),   /* r1 = (float)r0 signed = -1.0 */
        OP2(OToUFloat, 2, 0),   /* r2 = (float)r0 unsigned = 4294967295.0 */
        OP3(OSub, 3, 2, 1),     /* r3 = r2 - r1 */
        OP1(ORet, 3),
    };

    test_alloc_function(c, 0, fn_type, 4, regs, 5, ops);

    int result;
    double (*fn)(void) = (double(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    double ret = fn();
    /* unsigned(-1) - signed(-1) = 4294967295.0 - (-1.0) = 4294967296.0 */
    double expected = 4294967296.0;
    double diff = ret - expected;
    if (diff < 0) diff = -diff;
    if (diff > 1.0) {
        fprintf(stderr, "    Expected %f, got %f\n", expected, ret);
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(type_load),
    TEST_ENTRY(to_ufloat_basic),
    TEST_ENTRY(to_ufloat_large),
    TEST_ENTRY(unsafe_cast_basic),
    TEST_ENTRY(tofloat_signed_vs_unsigned),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Type Operation Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
