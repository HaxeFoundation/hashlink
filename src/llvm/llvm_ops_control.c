/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Control Flow Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_control_flow(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OLabel: {
        /* Label is handled by basic block creation in pre-scan */
        /* Just ensure we're in the right block */
        break;
    }

    case ORet: {
        /* Return from function */
        int src = op->p1;
        hl_type *ret_type = f->type->fun->ret;
        if (ret_type->kind == HVOID) {
            LLVMBuildRetVoid(ctx->builder);
        } else {
            LLVMValueRef val = llvm_load_vreg(ctx, f, src);
            LLVMBuildRet(ctx->builder, val);
        }
        break;
    }

    case OJAlways: {
        /* Unconditional jump */
        int offset = op->p1;
        LLVMBasicBlockRef target = llvm_get_block_for_offset(ctx, op_idx, offset);
        if (target) {
            LLVMBuildBr(ctx->builder, target);
        }
        break;
    }

    case OJTrue: {
        /* Jump if true (non-zero) */
        int cond_reg = op->p1;
        int offset = op->p2;
        LLVMValueRef cond = llvm_load_vreg(ctx, f, cond_reg);
        LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(cond), 0, false);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntNE, cond, zero, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJFalse: {
        /* Jump if false (zero) */
        int cond_reg = op->p1;
        int offset = op->p2;
        LLVMValueRef cond = llvm_load_vreg(ctx, f, cond_reg);
        LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(cond), 0, false);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, cond, zero, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJNull: {
        /* Jump if null */
        int src = op->p1;
        int offset = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef null_val = LLVMConstNull(ctx->ptr_type);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, val, null_val, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJNotNull: {
        /* Jump if not null */
        int src = op->p1;
        int offset = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef null_val = LLVMConstNull(ctx->ptr_type);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntNE, val, null_val, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJEq: {
        /* Jump if a == b */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntEQ, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJNotEq: {
        /* Jump if a != b */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealONE, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntNE, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJSLt: {
        /* Jump if a < b (signed) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealOLT, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLT, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJSGte: {
        /* Jump if a >= b (signed) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealOGE, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGE, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJSGt: {
        /* Jump if a > b (signed) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealOGT, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGT, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJSLte: {
        /* Jump if a <= b (signed) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealOLE, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLE, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJULt: {
        /* Jump if a < b (unsigned) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntULT, a, b, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJUGte: {
        /* Jump if a >= b (unsigned) */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp = LLVMBuildICmp(ctx->builder, LLVMIntUGE, a, b, "");
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJNotLt: {
        /* Jump if !(a < b) - NaN-aware for floats */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            /* For floats, use unordered-or-greater-or-equal for NaN handling */
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealUGE, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSGE, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OJNotGte: {
        /* Jump if !(a >= b) - NaN-aware for floats */
        int ra = op->p1;
        int rb = op->p2;
        int offset = op->p3;
        hl_type *t = f->regs[ra];
        LLVMValueRef a = llvm_load_vreg(ctx, f, ra);
        LLVMValueRef b = llvm_load_vreg(ctx, f, rb);
        LLVMValueRef cmp;
        if (llvm_is_float_type(t)) {
            /* For floats, use unordered-or-less-than for NaN handling */
            cmp = LLVMBuildFCmp(ctx->builder, LLVMRealULT, a, b, "");
        } else {
            cmp = LLVMBuildICmp(ctx->builder, LLVMIntSLT, a, b, "");
        }
        LLVMBasicBlockRef then_bb = llvm_get_block_for_offset(ctx, op_idx, offset);
        LLVMBasicBlockRef else_bb = llvm_get_block_for_offset(ctx, op_idx, 0); /* fallthrough */
        if (then_bb && else_bb) {
            LLVMBuildCondBr(ctx->builder, cmp, then_bb, else_bb);
        }
        break;
    }

    case OSwitch: {
        /* Multi-way branch */
        int src = op->p1;
        int ncases = op->p2;
        int default_offset = op->p3;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Ensure switch value is i32 */
        LLVMTypeRef val_type = LLVMTypeOf(val);
        if (LLVMGetTypeKind(val_type) != LLVMIntegerTypeKind ||
            LLVMGetIntTypeWidth(val_type) != 32) {
            val = LLVMBuildIntCast2(ctx->builder, val, ctx->i32_type, false, "");
        }

        LLVMBasicBlockRef default_bb = llvm_get_block_for_offset(ctx, op_idx, default_offset);
        if (!default_bb) {
            /* Create fallback default block with unreachable */
            default_bb = LLVMAppendBasicBlockInContext(ctx->context,
                ctx->current_function, "switch.default");
            LLVMBasicBlockRef current = LLVMGetInsertBlock(ctx->builder);
            LLVMPositionBuilderAtEnd(ctx->builder, default_bb);
            LLVMBuildUnreachable(ctx->builder);
            LLVMPositionBuilderAtEnd(ctx->builder, current);
        }

        LLVMValueRef switch_inst = LLVMBuildSwitch(ctx->builder, val, default_bb, ncases);
        for (int i = 0; i < ncases; i++) {
            int case_offset = op->extra[i];
            LLVMBasicBlockRef case_bb = llvm_get_block_for_offset(ctx, op_idx, case_offset);
            if (case_bb) {
                LLVMValueRef case_val = LLVMConstInt(ctx->i32_type, i, false);
                LLVMAddCase(switch_inst, case_val, case_bb);
            }
        }
        break;
    }

    case ONullCheck: {
        /* Null pointer check - call hl_null_access if null */
        int src = op->p1;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef null_val = LLVMConstNull(ctx->ptr_type);
        LLVMValueRef is_null = LLVMBuildICmp(ctx->builder, LLVMIntEQ, val, null_val, "");

        LLVMBasicBlockRef null_bb = LLVMAppendBasicBlockInContext(ctx->context,
            ctx->current_function, "nullcheck.fail");
        LLVMBasicBlockRef ok_bb = LLVMAppendBasicBlockInContext(ctx->context,
            ctx->current_function, "nullcheck.ok");

        LLVMBuildCondBr(ctx->builder, is_null, null_bb, ok_bb);

        LLVMPositionBuilderAtEnd(ctx->builder, null_bb);
        LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_null_access),
            ctx->rt_null_access, NULL, 0, "");
        LLVMBuildUnreachable(ctx->builder);

        LLVMPositionBuilderAtEnd(ctx->builder, ok_bb);
        break;
    }

    default:
        break;
    }
}
