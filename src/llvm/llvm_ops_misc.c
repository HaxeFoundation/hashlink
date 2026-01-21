/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Miscellaneous Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_misc(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case ONop: {
        /* No operation */
        break;
    }

    case OAssert: {
        /* Debug assertion - typically a breakpoint or trap */
        /* In release builds this is a no-op */
        /* For debug builds, we could emit a trap */
#ifdef DEBUG
        LLVMValueRef trap_fn = LLVMGetNamedFunction(ctx->module, "llvm.debugtrap");
        if (!trap_fn) {
            LLVMTypeRef trap_fn_type = LLVMFunctionType(ctx->void_type, NULL, 0, false);
            trap_fn = LLVMAddFunction(ctx->module, "llvm.debugtrap", trap_fn_type);
        }
        LLVMBuildCall2(ctx->builder,
            LLVMFunctionType(ctx->void_type, NULL, 0, false),
            trap_fn, NULL, 0, "");
#endif
        break;
    }

    case OPrefetch: {
        /* Memory prefetch hint - can be ignored or use LLVM prefetch intrinsic */
        int ptr_reg = op->p1;
        /* int mode = op->p2; */  /* Read/write hint */
        /* int locality = op->p3; */ /* Cache locality hint */

        LLVMValueRef ptr = llvm_load_vreg(ctx, f, ptr_reg);

        /* Get or declare llvm.prefetch intrinsic */
        LLVMValueRef prefetch_fn = LLVMGetNamedFunction(ctx->module, "llvm.prefetch.p0");
        if (!prefetch_fn) {
            /* llvm.prefetch(ptr, rw, locality, cache_type) */
            LLVMTypeRef param_types[] = { ctx->ptr_type, ctx->i32_type, ctx->i32_type, ctx->i32_type };
            LLVMTypeRef prefetch_fn_type = LLVMFunctionType(ctx->void_type, param_types, 4, false);
            prefetch_fn = LLVMAddFunction(ctx->module, "llvm.prefetch.p0", prefetch_fn_type);
        }

        LLVMValueRef args[] = {
            ptr,
            LLVMConstInt(ctx->i32_type, 0, false), /* 0 = read */
            LLVMConstInt(ctx->i32_type, 3, false), /* locality: 3 = high */
            LLVMConstInt(ctx->i32_type, 1, false)  /* cache type: 1 = data */
        };
        LLVMBuildCall2(ctx->builder,
            LLVMFunctionType(ctx->void_type,
                (LLVMTypeRef[]){ ctx->ptr_type, ctx->i32_type, ctx->i32_type, ctx->i32_type }, 4, false),
            prefetch_fn, args, 4, "");
        break;
    }

    case OAsm: {
        /* Inline assembly - not supported in LLVM backend */
        /* This opcode is rarely used */
        /* We could potentially translate to LLVM inline asm, but it's complex */
        /* For now, emit a warning comment and continue */
        break;
    }

    default:
        break;
    }
}
