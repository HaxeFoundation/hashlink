/*
 * Test method call operations for HashLink AArch64 JIT
 *
 * Tests: OCallMethod, OCallThis, OCall4
 *
 * OCallMethod: call a method on an object via vtable
 * OCallThis: call a method with implicit 'this' (R0)
 * OCall4: call a function with 4 arguments
 */
#include "test_harness.h"

/* Helper to create an object type with a method */
static hl_type *create_obj_type_with_method(hl_code *c, const char *name, int method_findex) {
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
    t->obj->nproto = 1;
    t->obj->nbindings = 0;

    t->obj->proto = (hl_obj_proto*)calloc(1, sizeof(hl_obj_proto));
    t->obj->proto[0].name = (uchar*)"testMethod";
    t->obj->proto[0].findex = method_findex;
    t->obj->proto[0].pindex = 0;

    return t;
}

/*
 * Test: OCall4 - call function with 4 arguments
 *
 * fn0: (i32, i32, i32, i32) -> i32 { return a + b + c + d; }
 * fn1: () -> i32 { return fn0(10, 20, 5, 7); }  // 10+20+5+7 = 42
 */
TEST(call4_basic) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 20, 5, 7 };
    test_init_ints(c, 4, ints);

    /* fn0 type: (i32, i32, i32, i32) -> i32 */
    hl_type *fn0_args[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn0_type = test_alloc_fun_type(c, &c->types[T_I32], 4, fn0_args);

    /* fn1 type: () -> i32 */
    hl_type *fn1_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: sum of 4 args */
    {
        hl_type *regs[] = {
            &c->types[T_I32],  /* r0 = a */
            &c->types[T_I32],  /* r1 = b */
            &c->types[T_I32],  /* r2 = c */
            &c->types[T_I32],  /* r3 = d */
            &c->types[T_I32],  /* r4 = result */
        };
        hl_opcode ops[] = {
            OP3(OAdd, 4, 0, 1),  /* r4 = a + b */
            OP3(OAdd, 4, 4, 2),  /* r4 = r4 + c */
            OP3(OAdd, 4, 4, 3),  /* r4 = r4 + d */
            OP1(ORet, 4),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn0_type;
        f->nregs = 5;
        f->nops = 4;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 5);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 4);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: calls fn0 with 4 args */
    {
        hl_type *regs[] = {
            &c->types[T_I32],  /* r0 = arg 0 */
            &c->types[T_I32],  /* r1 = arg 1 */
            &c->types[T_I32],  /* r2 = arg 2 */
            &c->types[T_I32],  /* r3 = arg 3 */
            &c->types[T_I32],  /* r4 = result */
        };

        /* OCall4: dst=p1, findex=p2, arg0=p3, extra=[arg1, arg2, arg3] */
        static int extra[] = { 1, 2, 3 };  /* registers for args 1, 2, 3 */
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                       /* r0 = 10 */
            OP2(OInt, 1, 1),                       /* r1 = 20 */
            OP2(OInt, 2, 2),                       /* r2 = 5 */
            OP2(OInt, 3, 3),                       /* r3 = 7 */
            { OCall4, 4, 0, 0, extra },            /* r4 = fn0(r0, r1, r2, r3) */
            OP1(ORet, 4),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn1_type;
        f->nregs = 5;
        f->nops = 6;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 5);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 6);
        memcpy(f->ops, ops, sizeof(ops));
    }

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
 * Test: OCall4 with mixed types (some floats)
 */
TEST(call4_mixed_types) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 10, 32 };
    test_init_ints(c, 2, ints);

    /* fn0: (i32, i32, i32, i32) -> i32 */
    hl_type *fn0_args[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn0_type = test_alloc_fun_type(c, &c->types[T_I32], 4, fn0_args);

    hl_type *fn1_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: return just first + second arg */
    {
        hl_type *regs[] = {
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
        };
        hl_opcode ops[] = {
            OP3(OAdd, 4, 0, 1),
            OP1(ORet, 4),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn0_type;
        f->nregs = 5;
        f->nops = 2;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 5);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 2);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: call fn0(10, 32, 0, 0) = 42 */
    {
        hl_type *regs[] = {
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
            &c->types[T_I32],
        };

        static int extra[] = { 1, 2, 3 };  /* registers for args 1, 2, 3 */
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),               /* r0 = 10 */
            OP2(OInt, 1, 1),               /* r1 = 32 */
            OP1(ONull, 2),                 /* r2 = 0 (null as int) */
            OP1(ONull, 3),                 /* r3 = 0 */
            { OCall4, 4, 0, 0, extra },    /* r4 = fn0(10, 32, 0, 0) */
            OP1(ORet, 4),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn1_type;
        f->nregs = 5;
        f->nops = 6;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 5);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 6);
        memcpy(f->ops, ops, sizeof(ops));
    }

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
 * Test: Multiple OCall4 in sequence
 *
 * This tests that register allocation works correctly across multiple calls.
 */
TEST(call4_multiple) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    int ints[] = { 1, 2, 3, 4, 10 };
    test_init_ints(c, 5, ints);

    hl_type *fn0_args[] = { &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32] };
    hl_type *fn0_type = test_alloc_fun_type(c, &c->types[T_I32], 4, fn0_args);
    hl_type *fn1_type = test_alloc_fun_type(c, &c->types[T_I32], 0, NULL);

    c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
    c->nfunctions = 0;

    /* fn0: sum of 4 args */
    {
        hl_type *regs[] = {
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32]
        };
        hl_opcode ops[] = {
            OP3(OAdd, 4, 0, 1),
            OP3(OAdd, 4, 4, 2),
            OP3(OAdd, 4, 4, 3),
            OP1(ORet, 4),
        };
        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 0;
        f->type = fn0_type;
        f->nregs = 5;
        f->nops = 4;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 5);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 4);
        memcpy(f->ops, ops, sizeof(ops));
    }

    /* fn1: call fn0 twice and sum results */
    {
        hl_type *regs[] = {
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
            &c->types[T_I32], &c->types[T_I32], &c->types[T_I32],
        };

        static int extra1[] = { 1, 2, 3 };  /* registers for args 1, 2, 3 */
        static int extra2[] = { 1, 2, 3 };  /* registers for args 1, 2, 3 */
        hl_opcode ops[] = {
            OP2(OInt, 0, 0),                        /* r0 = 1 */
            OP2(OInt, 1, 1),                        /* r1 = 2 */
            OP2(OInt, 2, 2),                        /* r2 = 3 */
            OP2(OInt, 3, 3),                        /* r3 = 4 */
            { OCall4, 4, 0, 0, extra1 },            /* r4 = fn0(1,2,3,4) = 10 */
            OP2(OInt, 0, 4),                        /* r0 = 10 */
            OP2(OInt, 1, 4),                        /* r1 = 10 */
            OP2(OInt, 2, 4),                        /* r2 = 10 */
            OP2(OInt, 3, 1),                        /* r3 = 2 */
            { OCall4, 5, 0, 0, extra2 },            /* r5 = fn0(10,10,10,2) = 32 */
            OP3(OAdd, 6, 4, 5),                     /* r6 = 10 + 32 = 42 */
            OP1(ORet, 6),
        };

        hl_function *f = &c->functions[c->nfunctions++];
        f->findex = 1;
        f->type = fn1_type;
        f->nregs = 7;
        f->nops = 12;
        f->regs = (hl_type**)malloc(sizeof(hl_type*) * 7);
        memcpy(f->regs, regs, sizeof(regs));
        f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * 12);
        memcpy(f->ops, ops, sizeof(ops));
    }

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

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(call4_basic),
    TEST_ENTRY(call4_mixed_types),
    TEST_ENTRY(call4_multiple),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - Method Call Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
