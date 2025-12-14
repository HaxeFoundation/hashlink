/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Constant and Movement Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_constants(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OMov: {
        /* dst = src */
        int dst = op->p1;
        int src = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OInt: {
        /* dst = ints[p2] */
        int dst = op->p1;
        int val = ctx->code->ints[op->p2];
        LLVMValueRef const_val = LLVMConstInt(ctx->i32_type, val, true);
        llvm_store_vreg(ctx, f, dst, const_val);
        break;
    }

    case OFloat: {
        /* dst = floats[p2] */
        int dst = op->p1;
        double val = ctx->code->floats[op->p2];
        hl_type *t = f->regs[dst];
        LLVMValueRef const_val;
        if (t->kind == HF32) {
            const_val = LLVMConstReal(ctx->f32_type, val);
        } else {
            const_val = LLVMConstReal(ctx->f64_type, val);
        }
        llvm_store_vreg(ctx, f, dst, const_val);
        break;
    }

    case OBool: {
        /* dst = p2 (0 or 1) */
        int dst = op->p1;
        int val = op->p2;
        LLVMValueRef const_val = LLVMConstInt(ctx->i8_type, val ? 1 : 0, false);
        llvm_store_vreg(ctx, f, dst, const_val);
        break;
    }

    case OBytes: {
        /* dst = bytes[p2] */
        int dst = op->p1;
        int idx = op->p2;
        LLVMValueRef bytes_ptr = llvm_get_bytes(ctx, idx);
        /* Get pointer to first element */
        LLVMValueRef indices[] = {
            LLVMConstInt(ctx->i32_type, 0, false),
            LLVMConstInt(ctx->i32_type, 0, false)
        };
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder,
            LLVMGetElementType(LLVMTypeOf(bytes_ptr)), bytes_ptr, indices, 2, "");
        llvm_store_vreg(ctx, f, dst, ptr);
        break;
    }

    case OString: {
        /* dst = strings[p2] (as UTF-16) */
        int dst = op->p1;
        int idx = op->p2;
        LLVMValueRef str_ptr = llvm_get_string(ctx, idx);
        /* Get pointer to first element */
        LLVMValueRef indices[] = {
            LLVMConstInt(ctx->i32_type, 0, false),
            LLVMConstInt(ctx->i32_type, 0, false)
        };
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder,
            LLVMGetElementType(LLVMTypeOf(str_ptr)), str_ptr, indices, 2, "");
        llvm_store_vreg(ctx, f, dst, ptr);
        break;
    }

    case ONull: {
        /* dst = null */
        int dst = op->p1;
        LLVMValueRef null_val = LLVMConstNull(ctx->ptr_type);
        llvm_store_vreg(ctx, f, dst, null_val);
        break;
    }

    default:
        break;
    }
}
