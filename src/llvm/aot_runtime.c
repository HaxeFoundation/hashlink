/*
 * AOT Runtime Helper
 * Provides module loading functions for AOT-compiled binaries
 */
#include <hl.h>
#include <hlmodule.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Global types array - set after module initialization.
 *
 * AOT-compiled code needs access to runtime type information for:
 *   - Object allocation (hl_alloc_obj needs hl_type with initialized obj->rt)
 *   - Type casting and checks
 *   - Virtual method dispatch
 *
 * This global points to the types array from the loaded module, which has
 * all the runtime type info (obj->rt) properly initialized by hl_module_init().
 * AOT code accesses types as &aot_types[type_idx].
 */
hl_type *aot_types = NULL;
int aot_ntypes = 0;

/*
 * Global variables storage - set after module initialization.
 *
 * HashLink globals are stored in a byte buffer (globals_data) with
 * per-global offsets (globals_indexes). hl_module_init() initializes
 * these arrays.
 */
static unsigned char *aot_globals_data = NULL;
static int *aot_globals_indexes = NULL;
static int aot_nglobals = 0;

/*
 * Get a type pointer by index.
 * Used by AOT-compiled code to get properly initialized type pointers.
 */
void *aot_get_type(int idx) {
    if (idx >= 0 && idx < aot_ntypes && aot_types != NULL) {
        return (void *)&aot_types[idx];
    }
    return NULL;
}

/*
 * Get a global variable pointer by index.
 * Returns a pointer to the global's storage location in globals_data.
 */
void *aot_get_global(int idx) {
    if (idx >= 0 && idx < aot_nglobals && aot_globals_data != NULL && aot_globals_indexes != NULL) {
        return (void*)(aot_globals_data + aot_globals_indexes[idx]);
    }
    return NULL;
}

/*
 * JIT stub functions for AOT runtime.
 *
 * AOT-compiled binaries don't need JIT compilation, but hl_module_init() in
 * module.c calls these functions. We provide minimal stubs that:
 *   - Return non-NULL from hl_jit_alloc() so init doesn't fail
 *   - Return success (>=0) from hl_jit_function() for each function
 *   - Return a dummy code pointer from hl_jit_code()
 *
 * The actual function code is already AOT-compiled into the binary, so
 * we don't need to generate anything. The module init will set up
 * functions_ptrs based on the "JIT code" we return, but for AOT we'll
 * override those pointers later or use our own dispatch.
 */
static int dummy_jit_ctx;  /* Non-NULL pointer for hl_jit_alloc */
static char dummy_code[16] = {0};  /* Dummy code buffer */

jit_ctx *hl_jit_alloc(void) { return (jit_ctx *)&dummy_jit_ctx; }
void hl_jit_init(jit_ctx *ctx, hl_module *m) { (void)ctx; (void)m; }
int hl_jit_function(jit_ctx *ctx, hl_module *m, hl_function *f) {
    (void)ctx; (void)m; (void)f;
    return 0;  /* Return offset 0 - all functions point to start of dummy_code */
}
void hl_jit_free(jit_ctx *ctx, h_bool can_reset) { (void)ctx; (void)can_reset; }
void *hl_jit_code(jit_ctx *ctx, hl_module *m, int *size, hl_debug_infos **dbg, hl_module *prev) {
    (void)ctx; (void)m; (void)prev;
    if (size) *size = sizeof(dummy_code);
    if (dbg) *dbg = NULL;
    return dummy_code;  /* Return pointer to dummy code buffer */
}
void hl_jit_reset(jit_ctx *ctx, hl_module *m) { (void)ctx; (void)m; }
void hl_jit_patch_method(void *old_fun, void **new_fun) { (void)old_fun; (void)new_fun; }

/*
 * C2HL trampoline for dynamic function calls (AArch64).
 *
 * This is the equivalent of jit_c2hl in the JIT. It takes:
 *   X0 = function pointer to call
 *   X1 = pointer to register args (X0-X7, then D0-D7)
 *   X2 = pointer to stack args end
 *
 * It loads arguments from the prepared buffers and calls the function.
 */
#if defined(__aarch64__)
__asm__ (
    ".global aot_c2hl_trampoline\n"
    ".type aot_c2hl_trampoline, %function\n"
    "aot_c2hl_trampoline:\n"
    /* Save frame */
    "stp x29, x30, [sp, #-16]!\n"
    "mov x29, sp\n"

    /* Save function pointer and stack args */
    "mov x9, x0\n"      /* X9 = function to call */
    "mov x10, x1\n"     /* X10 = reg args ptr */
    "mov x11, x2\n"     /* X11 = stack args end */

    /* Load integer registers X0-X7 from [X10] */
    "ldp x0, x1, [x10, #0]\n"
    "ldp x2, x3, [x10, #16]\n"
    "ldp x4, x5, [x10, #32]\n"
    "ldp x6, x7, [x10, #48]\n"

    /* Load FP registers D0-D7 from [X10 + 64] */
    "ldp d0, d1, [x10, #64]\n"
    "ldp d2, d3, [x10, #80]\n"
    "ldp d4, d5, [x10, #96]\n"
    "ldp d6, d7, [x10, #112]\n"

    /* Push stack args: X11 points past end, X10+128 points to start */
    /* Stack args are between [X10+128, X11) */
    "add x12, x10, #128\n"  /* X12 = start of stack args */
    "1:\n"
    "cmp x12, x11\n"
    "b.ge 2f\n"
    "ldr x13, [x11, #-8]!\n"
    "str x13, [sp, #-16]!\n"
    "b 1b\n"
    "2:\n"

    /* Call the function */
    "blr x9\n"

    /* Restore frame and return */
    "mov sp, x29\n"
    "ldp x29, x30, [sp], #16\n"
    "ret\n"
);
extern void *aot_c2hl_trampoline(void *func, void *regs, void *stack_end);
#else
/* Stub for non-AArch64 platforms */
static void *aot_c2hl_trampoline(void *f, void *regs, void *stack) {
    (void)f; (void)regs; (void)stack;
    hl_error("AOT C2HL trampoline not implemented for this platform");
    return NULL;
}
#endif

/* Number of register arguments (X0-X7 for ints, D0-D7 for floats) */
#define CALL_NREGS 8
#define MAX_ARGS 64

/*
 * Select which register to use for an argument (C2HL direction).
 * Returns register index (0-7 for int, 8-15 for FP) or -1 for stack.
 */
static int select_call_reg_c2hl(int *nextCpu, int *nextFpu, hl_type *t) {
    switch (t->kind) {
    case HF32:
    case HF64:
        if (*nextFpu < CALL_NREGS) return CALL_NREGS + (*nextFpu)++;
        return -1;
    default:
        if (*nextCpu < CALL_NREGS) return (*nextCpu)++;
        return -1;
    }
}

/*
 * Get stack size for a type.
 */
static int stack_size_c2hl(hl_type *t) {
    switch (t->kind) {
    case HUI8:
    case HBOOL:
        return 1;
    case HUI16:
        return 2;
    case HI32:
    case HF32:
        return 4;
    default:
        return 8;
    }
}

/*
 * Callback for dynamic function calls (C -> HL direction).
 * Called by hl_dyn_call/hl_call_method to invoke AOT functions dynamically.
 */
static void *aot_callback_c2hl(void **f, hl_type *t, void **args, vdynamic *ret) {
    unsigned char stack[MAX_ARGS * 16];
    int nextCpu = 0, nextFpu = 0;
    int mappedRegs[MAX_ARGS];

    memset(stack, 0, sizeof(stack));

    if (t->fun->nargs > MAX_ARGS)
        hl_error("Too many arguments for dynamic call");

    /* First pass: determine register assignments and stack size */
    int i, size = 0;
    for (i = 0; i < t->fun->nargs; i++) {
        hl_type *at = t->fun->args[i];
        int creg = select_call_reg_c2hl(&nextCpu, &nextFpu, at);
        mappedRegs[i] = creg;
        if (creg < 0) {
            int tsize = stack_size_c2hl(at);
            if (tsize < 8) tsize = 8;
            size += tsize;
        }
    }

    /* Align stack size to 16 bytes */
    int pad = (-size) & 15;
    size += pad;

    /* Second pass: copy arguments to appropriate locations */
    /* stack layout: [0..64) = X0-X7, [64..128) = D0-D7, [128..) = stack args */
    /*
     * args[i] points to a vdynamic struct: { hl_type *t; union v; }
     * The actual value is in the 'v' union at offset 8, not at offset 0.
     * We need to read from the correct offset based on the expected type.
     */
    int pos = 128;  /* Stack args start after register save area */
    for (i = 0; i < t->fun->nargs; i++) {
        hl_type *at = t->fun->args[i];
        vdynamic *dyn = (vdynamic*)args[i];
        int creg = mappedRegs[i];
        void *store;

        if (creg >= 0) {
            store = stack + creg * 8;
        } else {
            store = stack + pos;
            pos += 8;
        }

        switch (at->kind) {
        case HUI8:
        case HBOOL:
            *(int*)store = dyn->v.ui8;
            break;
        case HUI16:
            *(int*)store = dyn->v.ui16;
            break;
        case HI32:
            *(int*)store = dyn->v.i;
            break;
        case HI64:
            *(int_val*)store = dyn->v.i64;
            break;
        case HF32:
            *(float*)store = dyn->v.f;
            break;
        case HF64:
            *(double*)store = dyn->v.d;
            break;
        default:
            *(void**)store = dyn->v.ptr;
            break;
        }
    }

    /* Call through trampoline */
    /* The casts for float/double are intentional - the trampoline returns
     * in D0 for FP types, and the cast tells the compiler to read from D0. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbad-function-cast"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    switch (t->fun->ret->kind) {
    case HVOID:
        aot_c2hl_trampoline(*f, stack, stack + pos);
        return NULL;
    case HUI8:
    case HBOOL:
    case HUI16:
    case HI32:
        ret->v.i = (int)(int_val)aot_c2hl_trampoline(*f, stack, stack + pos);
        return &ret->v.i;
    case HI64:
        ret->v.i64 = (int_val)aot_c2hl_trampoline(*f, stack, stack + pos);
        return &ret->v.i64;
    case HF32: {
        float (*fp)(void*,void*,void*) = (float(*)(void*,void*,void*))(void*)aot_c2hl_trampoline;
        ret->v.f = fp(*f, stack, stack + pos);
        return &ret->v.f;
    }
    case HF64: {
        double (*dp)(void*,void*,void*) = (double(*)(void*,void*,void*))(void*)aot_c2hl_trampoline;
        ret->v.d = dp(*f, stack, stack + pos);
        return &ret->v.d;
    }
    default:
        return aot_c2hl_trampoline(*f, stack, stack + pos);
    }
#pragma GCC diagnostic pop
}

/*
 * Get wrapper function for a given function index.
 * For AOT, we return NULL since we don't support HL->C wrapping yet.
 */
static void *aot_get_wrapper(int findex) {
    (void)findex;
    return NULL;
}

/* Export wrapper functions that can be linked */
static hl_code *aot_code_read(const char *path, char **error_msg) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (error_msg) *error_msg = "Cannot open file";
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    int size = (int)ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *data = (unsigned char *)malloc(size);
    if (fread(data, 1, size, f) != (size_t)size) {
        free(data);
        fclose(f);
        if (error_msg) *error_msg = "Failed to read file";
        return NULL;
    }
    fclose(f);
    hl_code *code = hl_code_read(data, size, error_msg);
    free(data);
    return code;
}

/*
 * AOT function pointer table - generated by LLVM codegen.
 * Maps findex -> function pointer for HL functions.
 * Native function entries are NULL (resolved by hl_module_init).
 */
extern void *aot_function_table[];
extern int aot_function_count;

/*
 * Initialize module from embedded bytecode data.
 *
 * AOT-compiled binaries embed the .hl bytecode directly in the executable
 * as a global byte array. This allows the binary to be fully standalone
 * without needing the original .hl file at runtime.
 *
 * The bytecode is needed at runtime because it contains type metadata
 * (hl_type structures, vtables, field layouts) that the runtime uses for:
 *   - Object allocation (needs type size and layout)
 *   - Type casting and runtime type checks
 *   - Dynamic dispatch through vtables
 *   - Field access offset calculations
 *
 * While the function *code* is AOT-compiled to native instructions,
 * the type *metadata* is loaded from the embedded bytecode and used
 * to initialize the runtime type system.
 */
int aot_init_module_data(const unsigned char *data, int size) {
    char *error_msg = NULL;
    hl_code *code = hl_code_read(data, size, &error_msg);
    if (!code) {
        fprintf(stderr, "Failed to read embedded HL code: %s\n", error_msg ? error_msg : "unknown error");
        return 0;
    }
    hl_module *m = hl_module_alloc(code);
    if (!m) {
        fprintf(stderr, "Failed to allocate module\n");
        return 0;
    }
    /* hl_module_init(module, hot_reload, vtune_later) */
    if (!hl_module_init(m, 0, 0)) {
        fprintf(stderr, "Failed to initialize module\n");
        return 0;
    }

    /*
     * Export the types and globals arrays for AOT code to access.
     * After hl_module_init(), all types have their runtime info (obj->rt)
     * properly initialized, and globals are allocated/initialized.
     */
    aot_types = code->types;
    aot_ntypes = code->ntypes;

    aot_globals_data = m->globals_data;
    aot_globals_indexes = m->globals_indexes;
    aot_nglobals = code->nglobals;

    /*
     * Patch functions_ptrs with AOT-compiled function addresses.
     *
     * After hl_module_init(), m->functions_ptrs contains:
     *   - For natives: properly resolved function pointers from shared libraries
     *   - For HL functions: pointers to dummy_code (useless)
     *
     * We patch the HL function entries with the actual AOT-compiled addresses
     * from aot_function_table. Native entries (NULL in the table) are left alone.
     *
     * This is critical for closures and method dispatch to work correctly.
     */
    for (int i = 0; i < aot_function_count; i++) {
        if (aot_function_table[i] != NULL) {
            m->functions_ptrs[i] = aot_function_table[i];
        }
    }

    /*
     * Set up callbacks for dynamic function calls.
     *
     * hl_dyn_call and hl_call_method use hlc_static_call to invoke functions
     * dynamically. We provide our AOT-compatible callback that uses a trampoline
     * to call AOT functions with the correct calling convention.
     *
     * The flags=1 tells the runtime to use cl->fun directly (not &cl->fun).
     */
    hl_setup_callbacks2(aot_callback_c2hl, aot_get_wrapper, 1);

    return 1;
}
