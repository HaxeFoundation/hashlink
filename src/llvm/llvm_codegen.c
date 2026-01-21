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
#include "llvm_codegen.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations */
static void compile_opcode(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
static void scan_for_blocks(llvm_ctx *ctx, hl_function *f);
static void create_basic_blocks(llvm_ctx *ctx, hl_function *f);
static void create_function_allocas(llvm_ctx *ctx, hl_function *f);

llvm_ctx *llvm_create_context(void) {
    llvm_ctx *ctx = (llvm_ctx *)calloc(1, sizeof(llvm_ctx));
    if (!ctx) return NULL;

    /* Initialize LLVM targets */
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    /* Create LLVM context */
    ctx->context = LLVMContextCreate();
    if (!ctx->context) {
        free(ctx);
        return NULL;
    }

    ctx->builder = LLVMCreateBuilderInContext(ctx->context);

    /* Initialize common types */
    ctx->void_type = LLVMVoidTypeInContext(ctx->context);
    ctx->i1_type = LLVMInt1TypeInContext(ctx->context);
    ctx->i8_type = LLVMInt8TypeInContext(ctx->context);
    ctx->i16_type = LLVMInt16TypeInContext(ctx->context);
    ctx->i32_type = LLVMInt32TypeInContext(ctx->context);
    ctx->i64_type = LLVMInt64TypeInContext(ctx->context);
    ctx->f32_type = LLVMFloatTypeInContext(ctx->context);
    ctx->f64_type = LLVMDoubleTypeInContext(ctx->context);
    ctx->ptr_type = LLVMPointerTypeInContext(ctx->context, 0);

    ctx->opt_level = LLVM_OPT_DEFAULT;
    ctx->emit_debug_info = false;

    return ctx;
}

void llvm_destroy_context(llvm_ctx *ctx) {
    if (!ctx) return;

    if (ctx->error_msg) free(ctx->error_msg);
    if (ctx->basic_blocks) free(ctx->basic_blocks);
    if (ctx->vreg_allocs) free(ctx->vreg_allocs);
    if (ctx->is_block_start) free(ctx->is_block_start);
    if (ctx->hl_type_cache) free(ctx->hl_type_cache);
    if (ctx->functions) free(ctx->functions);
    if (ctx->function_types) free(ctx->function_types);
    if (ctx->global_refs) free(ctx->global_refs);
    if (ctx->string_constants) free(ctx->string_constants);
    if (ctx->bytes_constants) free(ctx->bytes_constants);
    if (ctx->type_constants) free(ctx->type_constants);

    if (ctx->target_machine) LLVMDisposeTargetMachine(ctx->target_machine);
    if (ctx->builder) LLVMDisposeBuilder(ctx->builder);
    if (ctx->module) LLVMDisposeModule(ctx->module);
    if (ctx->context) LLVMContextDispose(ctx->context);

    free(ctx);
}

bool llvm_init_module(llvm_ctx *ctx, hl_code *code, const char *module_name) {
    ctx->code = code;

    /* Create module */
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    if (!ctx->module) {
        ctx->error_msg = strdup("Failed to create LLVM module");
        return false;
    }

    /* Set target triple */
    char *triple = LLVMGetDefaultTargetTriple();
    LLVMSetTarget(ctx->module, triple);

    /* Create target machine */
    char *error = NULL;
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(triple, &target, &error) != 0) {
        ctx->error_msg = error;
        LLVMDisposeMessage(triple);
        return false;
    }

    /* Get native CPU and features for best performance */
    char *cpu = LLVMGetHostCPUName();
    char *features = LLVMGetHostCPUFeatures();

    ctx->target_machine = LLVMCreateTargetMachine(
        target, triple, cpu, features,
        (LLVMCodeGenOptLevel)ctx->opt_level,
        LLVMRelocDefault,
        LLVMCodeModelDefault
    );

    LLVMDisposeMessage(cpu);
    LLVMDisposeMessage(features);
    LLVMDisposeMessage(triple);

    if (!ctx->target_machine) {
        ctx->error_msg = strdup("Failed to create target machine");
        return false;
    }

    ctx->target_data = LLVMCreateTargetDataLayout(ctx->target_machine);
    LLVMSetModuleDataLayout(ctx->module, ctx->target_data);

    /* Allocate type cache */
    ctx->num_types = code->ntypes;
    ctx->hl_type_cache = (LLVMTypeRef *)calloc(code->ntypes, sizeof(LLVMTypeRef));

    /* Allocate function arrays */
    ctx->num_functions = code->nfunctions + code->nnatives;
    ctx->functions = (LLVMValueRef *)calloc(ctx->num_functions, sizeof(LLVMValueRef));
    ctx->function_types = (LLVMTypeRef *)calloc(ctx->num_functions, sizeof(LLVMTypeRef));

    /* Allocate globals */
    ctx->num_globals = code->nglobals;
    ctx->global_refs = (LLVMValueRef *)calloc(code->nglobals, sizeof(LLVMValueRef));

    /* Allocate string/bytes constants */
    ctx->num_strings = code->nstrings;
    ctx->string_constants = (LLVMValueRef *)calloc(code->nstrings, sizeof(LLVMValueRef));
    ctx->num_bytes = code->nbytes;
    ctx->bytes_constants = (LLVMValueRef *)calloc(code->nbytes, sizeof(LLVMValueRef));

    /* Allocate type constants */
    ctx->type_constants = (LLVMValueRef *)calloc(code->ntypes, sizeof(LLVMValueRef));

    /* Declare runtime functions */
    llvm_declare_runtime(ctx);

    /* Create function declarations for all HL functions */
    for (int i = 0; i < code->nfunctions; i++) {
        hl_function *f = &code->functions[i];
        LLVMTypeRef fn_type = llvm_get_function_type(ctx, f->type);
        ctx->function_types[f->findex] = fn_type;

        char name[64];
        snprintf(name, sizeof(name), "hl_fun_%d", f->findex);
        ctx->functions[f->findex] = LLVMAddFunction(ctx->module, name, fn_type);
    }

    /* Create declarations for native functions */
    for (int i = 0; i < code->nnatives; i++) {
        hl_native *n = &code->natives[i];

        /* Map native function names to actual symbols:
         * - "std" library functions use "hl_" prefix (built into libhl.so)
         * - Other libraries use their lib name as prefix (in hdll files)
         */
        char name[256];
        if (strcmp(n->lib, "std") == 0) {
            snprintf(name, sizeof(name), "hl_%s", n->name);
        } else {
            snprintf(name, sizeof(name), "%s_%s", n->lib, n->name);
        }
        /* Check if function already declared by runtime declarations.
         * If so, use existing to avoid LLVM creating suffixed symbols.
         * Also use the existing function's type for consistency. */
        LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
        if (existing) {
            ctx->functions[n->findex] = existing;
            ctx->function_types[n->findex] = LLVMGlobalGetValueType(existing);
        } else {
            LLVMTypeRef fn_type = llvm_get_function_type(ctx, n->t);
            ctx->function_types[n->findex] = fn_type;
            ctx->functions[n->findex] = LLVMAddFunction(ctx->module, name, fn_type);
            LLVMSetLinkage(ctx->functions[n->findex], LLVMExternalLinkage);
        }
    }

    /* Create global variables storage */
    LLVMTypeRef globals_type = LLVMArrayType(ctx->i8_type, code->nglobals * 8);
    ctx->globals_base = LLVMAddGlobal(ctx->module, globals_type, "hl_globals");
    LLVMSetInitializer(ctx->globals_base, LLVMConstNull(globals_type));

    /* Create pointers to individual globals (each global is at offset i*8 in the array) */
    for (int i = 0; i < code->nglobals; i++) {
        LLVMValueRef indices[] = {
            LLVMConstInt(ctx->i64_type, 0, false),
            LLVMConstInt(ctx->i64_type, i * 8, false)
        };
        ctx->global_refs[i] = LLVMConstGEP2(globals_type, ctx->globals_base, indices, 2);
    }

    /* Create string constants */
    for (int i = 0; i < code->nstrings; i++) {
        const char *str = code->strings[i];
        int len = code->strings_lens[i];
        char name[32];
        snprintf(name, sizeof(name), ".str.%d", i);

        /* Create global string constant */
        LLVMValueRef str_val = LLVMConstStringInContext(ctx->context, str, len, 1);
        LLVMValueRef global = LLVMAddGlobal(ctx->module, LLVMTypeOf(str_val), name);
        LLVMSetInitializer(global, str_val);
        LLVMSetLinkage(global, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(global, 1);
        ctx->string_constants[i] = global;
    }

    /* Create bytes constants */
    for (int i = 0; i < code->nbytes; i++) {
        int pos = code->bytes_pos[i];
        int len = (i + 1 < code->nbytes) ? code->bytes_pos[i + 1] - pos : 0;
        char name[32];
        snprintf(name, sizeof(name), ".bytes.%d", i);

        LLVMValueRef bytes_val = LLVMConstStringInContext(
            ctx->context, code->bytes + pos, len, 1);
        LLVMValueRef global = LLVMAddGlobal(ctx->module, LLVMTypeOf(bytes_val), name);
        LLVMSetInitializer(global, bytes_val);
        LLVMSetLinkage(global, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(global, 1);
        ctx->bytes_constants[i] = global;
    }

    /*
     * Type access: instead of embedding type pointers as globals (which would need
     * runtime relocation), we call aot_get_type(idx) at runtime to get properly
     * initialized type pointers from the loaded module.
     *
     * The type_constants array is no longer used - llvm_get_type_ptr() generates
     * calls to aot_get_type() instead.
     */

    return true;
}

bool llvm_compile_function(llvm_ctx *ctx, hl_function *f) {
    LLVMValueRef func = ctx->functions[f->findex];
    if (!func) {
        ctx->error_msg = strdup("Function not declared");
        return false;
    }

    ctx->current_function = func;
    ctx->num_vregs = f->nregs;

    /* Scan opcodes to find basic block boundaries */
    scan_for_blocks(ctx, f);

    /* Create basic blocks */
    create_basic_blocks(ctx, f);

    /* Create a dedicated entry block for allocas (LLVM entry block can't have predecessors) */
    ctx->entry_block = LLVMAppendBasicBlockInContext(
        ctx->context, ctx->current_function, "entry");
    /* Move entry block to the front */
    LLVMMoveBasicBlockBefore(ctx->entry_block, ctx->basic_blocks[0]);

    /* Position at entry block and create allocas */
    LLVMPositionBuilderAtEnd(ctx->builder, ctx->entry_block);
    create_function_allocas(ctx, f);

    /* Store function parameters into their allocas */
    hl_type_fun *ft = f->type->fun;
    for (int i = 0; i < ft->nargs; i++) {
        LLVMValueRef param = LLVMGetParam(func, i);
        LLVMBuildStore(ctx->builder, param, ctx->vreg_allocs[i]);
    }

    /* Branch from entry to first code block */
    LLVMBuildBr(ctx->builder, ctx->basic_blocks[0]);

    /* Position at first code block */
    LLVMPositionBuilderAtEnd(ctx->builder, ctx->basic_blocks[0]);

    /* Track if current block is already terminated */
    bool block_terminated = false;

    /* Compile each opcode */
    for (int i = 0; i < f->nops; i++) {
        /* Switch to appropriate basic block if this starts one */
        if (ctx->is_block_start[i] && i > 0) {
            LLVMBasicBlockRef block = ctx->basic_blocks[i];
            if (block) {
                /* Add fallthrough branch if previous block not terminated */
                if (!block_terminated) {
                    LLVMBuildBr(ctx->builder, block);
                }
                /* Position at the new block */
                LLVMPositionBuilderAtEnd(ctx->builder, block);
                block_terminated = false;
            }
        }

        /* Skip instructions if block already terminated (dead code) */
        if (block_terminated) {
            continue;
        }

        compile_opcode(ctx, f, &f->ops[i], i);

        /* Check if block actually has a terminator now */
        LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
        if (current && LLVMGetBasicBlockTerminator(current)) {
            block_terminated = true;
        }
    }

    /* Ensure final block is terminated */
    if (!block_terminated) {
        llvm_ensure_block_terminated(ctx, NULL);
    }

    /* Clean up per-function state */
    free(ctx->basic_blocks);
    ctx->basic_blocks = NULL;
    free(ctx->vreg_allocs);
    ctx->vreg_allocs = NULL;
    free(ctx->is_block_start);
    ctx->is_block_start = NULL;
    ctx->current_function = NULL;

    return true;
}

static void scan_for_blocks(llvm_ctx *ctx, hl_function *f) {
    ctx->is_block_start = (bool *)calloc(f->nops + 1, sizeof(bool));
    ctx->is_block_start[0] = true;  /* Entry block */

    for (int i = 0; i < f->nops; i++) {
        hl_opcode *op = &f->ops[i];
        switch (op->op) {
        case OLabel:
            ctx->is_block_start[i] = true;
            break;

        case OJTrue:
        case OJFalse:
        case OJNull:
        case OJNotNull:
            ctx->is_block_start[i + 1] = true;
            ctx->is_block_start[i + 1 + op->p2] = true;
            break;

        case OJSLt:
        case OJSGte:
        case OJSGt:
        case OJSLte:
        case OJULt:
        case OJUGte:
        case OJNotLt:
        case OJNotGte:
        case OJEq:
        case OJNotEq:
            ctx->is_block_start[i + 1] = true;
            ctx->is_block_start[i + 1 + op->p3] = true;
            break;

        case OJAlways:
            ctx->is_block_start[i + 1 + op->p1] = true;
            if (i + 1 < f->nops)
                ctx->is_block_start[i + 1] = true;
            break;

        case OSwitch: {
            int ncases = op->p2;
            for (int j = 0; j < ncases; j++) {
                int target = i + 1 + op->extra[j];
                if (target >= 0 && target < f->nops)
                    ctx->is_block_start[target] = true;
            }
            /* Default case is next instruction */
            if (i + 1 < f->nops)
                ctx->is_block_start[i + 1] = true;
            break;
        }

        case OTrap:
            ctx->is_block_start[i + 1] = true;
            ctx->is_block_start[i + 1 + op->p2] = true;
            break;

        case ORet:
        case OThrow:
        case ORethrow:
            if (i + 1 < f->nops)
                ctx->is_block_start[i + 1] = true;
            break;

        default:
            break;
        }
    }

}

static void create_basic_blocks(llvm_ctx *ctx, hl_function *f) {
    ctx->num_blocks = f->nops + 1;  /* Array size, indexed by opcode position */
    ctx->basic_blocks = (LLVMBasicBlockRef *)calloc(ctx->num_blocks, sizeof(LLVMBasicBlockRef));

    for (int i = 0; i <= f->nops; i++) {
        if (ctx->is_block_start[i]) {
            char name[32];
            snprintf(name, sizeof(name), "bb%d", i);
            ctx->basic_blocks[i] = LLVMAppendBasicBlockInContext(
                ctx->context, ctx->current_function, name);
        }
    }
}

static void create_function_allocas(llvm_ctx *ctx, hl_function *f) {
    ctx->vreg_allocs = (LLVMValueRef *)calloc(f->nregs, sizeof(LLVMValueRef));

    for (int i = 0; i < f->nregs; i++) {
        hl_type *t = f->regs[i];

        /* Skip void types - can't allocate void */
        if (!t || t->kind == HVOID) {
            ctx->vreg_allocs[i] = NULL;
            continue;
        }

        LLVMTypeRef llvm_type = llvm_get_type(ctx, t);

        char name[32];
        snprintf(name, sizeof(name), "r%d", i);
        ctx->vreg_allocs[i] = LLVMBuildAlloca(ctx->builder, llvm_type, name);
    }
}

static void compile_opcode(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    /* Constants */
    case OMov:
    case OInt:
    case OFloat:
    case OBool:
    case OBytes:
    case OString:
    case ONull:
        llvm_emit_constants(ctx, f, op, op_idx);
        break;

    /* Arithmetic */
    case OAdd:
    case OSub:
    case OMul:
    case OSDiv:
    case OUDiv:
    case OSMod:
    case OUMod:
    case OShl:
    case OSShr:
    case OUShr:
    case OAnd:
    case OOr:
    case OXor:
    case ONeg:
    case ONot:
    case OIncr:
    case ODecr:
        llvm_emit_arithmetic(ctx, f, op, op_idx);
        break;

    /* Control flow */
    case OLabel:
    case ORet:
    case OJTrue:
    case OJFalse:
    case OJNull:
    case OJNotNull:
    case OJSLt:
    case OJSGte:
    case OJSGt:
    case OJSLte:
    case OJULt:
    case OJUGte:
    case OJNotLt:
    case OJNotGte:
    case OJEq:
    case OJNotEq:
    case OJAlways:
    case OSwitch:
        llvm_emit_control_flow(ctx, f, op, op_idx);
        break;

    /* Memory */
    case OGetGlobal:
    case OSetGlobal:
    case OField:
    case OSetField:
    case OGetThis:
    case OSetThis:
    case OGetI8:
    case OGetI16:
    case OGetMem:
    case OGetArray:
    case OSetI8:
    case OSetI16:
    case OSetMem:
    case OSetArray:
        llvm_emit_memory(ctx, f, op, op_idx);
        break;

    /* Calls */
    case OCall0:
    case OCall1:
    case OCall2:
    case OCall3:
    case OCall4:
    case OCallN:
    case OCallMethod:
    case OCallThis:
    case OCallClosure:
        llvm_emit_calls(ctx, f, op, op_idx);
        break;

    /* Closures */
    case OStaticClosure:
    case OInstanceClosure:
    case OVirtualClosure:
        llvm_emit_closures(ctx, f, op, op_idx);
        break;

    /* Type operations */
    case OToDyn:
    case OToSFloat:
    case OToUFloat:
    case OToInt:
    case OSafeCast:
    case OUnsafeCast:
    case OToVirtual:
    case OType:
    case OGetType:
    case OGetTID:
        llvm_emit_types(ctx, f, op, op_idx);
        break;

    /* Null check is control flow (branches) */
    case ONullCheck:
        llvm_emit_control_flow(ctx, f, op, op_idx);
        break;

    /* Objects */
    case ONew:
    case OArraySize:
    case ODynGet:
    case ODynSet:
        llvm_emit_objects(ctx, f, op, op_idx);
        break;

    /* Enums */
    case OMakeEnum:
    case OEnumAlloc:
    case OEnumIndex:
    case OEnumField:
    case OSetEnumField:
        llvm_emit_enums(ctx, f, op, op_idx);
        break;

    /* References */
    case ORef:
    case OUnref:
    case OSetref:
    case ORefData:
    case ORefOffset:
        llvm_emit_refs(ctx, f, op, op_idx);
        break;

    /* Exceptions */
    case OThrow:
    case ORethrow:
    case OTrap:
    case OEndTrap:
    case OCatch:
        llvm_emit_exceptions(ctx, f, op, op_idx);
        break;

    /* Miscellaneous */
    case OAssert:
    case ONop:
    case OPrefetch:
    case OAsm:
        llvm_emit_misc(ctx, f, op, op_idx);
        break;

    default:
        fprintf(stderr, "Warning: Unhandled opcode %d at position %d\n", op->op, op_idx);
        break;
    }
}

bool llvm_finalize_module(llvm_ctx *ctx) {
    return true;
}

bool llvm_verify(llvm_ctx *ctx) {
    char *error = NULL;
    if (LLVMVerifyModule(ctx->module, LLVMReturnStatusAction, &error) != 0) {
        ctx->error_msg = error;
        return false;
    }
    if (error) LLVMDisposeMessage(error);
    return true;
}

void llvm_optimize(llvm_ctx *ctx) {
    if (ctx->opt_level == LLVM_OPT_NONE) return;

    /* Use the new pass manager (LLVM 13+) via LLVMRunPasses */
    const char *passes;
    switch (ctx->opt_level) {
    case LLVM_OPT_LESS:
        passes = "default<O1>";
        break;
    case LLVM_OPT_DEFAULT:
        passes = "default<O2>";
        break;
    case LLVM_OPT_AGGRESSIVE:
        passes = "default<O3>";
        break;
    default:
        return;
    }

    LLVMPassBuilderOptionsRef options = LLVMCreatePassBuilderOptions();
    LLVMErrorRef err = LLVMRunPasses(ctx->module, passes, ctx->target_machine, options);
    if (err) {
        char *msg = LLVMGetErrorMessage(err);
        fprintf(stderr, "Optimization error: %s\n", msg);
        LLVMDisposeErrorMessage(msg);
    }
    LLVMDisposePassBuilderOptions(options);
}

bool llvm_output(llvm_ctx *ctx, const char *filename, llvm_output_format format) {
    char *error = NULL;

    switch (format) {
    case LLVM_OUTPUT_LLVM_IR:
        if (LLVMPrintModuleToFile(ctx->module, filename, &error) != 0) {
            ctx->error_msg = error;
            return false;
        }
        break;

    case LLVM_OUTPUT_BITCODE:
        if (LLVMWriteBitcodeToFile(ctx->module, filename) != 0) {
            ctx->error_msg = strdup("Failed to write bitcode");
            return false;
        }
        break;

    case LLVM_OUTPUT_ASSEMBLY:
        if (LLVMTargetMachineEmitToFile(ctx->target_machine, ctx->module,
                                         (char *)filename, LLVMAssemblyFile, &error) != 0) {
            ctx->error_msg = error;
            return false;
        }
        break;

    case LLVM_OUTPUT_OBJECT:
        if (LLVMTargetMachineEmitToFile(ctx->target_machine, ctx->module,
                                         (char *)filename, LLVMObjectFile, &error) != 0) {
            ctx->error_msg = error;
            return false;
        }
        break;
    }

    return true;
}

/* Helper functions */

LLVMValueRef llvm_load_vreg(llvm_ctx *ctx, hl_function *f, int vreg_idx) {
    if (vreg_idx < 0 || vreg_idx >= f->nregs) {
        fprintf(stderr, "Invalid vreg index: %d\n", vreg_idx);
        return LLVMConstNull(ctx->ptr_type);
    }

    /* Void types have no alloca - return undef */
    if (!ctx->vreg_allocs[vreg_idx]) {
        return LLVMGetUndef(ctx->ptr_type);
    }

    LLVMTypeRef type = llvm_get_type(ctx, f->regs[vreg_idx]);
    return LLVMBuildLoad2(ctx->builder, type, ctx->vreg_allocs[vreg_idx], "");
}

void llvm_store_vreg(llvm_ctx *ctx, hl_function *f, int vreg_idx, LLVMValueRef value) {
    if (vreg_idx < 0 || vreg_idx >= f->nregs) {
        fprintf(stderr, "Invalid vreg index: %d\n", vreg_idx);
        return;
    }

    /* Void types have no alloca - skip store */
    if (!ctx->vreg_allocs[vreg_idx]) {
        return;
    }

    LLVMBuildStore(ctx->builder, value, ctx->vreg_allocs[vreg_idx]);
}

LLVMBasicBlockRef llvm_get_block_for_offset(llvm_ctx *ctx, int current_op, int offset) {
    int target = current_op + 1 + offset;
    if (target >= 0 && target < ctx->num_blocks && ctx->basic_blocks[target]) {
        return ctx->basic_blocks[target];
    }
    return NULL;
}

void llvm_ensure_block_terminated(llvm_ctx *ctx, LLVMBasicBlockRef next_block) {
    LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
    if (!current) return;

    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(current);
    if (!terminator) {
        if (next_block) {
            LLVMBuildBr(ctx->builder, next_block);
        } else {
            /* End of function - add appropriate return */
            LLVMTypeRef fn_type = LLVMGlobalGetValueType(ctx->current_function);
            LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
            if (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind) {
                LLVMBuildRetVoid(ctx->builder);
            } else {
                /* Return undef for non-void functions that fall through */
                LLVMBuildRet(ctx->builder, LLVMGetUndef(ret_type));
            }
        }
    }
}

LLVMValueRef llvm_get_function_ptr(llvm_ctx *ctx, int findex) {
    if (findex >= 0 && findex < ctx->num_functions) {
        return ctx->functions[findex];
    }
    return NULL;
}

LLVMValueRef llvm_get_type_ptr(llvm_ctx *ctx, int type_idx) {
    /*
     * Call aot_get_type(type_idx) to get the type pointer at runtime.
     * This returns &aot_types[type_idx] where aot_types points to the
     * module's types array (initialized by aot_init_module_data).
     */
    LLVMValueRef idx = LLVMConstInt(ctx->i32_type, type_idx, false);
    LLVMValueRef args[] = { idx };
    LLVMTypeRef fn_type = LLVMFunctionType(ctx->ptr_type, (LLVMTypeRef[]){ ctx->i32_type }, 1, false);
    return LLVMBuildCall2(ctx->builder, fn_type, ctx->rt_aot_get_type, args, 1, "type_ptr");
}

LLVMValueRef llvm_get_string(llvm_ctx *ctx, int str_idx) {
    if (str_idx >= 0 && str_idx < ctx->num_strings) {
        return ctx->string_constants[str_idx];
    }
    return LLVMConstNull(ctx->ptr_type);
}

LLVMValueRef llvm_get_bytes(llvm_ctx *ctx, int bytes_idx) {
    if (bytes_idx >= 0 && bytes_idx < ctx->num_bytes) {
        return ctx->bytes_constants[bytes_idx];
    }
    return LLVMConstNull(ctx->ptr_type);
}

LLVMValueRef llvm_create_entry_alloca(llvm_ctx *ctx, LLVMTypeRef type, const char *name) {
    /*
     * Create an alloca in the entry block to avoid stack growth in loops.
     * This is the LLVM-recommended pattern for temporary stack allocations.
     */
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(ctx->builder);

    /* Position at end of entry block (before the terminator) */
    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(ctx->entry_block);
    if (terminator) {
        LLVMPositionBuilderBefore(ctx->builder, terminator);
    } else {
        LLVMPositionBuilderAtEnd(ctx->builder, ctx->entry_block);
    }

    /* Create the alloca */
    LLVMValueRef alloca = LLVMBuildAlloca(ctx->builder, type, name);

    /* Restore builder position */
    LLVMPositionBuilderAtEnd(ctx->builder, current_block);

    return alloca;
}

bool llvm_generate_entry_point(llvm_ctx *ctx, int entry_findex) {
    /*
     * Generate main() entry point that:
     * 1. Calls hl_global_init() to initialize GC
     * 2. Calls hl_sys_init() to initialize runtime
     * 3. Calls hl_register_thread() to register current thread
     * 4. Loads the .hl module for type information
     * 5. Patches function pointers for AOT functions
     * 6. Calls the HashLink entry point function
     * 7. Returns 0
     */

    /*
     * Generate the AOT function pointer table.
     * This maps findex -> function pointer for HL functions.
     * Native function entries are NULL (resolved by runtime).
     *
     * The table is used by aot_init_module_data to patch m->functions_ptrs
     * so that closures and method dispatch work correctly.
     */
    int total_funcs = ctx->code->nfunctions + ctx->code->nnatives;
    LLVMTypeRef func_table_type = LLVMArrayType(ctx->ptr_type, total_funcs);
    LLVMValueRef func_table = LLVMAddGlobal(ctx->module, func_table_type, "aot_function_table");
    LLVMSetLinkage(func_table, LLVMExternalLinkage);

    /* Build initializer - HL functions get their pointer, natives get NULL */
    LLVMValueRef *func_ptrs = malloc(sizeof(LLVMValueRef) * total_funcs);
    for (int i = 0; i < total_funcs; i++) {
        func_ptrs[i] = LLVMConstNull(ctx->ptr_type);  /* Default to NULL (natives) */
    }

    /* Fill in HL function pointers */
    for (int i = 0; i < ctx->code->nfunctions; i++) {
        hl_function *f = ctx->code->functions + i;
        if (ctx->functions[f->findex]) {
            func_ptrs[f->findex] = ctx->functions[f->findex];
        }
    }

    LLVMValueRef table_init = LLVMConstArray(ctx->ptr_type, func_ptrs, total_funcs);
    LLVMSetInitializer(func_table, table_init);
    free(func_ptrs);

    /* Generate the function count constant */
    LLVMValueRef func_count = LLVMAddGlobal(ctx->module, ctx->i32_type, "aot_function_count");
    LLVMSetLinkage(func_count, LLVMExternalLinkage);
    LLVMSetInitializer(func_count, LLVMConstInt(ctx->i32_type, total_funcs, false));

    /* Declare external runtime init functions */
    LLVMTypeRef void_fn_type = LLVMFunctionType(ctx->void_type, NULL, 0, false);

    LLVMValueRef sys_init = LLVMAddFunction(ctx->module, "hl_sys_init",
        LLVMFunctionType(ctx->void_type, (LLVMTypeRef[]){ ctx->ptr_type, ctx->i32_type, ctx->ptr_type }, 3, false));
    LLVMSetLinkage(sys_init, LLVMExternalLinkage);

    LLVMValueRef register_thread = LLVMAddFunction(ctx->module, "hl_register_thread",
        LLVMFunctionType(ctx->void_type, (LLVMTypeRef[]){ ctx->ptr_type }, 1, false));
    LLVMSetLinkage(register_thread, LLVMExternalLinkage);

    LLVMValueRef global_init = LLVMAddFunction(ctx->module, "hl_global_init",
        void_fn_type);
    LLVMSetLinkage(global_init, LLVMExternalLinkage);

    /* AOT runtime helper function - initializes module from embedded bytecode */
    LLVMValueRef aot_init = LLVMAddFunction(ctx->module, "aot_init_module_data",
        LLVMFunctionType(ctx->i32_type, (LLVMTypeRef[]){ ctx->ptr_type, ctx->i32_type }, 2, false));
    LLVMSetLinkage(aot_init, LLVMExternalLinkage);

    /*
     * Embed the .hl bytecode as a global constant array.
     *
     * This makes the AOT binary fully standalone - it doesn't need the original
     * .hl file at runtime. The bytecode contains type metadata (hl_type structures,
     * vtables, field layouts) that the runtime needs for:
     *   - Object allocation (type size and layout)
     *   - Type casting and runtime type checks
     *   - Dynamic dispatch through vtables
     *   - Field access offset calculations
     *
     * While function code is AOT-compiled to native instructions, the type metadata
     * is parsed from this embedded bytecode at startup to initialize the runtime.
     */
    LLVMValueRef bytecode_global = NULL;
    if (ctx->bytecode_data && ctx->bytecode_size > 0) {
        LLVMTypeRef bytecode_type = LLVMArrayType(ctx->i8_type, ctx->bytecode_size);
        bytecode_global = LLVMAddGlobal(ctx->module, bytecode_type, "hl_bytecode");
        LLVMSetLinkage(bytecode_global, LLVMPrivateLinkage);
        LLVMSetGlobalConstant(bytecode_global, 1);

        /* Create initializer from bytecode data */
        LLVMValueRef *bytes = malloc(sizeof(LLVMValueRef) * ctx->bytecode_size);
        for (int i = 0; i < ctx->bytecode_size; i++) {
            bytes[i] = LLVMConstInt(ctx->i8_type, ctx->bytecode_data[i], 0);
        }
        LLVMValueRef init = LLVMConstArray(ctx->i8_type, bytes, ctx->bytecode_size);
        LLVMSetInitializer(bytecode_global, init);
        free(bytes);
    }

    /* Create main function */
    LLVMTypeRef main_params[] = { ctx->i32_type, ctx->ptr_type };
    LLVMTypeRef main_type = LLVMFunctionType(ctx->i32_type, main_params, 2, false);
    LLVMValueRef main_func = LLVMAddFunction(ctx->module, "main", main_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx->context, main_func, "entry");
    LLVMPositionBuilderAtEnd(ctx->builder, entry);

    /* Get argc and argv */
    LLVMValueRef argc = LLVMGetParam(main_func, 0);
    LLVMValueRef argv = LLVMGetParam(main_func, 1);

    /* Allocate thread info on stack */
    LLVMTypeRef thread_info_type = LLVMArrayType(ctx->i8_type, 512); /* Approximate size */
    LLVMValueRef thread_info = LLVMBuildAlloca(ctx->builder, thread_info_type, "thread_info");

    /* Call hl_global_init() first - initializes GC mutex needed by other functions */
    LLVMBuildCall2(ctx->builder, void_fn_type, global_init, NULL, 0, "");

    /* Call hl_sys_init(NULL, argc, argv) */
    LLVMValueRef null_ptr = LLVMConstNull(ctx->ptr_type);
    LLVMValueRef init_args[] = { null_ptr, argc, argv };
    LLVMBuildCall2(ctx->builder,
        LLVMFunctionType(ctx->void_type, (LLVMTypeRef[]){ ctx->ptr_type, ctx->i32_type, ctx->ptr_type }, 3, false),
        sys_init, init_args, 3, "");

    /* Call hl_register_thread(thread_info) */
    LLVMValueRef thread_ptr = LLVMBuildBitCast(ctx->builder, thread_info, ctx->ptr_type, "");
    LLVMValueRef register_args[] = { thread_ptr };
    LLVMBuildCall2(ctx->builder,
        LLVMFunctionType(ctx->void_type, (LLVMTypeRef[]){ ctx->ptr_type }, 1, false),
        register_thread, register_args, 1, "");

    /* Initialize module from embedded bytecode */
    LLVMValueRef init_result;
    if (bytecode_global) {
        /* Call aot_init_module_data(bytecode_ptr, bytecode_size) */
        LLVMValueRef bytecode_ptr = LLVMBuildBitCast(ctx->builder, bytecode_global, ctx->ptr_type, "");
        LLVMValueRef bytecode_size = LLVMConstInt(ctx->i32_type, ctx->bytecode_size, false);
        LLVMValueRef aot_args[] = { bytecode_ptr, bytecode_size };
        init_result = LLVMBuildCall2(ctx->builder,
            LLVMFunctionType(ctx->i32_type, (LLVMTypeRef[]){ ctx->ptr_type, ctx->i32_type }, 2, false),
            aot_init, aot_args, 2, "init_result");
    } else {
        /* No bytecode embedded - fail initialization */
        init_result = LLVMConstInt(ctx->i32_type, 0, false);
    }

    /* Check if init failed, exit with error code 1 */
    LLVMValueRef init_failed = LLVMBuildICmp(ctx->builder, LLVMIntEQ, init_result,
        LLVMConstInt(ctx->i32_type, 0, false), "init_failed");
    LLVMBasicBlockRef fail_bb = LLVMAppendBasicBlockInContext(ctx->context, main_func, "init_fail");
    LLVMBasicBlockRef cont_bb = LLVMAppendBasicBlockInContext(ctx->context, main_func, "init_ok");
    LLVMBuildCondBr(ctx->builder, init_failed, fail_bb, cont_bb);

    /* Fail block: return 1 */
    LLVMPositionBuilderAtEnd(ctx->builder, fail_bb);
    LLVMBuildRet(ctx->builder, LLVMConstInt(ctx->i32_type, 1, false));

    /* Continue block */
    LLVMPositionBuilderAtEnd(ctx->builder, cont_bb);

    /* Call entry point function */
    LLVMValueRef entry_func = ctx->functions[entry_findex];
    if (!entry_func) {
        ctx->error_msg = strdup("Entry point function not found");
        return false;
    }

    /*
     * Set up exception trap around entry point call.
     * This catches any uncaught exceptions and prints them nicely.
     *
     * Equivalent to:
     *   hl_trap_ctx trap;
     *   hl_trap(trap, exc, on_exception);
     *   entry_func();
     *   hl_endtrap(trap);
     *   return 0;
     * on_exception:
     *   hl_print_uncaught_exception(exc);
     *   return 1;
     */

    /* Compute offsets using NULL pointer trick */
    hl_trap_ctx *t = NULL;
    hl_thread_info *tinf = NULL;
    int offset_trap_current = (int)(int_val)&tinf->trap_current;
    int offset_exc_value = (int)(int_val)&tinf->exc_value;
    int offset_prev = (int)(int_val)&t->prev;
    int offset_tcheck = (int)(int_val)&t->tcheck;

    /* Allocate hl_trap_ctx on stack */
    int trap_size = (sizeof(hl_trap_ctx) + 15) & ~15;
    LLVMTypeRef trap_type = LLVMArrayType(ctx->i8_type, trap_size);
    LLVMValueRef trap = LLVMBuildAlloca(ctx->builder, trap_type, "trap_ctx");

    /* Get thread info */
    LLVMValueRef thread = LLVMBuildCall2(ctx->builder,
        LLVMGlobalGetValueType(ctx->rt_get_thread),
        ctx->rt_get_thread, NULL, 0, "thread");

    /* trap->tcheck = NULL */
    LLVMValueRef tcheck_offset = LLVMConstInt(ctx->i64_type, offset_tcheck, false);
    LLVMValueRef tcheck_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
        trap, &tcheck_offset, 1, "tcheck_ptr");
    LLVMBuildStore(ctx->builder, LLVMConstNull(ctx->ptr_type), tcheck_ptr);

    /* trap->prev = thread->trap_current */
    LLVMValueRef trap_current_offset = LLVMConstInt(ctx->i64_type, offset_trap_current, false);
    LLVMValueRef trap_current_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
        thread, &trap_current_offset, 1, "trap_current_ptr");
    LLVMValueRef old_trap = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, trap_current_ptr, "old_trap");

    LLVMValueRef prev_offset = LLVMConstInt(ctx->i64_type, offset_prev, false);
    LLVMValueRef prev_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
        trap, &prev_offset, 1, "prev_ptr");
    LLVMBuildStore(ctx->builder, old_trap, prev_ptr);

    /* thread->trap_current = trap */
    LLVMBuildStore(ctx->builder, trap, trap_current_ptr);

    /* Call setjmp */
    LLVMValueRef setjmp_args[] = { trap };
    LLVMTypeRef setjmp_fn_type = LLVMFunctionType(ctx->i32_type,
        (LLVMTypeRef[]){ ctx->ptr_type }, 1, false);
    LLVMValueRef setjmp_result = LLVMBuildCall2(ctx->builder, setjmp_fn_type,
        ctx->rt_setjmp, setjmp_args, 1, "setjmp_result");

    /* Branch based on setjmp result */
    LLVMValueRef zero = LLVMConstInt(ctx->i32_type, 0, false);
    LLVMValueRef caught = LLVMBuildICmp(ctx->builder, LLVMIntNE, setjmp_result, zero, "caught");

    LLVMBasicBlockRef normal_bb = LLVMAppendBasicBlockInContext(ctx->context, main_func, "normal");
    LLVMBasicBlockRef exception_bb = LLVMAppendBasicBlockInContext(ctx->context, main_func, "exception");
    LLVMBuildCondBr(ctx->builder, caught, exception_bb, normal_bb);

    /* Normal path: call entry function, clean up trap, return 0 */
    LLVMPositionBuilderAtEnd(ctx->builder, normal_bb);

    LLVMTypeRef entry_fn_type = ctx->function_types[entry_findex];
    LLVMBuildCall2(ctx->builder, entry_fn_type, entry_func, NULL, 0, "");

    /* hl_endtrap: thread->trap_current = trap->prev */
    LLVMValueRef thread2 = LLVMBuildCall2(ctx->builder,
        LLVMGlobalGetValueType(ctx->rt_get_thread),
        ctx->rt_get_thread, NULL, 0, "thread2");
    LLVMValueRef trap_current_ptr2 = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
        thread2, &trap_current_offset, 1, "trap_current_ptr2");
    LLVMValueRef prev_trap = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, prev_ptr, "prev_trap");
    LLVMBuildStore(ctx->builder, prev_trap, trap_current_ptr2);

    LLVMBuildRet(ctx->builder, LLVMConstInt(ctx->i32_type, 0, false));

    /* Exception path: print exception, return 1 */
    LLVMPositionBuilderAtEnd(ctx->builder, exception_bb);

    /* Declare hl_print_uncaught_exception if needed */
    LLVMValueRef print_exc = LLVMGetNamedFunction(ctx->module, "hl_print_uncaught_exception");
    if (!print_exc) {
        LLVMTypeRef print_params[] = { ctx->ptr_type };
        LLVMTypeRef print_type = LLVMFunctionType(ctx->void_type, print_params, 1, false);
        print_exc = LLVMAddFunction(ctx->module, "hl_print_uncaught_exception", print_type);
        LLVMSetLinkage(print_exc, LLVMExternalLinkage);
    }

    /* Get exception value from thread */
    LLVMValueRef thread3 = LLVMBuildCall2(ctx->builder,
        LLVMGlobalGetValueType(ctx->rt_get_thread),
        ctx->rt_get_thread, NULL, 0, "thread3");
    LLVMValueRef exc_value_offset = LLVMConstInt(ctx->i64_type, offset_exc_value, false);
    LLVMValueRef exc_value_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
        thread3, &exc_value_offset, 1, "exc_value_ptr");
    LLVMValueRef exc_value = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, exc_value_ptr, "exc_value");

    /* Call hl_print_uncaught_exception(exc) */
    LLVMValueRef print_args[] = { exc_value };
    LLVMBuildCall2(ctx->builder,
        LLVMFunctionType(ctx->void_type, (LLVMTypeRef[]){ ctx->ptr_type }, 1, false),
        print_exc, print_args, 1, "");

    LLVMBuildRet(ctx->builder, LLVMConstInt(ctx->i32_type, 1, false));

    return true;
}
