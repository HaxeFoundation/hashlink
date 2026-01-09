/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Reference Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_refs(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case ORef: {
        /* dst = &src (create reference to value) */
        int dst = op->p1;
        int src = op->p2;

        /* References are represented as pointers to the vreg alloca */
        /* Get the address of the source vreg */
        LLVMValueRef ref_ptr = ctx->vreg_allocs[src];
        llvm_store_vreg(ctx, f, dst, ref_ptr);
        break;
    }

    case OUnref: {
        /* dst = *src (dereference) */
        int dst = op->p1;
        int src = op->p2;
        hl_type *dst_type = f->regs[dst];
        LLVMTypeRef val_type = llvm_get_type(ctx, dst_type);

        LLVMValueRef ref_ptr = llvm_load_vreg(ctx, f, src);
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, val_type, ref_ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OSetref: {
        /* *dst = src */
        int dst = op->p1;
        int src = op->p2;

        LLVMValueRef ref_ptr = llvm_load_vreg(ctx, f, dst);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMBuildStore(ctx->builder, val, ref_ptr);
        break;
    }

    case ORefData: {
        /* dst = bytes.data (get pointer to bytes/array data) */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];

        LLVMValueRef obj = llvm_load_vreg(ctx, f, src);

        /* For bytes, data is the pointer itself */
        /* For arrays, data starts at offset 16 (after type and size) */
        if (src_type->kind == HARRAY) {
            LLVMValueRef data_offset = LLVMConstInt(ctx->i64_type, 16, false);
            LLVMValueRef data_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
                obj, &data_offset, 1, "");
            llvm_store_vreg(ctx, f, dst, data_ptr);
        } else {
            /* For bytes/other, just return the pointer */
            llvm_store_vreg(ctx, f, dst, obj);
        }
        break;
    }

    case ORefOffset: {
        /* dst = src + offset (pointer arithmetic) */
        int dst = op->p1;
        int src = op->p2;
        int offset_reg = op->p3;
        hl_type *dst_type = f->regs[dst];

        LLVMValueRef base = llvm_load_vreg(ctx, f, src);
        LLVMValueRef offset = llvm_load_vreg(ctx, f, offset_reg);

        /* Determine element size from destination type */
        int elem_size = 1; /* Default to byte size */
        if (dst_type->kind == HREF && dst_type->tparam) {
            elem_size = llvm_type_size(ctx, dst_type->tparam);
        }

        /* Calculate byte offset */
        LLVMValueRef byte_offset;
        if (elem_size == 1) {
            byte_offset = offset;
        } else {
            LLVMValueRef size_val = LLVMConstInt(ctx->i32_type, elem_size, false);
            byte_offset = LLVMBuildMul(ctx->builder, offset, size_val, "");
        }

        /* Extend to i64 for GEP */
        LLVMValueRef byte_offset64 = LLVMBuildSExt(ctx->builder, byte_offset, ctx->i64_type, "");

        LLVMValueRef result = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            base, &byte_offset64, 1, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    default:
        break;
    }
}
