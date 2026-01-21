/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Exception Handling Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_exceptions(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OTrap: {
        /*
         * Setup exception trap using setjmp/longjmp.
         *
         * This mirrors the hl_trap macro from hl.h:
         *   ctx.tcheck = NULL;
         *   ctx.prev = __tinf->trap_current;
         *   __tinf->trap_current = &ctx;
         *   if (setjmp(ctx.buf)) { r = __tinf->exc_value; goto label; }
         *
         * hl_trap_ctx layout:
         *   jmp_buf buf;        // offset 0
         *   hl_trap_ctx *prev;  // offset sizeof(jmp_buf)
         *   vdynamic *tcheck;   // offset sizeof(jmp_buf) + 8
         */
        int exc_reg = op->p1;
        int handler_offset = op->p2;

        /* Compute offsets using NULL pointer trick (like x86/aarch64 JIT) */
        hl_trap_ctx *t = NULL;
        hl_thread_info *tinf = NULL;
        int offset_trap_current = (int)(int_val)&tinf->trap_current;
        int offset_exc_value = (int)(int_val)&tinf->exc_value;
        int offset_prev = (int)(int_val)&t->prev;
        int offset_tcheck = (int)(int_val)&t->tcheck;

        /* Allocate hl_trap_ctx on stack in entry block to avoid stack growth in loops */
        /* Use sizeof(hl_trap_ctx) rounded up to 16-byte alignment */
        int trap_size = (sizeof(hl_trap_ctx) + 15) & ~15;
        LLVMTypeRef trap_type = LLVMArrayType(ctx->i8_type, trap_size);
        LLVMValueRef trap = llvm_create_entry_alloca(ctx, trap_type, "trap_ctx");

        /* Step 1: Call hl_get_thread() to get thread info pointer */
        LLVMValueRef thread = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_thread),
            ctx->rt_get_thread, NULL, 0, "thread");

        /* Step 2: trap->tcheck = NULL */
        LLVMValueRef tcheck_offset = LLVMConstInt(ctx->i64_type, offset_tcheck, false);
        LLVMValueRef tcheck_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            trap, &tcheck_offset, 1, "tcheck_ptr");
        LLVMBuildStore(ctx->builder, LLVMConstNull(ctx->ptr_type), tcheck_ptr);

        /* Step 3: trap->prev = thread->trap_current */
        LLVMValueRef trap_current_offset = LLVMConstInt(ctx->i64_type, offset_trap_current, false);
        LLVMValueRef trap_current_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            thread, &trap_current_offset, 1, "trap_current_ptr");
        LLVMValueRef old_trap = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, trap_current_ptr, "old_trap");

        LLVMValueRef prev_offset = LLVMConstInt(ctx->i64_type, offset_prev, false);
        LLVMValueRef prev_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            trap, &prev_offset, 1, "prev_ptr");
        LLVMBuildStore(ctx->builder, old_trap, prev_ptr);

        /* Step 4: thread->trap_current = trap */
        LLVMBuildStore(ctx->builder, trap, trap_current_ptr);

        /* Step 5: Call setjmp(trap->buf) - buf is at offset 0, so trap ptr = buf ptr */
        LLVMValueRef setjmp_args[] = { trap };
        LLVMTypeRef setjmp_fn_type = LLVMFunctionType(ctx->i32_type,
            (LLVMTypeRef[]){ ctx->ptr_type }, 1, false);
        LLVMValueRef setjmp_result = LLVMBuildCall2(ctx->builder, setjmp_fn_type,
            ctx->rt_setjmp, setjmp_args, 1, "setjmp_result");

        /* Mark setjmp call as returns_twice */
        LLVMSetInstructionCallConv(setjmp_result, LLVMCCallConv);

        /* Step 6: Branch based on setjmp result */
        /* If setjmp returns 0: continue normally */
        /* If setjmp returns non-zero: exception was caught */
        LLVMValueRef zero = LLVMConstInt(ctx->i32_type, 0, false);
        LLVMValueRef caught = LLVMBuildICmp(ctx->builder, LLVMIntNE, setjmp_result, zero, "caught");

        /* Create basic blocks for the two paths */
        /* We need to create new blocks because the current block is being terminated */
        LLVMBasicBlockRef caught_bb = LLVMAppendBasicBlockInContext(ctx->context,
            ctx->current_function, "trap_caught");
        LLVMBasicBlockRef continue_bb = LLVMAppendBasicBlockInContext(ctx->context,
            ctx->current_function, "trap_continue");
        LLVMBasicBlockRef handler_bb = llvm_get_block_for_offset(ctx, op_idx, handler_offset);

        LLVMBuildCondBr(ctx->builder, caught, caught_bb, continue_bb);

        /* Step 7: In caught_bb, load exception and jump to handler */
        LLVMPositionBuilderAtEnd(ctx->builder, caught_bb);

        /* Call hl_get_thread() again to get fresh thread pointer */
        LLVMValueRef thread2 = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_thread),
            ctx->rt_get_thread, NULL, 0, "thread2");

        /* Load exc_value from thread */
        LLVMValueRef exc_value_offset = LLVMConstInt(ctx->i64_type, offset_exc_value, false);
        LLVMValueRef exc_value_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            thread2, &exc_value_offset, 1, "exc_value_ptr");
        LLVMValueRef exc_value = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, exc_value_ptr, "exc_value");

        /* Store exception to destination register */
        llvm_store_vreg(ctx, f, exc_reg, exc_value);

        /* Jump to handler */
        if (handler_bb) {
            LLVMBuildBr(ctx->builder, handler_bb);
        } else {
            LLVMBuildUnreachable(ctx->builder);
        }

        /* Position builder at continue_bb for subsequent opcodes */
        LLVMPositionBuilderAtEnd(ctx->builder, continue_bb);
        break;
    }

    case OEndTrap: {
        /*
         * End exception trap - restore previous trap context.
         *
         * This mirrors the hl_endtrap macro from hl.h:
         *   hl_get_thread()->trap_current = ctx.prev
         *
         * We need to:
         * 1. Get thread info
         * 2. Load current trap from thread->trap_current
         * 3. Load prev from trap->prev
         * 4. Store prev to thread->trap_current
         */

        /* Compute offsets using NULL pointer trick */
        hl_trap_ctx *t = NULL;
        hl_thread_info *tinf = NULL;
        int offset_trap_current = (int)(int_val)&tinf->trap_current;
        int offset_prev = (int)(int_val)&t->prev;

        /* Step 1: Call hl_get_thread() */
        LLVMValueRef thread = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_thread),
            ctx->rt_get_thread, NULL, 0, "thread");

        /* Step 2: Load current trap = thread->trap_current */
        LLVMValueRef trap_current_offset = LLVMConstInt(ctx->i64_type, offset_trap_current, false);
        LLVMValueRef trap_current_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            thread, &trap_current_offset, 1, "trap_current_ptr");
        LLVMValueRef current_trap = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, trap_current_ptr, "current_trap");

        /* Step 3: Load prev = trap->prev */
        LLVMValueRef prev_offset = LLVMConstInt(ctx->i64_type, offset_prev, false);
        LLVMValueRef prev_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            current_trap, &prev_offset, 1, "prev_ptr");
        LLVMValueRef prev_trap = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, prev_ptr, "prev_trap");

        /* Step 4: thread->trap_current = prev */
        LLVMBuildStore(ctx->builder, prev_trap, trap_current_ptr);

        /* Continue to next instruction (implicit fallthrough) */
        LLVMBasicBlockRef next_bb = llvm_get_block_for_offset(ctx, op_idx, 0);
        if (next_bb) {
            llvm_ensure_block_terminated(ctx, next_bb);
        }
        break;
    }

    case OThrow: {
        /* Throw exception - calls hl_throw (noreturn) */
        int exc = op->p1;
        LLVMValueRef exc_val = llvm_load_vreg(ctx, f, exc);

        LLVMValueRef args[] = { exc_val };
        LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_throw),
            ctx->rt_throw, args, 1, "");
        LLVMBuildUnreachable(ctx->builder);
        break;
    }

    case ORethrow: {
        /* Rethrow exception - calls hl_rethrow (noreturn) */
        int exc = op->p1;
        LLVMValueRef exc_val = llvm_load_vreg(ctx, f, exc);

        LLVMValueRef args[] = { exc_val };
        LLVMBuildCall2(ctx->builder, LLVMGlobalGetValueType(ctx->rt_rethrow),
            ctx->rt_rethrow, args, 1, "");
        LLVMBuildUnreachable(ctx->builder);
        break;
    }

    case OCatch: {
        /*
         * Get caught exception from thread info.
         * OCatch is used for typing purposes by OTrap - in most cases
         * the exception is already loaded by OTrap's caught path.
         * But we implement it properly in case it's used standalone.
         */
        int dst = op->p1;

        /* Compute offset using NULL pointer trick */
        hl_thread_info *tinf = NULL;
        int offset_exc_value = (int)(int_val)&tinf->exc_value;

        /* Call hl_get_thread() to get thread info */
        LLVMValueRef thread = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_thread),
            ctx->rt_get_thread, NULL, 0, "thread");

        /* Load exc_value from thread */
        LLVMValueRef exc_off_val = LLVMConstInt(ctx->i64_type, offset_exc_value, false);
        LLVMValueRef exc_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            thread, &exc_off_val, 1, "exc_ptr");
        LLVMValueRef exc = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, exc_ptr, "exc_value");
        llvm_store_vreg(ctx, f, dst, exc);
        break;
    }

    default:
        break;
    }
}
