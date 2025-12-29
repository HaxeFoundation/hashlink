/*
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * hl2llvm - HashLink bytecode to LLVM IR AOT compiler
 *
 * Usage: hl2llvm [options] input.hl -o output
 *
 * Options:
 *   -o <file>      Output file (required)
 *   --emit-llvm    Output LLVM IR text (.ll)
 *   --emit-bc      Output LLVM bitcode (.bc)
 *   --emit-asm     Output native assembly (.s)
 *   --emit-obj     Output object file (.o) [default]
 *   -O0            No optimization
 *   -O1            Light optimization
 *   -O2            Default optimization [default]
 *   -O3            Aggressive optimization
 *   -g             Emit debug info
 *   -v             Verbose output
 *   --help         Show this help
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "llvm_codegen.h"

static void print_usage(const char *prog) {
    printf("HashLink bytecode to LLVM IR AOT compiler\n\n");
    printf("Usage: %s [options] input.hl -o output\n\n", prog);
    printf("Options:\n");
    printf("  -o <file>      Output file (required)\n");
    printf("  --emit-llvm    Output LLVM IR text (.ll)\n");
    printf("  --emit-bc      Output LLVM bitcode (.bc)\n");
    printf("  --emit-asm     Output native assembly (.s)\n");
    printf("  --emit-obj     Output object file (.o) [default]\n");
    printf("  -O0            No optimization\n");
    printf("  -O1            Light optimization\n");
    printf("  -O2            Default optimization [default]\n");
    printf("  -O3            Aggressive optimization\n");
    printf("  -g             Emit debug info\n");
    printf("  -v             Verbose output\n");
    printf("  --help         Show this help\n");
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = NULL;
    llvm_output_format format = LLVM_OUTPUT_OBJECT;
    llvm_opt_level opt_level = LLVM_OPT_DEFAULT;
    bool emit_debug = false;
    bool verbose = false;

    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
            output_file = argv[++i];
        } else if (strcmp(argv[i], "--emit-llvm") == 0) {
            format = LLVM_OUTPUT_LLVM_IR;
        } else if (strcmp(argv[i], "--emit-bc") == 0) {
            format = LLVM_OUTPUT_BITCODE;
        } else if (strcmp(argv[i], "--emit-asm") == 0) {
            format = LLVM_OUTPUT_ASSEMBLY;
        } else if (strcmp(argv[i], "--emit-obj") == 0) {
            format = LLVM_OUTPUT_OBJECT;
        } else if (strcmp(argv[i], "-O0") == 0) {
            opt_level = LLVM_OPT_NONE;
        } else if (strcmp(argv[i], "-O1") == 0) {
            opt_level = LLVM_OPT_LESS;
        } else if (strcmp(argv[i], "-O2") == 0) {
            opt_level = LLVM_OPT_DEFAULT;
        } else if (strcmp(argv[i], "-O3") == 0) {
            opt_level = LLVM_OPT_AGGRESSIVE;
        } else if (strcmp(argv[i], "-g") == 0) {
            emit_debug = true;
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        } else {
            if (input_file != NULL) {
                fprintf(stderr, "Error: Multiple input files specified\n");
                return 1;
            }
            input_file = argv[i];
        }
    }

    if (input_file == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    if (output_file == NULL) {
        fprintf(stderr, "Error: No output file specified (use -o)\n");
        return 1;
    }

    /* Initialize HashLink runtime (needed for hl_code_read) */
    hl_global_init();

    /* Load the HashLink bytecode file */
    if (verbose) {
        printf("Loading %s...\n", input_file);
    }

    FILE *f = fopen(input_file, "rb");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    int size = (int)ftell(f);
    fseek(f, 0, SEEK_SET);

    char *fdata = (char *)malloc(size);
    if (!fdata) {
        fprintf(stderr, "Error: Out of memory\n");
        fclose(f);
        return 1;
    }

    if (fread(fdata, 1, size, f) != (size_t)size) {
        fprintf(stderr, "Error: Failed to read %s\n", input_file);
        free(fdata);
        fclose(f);
        return 1;
    }
    fclose(f);

    char *error_msg = NULL;
    hl_code *code = hl_code_read((unsigned char *)fdata, size, &error_msg);
    /* Keep fdata around - we'll embed it in the binary */

    if (code == NULL) {
        free(fdata);
        fprintf(stderr, "Error: Failed to load %s: %s\n", input_file,
                error_msg ? error_msg : "unknown error");
        return 1;
    }

    if (verbose) {
        printf("Loaded bytecode: %d functions, %d types, %d globals\n",
               code->nfunctions, code->ntypes, code->nglobals);
    }

    /* Create minimal module context - needed for hl_get_obj_rt() during compilation.
     * We don't use hl_module_alloc() because it's not exported from libhl and
     * pulls in JIT dependencies we don't need. */
    static hl_module_context module_ctx;
    memset(&module_ctx, 0, sizeof(module_ctx));
    hl_alloc_init(&module_ctx.alloc);

    /* Set up functions_types array - needed by hl_get_obj_rt() for method lookups */
    int total_functions = code->nfunctions + code->nnatives;
    module_ctx.functions_types = (hl_type **)malloc(sizeof(hl_type *) * total_functions);
    memset(module_ctx.functions_types, 0, sizeof(hl_type *) * total_functions);
    for (int i = 0; i < code->nfunctions; i++) {
        hl_function *fn = &code->functions[i];
        module_ctx.functions_types[fn->findex] = fn->type;
    }
    for (int i = 0; i < code->nnatives; i++) {
        hl_native *n = &code->natives[i];
        module_ctx.functions_types[n->findex] = n->t;
    }

    /* Set module context on object types so hl_get_obj_rt() can compute field offsets */
    for (int i = 0; i < code->ntypes; i++) {
        hl_type *t = &code->types[i];
        if ((t->kind == HOBJ || t->kind == HSTRUCT) && t->obj) {
            t->obj->m = &module_ctx;
        }
    }

    /* Create LLVM context */
    llvm_ctx *ctx = llvm_create_context();
    if (ctx == NULL) {
        fprintf(stderr, "Error: Failed to create LLVM context\n");
        hl_code_free(code);
        return 1;
    }

    ctx->opt_level = opt_level;
    ctx->emit_debug_info = emit_debug;
    ctx->bytecode_data = (unsigned char *)fdata;
    ctx->bytecode_size = size;

    /* Initialize the module */
    if (verbose) {
        printf("Initializing LLVM module...\n");
    }

    if (!llvm_init_module(ctx, code, input_file)) {
        fprintf(stderr, "Error: Failed to initialize module: %s\n",
                ctx->error_msg ? ctx->error_msg : "unknown error");
        llvm_destroy_context(ctx);
        hl_code_free(code);
        return 1;
    }

    /* Compile all functions */
    if (verbose) {
        printf("Compiling %d functions...\n", code->nfunctions);
    }

    for (int i = 0; i < code->nfunctions; i++) {
        hl_function *f = &code->functions[i];
        if (verbose && (i % 100 == 0 || i == code->nfunctions - 1)) {
            printf("  Compiling function %d/%d...\n", i + 1, code->nfunctions);
        }
        if (!llvm_compile_function(ctx, f)) {
            fprintf(stderr, "Error: Failed to compile function %d: %s\n",
                    i, ctx->error_msg ? ctx->error_msg : "unknown error");
            llvm_destroy_context(ctx);
            hl_code_free(code);
            return 1;
        }
    }

    /* Generate entry point */
    if (verbose) {
        printf("Generating entry point...\n");
    }

    if (!llvm_generate_entry_point(ctx, code->entrypoint)) {
        fprintf(stderr, "Error: Failed to generate entry point: %s\n",
                ctx->error_msg ? ctx->error_msg : "unknown error");
        llvm_destroy_context(ctx);
        hl_code_free(code);
        return 1;
    }

    /* Finalize the module */
    if (!llvm_finalize_module(ctx)) {
        fprintf(stderr, "Error: Failed to finalize module: %s\n",
                ctx->error_msg ? ctx->error_msg : "unknown error");
        llvm_destroy_context(ctx);
        hl_code_free(code);
        return 1;
    }

    /* Verify the module */
    if (verbose) {
        printf("Verifying module...\n");
    }

    if (!llvm_verify(ctx)) {
        fprintf(stderr, "Error: Module verification failed: %s\n",
                ctx->error_msg ? ctx->error_msg : "unknown error");
        llvm_destroy_context(ctx);
        hl_code_free(code);
        return 1;
    }

    /* Optimize if requested */
    if (opt_level > LLVM_OPT_NONE) {
        if (verbose) {
            printf("Optimizing (level %d)...\n", opt_level);
        }
        llvm_optimize(ctx);
    }

    /* Write output */
    if (verbose) {
        const char *format_name;
        switch (format) {
        case LLVM_OUTPUT_LLVM_IR: format_name = "LLVM IR"; break;
        case LLVM_OUTPUT_BITCODE: format_name = "bitcode"; break;
        case LLVM_OUTPUT_ASSEMBLY: format_name = "assembly"; break;
        case LLVM_OUTPUT_OBJECT: format_name = "object"; break;
        default: format_name = "unknown"; break;
        }
        printf("Writing %s to %s...\n", format_name, output_file);
    }

    if (!llvm_output(ctx, output_file, format)) {
        fprintf(stderr, "Error: Failed to write output: %s\n",
                ctx->error_msg ? ctx->error_msg : "unknown error");
        llvm_destroy_context(ctx);
        hl_code_free(code);
        return 1;
    }

    if (verbose) {
        printf("Done!\n");
    }

    /* Cleanup */
    llvm_destroy_context(ctx);
    hl_free(&module_ctx.alloc);
    free(module_ctx.functions_types);
    free(fdata);
    hl_code_free(code);

    return 0;
}
