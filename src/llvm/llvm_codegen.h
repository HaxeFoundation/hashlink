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
#ifndef LLVM_CODEGEN_H
#define LLVM_CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Error.h>
#include <llvm-c/Transforms/PassBuilder.h>

#include "../hl.h"
#include "../hlmodule.h"

/* Output format options */
typedef enum {
    LLVM_OUTPUT_LLVM_IR,    /* Text LLVM IR (.ll) */
    LLVM_OUTPUT_BITCODE,    /* LLVM bitcode (.bc) */
    LLVM_OUTPUT_ASSEMBLY,   /* Native assembly (.s) */
    LLVM_OUTPUT_OBJECT      /* Object file (.o) */
} llvm_output_format;

/* Optimization level */
typedef enum {
    LLVM_OPT_NONE = 0,
    LLVM_OPT_LESS = 1,
    LLVM_OPT_DEFAULT = 2,
    LLVM_OPT_AGGRESSIVE = 3
} llvm_opt_level;

/* LLVM codegen context */
typedef struct {
    /* LLVM core objects */
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;
    LLVMTargetMachineRef target_machine;
    LLVMTargetDataRef target_data;

    /* Current function compilation state */
    LLVMValueRef current_function;
    LLVMBasicBlockRef entry_block;      /* Entry block for allocas */
    LLVMBasicBlockRef *basic_blocks;    /* Array indexed by opcode position */
    int num_blocks;
    LLVMValueRef *vreg_allocs;          /* Stack allocas for each vreg */
    int num_vregs;
    bool *is_block_start;               /* Marks which opcodes start blocks */

    /* HashLink module being compiled */
    hl_code *code;

    /* Type caches */
    LLVMTypeRef *hl_type_cache;         /* LLVM type for each hl_type index */
    int num_types;

    /* Function references */
    LLVMValueRef *functions;            /* LLVMValueRef for each function */
    LLVMTypeRef *function_types;        /* LLVMTypeRef for each function */
    int num_functions;

    /* Global variable storage */
    LLVMValueRef globals_base;          /* Pointer to globals data */
    LLVMValueRef *global_refs;          /* Individual global pointers */
    int num_globals;

    /* String/bytes constants */
    LLVMValueRef *string_constants;
    int num_strings;
    LLVMValueRef *bytes_constants;
    int num_bytes;

    /* Type constant pointers - no longer used, types accessed via aot_get_type */
    LLVMValueRef *type_constants;

    /* AOT runtime accessors */
    LLVMValueRef rt_aot_get_type;
    LLVMValueRef rt_aot_get_global;

    /* Runtime function declarations */
    LLVMValueRef rt_alloc_obj;
    LLVMValueRef rt_alloc_array;
    LLVMValueRef rt_alloc_enum;
    LLVMValueRef rt_alloc_virtual;
    LLVMValueRef rt_alloc_dynobj;
    LLVMValueRef rt_alloc_closure_void;
    LLVMValueRef rt_alloc_closure_ptr;
    LLVMValueRef rt_alloc_bytes;
    LLVMValueRef rt_throw;
    LLVMValueRef rt_rethrow;
    LLVMValueRef rt_null_access;
    LLVMValueRef rt_get_obj_rt;
    LLVMValueRef rt_to_virtual;
    LLVMValueRef rt_safe_cast;
    LLVMValueRef rt_make_dyn;
    LLVMValueRef rt_dyn_call;
    LLVMValueRef rt_dyn_call_safe;
    LLVMValueRef rt_get_thread;

    /* Dynamic field access */
    LLVMValueRef rt_dyn_geti;
    LLVMValueRef rt_dyn_geti64;
    LLVMValueRef rt_dyn_getf;
    LLVMValueRef rt_dyn_getd;
    LLVMValueRef rt_dyn_getp;
    LLVMValueRef rt_dyn_seti;
    LLVMValueRef rt_dyn_seti64;
    LLVMValueRef rt_dyn_setf;
    LLVMValueRef rt_dyn_setd;
    LLVMValueRef rt_dyn_setp;
    LLVMValueRef rt_dyn_casti;
    LLVMValueRef rt_dyn_casti64;
    LLVMValueRef rt_dyn_castf;
    LLVMValueRef rt_dyn_castd;
    LLVMValueRef rt_dyn_castp;

    /* Hash functions */
    LLVMValueRef rt_hash;
    LLVMValueRef rt_hash_gen;

    /* setjmp/longjmp for exceptions */
    LLVMValueRef rt_setjmp;
    LLVMValueRef rt_longjmp;

    /* Common LLVM types */
    LLVMTypeRef void_type;
    LLVMTypeRef i1_type;
    LLVMTypeRef i8_type;
    LLVMTypeRef i16_type;
    LLVMTypeRef i32_type;
    LLVMTypeRef i64_type;
    LLVMTypeRef f32_type;
    LLVMTypeRef f64_type;
    LLVMTypeRef ptr_type;

    /* Struct types for runtime objects */
    LLVMTypeRef vdynamic_type;
    LLVMTypeRef vclosure_type;
    LLVMTypeRef varray_type;
    LLVMTypeRef venum_type;
    LLVMTypeRef vvirtual_type;

    /* Options */
    llvm_opt_level opt_level;
    bool emit_debug_info;

    /* Embedded bytecode for standalone binary */
    const unsigned char *bytecode_data;
    int bytecode_size;

    /* Error handling */
    char *error_msg;
} llvm_ctx;

/* Main API */
llvm_ctx *llvm_create_context(void);
void llvm_destroy_context(llvm_ctx *ctx);

bool llvm_init_module(llvm_ctx *ctx, hl_code *code, const char *module_name);
bool llvm_compile_function(llvm_ctx *ctx, hl_function *f);
bool llvm_finalize_module(llvm_ctx *ctx);

bool llvm_output(llvm_ctx *ctx, const char *filename, llvm_output_format format);
bool llvm_verify(llvm_ctx *ctx);
void llvm_optimize(llvm_ctx *ctx);

/* Type mapping */
LLVMTypeRef llvm_get_type(llvm_ctx *ctx, hl_type *t);
LLVMTypeRef llvm_get_function_type(llvm_ctx *ctx, hl_type *t);
int llvm_type_size(llvm_ctx *ctx, hl_type *t);
bool llvm_is_float_type(hl_type *t);
bool llvm_is_ptr_type(hl_type *t);

/* Runtime declarations */
void llvm_declare_runtime(llvm_ctx *ctx);

/* Opcode handlers - organized by category */
void llvm_emit_constants(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_arithmetic(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_control_flow(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_memory(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_calls(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_closures(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_types(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_objects(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_enums(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_refs(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_exceptions(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);
void llvm_emit_misc(llvm_ctx *ctx, hl_function *f, hl_opcode *op, int op_idx);

/* Helpers */
LLVMValueRef llvm_load_vreg(llvm_ctx *ctx, hl_function *f, int vreg_idx);
void llvm_store_vreg(llvm_ctx *ctx, hl_function *f, int vreg_idx, LLVMValueRef value);
LLVMBasicBlockRef llvm_get_block_for_offset(llvm_ctx *ctx, int current_op, int offset);
void llvm_ensure_block_terminated(llvm_ctx *ctx, LLVMBasicBlockRef next_block);
LLVMValueRef llvm_get_function_ptr(llvm_ctx *ctx, int findex);
LLVMValueRef llvm_get_type_ptr(llvm_ctx *ctx, int type_idx);
LLVMValueRef llvm_get_string(llvm_ctx *ctx, int str_idx);
LLVMValueRef llvm_get_bytes(llvm_ctx *ctx, int bytes_idx);
LLVMValueRef llvm_create_entry_alloca(llvm_ctx *ctx, LLVMTypeRef type, const char *name);

/* Entry point generation */
bool llvm_generate_entry_point(llvm_ctx *ctx, int entry_findex);

#endif /* LLVM_CODEGEN_H */
