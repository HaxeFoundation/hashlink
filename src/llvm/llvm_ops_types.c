/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Type Conversion and Casting Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_types(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OType: {
        /* dst = type pointer for type index p2 */
        int dst = op->p1;
        int type_idx = op->p2;
        LLVMValueRef type_ptr = llvm_get_type_ptr(ctx, type_idx);
        llvm_store_vreg(ctx, f, dst, type_ptr);
        break;
    }

    case OGetType: {
        /* dst = obj->t (type of object) */
        int dst = op->p1;
        int src = op->p2;
        LLVMValueRef obj = llvm_load_vreg(ctx, f, src);
        /* First field of any object is hl_type* t */
        LLVMValueRef type_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, obj, "");
        llvm_store_vreg(ctx, f, dst, type_ptr);
        break;
    }

    case OGetTID: {
        /* dst = type->kind */
        int dst = op->p1;
        int src = op->p2;
        LLVMValueRef type_ptr = llvm_load_vreg(ctx, f, src);
        /* hl_type has kind as first field (int) */
        LLVMValueRef kind = LLVMBuildLoad2(ctx->builder, ctx->i32_type, type_ptr, "");
        llvm_store_vreg(ctx, f, dst, kind);
        break;
    }

    case OToDyn: {
        /* dst = wrap value as dynamic */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];

        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* For pointer types, just use the value directly */
        if (llvm_is_ptr_type(src_type)) {
            llvm_store_vreg(ctx, f, dst, val);
        } else {
            /* For value types, need to call hl_make_dyn */
            /* First store value to memory, then pass pointer */
            /* Use entry block alloca to avoid stack growth in loops */
            LLVMValueRef val_alloca = llvm_create_entry_alloca(ctx,
                llvm_get_type(ctx, src_type), "todyn_tmp");
            LLVMBuildStore(ctx->builder, val, val_alloca);

            /* Get type pointer for the source type */
            int type_idx = -1;
            for (int i = 0; i < ctx->code->ntypes; i++) {
                if (ctx->code->types + i == src_type) {
                    type_idx = i;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);

            LLVMValueRef args[] = { val_alloca, type_ptr };
            LLVMValueRef result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_make_dyn),
                ctx->rt_make_dyn, args, 2, "");
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OToSFloat: {
        /* dst = (float)src - convert to float type */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];
        hl_type *dst_type = f->regs[dst];
        LLVMTypeRef target_type = llvm_get_type(ctx, dst_type);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        LLVMValueRef result;
        if (llvm_is_float_type(src_type)) {
            /* Float to float - use fptrunc or fpext */
            if (src_type->kind == HF64 && dst_type->kind == HF32) {
                result = LLVMBuildFPTrunc(ctx->builder, val, target_type, "");
            } else if (src_type->kind == HF32 && dst_type->kind == HF64) {
                result = LLVMBuildFPExt(ctx->builder, val, target_type, "");
            } else {
                result = val; /* Same type */
            }
        } else {
            /* Integer to float - use sitofp */
            result = LLVMBuildSIToFP(ctx->builder, val, target_type, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OToUFloat: {
        /* dst = (float)src - convert to float type (unsigned source) */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];
        hl_type *dst_type = f->regs[dst];
        LLVMTypeRef target_type = llvm_get_type(ctx, dst_type);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        LLVMValueRef result;
        if (llvm_is_float_type(src_type)) {
            /* Float to float - use fptrunc or fpext */
            if (src_type->kind == HF64 && dst_type->kind == HF32) {
                result = LLVMBuildFPTrunc(ctx->builder, val, target_type, "");
            } else if (src_type->kind == HF32 && dst_type->kind == HF64) {
                result = LLVMBuildFPExt(ctx->builder, val, target_type, "");
            } else {
                result = val; /* Same type */
            }
        } else {
            /* Unsigned integer to float - use uitofp */
            result = LLVMBuildUIToFP(ctx->builder, val, target_type, "");
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OToInt: {
        /* dst = (int)src */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];
        hl_type *dst_type = f->regs[dst];
        LLVMTypeRef target_type = llvm_get_type(ctx, dst_type);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        LLVMValueRef result;
        if (llvm_is_float_type(src_type)) {
            result = LLVMBuildFPToSI(ctx->builder, val, target_type, "");
        } else {
            /* Integer to integer - handle width differences */
            unsigned src_bits = LLVMGetIntTypeWidth(LLVMTypeOf(val));
            unsigned dst_bits = LLVMGetIntTypeWidth(target_type);
            if (src_bits > dst_bits) {
                result = LLVMBuildTrunc(ctx->builder, val, target_type, "");
            } else if (src_bits < dst_bits) {
                result = LLVMBuildSExt(ctx->builder, val, target_type, "");
            } else {
                result = val;
            }
        }
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OSafeCast: {
        /*
         * dst = safe_cast(src, target_type)
         *
         * Different cast functions are used based on destination type:
         * - hl_dyn_casti(ptr, src_type, dst_type) -> i32 for HBOOL, HI32, HUI8, HUI16
         * - hl_dyn_casti64(ptr, src_type) -> i64 for HI64
         * - hl_dyn_castf(ptr, src_type) -> f32 for HF32
         * - hl_dyn_castd(ptr, src_type) -> f64 for HF64
         * - hl_dyn_castp(ptr, src_type, dst_type) -> ptr for pointer types
         */
        int dst = op->p1;
        int src = op->p2;
        hl_type *src_type = f->regs[src];
        hl_type *dst_type = f->regs[dst];

        /* Get source type pointer (static compile-time type) */
        int src_type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == src_type) {
                src_type_idx = i;
                break;
            }
        }
        LLVMValueRef src_type_ptr = src_type_idx >= 0 ? llvm_get_type_ptr(ctx, src_type_idx)
            : LLVMConstNull(ctx->ptr_type);

        /* Get destination type pointer (needed for some cast functions) */
        int dst_type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == dst_type) {
                dst_type_idx = i;
                break;
            }
        }
        LLVMValueRef dst_type_ptr = dst_type_idx >= 0 ? llvm_get_type_ptr(ctx, dst_type_idx)
            : LLVMConstNull(ctx->ptr_type);

        /* Get address of source vreg (pointer to stack slot containing the value) */
        LLVMValueRef src_addr = ctx->vreg_allocs[src];

        LLVMValueRef result;
        switch (dst_type->kind) {
        case HF32: {
            /* hl_dyn_castf(ptr, src_type) -> float */
            LLVMValueRef args[] = { src_addr, src_type_ptr };
            result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_dyn_castf),
                ctx->rt_dyn_castf, args, 2, "");
            break;
        }
        case HF64: {
            /* hl_dyn_castd(ptr, src_type) -> double */
            LLVMValueRef args[] = { src_addr, src_type_ptr };
            result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_dyn_castd),
                ctx->rt_dyn_castd, args, 2, "");
            break;
        }
        case HI64: {
            /* hl_dyn_casti64(ptr, src_type) -> i64 */
            LLVMValueRef args[] = { src_addr, src_type_ptr };
            result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_dyn_casti64),
                ctx->rt_dyn_casti64, args, 2, "");
            break;
        }
        case HI32:
        case HUI16:
        case HUI8:
        case HBOOL: {
            /* hl_dyn_casti(ptr, src_type, dst_type) -> i32 */
            LLVMValueRef args[] = { src_addr, src_type_ptr, dst_type_ptr };
            LLVMValueRef i32_result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_dyn_casti),
                ctx->rt_dyn_casti, args, 3, "");
            /* Truncate to actual destination size */
            LLVMTypeRef target_type = llvm_get_type(ctx, dst_type);
            if (dst_type->kind == HBOOL || dst_type->kind == HUI8) {
                result = LLVMBuildTrunc(ctx->builder, i32_result, target_type, "");
            } else if (dst_type->kind == HUI16) {
                result = LLVMBuildTrunc(ctx->builder, i32_result, target_type, "");
            } else {
                result = i32_result;
            }
            break;
        }
        default: {
            /* hl_dyn_castp(ptr, src_type, dst_type) -> ptr */
            LLVMValueRef args[] = { src_addr, src_type_ptr, dst_type_ptr };
            result = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_dyn_castp),
                ctx->rt_dyn_castp, args, 3, "");
            break;
        }
        }

        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OUnsafeCast: {
        /* dst = (dst_type)src - no runtime check */
        int dst = op->p1;
        int src = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        /* Just pass through for pointer types */
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OToVirtual: {
        /* dst = to_virtual(src, virtual_type) */
        int dst = op->p1;
        int src = op->p2;
        hl_type *dst_type = f->regs[dst];

        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Get destination type pointer */
        int type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == dst_type) {
                type_idx = i;
                break;
            }
        }
        LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
            : LLVMConstNull(ctx->ptr_type);

        /* Call hl_to_virtual */
        LLVMValueRef args[] = { type_ptr, val };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_to_virtual),
            ctx->rt_to_virtual, args, 2, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    default:
        break;
    }
}
