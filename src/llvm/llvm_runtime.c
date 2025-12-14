/*
 * Copyright (C)2005-2016 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "llvm_codegen.h"

/* Helper to declare a function with specific signature.
 * If function already exists, return the existing one to avoid
 * LLVM creating duplicate symbols with suffixes like .1, .2, etc. */
static LLVMValueRef declare_func(llvm_ctx *ctx, const char *name,
                                  LLVMTypeRef ret_type,
                                  LLVMTypeRef *param_types, int nparams,
                                  bool is_vararg) {
    LLVMValueRef existing = LLVMGetNamedFunction(ctx->module, name);
    if (existing) return existing;
    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, param_types, nparams, is_vararg);
    return LLVMAddFunction(ctx->module, name, fn_type);
}

/* Add noreturn attribute to a function */
static void add_noreturn(llvm_ctx *ctx, LLVMValueRef func) {
    unsigned kind = LLVMGetEnumAttributeKindForName("noreturn", 8);
    LLVMAttributeRef attr = LLVMCreateEnumAttribute(ctx->context, kind, 0);
    LLVMAddAttributeAtIndex(func, LLVMAttributeFunctionIndex, attr);
}

void llvm_declare_runtime(llvm_ctx *ctx) {
    LLVMTypeRef ptr = ctx->ptr_type;
    LLVMTypeRef i32 = ctx->i32_type;
    LLVMTypeRef i64 = ctx->i64_type;
    LLVMTypeRef f32 = ctx->f32_type;
    LLVMTypeRef f64 = ctx->f64_type;
    LLVMTypeRef void_t = ctx->void_type;

    /* hl_alloc_obj(hl_type*) -> vdynamic* */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_alloc_obj = declare_func(ctx, "hl_alloc_obj", ptr, params, 1, false);
    }

    /* hl_alloc_array(hl_type*, int) -> varray* */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_alloc_array = declare_func(ctx, "hl_alloc_array", ptr, params, 2, false);
    }

    /* hl_alloc_enum(hl_type*, int) -> venum* */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_alloc_enum = declare_func(ctx, "hl_alloc_enum", ptr, params, 2, false);
    }

    /* hl_alloc_virtual(hl_type*) -> vvirtual* */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_alloc_virtual = declare_func(ctx, "hl_alloc_virtual", ptr, params, 1, false);
    }

    /* hl_alloc_dynobj() -> vdynobj* */
    {
        ctx->rt_alloc_dynobj = declare_func(ctx, "hl_alloc_dynobj", ptr, NULL, 0, false);
    }

    /* hl_alloc_closure_void(hl_type*, void*) -> vclosure* */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_alloc_closure_void = declare_func(ctx, "hl_alloc_closure_void", ptr, params, 2, false);
    }

    /* hl_alloc_closure_ptr(hl_type*, void*, void*) -> vclosure* */
    {
        LLVMTypeRef params[] = { ptr, ptr, ptr };
        ctx->rt_alloc_closure_ptr = declare_func(ctx, "hl_alloc_closure_ptr", ptr, params, 3, false);
    }

    /* hl_alloc_bytes(int) -> vbyte* */
    {
        LLVMTypeRef params[] = { i32 };
        ctx->rt_alloc_bytes = declare_func(ctx, "hl_alloc_bytes", ptr, params, 1, false);
    }

    /* hl_throw(vdynamic*) -> noreturn */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_throw = declare_func(ctx, "hl_throw", void_t, params, 1, false);
        add_noreturn(ctx, ctx->rt_throw);
    }

    /* hl_rethrow(vdynamic*) -> noreturn */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_rethrow = declare_func(ctx, "hl_rethrow", void_t, params, 1, false);
        add_noreturn(ctx, ctx->rt_rethrow);
    }

    /* hl_null_access() -> noreturn */
    {
        ctx->rt_null_access = declare_func(ctx, "hl_null_access", void_t, NULL, 0, false);
        add_noreturn(ctx, ctx->rt_null_access);
    }

    /* hl_get_obj_rt(hl_type*) -> hl_runtime_obj* */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_get_obj_rt = declare_func(ctx, "hl_get_obj_rt", ptr, params, 1, false);
    }

    /* hl_to_virtual(hl_type*, vdynamic*) -> vvirtual* */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_to_virtual = declare_func(ctx, "hl_to_virtual", ptr, params, 2, false);
    }

    /* hl_safe_cast(hl_type*, hl_type*) -> bool */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_safe_cast = declare_func(ctx, "hl_safe_cast", ctx->i8_type, params, 2, false);
    }

    /* hl_make_dyn(void*, hl_type*) -> vdynamic* */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_make_dyn = declare_func(ctx, "hl_make_dyn", ptr, params, 2, false);
    }

    /* hl_dyn_call(vclosure*, vdynamic**, int) -> vdynamic* */
    {
        LLVMTypeRef params[] = { ptr, ptr, i32 };
        ctx->rt_dyn_call = declare_func(ctx, "hl_dyn_call", ptr, params, 3, false);
    }

    /* hl_dyn_call_safe(vclosure*, vdynamic**, int, bool*) -> vdynamic* */
    {
        LLVMTypeRef params[] = { ptr, ptr, i32, ptr };
        ctx->rt_dyn_call_safe = declare_func(ctx, "hl_dyn_call_safe", ptr, params, 4, false);
    }

    /* hl_get_thread() -> hl_thread_info* */
    {
        ctx->rt_get_thread = declare_func(ctx, "hl_get_thread", ptr, NULL, 0, false);
    }

    /* Dynamic field getters */
    /* hl_dyn_geti(vdynamic*, int, hl_type*) -> int */
    {
        LLVMTypeRef params[] = { ptr, i32, ptr };
        ctx->rt_dyn_geti = declare_func(ctx, "hl_dyn_geti", i32, params, 3, false);
    }

    /* hl_dyn_geti64(vdynamic*, int) -> int64 */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_dyn_geti64 = declare_func(ctx, "hl_dyn_geti64", i64, params, 2, false);
    }

    /* hl_dyn_getf(vdynamic*, int) -> float */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_dyn_getf = declare_func(ctx, "hl_dyn_getf", f32, params, 2, false);
    }

    /* hl_dyn_getd(vdynamic*, int) -> double */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_dyn_getd = declare_func(ctx, "hl_dyn_getd", f64, params, 2, false);
    }

    /* hl_dyn_getp(vdynamic*, int, hl_type*) -> void* */
    {
        LLVMTypeRef params[] = { ptr, i32, ptr };
        ctx->rt_dyn_getp = declare_func(ctx, "hl_dyn_getp", ptr, params, 3, false);
    }

    /* Dynamic field setters */
    /* hl_dyn_seti(vdynamic*, int, hl_type*, int) */
    {
        LLVMTypeRef params[] = { ptr, i32, ptr, i32 };
        ctx->rt_dyn_seti = declare_func(ctx, "hl_dyn_seti", void_t, params, 4, false);
    }

    /* hl_dyn_seti64(vdynamic*, int, int64) */
    {
        LLVMTypeRef params[] = { ptr, i32, i64 };
        ctx->rt_dyn_seti64 = declare_func(ctx, "hl_dyn_seti64", void_t, params, 3, false);
    }

    /* hl_dyn_setf(vdynamic*, int, float) */
    {
        LLVMTypeRef params[] = { ptr, i32, f32 };
        ctx->rt_dyn_setf = declare_func(ctx, "hl_dyn_setf", void_t, params, 3, false);
    }

    /* hl_dyn_setd(vdynamic*, int, double) */
    {
        LLVMTypeRef params[] = { ptr, i32, f64 };
        ctx->rt_dyn_setd = declare_func(ctx, "hl_dyn_setd", void_t, params, 3, false);
    }

    /* hl_dyn_setp(vdynamic*, int, hl_type*, void*) */
    {
        LLVMTypeRef params[] = { ptr, i32, ptr, ptr };
        ctx->rt_dyn_setp = declare_func(ctx, "hl_dyn_setp", void_t, params, 4, false);
    }

    /* Dynamic cast functions */
    /* hl_dyn_casti(void*, hl_type*, hl_type*) -> int */
    {
        LLVMTypeRef params[] = { ptr, ptr, ptr };
        ctx->rt_dyn_casti = declare_func(ctx, "hl_dyn_casti", i32, params, 3, false);
    }

    /* hl_dyn_casti64(void*, hl_type*) -> int64 */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_dyn_casti64 = declare_func(ctx, "hl_dyn_casti64", i64, params, 2, false);
    }

    /* hl_dyn_castf(void*, hl_type*) -> float */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_dyn_castf = declare_func(ctx, "hl_dyn_castf", f32, params, 2, false);
    }

    /* hl_dyn_castd(void*, hl_type*) -> double */
    {
        LLVMTypeRef params[] = { ptr, ptr };
        ctx->rt_dyn_castd = declare_func(ctx, "hl_dyn_castd", f64, params, 2, false);
    }

    /* hl_dyn_castp(void*, hl_type*, hl_type*) -> void* */
    {
        LLVMTypeRef params[] = { ptr, ptr, ptr };
        ctx->rt_dyn_castp = declare_func(ctx, "hl_dyn_castp", ptr, params, 3, false);
    }

    /* Hash functions */
    /* hl_hash(vbyte*) -> int */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_hash = declare_func(ctx, "hl_hash", i32, params, 1, false);
    }

    /* hl_hash_gen(uchar*, bool) -> int */
    {
        LLVMTypeRef params[] = { ptr, ctx->i8_type };
        ctx->rt_hash_gen = declare_func(ctx, "hl_hash_gen", i32, params, 2, false);
    }

    /* setjmp/longjmp for exception handling */
    /* setjmp(jmp_buf) -> int */
    {
        LLVMTypeRef params[] = { ptr };
        ctx->rt_setjmp = declare_func(ctx, "setjmp", i32, params, 1, false);
    }

    /* longjmp(jmp_buf, int) -> noreturn */
    {
        LLVMTypeRef params[] = { ptr, i32 };
        ctx->rt_longjmp = declare_func(ctx, "longjmp", void_t, params, 2, false);
        add_noreturn(ctx, ctx->rt_longjmp);
    }

    /* aot_get_type(int) -> void* - AOT runtime type accessor
     * Mark as pure function so LLVM can CSE and hoist out of loops.
     * The function just does: return &types_array[index]; */
    {
        LLVMTypeRef params[] = { i32 };
        ctx->rt_aot_get_type = declare_func(ctx, "aot_get_type", ptr, params, 1, false);
        /* memory(none) - function doesn't read or write any memory visible to caller.
         * It returns a pointer computed from a global base + offset. */
        unsigned mem_kind = LLVMGetEnumAttributeKindForName("memory", 6);
        LLVMAttributeRef mem_attr = LLVMCreateEnumAttribute(ctx->context, mem_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_type, LLVMAttributeFunctionIndex, mem_attr);
        /* nounwind - doesn't throw */
        unsigned nounwind_kind = LLVMGetEnumAttributeKindForName("nounwind", 8);
        LLVMAttributeRef nounwind_attr = LLVMCreateEnumAttribute(ctx->context, nounwind_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_type, LLVMAttributeFunctionIndex, nounwind_attr);
        /* willreturn - always returns */
        unsigned willreturn_kind = LLVMGetEnumAttributeKindForName("willreturn", 10);
        LLVMAttributeRef willreturn_attr = LLVMCreateEnumAttribute(ctx->context, willreturn_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_type, LLVMAttributeFunctionIndex, willreturn_attr);
        /* speculatable - can be safely speculated/hoisted */
        unsigned spec_kind = LLVMGetEnumAttributeKindForName("speculatable", 12);
        if (spec_kind) {
            LLVMAttributeRef spec_attr = LLVMCreateEnumAttribute(ctx->context, spec_kind, 0);
            LLVMAddAttributeAtIndex(ctx->rt_aot_get_type, LLVMAttributeFunctionIndex, spec_attr);
        }
    }

    /* aot_get_global(int) -> void** - AOT runtime global accessor
     * Mark as pure function so LLVM can CSE and hoist out of loops.
     * The function just does: return &globals_array[index]; */
    {
        LLVMTypeRef params[] = { i32 };
        ctx->rt_aot_get_global = declare_func(ctx, "aot_get_global", ptr, params, 1, false);
        /* memory(none) - function doesn't read or write any memory visible to caller */
        unsigned mem_kind = LLVMGetEnumAttributeKindForName("memory", 6);
        LLVMAttributeRef mem_attr = LLVMCreateEnumAttribute(ctx->context, mem_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_global, LLVMAttributeFunctionIndex, mem_attr);
        /* nounwind - doesn't throw */
        unsigned nounwind_kind = LLVMGetEnumAttributeKindForName("nounwind", 8);
        LLVMAttributeRef nounwind_attr = LLVMCreateEnumAttribute(ctx->context, nounwind_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_global, LLVMAttributeFunctionIndex, nounwind_attr);
        /* willreturn - always returns */
        unsigned willreturn_kind = LLVMGetEnumAttributeKindForName("willreturn", 10);
        LLVMAttributeRef willreturn_attr = LLVMCreateEnumAttribute(ctx->context, willreturn_kind, 0);
        LLVMAddAttributeAtIndex(ctx->rt_aot_get_global, LLVMAttributeFunctionIndex, willreturn_attr);
        /* speculatable - can be safely speculated/hoisted */
        unsigned spec_kind = LLVMGetEnumAttributeKindForName("speculatable", 12);
        if (spec_kind) {
            LLVMAttributeRef spec_attr = LLVMCreateEnumAttribute(ctx->context, spec_kind, 0);
            LLVMAddAttributeAtIndex(ctx->rt_aot_get_global, LLVMAttributeFunctionIndex, spec_attr);
        }
    }
}
