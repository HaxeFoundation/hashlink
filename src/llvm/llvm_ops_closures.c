/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Closure Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_closures(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OStaticClosure: {
        /* dst = closure for function findex (no captured value) */
        int dst = op->p1;
        int findex = op->p2;

        /* Get function pointer */
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);

        /* Get type pointer from destination register - this is the closure's type
         * as determined by the Haxe compiler, not the function's internal type */
        hl_type *closure_type = f->regs[dst];
        int type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == closure_type) {
                type_idx = i;
                break;
            }
        }
        LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx) : LLVMConstNull(ctx->ptr_type);

        /* Call hl_alloc_closure_void(type, fun) */
        LLVMValueRef args[] = { type_ptr, func };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_alloc_closure_void),
            ctx->rt_alloc_closure_void, args, 2, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OInstanceClosure: {
        /* dst = closure for function findex with captured object */
        int dst = op->p1;
        int findex = op->p2;
        int obj = op->p3;

        /* Get function pointer */
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);

        /* Get captured object */
        LLVMValueRef obj_val = llvm_load_vreg(ctx, f, obj);

        /* Get type pointer from destination register - this is the closure's type
         * as determined by the Haxe compiler, not the function's internal type */
        hl_type *closure_type = f->regs[dst];
        int type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == closure_type) {
                type_idx = i;
                break;
            }
        }
        LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx) : LLVMConstNull(ctx->ptr_type);

        /* Call hl_alloc_closure_ptr(type, fun, obj) */
        LLVMValueRef args[] = { type_ptr, func, obj_val };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_alloc_closure_ptr),
            ctx->rt_alloc_closure_ptr, args, 3, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    case OVirtualClosure: {
        /* dst = virtual method closure with captured object */
        int dst = op->p1;
        int obj = op->p2;
        int method_idx = op->p3;

        LLVMValueRef obj_val = llvm_load_vreg(ctx, f, obj);
        hl_type *obj_type = f->regs[obj];

        /* Load type pointer from object */
        LLVMValueRef type_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, obj_val, "");

        /* Get runtime object */
        LLVMValueRef rt_args[] = { type_ptr };
        LLVMValueRef rt = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_obj_rt),
            ctx->rt_get_obj_rt, rt_args, 1, "");

        /* Load method from proto array */
        int proto_offset = 8 + 4 * 7; /* Approximate offset to proto array */
        LLVMValueRef proto_off_val = LLVMConstInt(ctx->i64_type, proto_offset, false);
        LLVMValueRef proto_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
            LLVMBuildGEP2(ctx->builder, ctx->i8_type, rt, &proto_off_val, 1, ""), "");

        int entry_size = 32;
        LLVMValueRef method_off = LLVMConstInt(ctx->i64_type, method_idx * entry_size, false);
        LLVMValueRef method_entry = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            proto_ptr, &method_off, 1, "");
        LLVMValueRef fptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, method_entry, "");

        /* Get closure type from destination register - this is the correct type
         * as determined by the Haxe compiler */
        hl_type *closure_type = f->regs[dst];
        int type_idx = -1;
        for (int i = 0; i < ctx->code->ntypes; i++) {
            if (ctx->code->types + i == closure_type) {
                type_idx = i;
                break;
            }
        }
        LLVMValueRef closure_type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx) : type_ptr;

        /* Call hl_alloc_closure_ptr(type, fun, obj) */
        LLVMValueRef args[] = { closure_type_ptr, fptr, obj_val };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_alloc_closure_ptr),
            ctx->rt_alloc_closure_ptr, args, 3, "");
        llvm_store_vreg(ctx, f, dst, result);
        break;
    }

    default:
        break;
    }
}
