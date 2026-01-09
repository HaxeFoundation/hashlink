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

LLVMTypeRef llvm_get_type(llvm_ctx *ctx, hl_type *t) {
    if (!t) return ctx->ptr_type;

    switch (t->kind) {
    case HVOID:
        return ctx->void_type;

    case HUI8:
        return ctx->i8_type;

    case HUI16:
        return ctx->i16_type;

    case HI32:
        return ctx->i32_type;

    case HI64:
        return ctx->i64_type;

    case HF32:
        return ctx->f32_type;

    case HF64:
        return ctx->f64_type;

    case HBOOL:
        return ctx->i8_type;

    /* All pointer/object types become opaque pointers */
    case HBYTES:
    case HDYN:
    case HFUN:
    case HOBJ:
    case HARRAY:
    case HTYPE:
    case HREF:
    case HVIRTUAL:
    case HDYNOBJ:
    case HABSTRACT:
    case HENUM:
    case HNULL:
    case HMETHOD:
    case HSTRUCT:
    case HPACKED:
    case HGUID:
        return ctx->ptr_type;

    default:
        return ctx->ptr_type;
    }
}

LLVMTypeRef llvm_get_function_type(llvm_ctx *ctx, hl_type *t) {
    if (!t || t->kind != HFUN) {
        /* Default to void function */
        return LLVMFunctionType(ctx->void_type, NULL, 0, false);
    }

    hl_type_fun *ft = t->fun;

    /* Get return type */
    LLVMTypeRef ret_type = llvm_get_type(ctx, ft->ret);
    if (ft->ret->kind == HVOID) {
        ret_type = ctx->void_type;
    }

    /* Get parameter types */
    int nargs = ft->nargs;
    LLVMTypeRef *param_types = NULL;
    if (nargs > 0) {
        param_types = (LLVMTypeRef *)malloc(sizeof(LLVMTypeRef) * nargs);
        for (int i = 0; i < nargs; i++) {
            param_types[i] = llvm_get_type(ctx, ft->args[i]);
        }
    }

    LLVMTypeRef fn_type = LLVMFunctionType(ret_type, param_types, nargs, false);

    if (param_types) free(param_types);

    return fn_type;
}

int llvm_type_size(llvm_ctx *ctx, hl_type *t) {
    if (!t) return 8;  /* Pointer size */

    switch (t->kind) {
    case HVOID:
        return 0;
    case HUI8:
    case HBOOL:
        return 1;
    case HUI16:
        return 2;
    case HI32:
    case HF32:
        return 4;
    case HI64:
    case HF64:
        return 8;
    default:
        return 8;  /* Pointer size */
    }
}

bool llvm_is_float_type(hl_type *t) {
    return t && (t->kind == HF32 || t->kind == HF64);
}

bool llvm_is_ptr_type(hl_type *t) {
    return t && t->kind >= HBYTES;
}
