/*
 * Minimal JIT Test Harness for HashLink AArch64 JIT
 *
 * This provides helpers to construct hl_code structures directly in memory,
 * bypassing the bytecode file format. This allows testing individual opcodes
 * without pulling in the entire Haxe stdlib.
 */
#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include <hl.h>
#include <hlmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test result codes */
#define TEST_PASS 0
#define TEST_FAIL 1
#define TEST_SKIP 2

/* Colors for output */
#define GREEN "\033[32m"
#define RED   "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

/* Helper to create a minimal hl_code structure */
static hl_code *test_alloc_code(void) {
    hl_code *c = (hl_code*)calloc(1, sizeof(hl_code));
    c->version = 5;
    hl_alloc_init(&c->alloc);
    hl_alloc_init(&c->falloc);
    return c;
}

/* Predefined types - indices into types array */
#define T_VOID   0
#define T_I32    1
#define T_I64    2
#define T_F32    3
#define T_F64    4
#define T_BOOL   5
#define T_BYTES  6
#define T_TYPE   7  /* HTYPE - for type pointers, size = pointer size */

/* Base types array - common types needed for most tests */
#define BASE_TYPES_COUNT 8
#define MAX_TYPES 32  /* Pre-allocate space for additional types */

static void test_init_base_types(hl_code *c) {
    /* Pre-allocate space for base types + function types */
    c->types = (hl_type*)calloc(MAX_TYPES, sizeof(hl_type));
    c->ntypes = BASE_TYPES_COUNT;
    c->types[T_VOID].kind = HVOID;
    c->types[T_I32].kind = HI32;
    c->types[T_I64].kind = HI64;
    c->types[T_F32].kind = HF32;
    c->types[T_F64].kind = HF64;
    c->types[T_BOOL].kind = HBOOL;
    c->types[T_BYTES].kind = HBYTES;
    c->types[T_TYPE].kind = HTYPE;
}

/* Allocate a function type: fun(args...) -> ret */
static hl_type *test_alloc_fun_type(hl_code *c, hl_type *ret, int nargs, hl_type **args) {
    if (c->ntypes >= MAX_TYPES) {
        fprintf(stderr, "Too many types (max %d)\n", MAX_TYPES);
        return NULL;
    }

    int idx = c->ntypes++;
    hl_type *t = &c->types[idx];
    memset(t, 0, sizeof(hl_type));

    t->kind = HFUN;
    t->fun = (hl_type_fun*)calloc(1, sizeof(hl_type_fun));
    t->fun->ret = ret;
    t->fun->nargs = nargs;
    if (nargs > 0) {
        t->fun->args = (hl_type**)malloc(sizeof(hl_type*) * nargs);
        memcpy(t->fun->args, args, sizeof(hl_type*) * nargs);
    }
    return t;
}

/* Max functions for pre-allocation */
#define MAX_FUNCTIONS 16

/* Allocate a function */
static hl_function *test_alloc_function(hl_code *c, int findex, hl_type *type,
                                        int nregs, hl_type **regs,
                                        int nops, hl_opcode *ops) {
    if (c->functions == NULL) {
        c->functions = (hl_function*)calloc(MAX_FUNCTIONS, sizeof(hl_function));
        c->nfunctions = 0;
    }

    if (c->nfunctions >= MAX_FUNCTIONS) {
        fprintf(stderr, "Too many functions (max %d)\n", MAX_FUNCTIONS);
        return NULL;
    }

    hl_function *f = &c->functions[c->nfunctions++];
    f->findex = findex;
    f->type = type;
    f->nregs = nregs;
    f->nops = nops;

    f->regs = (hl_type**)malloc(sizeof(hl_type*) * nregs);
    memcpy(f->regs, regs, sizeof(hl_type*) * nregs);

    f->ops = (hl_opcode*)malloc(sizeof(hl_opcode) * nops);
    memcpy(f->ops, ops, sizeof(hl_opcode) * nops);

    /* No debug info for minimal tests */
    f->debug = NULL;
    f->obj = NULL;
    f->field.ref = NULL;
    f->ref = 0;

    return f;
}

/* Helper macro for creating opcodes */
#define OP0(opcode)             {opcode, 0, 0, 0, NULL}
#define OP1(opcode, a)          {opcode, a, 0, 0, NULL}
#define OP2(opcode, a, b)       {opcode, a, b, 0, NULL}
#define OP3(opcode, a, b, c)    {opcode, a, b, c, NULL}

/*
 * For OCall2, the extra field stores the 4th parameter as an int cast to pointer.
 * Usage: OP4_CALL2(OCall2, dst, findex, arg1, arg2)
 */
#define OP4_CALL2(opcode, a, b, c, d)  {opcode, a, b, c, (int*)(intptr_t)(d)}

/* Initialize integers pool */
static void test_init_ints(hl_code *c, int count, int *values) {
    c->nints = count;
    c->ints = (int*)malloc(sizeof(int) * count);
    memcpy(c->ints, values, sizeof(int) * count);
}

/* Initialize floats pool */
static void test_init_floats(hl_code *c, int count, double *values) {
    c->nfloats = count;
    c->floats = (double*)malloc(sizeof(double) * count);
    memcpy(c->floats, values, sizeof(double) * count);
}

/* Native function pointer registry
 * Since hl_native doesn't have a ptr field, we track them separately */
#define MAX_NATIVE_PTRS 16
static struct {
    int findex;
    void *ptr;
} g_native_ptrs[MAX_NATIVE_PTRS];
static int g_native_ptr_count = 0;

static void test_register_native_ptr(int findex, void *ptr) {
    if (g_native_ptr_count >= MAX_NATIVE_PTRS) {
        fprintf(stderr, "Too many native functions (max %d)\n", MAX_NATIVE_PTRS);
        return;
    }
    g_native_ptrs[g_native_ptr_count].findex = findex;
    g_native_ptrs[g_native_ptr_count].ptr = ptr;
    g_native_ptr_count++;
}

static void test_clear_native_ptrs(void) {
    g_native_ptr_count = 0;
}

/* Add a native function to the code structure */
static void test_add_native(hl_code *c, int findex, const char *lib, const char *name,
                            hl_type *fn_type, void *func_ptr) {
    if (c->natives == NULL) {
        c->natives = (hl_native*)calloc(MAX_NATIVE_PTRS, sizeof(hl_native));
        c->nnatives = 0;
    }

    hl_native *n = &c->natives[c->nnatives++];
    n->findex = findex;
    n->lib = lib;
    n->name = name;
    n->t = fn_type;

    /* Register the function pointer separately */
    test_register_native_ptr(findex, func_ptr);
}

/* Build and JIT compile the code, returns the function pointer */
typedef void *(*jit_func_t)(void);

static void *test_jit_compile(hl_code *c, int *out_result) {
    /* Set entrypoint if not set */
    if (c->nfunctions > 0 && c->entrypoint == 0) {
        c->entrypoint = c->functions[0].findex;
    }

    /* Ensure we have globals array (can be empty) */
    if (c->globals == NULL) {
        c->nglobals = 0;
        c->globals = NULL;
    }

    /* Natives are optional - keep if set */
    if (c->natives == NULL) {
        c->nnatives = 0;
    }

    /* No constants */
    c->nconstants = 0;
    c->constants = NULL;

    /* No strings/bytes for now */
    if (c->strings == NULL) {
        c->nstrings = 0;
        c->strings = NULL;
        c->strings_lens = NULL;
        c->ustrings = NULL;
    }
    c->nbytes = 0;
    c->bytes = NULL;
    c->bytes_pos = NULL;

    /* No debug */
    c->hasdebug = false;
    c->ndebugfiles = 0;
    c->debugfiles = NULL;
    c->debugfiles_lens = NULL;

    /* Allocate module */
    hl_module *m = hl_module_alloc(c);
    if (m == NULL) {
        fprintf(stderr, "Failed to allocate module\n");
        *out_result = TEST_FAIL;
        return NULL;
    }

    /* Setup module context for object types (needed for hl_get_obj_rt allocator) */
    for (int i = 0; i < c->ntypes; i++) {
        if (c->types[i].kind == HOBJ && c->types[i].obj != NULL) {
            c->types[i].obj->m = &m->ctx;
        }
    }

    /* Setup function indexes */
    for (int i = 0; i < c->nfunctions; i++) {
        hl_function *f = c->functions + i;
        m->functions_indexes[f->findex] = i;
        m->ctx.functions_types[f->findex] = f->type;
    }

    /* Setup native function indexes and pointers */
    for (int i = 0; i < c->nnatives; i++) {
        hl_native *n = &c->natives[i];
        m->functions_indexes[n->findex] = i + c->nfunctions;  /* natives come after functions */
        m->ctx.functions_types[n->findex] = n->t;
    }
    for (int i = 0; i < g_native_ptr_count; i++) {
        m->functions_ptrs[g_native_ptrs[i].findex] = g_native_ptrs[i].ptr;
    }
    test_clear_native_ptrs();  /* Reset for next test */

    /* JIT compile */
    jit_ctx *ctx = hl_jit_alloc();
    if (ctx == NULL) {
        fprintf(stderr, "Failed to allocate JIT context\n");
        hl_module_free(m);
        *out_result = TEST_FAIL;
        return NULL;
    }

    hl_jit_init(ctx, m);

    for (int i = 0; i < c->nfunctions; i++) {
        hl_function *f = c->functions + i;
        int fpos = hl_jit_function(ctx, m, f);
        if (fpos < 0) {
            fprintf(stderr, "Failed to JIT function %d\n", f->findex);
            hl_jit_free(ctx, false);
            hl_module_free(m);
            *out_result = TEST_FAIL;
            return NULL;
        }
        m->functions_ptrs[f->findex] = (void*)(intptr_t)fpos;
    }

    int codesize;
    hl_debug_infos *debug_info = NULL;
    void *jit_code = hl_jit_code(ctx, m, &codesize, &debug_info, NULL);

    if (jit_code == NULL) {
        fprintf(stderr, "Failed to finalize JIT code\n");
        hl_jit_free(ctx, false);
        hl_module_free(m);
        *out_result = TEST_FAIL;
        return NULL;
    }

    /* Fix up function pointers */
    for (int i = 0; i < c->nfunctions; i++) {
        hl_function *f = c->functions + i;
        m->functions_ptrs[f->findex] = (unsigned char*)jit_code + (intptr_t)m->functions_ptrs[f->findex];
    }

    m->jit_code = jit_code;
    m->codesize = codesize;

    hl_jit_free(ctx, false);

    *out_result = TEST_PASS;

    /* Return pointer to entry function */
    return m->functions_ptrs[c->entrypoint];
}

/* Test runner infrastructure */
typedef int (*test_func_t)(void);

typedef struct {
    const char *name;
    test_func_t func;
} test_entry_t;

static int run_tests(test_entry_t *tests, int count) {
    int passed = 0, failed = 0, skipped = 0;

    printf("\n=== Running %d tests ===\n\n", count);

    for (int i = 0; i < count; i++) {
        printf("  [%d/%d] %s ... ", i + 1, count, tests[i].name);
        fflush(stdout);

        int result = tests[i].func();

        switch (result) {
            case TEST_PASS:
                printf(GREEN "PASS" RESET "\n");
                passed++;
                break;
            case TEST_FAIL:
                printf(RED "FAIL" RESET "\n");
                failed++;
                break;
            case TEST_SKIP:
                printf(YELLOW "SKIP" RESET "\n");
                skipped++;
                break;
        }
    }

    printf("\n=== Results: %d passed, %d failed, %d skipped ===\n\n",
           passed, failed, skipped);

    return failed > 0 ? 1 : 0;
}

/* Convenience macro to define a test */
#define TEST(name) static int test_##name(void)
#define TEST_ENTRY(name) { #name, test_##name }

/* Stub functions for exception handling */
static uchar *test_resolve_symbol(void *addr, uchar *out, int *outSize) {
    (void)addr; (void)out; (void)outSize;
    return NULL;  /* No symbol resolution in minimal tests */
}

static int test_capture_stack(void **stack, int size) {
    (void)stack; (void)size;
    return 0;  /* No stack capture in minimal tests */
}

/* Initialize HL runtime - call once at start */
static void test_init_runtime(void) {
    static int initialized = 0;
    if (!initialized) {
        hl_global_init();
        static int ctx;
        hl_register_thread(&ctx);
        /* Set up exception handling - REQUIRED for hl_throw to work! */
        hl_setup.resolve_symbol = test_resolve_symbol;
        hl_setup.capture_stack = test_capture_stack;
        initialized = 1;
    }
}

#endif /* TEST_HARNESS_H */
