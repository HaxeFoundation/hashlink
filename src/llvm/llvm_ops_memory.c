/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Memory Access Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_memory(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OGetGlobal: {
        /*
         * dst = globals[idx]
         *
         * Call aot_get_global(idx) to get pointer to the global's storage.
         * This returns &aot_globals[idx] where aot_globals points to the
         * module's globals array (initialized by aot_init_module_data).
         */
        int dst = op->p1;
        int idx = op->p2;
        hl_type *t = f->regs[dst];
        LLVMTypeRef llvm_type = llvm_get_type(ctx, t);

        /* Call aot_get_global(idx) to get pointer to global slot */
        LLVMValueRef idx_val = LLVMConstInt(ctx->i32_type, idx, false);
        LLVMValueRef args[] = { idx_val };
        LLVMTypeRef fn_type = LLVMFunctionType(ctx->ptr_type, (LLVMTypeRef[]){ ctx->i32_type }, 1, false);
        LLVMValueRef global_ptr = LLVMBuildCall2(ctx->builder, fn_type,
            ctx->rt_aot_get_global, args, 1, "global_ptr");

        /* Load the value from the global slot */
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, llvm_type, global_ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OSetGlobal: {
        /*
         * globals[idx] = src
         *
         * Call aot_get_global(idx) to get pointer to the global's storage,
         * then store the value there.
         */
        int idx = op->p1;
        int src = op->p2;
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Call aot_get_global(idx) to get pointer to global slot */
        LLVMValueRef idx_val = LLVMConstInt(ctx->i32_type, idx, false);
        LLVMValueRef args[] = { idx_val };
        LLVMTypeRef fn_type = LLVMFunctionType(ctx->ptr_type, (LLVMTypeRef[]){ ctx->i32_type }, 1, false);
        LLVMValueRef global_ptr = LLVMBuildCall2(ctx->builder, fn_type,
            ctx->rt_aot_get_global, args, 1, "global_ptr");

        LLVMBuildStore(ctx->builder, val, global_ptr);
        break;
    }

    case OField: {
        /* dst = obj.field[idx] */
        int dst = op->p1;
        int obj = op->p2;
        int field_idx = op->p3;
        hl_type *obj_type = f->regs[obj];
        hl_type *dst_type = f->regs[dst];

        LLVMValueRef obj_ptr = llvm_load_vreg(ctx, f, obj);
        LLVMTypeRef field_type = llvm_get_type(ctx, dst_type);

        /* Calculate field offset based on type */
        int offset = 0;
        if (obj_type->kind == HOBJ || obj_type->kind == HSTRUCT) {
            /* For objects/structs, get runtime field offsets */
            hl_runtime_obj *rt = hl_get_obj_rt(obj_type);
            if (rt && field_idx < rt->nfields) {
                offset = rt->fields_indexes[field_idx];
            }
        } else if (obj_type->kind == HVIRTUAL) {
            /* For virtuals, use vfields */
            if (obj_type->virt && field_idx < obj_type->virt->nfields) {
                /* vfields stores field offsets in the vvirtual structure */
                /* The actual data is after the vvirtual header */
                offset = sizeof(void*) * 2 + field_idx * 8; /* Approximate */
            }
        }

        /* GEP to field */
        LLVMValueRef offset_val = LLVMConstInt(ctx->i64_type, offset, false);
        LLVMValueRef field_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            obj_ptr, &offset_val, 1, "");
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, field_type, field_ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OSetField: {
        /* obj.field[idx] = src */
        int obj = op->p1;
        int field_idx = op->p2;
        int src = op->p3;
        hl_type *obj_type = f->regs[obj];

        LLVMValueRef obj_ptr = llvm_load_vreg(ctx, f, obj);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Calculate field offset based on type */
        int offset = 0;
        if (obj_type->kind == HOBJ || obj_type->kind == HSTRUCT) {
            hl_runtime_obj *rt = hl_get_obj_rt(obj_type);
            if (rt && field_idx < rt->nfields) {
                offset = rt->fields_indexes[field_idx];
            }
        } else if (obj_type->kind == HVIRTUAL) {
            if (obj_type->virt && field_idx < obj_type->virt->nfields) {
                offset = sizeof(void*) * 2 + field_idx * 8;
            }
        }

        /* GEP to field */
        LLVMValueRef offset_val = LLVMConstInt(ctx->i64_type, offset, false);
        LLVMValueRef field_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            obj_ptr, &offset_val, 1, "");
        LLVMBuildStore(ctx->builder, val, field_ptr);
        break;
    }

    case OGetThis: {
        /* dst = this.field[idx] (this is R(0)) */
        int dst = op->p1;
        int field_idx = op->p2;
        hl_type *this_type = f->regs[0];
        hl_type *dst_type = f->regs[dst];

        LLVMValueRef this_ptr = llvm_load_vreg(ctx, f, 0);
        LLVMTypeRef field_type = llvm_get_type(ctx, dst_type);

        int offset = 0;
        if (this_type->kind == HOBJ || this_type->kind == HSTRUCT) {
            hl_runtime_obj *rt = hl_get_obj_rt(this_type);
            if (rt && field_idx < rt->nfields) {
                offset = rt->fields_indexes[field_idx];
            }
        }

        LLVMValueRef offset_val = LLVMConstInt(ctx->i64_type, offset, false);
        LLVMValueRef field_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            this_ptr, &offset_val, 1, "");
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, field_type, field_ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OSetThis: {
        /* this.field[idx] = src (this is R(0)) */
        int field_idx = op->p1;
        int src = op->p2;
        hl_type *this_type = f->regs[0];

        LLVMValueRef this_ptr = llvm_load_vreg(ctx, f, 0);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        int offset = 0;
        if (this_type->kind == HOBJ || this_type->kind == HSTRUCT) {
            hl_runtime_obj *rt = hl_get_obj_rt(this_type);
            if (rt && field_idx < rt->nfields) {
                offset = rt->fields_indexes[field_idx];
            }
        }

        LLVMValueRef offset_val = LLVMConstInt(ctx->i64_type, offset, false);
        LLVMValueRef field_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            this_ptr, &offset_val, 1, "");
        LLVMBuildStore(ctx->builder, val, field_ptr);
        break;
    }

    case OGetI8: {
        /* dst = bytes[offset] as i8 */
        int dst = op->p1;
        int bytes = op->p2;
        int offset = op->p3;
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, ctx->i8_type, ptr, "");
        /* Zero-extend to i32 */
        LLVMValueRef ext = LLVMBuildZExt(ctx->builder, val, ctx->i32_type, "");
        llvm_store_vreg(ctx, f, dst, ext);
        break;
    }

    case OGetI16: {
        /* dst = bytes[offset] as i16 */
        int dst = op->p1;
        int bytes = op->p2;
        int offset = op->p3;
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, ctx->i16_type, ptr, "");
        /* Zero-extend to i32 */
        LLVMValueRef ext = LLVMBuildZExt(ctx->builder, val, ctx->i32_type, "");
        llvm_store_vreg(ctx, f, dst, ext);
        break;
    }

    case OGetMem: {
        /* dst = *(type*)(bytes + offset) */
        int dst = op->p1;
        int bytes = op->p2;
        int offset = op->p3;
        hl_type *dst_type = f->regs[dst];
        LLVMTypeRef val_type = llvm_get_type(ctx, dst_type);
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, val_type, ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OGetArray: {
        /* dst = array[index] */
        int dst = op->p1;
        int arr = op->p2;
        int idx = op->p3;
        hl_type *arr_type = f->regs[arr];
        hl_type *elem_type = NULL;

        /* Get element type from array type */
        if (arr_type->kind == HARRAY && arr_type->tparam) {
            elem_type = arr_type->tparam;
        } else {
            elem_type = f->regs[dst];
        }

        LLVMTypeRef val_type = llvm_get_type(ctx, elem_type);
        int elem_size = llvm_type_size(ctx, elem_type);

        LLVMValueRef arr_ptr = llvm_load_vreg(ctx, f, arr);
        LLVMValueRef index = llvm_load_vreg(ctx, f, idx);

        /* Data starts immediately after varray header */
        LLVMValueRef data_offset = LLVMConstInt(ctx->i64_type, sizeof(varray), false);
        LLVMValueRef data_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            arr_ptr, &data_offset, 1, "");

        /* Calculate element offset */
        LLVMValueRef elem_size_val = LLVMConstInt(ctx->i32_type, elem_size, false);
        LLVMValueRef byte_offset = LLVMBuildMul(ctx->builder, index, elem_size_val, "");
        LLVMValueRef byte_offset64 = LLVMBuildZExt(ctx->builder, byte_offset, ctx->i64_type, "");
        LLVMValueRef elem_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            data_ptr, &byte_offset64, 1, "");

        LLVMValueRef val = LLVMBuildLoad2(ctx->builder, val_type, elem_ptr, "");
        llvm_store_vreg(ctx, f, dst, val);
        break;
    }

    case OSetI8: {
        /* bytes[offset] = val as i8 */
        int bytes = op->p1;
        int offset = op->p2;
        int src = op->p3;
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        /* Truncate to i8 */
        LLVMValueRef trunc = LLVMBuildTrunc(ctx->builder, val, ctx->i8_type, "");
        LLVMBuildStore(ctx->builder, trunc, ptr);
        break;
    }

    case OSetI16: {
        /* bytes[offset] = val as i16 */
        int bytes = op->p1;
        int offset = op->p2;
        int src = op->p3;
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        /* Truncate to i16 */
        LLVMValueRef trunc = LLVMBuildTrunc(ctx->builder, val, ctx->i16_type, "");
        LLVMBuildStore(ctx->builder, trunc, ptr);
        break;
    }

    case OSetMem: {
        /* *(type*)(bytes + offset) = val */
        int bytes = op->p1;
        int offset = op->p2;
        int src = op->p3;
        LLVMValueRef base = llvm_load_vreg(ctx, f, bytes);
        LLVMValueRef off = llvm_load_vreg(ctx, f, offset);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);
        LLVMValueRef ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type, base, &off, 1, "");
        LLVMBuildStore(ctx->builder, val, ptr);
        break;
    }

    case OSetArray: {
        /* array[index] = val */
        int arr = op->p1;
        int idx = op->p2;
        int src = op->p3;
        hl_type *arr_type = f->regs[arr];
        hl_type *elem_type = NULL;

        if (arr_type->kind == HARRAY && arr_type->tparam) {
            elem_type = arr_type->tparam;
        } else {
            elem_type = f->regs[src];
        }

        int elem_size = llvm_type_size(ctx, elem_type);

        LLVMValueRef arr_ptr = llvm_load_vreg(ctx, f, arr);
        LLVMValueRef index = llvm_load_vreg(ctx, f, idx);
        LLVMValueRef val = llvm_load_vreg(ctx, f, src);

        /* Data starts immediately after varray header */
        LLVMValueRef data_offset = LLVMConstInt(ctx->i64_type, sizeof(varray), false);
        LLVMValueRef data_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            arr_ptr, &data_offset, 1, "");

        LLVMValueRef elem_size_val = LLVMConstInt(ctx->i32_type, elem_size, false);
        LLVMValueRef byte_offset = LLVMBuildMul(ctx->builder, index, elem_size_val, "");
        LLVMValueRef byte_offset64 = LLVMBuildZExt(ctx->builder, byte_offset, ctx->i64_type, "");
        LLVMValueRef elem_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            data_ptr, &byte_offset64, 1, "");

        LLVMBuildStore(ctx->builder, val, elem_ptr);
        break;
    }

    default:
        break;
    }
}
