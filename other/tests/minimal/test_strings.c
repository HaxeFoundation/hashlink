/*
 * Test string operations for HashLink AArch64 JIT
 *
 * Tests: OString, OBytes, string handling
 */
#include "test_harness.h"

/*
 * Test: Load a string constant and return its pointer
 *
 * op0: string r0, 0    ; r0 = "hello"
 * op1: ret r0          ; return pointer
 */
TEST(load_string) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Setup string pool */
    c->nstrings = 1;
    c->strings = (char**)malloc(sizeof(char*));
    c->strings[0] = "hello";
    c->strings_lens = (int*)malloc(sizeof(int));
    c->strings_lens[0] = 5;
    c->ustrings = (uchar**)calloc(1, sizeof(uchar*));

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);
    hl_type *regs[] = { &c->types[T_BYTES] };

    hl_opcode ops[] = {
        OP2(OString, 0, 0),   /* op0: r0 = string[0] = "hello" */
        OP1(ORet, 0),         /* op1: return r0 */
    };

    test_alloc_function(c, 0, fn_type, 1, regs, 2, ops);

    int result;
    uchar* (*fn)(void) = (uchar*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    uchar *ret = fn();
    if (ret == NULL) {
        fprintf(stderr, "    Got NULL pointer\n");
        return TEST_FAIL;
    }

    /* Check the string content - uchar is UTF-16, so each element is a 16-bit char */
    /* Note: uchar is 16-bit, so ret[0]='h', ret[1]='e', etc. for ASCII */
    if (ret[0] != 'h' || ret[1] != 'e' || ret[2] != 'l' || ret[3] != 'l' || ret[4] != 'o') {
        fprintf(stderr, "    String content mismatch: got 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
                ret[0], ret[1], ret[2], ret[3], ret[4]);
        fprintf(stderr, "    As chars: '%c' '%c' '%c' '%c' '%c'\n",
                (char)(ret[0] & 0xFF), (char)(ret[1] & 0xFF),
                (char)(ret[2] & 0xFF), (char)(ret[3] & 0xFF), (char)(ret[4] & 0xFF));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Load two different strings
 */
TEST(load_two_strings) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Setup string pool */
    c->nstrings = 2;
    c->strings = (char**)malloc(sizeof(char*) * 2);
    c->strings[0] = "first";
    c->strings[1] = "second";
    c->strings_lens = (int*)malloc(sizeof(int) * 2);
    c->strings_lens[0] = 5;
    c->strings_lens[1] = 6;
    c->ustrings = (uchar**)calloc(2, sizeof(uchar*));

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[T_BYTES], 0, NULL);
    hl_type *regs[] = { &c->types[T_BYTES], &c->types[T_BYTES] };

    hl_opcode ops[] = {
        OP2(OString, 0, 0),   /* op0: r0 = "first" */
        OP2(OString, 1, 1),   /* op1: r1 = "second" */
        OP1(ORet, 1),         /* op2: return r1 ("second") */
    };

    test_alloc_function(c, 0, fn_type, 2, regs, 3, ops);

    int result;
    uchar* (*fn)(void) = (uchar*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    uchar *ret = fn();
    if (ret == NULL) {
        fprintf(stderr, "    Got NULL pointer\n");
        return TEST_FAIL;
    }

    /* Should be "second" */
    if (ret[0] != 's' || ret[1] != 'e' || ret[2] != 'c') {
        fprintf(stderr, "    Expected 'second', got 0x%04x 0x%04x 0x%04x...\n",
                ret[0], ret[1], ret[2]);
        fprintf(stderr, "    As chars: '%c' '%c' '%c'...\n",
                (char)(ret[0] & 0xFF), (char)(ret[1] & 0xFF), (char)(ret[2] & 0xFF));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/*
 * Test: Store string in dynobj and retrieve it
 * This mimics what trace() does - store strings in dynamic object fields
 *
 * r0 = new DynObj
 * r1 = "hello"
 * dynset r0, fieldHash, r1   ; store string
 * r2 = dynget r0, fieldHash  ; retrieve string
 * return r2
 */
TEST(dynobj_string_roundtrip) {
    test_init_runtime();

    hl_code *c = test_alloc_code();
    test_init_base_types(c);

    /* Setup string pool - index 0 = field name "msg", index 1 = value "hello" */
    c->nstrings = 2;
    c->strings = (char**)malloc(sizeof(char*) * 2);
    c->strings[0] = "msg";      /* field name */
    c->strings[1] = "hello";    /* field value */
    c->strings_lens = (int*)malloc(sizeof(int) * 2);
    c->strings_lens[0] = 3;
    c->strings_lens[1] = 5;
    c->ustrings = (uchar**)calloc(2, sizeof(uchar*));

    /* Create HDYNOBJ type */
    if (c->ntypes >= MAX_TYPES) return TEST_FAIL;
    int dynobj_idx = c->ntypes++;
    c->types[dynobj_idx].kind = HDYNOBJ;

    /* Create HDYN type for the result */
    if (c->ntypes >= MAX_TYPES) return TEST_FAIL;
    int dyn_idx = c->ntypes++;
    c->types[dyn_idx].kind = HDYN;

    hl_type *fn_type = test_alloc_fun_type(c, &c->types[dyn_idx], 0, NULL);
    hl_type *regs[] = {
        &c->types[dynobj_idx],   /* r0: dynobj */
        &c->types[T_BYTES],      /* r1: string "hello" */
        &c->types[dyn_idx],      /* r2: retrieved value */
    };

    hl_opcode ops[] = {
        OP1(ONew, 0),             /* r0 = new DynObj */
        OP2(OString, 1, 1),       /* r1 = "hello" (string index 1) */
        OP3(ODynSet, 0, 0, 1),    /* dynset r0, field[0]="msg", r1 */
        OP3(ODynGet, 2, 0, 0),    /* r2 = dynget r0, field[0]="msg" */
        OP1(ORet, 2),             /* return r2 */
    };

    test_alloc_function(c, 0, fn_type, 3, regs, 5, ops);

    int result;
    vdynamic* (*fn)(void) = (vdynamic*(*)(void))test_jit_compile(c, &result);
    if (result != TEST_PASS) return result;

    vdynamic *ret = fn();
    if (ret == NULL) {
        fprintf(stderr, "    Got NULL vdynamic\n");
        return TEST_FAIL;
    }

    /* The returned value should be a string wrapped in vdynamic */
    /* For HBYTES, v.ptr points to the UTF-16 string */
    uchar *str = (uchar*)ret->v.ptr;
    if (str == NULL) {
        fprintf(stderr, "    Got NULL string pointer in vdynamic\n");
        return TEST_FAIL;
    }

    if (str[0] != 'h' || str[1] != 'e' || str[2] != 'l' || str[3] != 'l' || str[4] != 'o') {
        fprintf(stderr, "    String mismatch: got '%c%c%c%c%c'\n",
                (char)(str[0] & 0xFF), (char)(str[1] & 0xFF),
                (char)(str[2] & 0xFF), (char)(str[3] & 0xFF), (char)(str[4] & 0xFF));
        return TEST_FAIL;
    }

    return TEST_PASS;
}

/* Test list */
static test_entry_t tests[] = {
    TEST_ENTRY(load_string),
    TEST_ENTRY(load_two_strings),
    TEST_ENTRY(dynobj_string_roundtrip),
};

int main(int argc, char **argv) {
    printf("HashLink AArch64 JIT - String Operations Tests\n");
    return run_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
