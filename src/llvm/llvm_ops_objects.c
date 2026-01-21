/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Object and Dynamic Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_objects(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case ONew: {
        /* dst = new object of type from R(dst) */
        int dst = op->p1;
        hl_type *t = f->regs[dst];

        /* Get type pointer */
        int type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == t) {
                type_idx = i;
                break;
            }
        }
        LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
            : LLVMConstNull(ctx->ptr_type);

        LLVMValueRef result;
        switch (t->kind) {
        case HOBJ:
        case HSTRUCT: {
            /* Call hl_alloc_obj(type) */
            LLVMValueRef args[] = { type_ptr };
            LLVMTypeRef fn_type = LLVMGlobalGetValueType(ctx->rt_alloc_obj);
            result = LLVMBuildCall2(ctx->builder, fn_type,
                ctx->rt_alloc_obj, args, 1, "");
            break;
        }
        case HDYNOBJ: {
            /* Call hl_alloc_dynobj() - no arguments */
            LLVMTypeRef fn_type = LLVMGlobalGetValueType(ctx->rt_alloc_dynobj);
            result = LLVMBuildCall2(ctx->builder, fn_type,
                ctx->rt_alloc_dynobj, NULL, 0, "");
            break;
        }
        case HVIRTUAL: {
            /* Call hl_alloc_virtual */
            LLVMValueRef args[] = { type_ptr };
            LLVMTypeRef fn_type = LLVMGlobalGetValueType(ctx->rt_alloc_virtual);
            result = LLVMBuildCall2(ctx->builder, fn_type,
                ctx->rt_alloc_virtual, args, 1, "");
            break;
        }
        default:
            result = LLVMConstNull(ctx->ptr_type);
            break;
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OArraySize: {
        /* dst = array.length */
        int dst = op->p1;
        int arr = op->p2;
        LLVMValueRef arr_ptr = llvm_load_vreg(ctx, f, arr);

        /* varray layout: hl_type* t (0), hl_type* at (8), int size (16), int __pad (20) */
        LLVMValueRef size_offset = LLVMConstInt(ctx->i64_type, 16, false);
        LLVMValueRef size_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            arr_ptr, &size_offset, 1, "");
        LLVMValueRef size = LLVMBuildLoad2(ctx->builder, ctx->i32_type, size_ptr, "");
        llvm_store_vreg(ctx, f, dst, size);
        break;
    }

    case ODynGet: {
        /* dst = obj.field (dynamic field access by hash) */
        int dst = op->p1;
        int obj = op->p2;
        int field_hash = op->p3;
        hl_type *dst_type = f->regs[dst];

        LLVMValueRef obj_ptr = llvm_load_vreg(ctx, f, obj);
        LLVMValueRef hash_val = LLVMConstInt(ctx->i32_type, field_hash, false);

        /* Choose the right getter based on destination type */
        LLVMValueRef result;
        switch (dst_type->kind) {
        case HI32:
        case HUI8:
        case HUI16:
        case HBOOL: {
            /* Get type pointer for dst */
            int type_idx = -1;
            for (int i = 0; i < ctx->code->ntypes; i++) {
                if (ctx->code->types + i == dst_type) {
                    type_idx = i;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);
            LLVMValueRef args[] = { obj_ptr, hash_val, type_ptr };
            result = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_geti),
                ctx->rt_dyn_geti, args, 3, "");
            break;
        }
        case HI64: {
            LLVMValueRef args[] = { obj_ptr, hash_val };
            result = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_geti64),
                ctx->rt_dyn_geti64, args, 2, "");
            break;
        }
        case HF32: {
            LLVMValueRef args[] = { obj_ptr, hash_val };
            result = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_getf),
                ctx->rt_dyn_getf, args, 2, "");
            break;
        }
        case HF64: {
            LLVMValueRef args[] = { obj_ptr, hash_val };
            result = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_getd),
                ctx->rt_dyn_getd, args, 2, "");
            break;
        }
        default: {
            /* Pointer type */
            int type_idx = -1;
            for (int i = 0; i < ctx->code->ntypes; i++) {
                if (ctx->code->types + i == dst_type) {
                    type_idx = i;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);
            LLVMValueRef args[] = { obj_ptr, hash_val, type_ptr };
            result = LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_getp),
                ctx->rt_dyn_getp, args, 3, "");
            break;
        }
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case ODynSet: {
        /* obj.field = val (dynamic field set by hash) */
        int obj = op->p1;
        int field_hash = op->p2;
        int src = op->p3;
        hl_type *src_type = f->regs[src];

        LLVMValueRef obj_ptr = llvm_load_vreg(ctx, f, obj);
        LLVMValueRef hash_val = LLVMConstInt(ctx->i32_type, field_hash, false);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Choose the right setter based on source type */
        switch (src_type->kind) {
        case HI32:
        case HUI8:
        case HUI16:
        case HBOOL: {
            int type_idx = -1;
            for (int i = 0; i < ctx->code->ntypes; i++) {
                if (ctx->code->types + i == src_type) {
                    type_idx = i;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);
            /* Extend smaller integer types to i32 for hl_dyn_seti */
            LLVMValueRef val_i32 = val;
            if (src_type->kind == HUI8 || src_type->kind == HBOOL) {
                val_i32 = LLVMBuildZExt(ctx->builder, val, ctx->i32_type, "ext_i32");
            } else if (src_type->kind == HUI16) {
                val_i32 = LLVMBuildZExt(ctx->builder, val, ctx->i32_type, "ext_i32");
            }
            LLVMValueRef args[] = { obj_ptr, hash_val, type_ptr, val_i32 };
            LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_seti),
                ctx->rt_dyn_seti, args, 4, "");
            break;
        }
        case HI64: {
            LLVMValueRef args[] = { obj_ptr, hash_val, val };
            LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_seti64),
                ctx->rt_dyn_seti64, args, 3, "");
            break;
        }
        case HF32: {
            LLVMValueRef args[] = { obj_ptr, hash_val, val };
            LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_setf),
                ctx->rt_dyn_setf, args, 3, "");
            break;
        }
        case HF64: {
            LLVMValueRef args[] = { obj_ptr, hash_val, val };
            LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_setd),
                ctx->rt_dyn_setd, args, 3, "");
            break;
        }
        default: {
            int type_idx = -1;
            for (int i = 0; i < ctx->code->ntypes; i++) {
                if (ctx->code->types + i == src_type) {
                    type_idx = i;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);
            LLVMValueRef args[] = { obj_ptr, hash_val, type_ptr, val };
            LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_dyn_setp),
                ctx->rt_dyn_setp, args, 4, "");
            break;
        }
        }
        break;
    }

    default:
        break;
    }
}
