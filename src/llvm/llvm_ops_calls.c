/*
 * Copyright (C)2005-2016 Haxe Foundation
 * LLVM Backend - Function Call Opcodes
 */
#include "llvm_codegen.h"

void llvm_emit_calls(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx) {
    switch (op->op) {
    case OCall0: {
        /* dst = func() */
        int dst = op->p1;
        int findex = op->p2;
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, NULL, 0, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OCall1: {
        /* dst = func(arg0) */
        int dst = op->p1;
        int findex = op->p2;
        int arg0 = op->p3;
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef args[] = { llvm_load_vreg(ctx, f, arg0) };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, args, 1, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OCall2: {
        /* dst = func(arg0, arg1) */
        int dst = op->p1;
        int findex = op->p2;
        int arg0 = op->p3;
        int arg1 = (int)(int_val)op->extra; /* extra is direct int, not array */
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef args[] = {
            llvm_load_vreg(ctx, f, arg0),
            llvm_load_vreg(ctx, f, arg1)
        };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, args, 2, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OCall3: {
        /* dst = func(arg0, arg1, arg2) */
        int dst = op->p1;
        int findex = op->p2;
        int arg0 = op->p3;
        int arg1 = op->extra[0];
        int arg2 = op->extra[1];
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef args[] = {
            llvm_load_vreg(ctx, f, arg0),
            llvm_load_vreg(ctx, f, arg1),
            llvm_load_vreg(ctx, f, arg2)
        };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, args, 3, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OCall4: {
        /* dst = func(arg0, arg1, arg2, arg3) */
        int dst = op->p1;
        int findex = op->p2;
        int arg0 = op->p3;
        int arg1 = op->extra[0];
        int arg2 = op->extra[1];
        int arg3 = op->extra[2];
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef args[] = {
            llvm_load_vreg(ctx, f, arg0),
            llvm_load_vreg(ctx, f, arg1),
            llvm_load_vreg(ctx, f, arg2),
            llvm_load_vreg(ctx, f, arg3)
        };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, args, 4, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    case OCallN: {
        /* dst = func(args...) */
        int dst = op->p1;
        int findex = op->p2;
        int nargs = op->p3;
        LLVMValueRef func = llvm_get_function_ptr(ctx, findex);
        LLVMTypeRef fn_type = ctx->function_types[findex];
        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);

        LLVMValueRef *args = NULL;
        if (nargs > 0) {
            args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nargs);
            for (int i = 0; i < nargs; i++) {
                args[i] = llvm_load_vreg(ctx, f, op->extra[i]);
            }
        }

        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, func, args, nargs, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }

        if (args) free(args);
        break;
    }

    case OCallMethod: {
        /* dst = obj.method(args...) via vtable */
        int dst = op->p1;
        int method_idx = op->p2;
        int nargs = op->p3;
        int obj_reg = op->extra[0];

        hl_type *obj_type = f->regs[obj_reg];
        LLVMValueRef obj = llvm_load_vreg(ctx, f, obj_reg);

        /* Get method return type if possible */
        hl_type *method_type = NULL;
        if (obj_type->kind == HOBJ && obj_type->obj) {
            if (method_idx < obj_type->obj->nproto) {
                int findex = obj_type->obj->proto[method_idx].findex;
                if (findex >= 0 && findex < ctx->code->nfunctions) {
                    method_type = ctx->code->functions[findex].type;
                }
            }
        } else if (obj_type->kind == HVIRTUAL && obj_type->virt) {
            if (method_idx < obj_type->virt->nfields) {
                method_type = obj_type->virt->fields[method_idx].t;
            }
        }

        /* Build function type matching actual call arguments */
        LLVMTypeRef ret_llvm_type = ctx->ptr_type; /* Default return type */
        if (method_type && method_type->kind == HFUN && method_type->fun) {
            ret_llvm_type = llvm_get_type(ctx, method_type->fun->ret);
        }

        if (obj_type->kind == HVIRTUAL) {
            /*
             * HVIRTUAL method call:
             * vvirtual layout: hl_type* t (0), vdynamic* value (8), vvirtual* next (16), vfields[...] (24+)
             * vfield[method_idx] is at offset 24 + method_idx * 8
             * If vfield is not NULL, call it with obj->value as first arg
             * If vfield is NULL, call hl_dyn_call_obj (not implemented - will crash)
             */
            int vfield_offset = 24 + method_idx * 8;
            LLVMValueRef vfield_off_val = LLVMConstInt(ctx->i64_type, vfield_offset, false);
            LLVMValueRef vfield_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
                obj, &vfield_off_val, 1, "");
            LLVMValueRef vfield = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, vfield_ptr, "vfield");

            /* Load obj->value (at offset 8) - this is the actual object to pass */
            LLVMValueRef value_off = LLVMConstInt(ctx->i64_type, 8, false);
            LLVMValueRef value_ptr = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
                obj, &value_off, 1, "");
            LLVMValueRef obj_value = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, value_ptr, "obj_value");

            /* Build args array with obj->value as first arg */
            LLVMValueRef *args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nargs);
            args[0] = obj_value;
            for (int i = 1; i < nargs; i++) {
                args[i] = llvm_load_vreg(ctx, f, op->extra[i]);
            }

            /* Build param types - first is ptr (obj->value), rest from vregs */
            LLVMTypeRef *param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nargs);
            param_types[0] = ctx->ptr_type;
            for (int i = 1; i < nargs; i++) {
                param_types[i] = llvm_get_type(ctx, f->regs[op->extra[i]]);
            }
            LLVMTypeRef fn_type = LLVMFunctionType(ret_llvm_type, param_types, nargs, false);
            free(param_types);

            LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
            bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
            LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, vfield, args, nargs, "");
            if (f->regs[dst]->kind != HVOID && !returns_void) {
                llvm_store_vreg(ctx, f, dst, result);
            }

            free(args);
        } else {
            /* HOBJ method call via hl_get_obj_rt */
            LLVMValueRef *args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nargs);
            for (int i = 0; i < nargs; i++) {
                args[i] = llvm_load_vreg(ctx, f, op->extra[i]);
            }

            /* Build param types from actual argument vregs */
            LLVMTypeRef *param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nargs);
            for (int i = 0; i < nargs; i++) {
                param_types[i] = llvm_get_type(ctx, f->regs[op->extra[i]]);
            }
            LLVMTypeRef fn_type = LLVMFunctionType(ret_llvm_type, param_types, nargs, false);
            free(param_types);

            /* Load type pointer from object */
            LLVMValueRef type_ptr_offset = LLVMConstInt(ctx->i64_type, 0, false);
            LLVMValueRef type_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
                LLVMBuildGEP2(ctx->builder, ctx->i8_type, obj, &type_ptr_offset, 1, ""), "");

            /* Get hl_runtime_obj from type */
            LLVMValueRef rt_args[] = { type_ptr };
            LLVMValueRef rt = LLVMBuildCall2(ctx->builder,
                LLVMGlobalGetValueType(ctx->rt_get_obj_rt),
                ctx->rt_get_obj_rt, rt_args, 1, "");

            /* Load method from proto array */
            /* hl_runtime_obj has proto array, each entry has fptr */
            /* Offset to proto: depends on structure layout */
            int proto_offset = 8 + 4 * 7; /* Approximate offset to proto array pointer */
            LLVMValueRef proto_off_val = LLVMConstInt(ctx->i64_type, proto_offset, false);
            LLVMValueRef proto_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
                LLVMBuildGEP2(ctx->builder, ctx->i8_type, rt, &proto_off_val, 1, ""), "");

            /* Each proto entry is 24 bytes (name, findex, pindex, t, hashed_name) + fptr */
            int entry_size = 32; /* Approximate */
            LLVMValueRef method_off = LLVMConstInt(ctx->i64_type, method_idx * entry_size, false);
            LLVMValueRef method_entry = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
                proto_ptr, &method_off, 1, "");

            /* fptr is at offset 0 or near start of proto entry */
            LLVMValueRef fptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, method_entry, "");

            LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
            bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
            LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, fptr, args, nargs, "");
            if (f->regs[dst]->kind != HVOID && !returns_void) {
                llvm_store_vreg(ctx, f, dst, result);
            }

            free(args);
        }
        break;
    }

    case OCallThis: {
        /* dst = this.method(args...) where this is R(0) */
        int dst = op->p1;
        int method_idx = op->p2;
        int extra_nargs = op->p3; /* Args beyond 'this' */
        int nargs = extra_nargs + 1; /* Include 'this' */

        hl_type *this_type = f->regs[0];
        LLVMValueRef this_obj = llvm_load_vreg(ctx, f, 0);

        LLVMValueRef *args = (LLVMValueRef *)malloc(sizeof(LLVMValueRef) * nargs);
        args[0] = this_obj;
        for (int i = 0; i < extra_nargs; i++) {
            args[i + 1] = llvm_load_vreg(ctx, f, op->extra[i]);
        }

        /* Get method return type */
        hl_type *method_type = NULL;
        if (this_type->kind == HOBJ && this_type->obj) {
            if (method_idx < this_type->obj->nproto) {
                int findex = this_type->obj->proto[method_idx].findex;
                if (findex >= 0 && findex < ctx->code->nfunctions) {
                    method_type = ctx->code->functions[findex].type;
                }
            }
        }

        /* Build function type matching actual call arguments */
        LLVMTypeRef ret_llvm_type = ctx->ptr_type;
        if (method_type && method_type->kind == HFUN && method_type->fun) {
            ret_llvm_type = llvm_get_type(ctx, method_type->fun->ret);
        }

        /* Build param types from actual argument vregs */
        LLVMTypeRef *param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nargs);
        param_types[0] = llvm_get_type(ctx, f->regs[0]); /* this */
        for (int i = 0; i < extra_nargs; i++) {
            param_types[i + 1] = llvm_get_type(ctx, f->regs[op->extra[i]]);
        }
        LLVMTypeRef fn_type = LLVMFunctionType(ret_llvm_type, param_types, nargs, false);
        free(param_types);

        /* Load method pointer via vtable */
        LLVMValueRef type_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, this_obj, "");
        LLVMValueRef rt_args[] = { type_ptr };
        LLVMValueRef rt = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_get_obj_rt),
            ctx->rt_get_obj_rt, rt_args, 1, "");

        int proto_offset = 8 + 4 * 7;
        LLVMValueRef proto_off_val = LLVMConstInt(ctx->i64_type, proto_offset, false);
        LLVMValueRef proto_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
            LLVMBuildGEP2(ctx->builder, ctx->i8_type, rt, &proto_off_val, 1, ""), "");

        int entry_size = 32;
        LLVMValueRef method_off = LLVMConstInt(ctx->i64_type, method_idx * entry_size, false);
        LLVMValueRef method_entry = LLVMBuildGEP2(ctx->builder, ctx->i8_type,
            proto_ptr, &method_off, 1, "");
        LLVMValueRef fptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type, method_entry, "");

        LLVMTypeRef ret_type = LLVMGetReturnType(fn_type);
        bool returns_void = (LLVMGetTypeKind(ret_type) == LLVMVoidTypeKind);
        LLVMValueRef result = LLVMBuildCall2(ctx->builder, fn_type, fptr, args, nargs, "");
        if (f->regs[dst]->kind != HVOID && !returns_void) {
            llvm_store_vreg(ctx, f, dst, result);
        }

        free(args);
        break;
    }

    case OCallClosure: {
        /* dst = closure(args...) */
        int dst = op->p1;
        int closure_reg = op->p2;
        int nargs = op->p3;

        LLVMValueRef closure = llvm_load_vreg(ctx, f, closure_reg);

        /* vclosure structure:
         * hl_type* t;
         * void* fun;
         * int hasValue;
         * void* value;
         */

        /* Load function pointer from closure */
        LLVMValueRef fun_offset = LLVMConstInt(ctx->i64_type, 8, false); /* After hl_type* */
        LLVMValueRef fun_ptr = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
            LLVMBuildGEP2(ctx->builder, ctx->i8_type, closure, &fun_offset, 1, ""), "");

        /* Check hasValue and get value pointer */
        LLVMValueRef has_val_offset = LLVMConstInt(ctx->i64_type, 16, false);
        LLVMValueRef has_val = LLVMBuildLoad2(ctx->builder, ctx->i32_type,
            LLVMBuildGEP2(ctx->builder, ctx->i8_type, closure, &has_val_offset, 1, ""), "");

        LLVMValueRef val_offset = LLVMConstInt(ctx->i64_type, 24, false);
        LLVMValueRef closure_val = LLVMBuildLoad2(ctx->builder, ctx->ptr_type,
            LLVMBuildGEP2(ctx->builder, ctx->i8_type, closure, &val_offset, 1, ""), "");

        /* Build argument list */
        int actual_nargs = nargs;
        LLVMValueRef zero = LLVMConstInt(ctx->i32_type, 0, false);
        LLVMValueRef has_closure_arg = LLVMBuildICmp(ctx->builder, LLVMIntNE, has_val, zero, "");

        /* For simplicity, we'll use hl_dyn_call for closure calls */
        /* This handles the hasValue case properly */

        /*
         * hl_dyn_call expects vdynamic** args - an array of pointers to vdynamic structs.
         * We need to wrap each argument value in a vdynamic struct.
         * vdynamic layout: { hl_type* t; union { ..., void* ptr, ... } v; }
         */

        /* Allocate vdynamic array on stack (16 bytes each: type ptr + value) */
        /* Use entry block alloca to avoid stack growth if this is in a loop */
        LLVMTypeRef vdyn_type = LLVMArrayType(ctx->i8_type, 16);
        LLVMTypeRef vdyn_array_type = LLVMArrayType(vdyn_type, nargs > 0 ? nargs : 1);
        LLVMValueRef vdyn_array = llvm_create_entry_alloca(ctx, vdyn_array_type, "vdyn_array");

        /* Allocate pointer array to point to each vdynamic */
        LLVMTypeRef args_array_type = LLVMArrayType(ctx->ptr_type, nargs > 0 ? nargs : 1);
        LLVMValueRef args_array = llvm_create_entry_alloca(ctx, args_array_type, "args_array");

        for (int i = 0; i < nargs; i++) {
            int arg_reg = op->extra[i];
            hl_type *arg_type = f->regs[arg_reg];
            LLVMValueRef arg = llvm_load_vreg(ctx, f, arg_reg);

            /* Get pointer to this vdynamic slot using two-index GEP for nested array */
            LLVMValueRef vdyn_indices[] = {
                LLVMConstInt(ctx->i32_type, 0, false),  /* Dereference the array pointer */
                LLVMConstInt(ctx->i32_type, i, false)   /* Index into the array */
            };
            LLVMValueRef vdyn_ptr = LLVMBuildGEP2(ctx->builder, vdyn_array_type, vdyn_array, vdyn_indices, 2, "");

            /* Store type pointer at offset 0 */
            /* Find type index */
            int type_idx = -1;
            for (int ti = 0; ti < ctx->code->ntypes; ti++) {
                if (ctx->code->types + ti == arg_type) {
                    type_idx = ti;
                    break;
                }
            }
            LLVMValueRef type_ptr = type_idx >= 0 ? llvm_get_type_ptr(ctx, type_idx)
                : LLVMConstNull(ctx->ptr_type);
            LLVMValueRef type_slot = LLVMBuildBitCast(ctx->builder, vdyn_ptr, ctx->ptr_type, "");
            LLVMBuildStore(ctx->builder, type_ptr, type_slot);

            /* Store value at offset 8 */
            LLVMValueRef val_offset = LLVMConstInt(ctx->i64_type, 8, false);
            LLVMValueRef val_slot = LLVMBuildGEP2(ctx->builder, ctx->i8_type, vdyn_ptr, &val_offset, 1, "");
            LLVMValueRef val_slot_typed = LLVMBuildBitCast(ctx->builder, val_slot,
                LLVMPointerType(LLVMTypeOf(arg), 0), "");
            LLVMBuildStore(ctx->builder, arg, val_slot_typed);

            /* Store pointer to vdynamic in args array using two-index GEP */
            LLVMValueRef args_indices[] = {
                LLVMConstInt(ctx->i32_type, 0, false),
                LLVMConstInt(ctx->i32_type, i, false)
            };
            LLVMValueRef args_slot = LLVMBuildGEP2(ctx->builder, args_array_type, args_array, args_indices, 2, "");
            LLVMBuildStore(ctx->builder, vdyn_ptr, args_slot);
        }

        /* Call hl_dyn_call */
        LLVMValueRef call_args[] = { closure, args_array, LLVMConstInt(ctx->i32_type, nargs, false) };
        LLVMValueRef result = LLVMBuildCall2(ctx->builder,
            LLVMGlobalGetValueType(ctx->rt_dyn_call),
            ctx->rt_dyn_call, call_args, 3, "");

        if (f->regs[dst]->kind != HVOID) {
            llvm_store_vreg(ctx, f, dst, result);
        }
        break;
    }

    default:
        break;
    }
}
