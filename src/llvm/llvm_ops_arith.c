/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Arithmetic and Logic Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_arithmetic(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OAdd: {
        /* dst = a + b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        hl_type *t = f->regs[dst];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFAdd(ctx->builder, a, b, "");
        } else {
            result = LLVMBuildAdd(ctx->builder, a, b, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OSub: {
        /* dst = a - b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        hl_type *t = f->regs[dst];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFSub(ctx->builder, a, b, "");
        } else {
            result = LLVMBuildSub(ctx->builder, a, b, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OMul: {
        /* dst = a * b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        hl_type *t = f->regs[dst];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFMul(ctx->builder, a, b, "");
        } else {
            result = LLVMBuildMul(ctx->builder, a, b, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OSDiv: {
        /* dst = a / b (signed) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        hl_type *t = f->regs[dst];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFDiv(ctx->builder, a, b, "");
        } else {
            result = LLVMBuildSDiv(ctx->builder, a, b, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OUDiv: {
        /* dst = a / b (unsigned) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildUDiv(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OSMod: {
        /* dst = a % b (signed) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        hl_type *t = f->regs[dst];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFRem(ctx->builder, a, b, "");
        } else {
            result = LLVMBuildSRem(ctx->builder, a, b, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OUMod: {
        /* dst = a % b (unsigned) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildURem(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OShl: {
        /* dst = a << b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildShl(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OSShr: {
        /* dst = a >> b (signed/arithmetic) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildAShr(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OUShr: {
        /* dst = a >>> b (unsigned/logical) */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildLShr(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OAnd: {
        /* dst = a & b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildAnd(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OOr: {
        /* dst = a | b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildOr(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OXor: {
        /* dst = a ^ b */
        int dst = op->p1;
        int ra = op->p2;
        int rb = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef result = LLVMBuildXor(ctx->builder, a, b, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case ONeg: {
        /* dst = -src */
        int dst = op->p1;
        int src = op->p2;
        hl_type *t = f->regs[dst];
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            result = LLVMBuildFNeg(ctx->builder, val, "");
        } else {
            result = LLVMBuildNeg(ctx->builder, val, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case ONot: {
        /* dst = ~src (bitwise not) */
        int dst = op->p1;
        int src = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef minus_one = LLVMConstInt(LLVMTypeOf(val), -1, true);
        LLVMValueRef result = LLVMBuildXor(ctx->builder, val, minus_one, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OIncr: {
        /* dst++ (in-place increment) */
        int dst = op->p1;
        hl_type *t = f->regs[dst];
        LLVMValueRef val = llvm_load_vreg(ctx, f, dst);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            LLVMValueRef one = LLVMConstReal(llvm_get_type(ctx, t), 1.0);
            result = LLVMBuildFAdd(ctx->builder, val, one, "");
        } else {
            LLVMValueRef one = LLVMConstInt(llvm_get_type(ctx, t), 1, false);
            result = LLVMBuildAdd(ctx->builder, val, one, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case ODecr: {
        /* dst-- (in-place decrement) */
        int dst = op->p1;
        hl_type *t = f->regs[dst];
        LLVMValueRef val = llvm_load_vreg(ctx, f, dst);
        LLVMValueRef result;
        if (llvm_is_float_type(t)) {
            LLVMValueRef one = LLVMConstReal(llvm_get_type(ctx, t), 1.0);
            result = LLVMBuildFSub(ctx->builder, val, one, "");
        } else {
            LLVMValueRef one = LLVMConstInt(llvm_get_type(ctx, t), 1, false);
            result = LLVMBuildSub(ctx->builder, val, one, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    default:
        break;
    }
}
