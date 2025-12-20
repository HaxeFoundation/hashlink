/*
 * Copyright (C)2015-2016 Haxe Foundation
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

/*
 * AArch64 JIT Backend
 *
 * This file contains the AArch64-specific implementation of the HashLink JIT compiler.
 * It translates HashLink bytecode into native AArch64 machine code.
 *
 * Key differences from x86:
 * - Fixed 32-bit instruction encoding (vs variable-length on x86)
 * - Load/store architecture (no direct memory operands in ALU ops)
 * - AAPCS64 calling convention (8 register args vs 4/6 on x86-64)
 * - Condition codes with branch instructions (no conditional moves in base ISA)
 * - PC-relative addressing with ADRP/ADD for globals
 */

#if !defined(__aarch64__) && !defined(_M_ARM64)
#  error "This file is for AArch64 architecture only. Use jit_x86.c for x86/x64."
#endif

#include <math.h>
#include <string.h>
#include <stddef.h>
#include "jit_common.h"
#include "jit_aarch64_emit.h"
#include "hlsystem.h"

// Helper for LDR/STR scaled offset from struct field
#define FIELD_OFFSET_SCALED(type, field) (offsetof(type, field) / 8)

// ============================================================================
// AArch64 Register Configuration (AAPCS64)
// ============================================================================

/*
 * AAPCS64 (ARM Architecture Procedure Call Standard for ARM64)
 *
 * Register Usage:
 * - X0-X7:   Argument/result registers (caller-saved)
 * - X8:      Indirect result location register (caller-saved)
 * - X9-X15:  Temporary registers (caller-saved)
 * - X16-X17: Intra-procedure-call temporary registers (caller-saved)
 * - X18:     Platform register (avoid use - may be reserved by OS)
 * - X19-X28: Callee-saved registers
 * - X29:     Frame pointer (FP)
 * - X30:     Link register (LR)
 * - SP:      Stack pointer (must be 16-byte aligned)
 *
 * FP/SIMD Registers:
 * - V0-V7:   Argument/result registers (caller-saved)
 * - V8-V15:  Callee-saved (only lower 64 bits, D8-D15)
 * - V16-V31: Temporary registers (caller-saved)
 */

#define RCPU_COUNT 31  // X0-X30 (SP is not a general register)
#define RFPU_COUNT 32  // V0-V31

// Calling convention: first 8 args in X0-X7
#define CALL_NREGS 8
static const Arm64Reg CALL_REGS[] = { X0, X1, X2, X3, X4, X5, X6, X7 };
static const Arm64FpReg FP_CALL_REGS[] = { V0, V1, V2, V3, V4, V5, V6, V7 };

// Caller-saved (scratch) registers: X0-X17 (avoid X18)
// Note: We use X0-X17 as scratch, but X0-X7 are also argument registers
#define RCPU_SCRATCH_COUNT 18

// vdynamic structure: type (8 bytes) + value (8 bytes)
#define HDYN_VALUE 8
static const Arm64Reg RCPU_SCRATCH_REGS[] = {
	X0, X1, X2, X3, X4, X5, X6, X7,
	X8, X9, X10, X11, X12, X13, X14, X15,
	X16, X17
};

// FP register count for allocation pool (V0-V7, V16-V31 are caller-saved; V8-V15 are
// callee-saved per AAPCS64 but we don't save them, so we avoid allocating them)
#define RFPU_SCRATCH_COUNT 32

// Callee-saved registers: X19-X28
// X29 (FP) and X30 (LR) are also callee-saved but handled specially
#define RCPU_CALLEE_SAVED_COUNT 10
static const Arm64Reg RCPU_CALLEE_SAVED[] = {
	X19, X20, X21, X22, X23, X24, X25, X26, X27, X28
};

// Callee-saved registers available for allocation (excludes RTMP/RTMP2)
// These survive function calls, so we don't need to spill them before BLR
#define RCPU_CALLEE_ALLOC_COUNT 8
static const Arm64Reg RCPU_CALLEE_ALLOC[] = {
	X19, X20, X21, X22, X23, X24, X25, X26
};

// FP callee-saved: V8-V15 (only lower 64 bits per AAPCS64)
// NOTE: We intentionally do NOT allocate these registers because our prologue
// doesn't save them. This array is kept for documentation and is_callee_saved_fpu().
#define RFPU_CALLEE_SAVED_COUNT 8
static const Arm64FpReg RFPU_CALLEE_SAVED[] = {
	V8, V9, V10, V11, V12, V13, V14, V15
};

// Helper macros for accessing registers
#define REG_COUNT (RCPU_COUNT + RFPU_COUNT)
#define VFPR(i) ((i) + RCPU_COUNT)  // FP register index
#define PVFPR(i) REG_AT(VFPR(i))    // Pointer to FP register

// Reserved registers for JIT internal use
#define RTMP  X27  // Temporary register for multi-instruction sequences
#define RTMP2 X28  // Second temporary register

// Special purpose registers
#define RFP X29  // Frame pointer
#define RLR X30  // Link register

// Stack alignment requirement
#define STACK_ALIGN 16

// EMIT32 is defined in jit_common.h - use EMIT32(ctx,ctx, val)

// ============================================================================
// Error Handling
// ============================================================================

void _jit_error(jit_ctx *ctx, const char *msg, int line) {
	printf("JIT ERROR: %s (jit_aarch64.c:%d)\n", msg, line);
	if (ctx && ctx->f) {
		// hl_function doesn't have a 'name' field directly
		// The function object info would be in the module
		int func_index = (int)(ctx->f - ctx->m->code->functions);
		printf("In function at index %d\n", func_index);
	}
	jit_exit();
}

void on_jit_error(const char *msg, int_val line) {
	printf("JIT Runtime Error: %s (line %d)\n", msg, (int)line);
	jit_exit();
}

static void jit_null_fail(int fhash) {
	vbyte *field = hl_field_name(fhash);
	hl_buffer *b = hl_alloc_buffer();
	hl_buffer_str(b, USTR("Null access ."));
	hl_buffer_str(b, (uchar*)field);
	vdynamic *d = hl_alloc_dynamic(&hlt_bytes);
	d->v.ptr = hl_buffer_content(b, NULL);
	hl_throw(d);
}

#define JIT_ASSERT(cond) do { if (!(cond)) { \
	printf("JIT ASSERTION FAILED: %s (jit_aarch64.c:%d)\n", #cond, __LINE__); \
	jit_exit(); \
} } while(0)

// ============================================================================
// Register Allocation Helpers
// ============================================================================

/**
 * Check if a CPU register is a call (argument) register
 */
static bool is_call_reg(Arm64Reg r) {
	for (int i = 0; i < CALL_NREGS; i++) {
		if (CALL_REGS[i] == r)
			return true;
	}
	return false;
}

/**
 * Get the index of a register in the call register array
 * Returns -1 if not a call register
 */
static int call_reg_index(Arm64Reg r) {
	for (int i = 0; i < CALL_NREGS; i++) {
		if (CALL_REGS[i] == r)
			return i;
	}
	return -1;
}

/**
 * Check if a register is callee-saved (must be preserved across calls)
 */
static bool is_callee_saved_cpu(Arm64Reg r) {
	for (int i = 0; i < RCPU_CALLEE_SAVED_COUNT; i++) {
		if (RCPU_CALLEE_SAVED[i] == r)
			return true;
	}
	return r == RFP || r == RLR;
}

static bool is_callee_saved_fpu(Arm64FpReg r) {
	for (int i = 0; i < RFPU_CALLEE_SAVED_COUNT; i++) {
		if (RFPU_CALLEE_SAVED[i] == r)
			return true;
	}
	return false;
}

/**
 * Check if type is String (HOBJ with bytes:HBYTES + length:HI32)
 * Used for value-based string comparison per Haxe spec.
 */
static bool is_string_type(hl_type *t) {
	if (t->kind != HOBJ || !t->obj) return false;
	if (t->obj->nfields != 2) return false;
	return t->obj->fields[0].t->kind == HBYTES &&
	       t->obj->fields[1].t->kind == HI32;
}

// ============================================================================
// Register Allocation
// ============================================================================

// Forward declarations
static void free_reg(jit_ctx *ctx, preg *p);
static void patch_jump(jit_ctx *ctx, int pos, int target_pos);

/**
 * Find a free CPU register, evicting if necessary
 * @param k   Register kind (RCPU, RCPU_CALL, etc.)
 * @return    Pointer to allocated physical register
 */
static preg *alloc_cpu(jit_ctx *ctx, preg_kind k) {
	preg *p;
	int i;
	int start_idx = 0;
	int count = RCPU_SCRATCH_COUNT;
	const Arm64Reg *regs = RCPU_SCRATCH_REGS;

	// For RCPU_CALL, only use non-argument registers
	if (k == RCPU_CALL) {
		// Use registers that are NOT in CALL_REGS
		// For now, use X8-X17 (scratch registers that aren't args)
		start_idx = 8;  // Start from X8
	}

	// First pass: find a free scratch register (not holding anything and not locked)
	// Lock check: p->lock >= ctx->currentPos means locked at current operation
	for (i = start_idx; i < count; i++) {
		p = REG_AT(regs[i]);
		if (p->holds == NULL && p->lock < ctx->currentPos)
			return p;
	}

	// Second pass: try callee-saved registers (X19-X26) before evicting scratch
	// These survive function calls, so values don't need to be spilled before BLR
	for (i = 0; i < RCPU_CALLEE_ALLOC_COUNT; i++) {
		p = REG_AT(RCPU_CALLEE_ALLOC[i]);
		if (p->holds == NULL && p->lock < ctx->currentPos) {
			ctx->callee_saved_used |= (1 << i);  // Mark register as used for Phase 2 NOP patching
			return p;
		}
	}

	// Third pass: evict a callee-saved register if one is unlocked
	for (i = 0; i < RCPU_CALLEE_ALLOC_COUNT; i++) {
		p = REG_AT(RCPU_CALLEE_ALLOC[i]);
		if (p->lock < ctx->currentPos) {
			ctx->callee_saved_used |= (1 << i);  // Mark register as used for Phase 2 NOP patching
			free_reg(ctx, p);  // Spill to stack before reusing
			return p;
		}
	}

	// Fourth pass: evict a scratch register
	for (i = start_idx; i < count; i++) {
		p = REG_AT(regs[i]);
		if (p->lock < ctx->currentPos) {
			free_reg(ctx, p);  // Spill to stack before reusing
			return p;
		}
	}

	// All registers are locked - this is an error
	JIT_ASSERT(0);
	return NULL;
}

/**
 * Allocate a floating-point register
 *
 * IMPORTANT: We only use caller-saved FP registers (V0-V7, V16-V31).
 * V8-V15 are callee-saved per AAPCS64, and since our prologue/epilogue
 * doesn't save/restore them, we must not allocate them.
 *
 * This gives us 24 FP registers which is sufficient for most code.
 * If all are in use, we evict (spill to stack) the least recently used.
 */
static preg *alloc_fpu(jit_ctx *ctx) {
	preg *p;
	int i;

	// First pass: find a free caller-saved register (V0-V7, V16-V31)
	// Lock check: p->lock >= ctx->currentPos means locked at current operation
	for (i = 0; i < RFPU_COUNT; i++) {
		if (i >= 8 && i < 16)
			continue;  // NEVER use callee-saved V8-V15 - they aren't saved in prologue
		p = PVFPR(i);
		if (p->holds == NULL && p->lock < ctx->currentPos)
			return p;
	}

	// Second pass: evict an unlocked caller-saved register
	// Only iterate over V0-V7 and V16-V31, skip V8-V15
	for (i = 0; i < RFPU_COUNT; i++) {
		if (i >= 8 && i < 16)
			continue;  // NEVER use callee-saved V8-V15
		p = PVFPR(i);
		if (p->lock < ctx->currentPos) {
			free_reg(ctx, p);  // Spill to stack before reusing
			return p;
		}
	}

	JIT_ASSERT(0);
	return NULL;
}

/**
 * Allocate a register of the appropriate type based on the virtual register's type
 */
static preg *alloc_reg(jit_ctx *ctx, vreg *r, preg_kind k) {
	if (IS_FLOAT(r))
		return alloc_fpu(ctx);
	else
		return alloc_cpu(ctx, k);
}

// ============================================================================
// Register State Management
// ============================================================================

/**
 * Store a virtual register to its stack location
 */
static void store(jit_ctx *ctx, vreg *r, preg *p);  // Forward declaration
static void mov_reg_reg(jit_ctx *ctx, Arm64Reg dst, Arm64Reg src, bool is_64bit);  // Forward declaration
static void ldr_stack(jit_ctx *ctx, Arm64Reg dst, int stack_offset, int size);  // Forward declaration
static void emit_call_findex(jit_ctx *ctx, int findex, int stack_space);  // Forward declaration

/**
 * Free a physical register by storing its content to stack if needed
 */
static void free_reg(jit_ctx *ctx, preg *p) {
	vreg *r = p->holds;
	if (r != NULL) {
		store(ctx, r, p);
		r->current = NULL;
		p->holds = NULL;
	}
	// Unlock the register so it can be reused
	RUNLOCK(p);
}

/**
 * Discard the content of a physical register, storing if dirty.
 * Used when we're done using a register but the vreg might still be live.
 * If the vreg is dirty (modified but not yet on stack), we store it first.
 */
static void discard(jit_ctx *ctx, preg *p) {
	vreg *r = p->holds;
	if (r != NULL) {
		// If dirty, store to stack before clearing the binding
		if (r->dirty) {
			store(ctx, r, p);
		}
		r->current = NULL;
		p->holds = NULL;
	}
	// Unlock the register so it can be reused
	RUNLOCK(p);
}

/**
 * Spill all caller-saved registers to stack before a function call.
 * In AAPCS64: X0-X17 and V0-V7 are caller-saved and may be clobbered.
 *
 * This function:
 *   1. Stores each bound register's value to its vreg's stack slot
 *   2. Clears the register↔vreg bindings
 *
 * IMPORTANT: Must be called BEFORE the BLR instruction, not after!
 * At that point register values are still valid and can be spilled to stack.
 * After the call, caller-saved registers contain garbage from the callee.
 *
 * ARCHITECTURAL NOTE - Why AArch64 differs from x86:
 *
 * The x86 JIT's discard_regs() just clears register bindings without spilling.
 * This works because x86 (CISC) can use memory operands directly in ALU
 * instructions:
 *
 *     x86:    ADD [rbp-8], rax    ; Operate directly on stack slot
 *
 * So x86 treats stack slots as the "source of truth" - values are written
 * to stack as part of normal operations, and registers are just caches.
 * Clearing bindings is safe because the value is already on the stack.
 *
 * AArch64 (RISC load/store architecture) cannot do this:
 *
 *     AArch64: LDR x1, [fp, #-8]  ; Must load to register first
 *              ADD x0, x0, x1     ; Operate on registers only
 *              STR x0, [fp, #-8]  ; Separate store instruction
 *
 * Adding a store after every operation would cost ~1 extra instruction per op.
 * Instead, we keep values in registers (registers are "source of truth") and
 * only spill when necessary - specifically, before function calls that will
 * clobber caller-saved registers.
 *
 * This is not a workaround but the natural design for load/store architectures.
 */
static void spill_regs(jit_ctx *ctx) {
	int i;
	// Spill and discard CPU scratch registers (X0-X17) - these get clobbered by calls
	for (i = 0; i < 18; i++) {
		preg *r = &ctx->pregs[i];
		if (r->holds) {
			if (r->holds->dirty) {
				free_reg(ctx, r);  // Dirty: store to stack, then clear binding
			} else {
				discard(ctx, r);   // Clean: just clear binding (value already on stack)
			}
		}
	}
	// NOTE: Do NOT spill callee-saved registers (X19-X26)!
	// They survive function calls, so their values remain valid after BLR.
	// This is the key optimization - values in callee-saved don't need spilling.

	// Spill and discard FPU scratch registers (V0-V7, V16-V31) - these get clobbered by calls
	// NOTE: V8-V15 are callee-saved per AAPCS64, but we intentionally never allocate them
	// (see alloc_fpu) since our prologue doesn't save them. No need to handle them here.
	for (i = 0; i < 8; i++) {
		preg *r = &ctx->pregs[RCPU_COUNT + i];
		if (r->holds) {
			if (r->holds->dirty) {
				free_reg(ctx, r);  // Dirty: store to stack, then clear binding
			} else {
				discard(ctx, r);   // Clean: just clear binding (value already on stack)
			}
		}
	}
	// Also spill V16-V31 (caller-saved temporary FPU registers)
	for (i = 16; i < 32; i++) {
		preg *r = &ctx->pregs[RCPU_COUNT + i];
		if (r->holds) {
			if (r->holds->dirty) {
				free_reg(ctx, r);  // Dirty: store to stack, then clear binding
			} else {
				discard(ctx, r);   // Clean: just clear binding (value already on stack)
			}
		}
	}
}

/**
 * Spill callee-saved registers to stack.
 * Called before jumps to labels - callee-saved must be on stack at merge points.
 * NOTE: This is NOT called before function calls (callee-saved survive calls).
 */
static void spill_callee_saved(jit_ctx *ctx) {
	int i;
	// Spill callee-saved CPU registers (X19-X26) that are in use
	for (i = 0; i < RCPU_CALLEE_ALLOC_COUNT; i++) {
		preg *r = REG_AT(RCPU_CALLEE_ALLOC[i]);
		if (r->holds) {
			if (r->holds->dirty) {
				free_reg(ctx, r);  // Dirty: store to stack, then clear binding
			} else {
				discard(ctx, r);   // Clean: just clear binding
			}
		}
	}
}

/**
 * Ensure a virtual register is in a physical register
 * Loads from stack if necessary
 */
static preg *fetch(jit_ctx *ctx, vreg *r);  // Forward declaration

/**
 * Bind a vreg to a physical register (bidirectional association)
 * This is essential for proper spilling when the register is evicted
 */
static void reg_bind(jit_ctx *ctx, vreg *r, preg *p) {
	// If vreg was dirty in another register, store to stack first
	// This prevents losing values when rebinding (e.g., dst = dst op src)
	if (r->current && r->current != p) {
		if (r->dirty) {
			store(ctx, r, r->current);
		}
		r->current->holds = NULL;
	}
	// Set new binding
	r->current = p;
	p->holds = r;
}

/**
 * Allocate a destination register for a vreg
 * Helper function used by many operations
 * Binds the vreg to the allocated register for proper spilling
 */
static preg *alloc_dst(jit_ctx *ctx, vreg *r) {
	preg *p;
	if (IS_FLOAT(r)) {
		p = alloc_fpu(ctx);
	} else {
		p = alloc_cpu(ctx, RCPU);
	}
	// Bind the vreg to this register so we can spill it later if needed
	reg_bind(ctx, r, p);
	// Mark dirty: a new value is about to be written to this register,
	// and it's not on the stack yet. This ensures spill_regs() will
	// store it before the next call/jump.
	r->dirty = 1;
	return p;
}

// ============================================================================
// Basic Data Movement - Encoding Helpers
// ============================================================================

/**
 * Generate MOV instruction (register to register)
 * For integer: MOV Xd, Xn (using ORR Xd, XZR, Xn)
 * For float: FMOV Vd, Vn
 */
static void mov_reg_reg(jit_ctx *ctx, Arm64Reg dst, Arm64Reg src, bool is_64bit) {
	// SP (register 31) can't be used with ORR - must use ADD instead
	if (src == SP_REG || dst == SP_REG) {
		// MOV Xd, SP or MOV SP, Xn => ADD Xd, Xn, #0
		encode_add_sub_imm(ctx, is_64bit ? 1 : 0, 0, 0, 0, 0, src, dst);
	} else if (is_64bit) {
		// MOV Xd, Xn => ORR Xd, XZR, Xn
		encode_logical_reg(ctx, 1, 0x01, 0, 0, src, 0, XZR, dst);
	} else {
		// MOV Wd, Wn => ORR Wd, WZR, Wn
		encode_logical_reg(ctx, 0, 0x01, 0, 0, src, 0, XZR, dst);
	}
}

static void fmov_reg_reg(jit_ctx *ctx, Arm64FpReg dst, Arm64FpReg src, bool is_double) {
	// FMOV Vd, Vn (using FP 1-source with opcode 0)
	int type = is_double ? 0x01 : 0x00;  // 01=double, 00=single
	encode_fp_1src(ctx, 0, 0, type, 0, src, dst);
}

/**
 * Load from stack to register
 * Format: LDR/LDUR Xt, [FP, #offset]
 *
 * Uses LDUR for signed offsets in range [-256, +255] (single instruction)
 * Uses LDR with scaled unsigned offset for aligned positive offsets
 * Falls back to computing address in register for large offsets
 */
static void ldr_stack(jit_ctx *ctx, Arm64Reg dst, int stack_offset, int size) {
	int size_enc = (size == 8) ? 3 : ((size == 4) ? 2 : ((size == 2) ? 1 : 0));

	// Priority 1: Use LDUR for small signed offsets (-256 to +255)
	// This handles most negative stack offsets in a single instruction
	if (stack_offset >= -256 && stack_offset <= 255) {
		encode_ldur_stur(ctx, size_enc, 0, 0x01, stack_offset, RFP, dst);
		return;
	}

	// Priority 2: Use LDR with scaled unsigned offset for larger positive aligned offsets
	if (stack_offset >= 0 && (stack_offset % size == 0) && stack_offset < 4096 * size) {
		int scaled_offset = stack_offset / size;
		encode_ldr_str_imm(ctx, size_enc, 0, 0x01, scaled_offset, RFP, dst);
		return;
	}

	// Fallback: Compute address in register for large/unaligned offsets
	load_immediate(ctx, stack_offset, RTMP, true);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, RFP, RTMP);
	encode_ldr_str_imm(ctx, size_enc, 0, 0x01, 0, RTMP, dst);
}

/**
 * Load from stack to FP register
 * Format: LDR/LDUR Dt/St, [FP, #offset]
 */
static void ldr_stack_fp(jit_ctx *ctx, Arm64FpReg dst, int stack_offset, int size) {
	int size_enc = (size == 8) ? 3 : ((size == 4) ? 2 : 1);

	// Priority 1: Use LDUR for small signed offsets (-256 to +255)
	if (stack_offset >= -256 && stack_offset <= 255) {
		encode_ldur_stur(ctx, size_enc, 1, 0x01, stack_offset, RFP, dst);
		return;
	}

	// Priority 2: Use LDR with scaled unsigned offset for larger positive aligned offsets
	if (stack_offset >= 0 && (stack_offset % size == 0) && stack_offset < 4096 * size) {
		int scaled_offset = stack_offset / size;
		encode_ldr_str_imm(ctx, size_enc, 1, 0x01, scaled_offset, RFP, dst);
		return;
	}

	// Fallback: Compute address in register
	load_immediate(ctx, stack_offset, RTMP, true);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, RFP, RTMP);
	encode_ldr_str_imm(ctx, size_enc, 1, 0x01, 0, RTMP, dst);
}

/**
 * Store register to stack
 * Format: STR/STUR Xt, [FP, #offset]
 *
 * Uses STUR for signed offsets in range [-256, +255] (single instruction)
 * Uses STR with scaled unsigned offset for aligned positive offsets
 * Falls back to computing address in register for large offsets
 */
static void str_stack(jit_ctx *ctx, Arm64Reg src, int stack_offset, int size) {
	int size_enc = (size == 8) ? 3 : ((size == 4) ? 2 : ((size == 2) ? 1 : 0));

	// Priority 1: Use STUR for small signed offsets (-256 to +255)
	if (stack_offset >= -256 && stack_offset <= 255) {
		encode_ldur_stur(ctx, size_enc, 0, 0x00, stack_offset, RFP, src);
		return;
	}

	// Priority 2: Use STR with scaled unsigned offset for larger positive aligned offsets
	if (stack_offset >= 0 && (stack_offset % size == 0) && stack_offset < 4096 * size) {
		int scaled_offset = stack_offset / size;
		encode_ldr_str_imm(ctx, size_enc, 0, 0x00, scaled_offset, RFP, src);
		return;
	}

	// Fallback: Compute address in register
	load_immediate(ctx, stack_offset, RTMP, true);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, RFP, RTMP);
	encode_ldr_str_imm(ctx, size_enc, 0, 0x00, 0, RTMP, src);
}

/**
 * Store FP register to stack
 * Format: STR/STUR Dt/St, [FP, #offset]
 */
static void str_stack_fp(jit_ctx *ctx, Arm64FpReg src, int stack_offset, int size) {
	int size_enc = (size == 8) ? 3 : ((size == 4) ? 2 : 1);

	// Priority 1: Use STUR for small signed offsets (-256 to +255)
	if (stack_offset >= -256 && stack_offset <= 255) {
		encode_ldur_stur(ctx, size_enc, 1, 0x00, stack_offset, RFP, src);
		return;
	}

	// Priority 2: Use STR with scaled unsigned offset for larger positive aligned offsets
	if (stack_offset >= 0 && (stack_offset % size == 0) && stack_offset < 4096 * size) {
		int scaled_offset = stack_offset / size;
		encode_ldr_str_imm(ctx, size_enc, 1, 0x00, scaled_offset, RFP, src);
		return;
	}

	// Fallback: Compute address in register
	load_immediate(ctx, stack_offset, RTMP, true);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, RFP, RTMP);
	encode_ldr_str_imm(ctx, size_enc, 1, 0x00, 0, RTMP, src);
}

/**
 * STP with signed offset (no writeback) - for NOPpable callee-saved saves.
 * Format: STP Xt1, Xt2, [Xn, #imm]
 * This allows individual STPs to be patched to NOPs without affecting SP.
 */
static void stp_offset(jit_ctx *ctx, Arm64Reg rt, Arm64Reg rt2, Arm64Reg rn, int offset) {
	int imm7 = offset / 8;
	// opc=10 (64-bit), 101, addr_mode=10 (signed offset), L=0 (store), imm7, Rt2, Rn, Rt
	unsigned int insn = (2u << 30) | (5u << 27) | (2u << 23) | (0u << 22) |
	                    ((imm7 & 0x7F) << 15) | (rt2 << 10) | (rn << 5) | rt;
	EMIT32(ctx, insn);
}

/**
 * LDP with signed offset (no writeback) - for NOPpable callee-saved restores.
 * Format: LDP Xt1, Xt2, [Xn, #imm]
 */
static void ldp_offset(jit_ctx *ctx, Arm64Reg rt, Arm64Reg rt2, Arm64Reg rn, int offset) {
	int imm7 = offset / 8;
	// opc=10 (64-bit), 101, addr_mode=10 (signed offset), L=1 (load), imm7, Rt2, Rn, Rt
	unsigned int insn = (2u << 30) | (5u << 27) | (2u << 23) | (1u << 22) |
	                    ((imm7 & 0x7F) << 15) | (rt2 << 10) | (rn << 5) | rt;
	EMIT32(ctx, insn);
}

// ============================================================================
// Data Movement Operations
// ============================================================================

/**
 * Store a virtual register to its stack location
 */
static void store(jit_ctx *ctx, vreg *r, preg *p) {
	if (r == NULL || p == NULL || r->size == 0)
		return;

	int size = r->size;
	int offset = r->stackPos;

	if (p->kind == RCPU) {
		str_stack(ctx, p->id, offset, size);
	} else if (p->kind == RFPU) {
		str_stack_fp(ctx, p->id, offset, size);
	}

	r->dirty = 0;  // Stack is now up-to-date
}

/**
 * Mark a virtual register as dirty (register value differs from stack).
 * The value will be spilled at the next basic block boundary (jump, call, label).
 * This defers stores to reduce instruction count within basic blocks.
 */
static void mark_dirty(jit_ctx *ctx, vreg *r) {
	(void)ctx;  // unused, kept for consistency with other functions
	if (r != NULL && r->current != NULL && r->size > 0) {
		r->dirty = 1;
	}
}

/**
 * Store to a vreg's stack slot and clear any stale register binding.
 * Use this when storing directly (e.g., from X0 after a call) without
 * going through the normal register allocation path.
 *
 * This prevents spill_regs from later overwriting the correct stack
 * value with a stale register value.
 */
static void store_result(jit_ctx *ctx, vreg *dst) {
	// Clear any stale binding - the correct value is now on stack
	if (dst->current != NULL) {
		dst->current->holds = NULL;
		dst->current = NULL;
	}
}

/**
 * Load a virtual register from stack to a physical register
 */
static preg *fetch(jit_ctx *ctx, vreg *r) {
	preg *p;

	// HVOID registers have size 0 and no value to load
	if (r->size == 0)
		return UNUSED;

	// Check if already in a register
	if (r->current != NULL && r->current->kind != RSTACK) {
		// Lock the register to prevent eviction during subsequent allocations
		RLOCK(r->current);
		return r->current;
	}

	// Allocate a register
	p = alloc_reg(ctx, r, RCPU);

	// If the register we got already holds something, evict it
	if (p->holds != NULL)
		free_reg(ctx, p);

	// Load from stack
	int size = r->size;
	int offset = r->stackPos;

	if (IS_FLOAT(r)) {
		ldr_stack_fp(ctx, p->id, offset, size);
	} else {
		ldr_stack(ctx, p->id, offset, size);
	}

	// Bind vreg to register and lock it to prevent eviction by subsequent allocs
	reg_bind(ctx, r, p);
	RLOCK(p);

	return p;
}

/**
 * Copy data between locations (register, stack, immediate)
 * This is the main data movement workhorse function
 */
static void copy(jit_ctx *ctx, vreg *dst, preg *dst_p, vreg *src, preg *src_p) {
	if (src_p->kind == RCONST) {
		// Load immediate into destination
		int64_t val = src_p->id;

		if (IS_FLOAT(dst)) {
			// Load float constant: load bits as integer, then move to FP register
			preg *d = (dst_p && dst_p->kind == RFPU) ? dst_p : alloc_fpu(ctx);

			if (val == 0) {
				// FMOV Dd, XZR - zero the FP register
				EMIT32(ctx, (1 << 31) | (0 << 29) | (0x1E << 24) | (1 << 22) | (1 << 21) | (7 << 16) | (31 << 5) | d->id);
			} else {
				// Load bits to GPR, then FMOV to FPR
				load_immediate(ctx, val, RTMP, true);
				// FMOV Dd, Xn: sf=1, S=0, type=01, rmode=00, opcode=00111, Rn, Rd
				EMIT32(ctx, (0x9E670000) | (RTMP << 5) | d->id);
			}

			if (dst_p == NULL || dst_p != d) {
				reg_bind(ctx, dst, d);
			}
		} else {
			// Load integer immediate
			preg *d = (dst_p && dst_p->kind == RCPU) ? dst_p : fetch(ctx, dst);
			load_immediate(ctx, val, d->id, dst->size == 8);
			if (dst_p == NULL || dst_p != d) {
				reg_bind(ctx, dst, d);
			}
		}
	} else if (src_p->kind == RCPU && dst_p && dst_p->kind == RCPU) {
		// Register to register
		mov_reg_reg(ctx, dst_p->id, src_p->id, dst->size == 8);
	} else if (src_p->kind == RFPU && dst_p && dst_p->kind == RFPU) {
		// FP register to FP register
		fmov_reg_reg(ctx, dst_p->id, src_p->id, dst->size == 8);
	} else {
		// Generic case: fetch src, store to dst
		preg *s = (src_p && (src_p->kind == RCPU || src_p->kind == RFPU)) ? src_p : fetch(ctx, src);
		preg *d = (dst_p && (dst_p->kind == RCPU || dst_p->kind == RFPU)) ? dst_p : fetch(ctx, dst);

		if (IS_FLOAT(dst)) {
			fmov_reg_reg(ctx, d->id, s->id, dst->size == 8);
		} else {
			mov_reg_reg(ctx, d->id, s->id, dst->size == 8);
		}

		reg_bind(ctx, dst, d);
	}
}

// ============================================================================
// Opcode Handlers
// ============================================================================

/**
 * OMov - Move/copy a value from one register to another
 */
static void op_mov(jit_ctx *ctx, vreg *dst, vreg *src) {
	preg *r = fetch(ctx, src);

	// Handle special case for HF32 (32-bit float)
	// Ensure it's in an FP register
	if (src->t->kind == HF32 && r->kind != RFPU) {
		r = alloc_fpu(ctx);
		// Load from stack to FP register
		ldr_stack_fp(ctx, r->id, src->stackPos, src->size);
		reg_bind(ctx, src, r);
	}

	// Store to destination stack slot
	store(ctx, dst, r);

	// Clear dst's old register binding to prevent stale value from being spilled
	// The correct value is now on the stack from store() above
	if (dst->current != NULL) {
		dst->current->holds = NULL;
		dst->current = NULL;
	}
}

/**
 * Store a constant value to a virtual register
 */
static void store_const(jit_ctx *ctx, vreg *dst, int64_t val) {
	preg *p;

	if (IS_FLOAT(dst)) {
		// Allocate FPU register for float constants
		p = alloc_fpu(ctx);
		if (p->holds != NULL)
			free_reg(ctx, p);

		if (val == 0) {
			// FMOV Dd, XZR - zero the FP register
			EMIT32(ctx, (1 << 31) | (0 << 29) | (0x1E << 24) | (1 << 22) | (1 << 21) | (7 << 16) | (31 << 5) | p->id);
		} else {
			// Load bits to GPR, then FMOV to FPR
			load_immediate(ctx, val, RTMP, true);
			// FMOV Dd, Xn: sf=1, S=0, type=01, rmode=00, opcode=00111, Rn, Rd
			EMIT32(ctx, (0x9E670000) | (RTMP << 5) | p->id);
		}
	} else {
		p = alloc_reg(ctx, dst, RCPU);
		if (p->holds != NULL)
			free_reg(ctx, p);
		load_immediate(ctx, val, p->id, dst->size == 8);
	}

	reg_bind(ctx, dst, p);
	store(ctx, dst, p);  // Constants must be stored immediately for correct loop initialization
}

// ============================================================================
// Arithmetic Operations
// ============================================================================

// Forward declaration for op_call_native (used by floating-point modulo)
static void op_call_native(jit_ctx *ctx, vreg *dst, hl_type *ftype, void *func_ptr, vreg **args, int nargs);
// Forward declaration for prepare_call_args (used by op_jump for dynamic comparisons)
static int prepare_call_args(jit_ctx *ctx, hl_type **arg_types, vreg **args, int nargs, bool is_native);

/**
 * Binary arithmetic/logic operations handler
 * Handles: OAdd, OSub, OMul, OSDiv, OUDiv, OSMod, OUMod, OAnd, OOr, OXor, shifts
 */
static void op_binop(jit_ctx *ctx, vreg *dst, vreg *a, vreg *b, hl_op op) {
	bool is_64bit = dst->size == 8;
	int sf = is_64bit ? 1 : 0;

	// Handle floating-point operations
	if (IS_FLOAT(dst)) {
		preg *pa = fetch(ctx, a);
		preg *pb = fetch(ctx, b);
		preg *pd;

		// If dst == a, reuse pa as destination to avoid clobbering issues
		// when reg_bind tries to store the old (now stale) value
		if (dst == a) {
			pd = pa;
		} else {
			pd = alloc_fpu(ctx);
			if (pd->holds != NULL)
				free_reg(ctx, pd);
		}

		int type = (dst->t->kind == HF64) ? 0x01 : 0x00;  // 01=double, 00=single

		switch (op) {
		case OAdd:
			// FADD Vd, Vn, Vm
			encode_fp_arith(ctx, 0, 0, type, pb->id, 0x02, pa->id, pd->id);
			break;
		case OSub:
			// FSUB Vd, Vn, Vm
			encode_fp_arith(ctx, 0, 0, type, pb->id, 0x03, pa->id, pd->id);
			break;
		case OMul:
			// FMUL Vd, Vn, Vm
			encode_fp_arith(ctx, 0, 0, type, pb->id, 0x00, pa->id, pd->id);
			break;
		case OSDiv:
		case OUDiv:  // Same as OSDiv for floats
			// FDIV Vd, Vn, Vm
			encode_fp_arith(ctx, 0, 0, type, pb->id, 0x01, pa->id, pd->id);
			break;
		case OSMod:
		case OUMod: {
			// Floating-point modulo: call fmod/fmodf from C library
			// Need to discard pa/pb since op_call_native will spill
			discard(ctx, pa);
			discard(ctx, pb);
			void *mod_func = (dst->t->kind == HF64) ? (void*)fmod : (void*)fmodf;
			vreg *args[2] = { a, b };
			op_call_native(ctx, dst, NULL, mod_func, args, 2);
			return;  // op_call_native handles result storage
		}
		default:
			JIT_ASSERT(0);  // Invalid FP operation
		}

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
		return;
	}

	// Integer operations
	preg *pa = fetch(ctx, a);
	preg *pb = fetch(ctx, b);
	preg *pd;

	// If dst == a, reuse pa as destination to avoid clobbering issues
	// when reg_bind tries to store the old (now stale) value
	if (dst == a) {
		pd = pa;
	} else {
		pd = alloc_cpu(ctx, RCPU);
		if (pd->holds != NULL)
			free_reg(ctx, pd);
	}

	switch (op) {
	case OAdd:
		// ADD Xd, Xn, Xm
		encode_add_sub_reg(ctx, sf, 0, 0, 0, pb->id, 0, pa->id, pd->id);
		break;

	case OSub:
		// SUB Xd, Xn, Xm
		encode_add_sub_reg(ctx, sf, 1, 0, 0, pb->id, 0, pa->id, pd->id);
		break;

	case OMul:
		// MUL Xd, Xn, Xm  (using MADD with XZR as addend)
		encode_madd_msub(ctx, sf, 0, pb->id, XZR, pa->id, pd->id);
		break;

	case OSDiv:
		// SDIV Xd, Xn, Xm (signed division)
		// Note: encode_div U=1 means SDIV, U=0 means UDIV (per ARM ISA)
		encode_div(ctx, sf, 1, pb->id, pa->id, pd->id);
		break;

	case OUDiv:
		// UDIV Xd, Xn, Xm (unsigned division)
		encode_div(ctx, sf, 0, pb->id, pa->id, pd->id);
		break;

	case OSMod: {
		// Signed modulo with special case handling:
		// - divisor == 0: return 0 (avoid returning dividend)
		// - divisor == -1: return 0 (avoid MIN % -1 overflow)
		// CBZ divisor, zero_case
		int jz = BUF_POS();
		encode_cbz_cbnz(ctx, sf, 0, 0, pb->id);  // CBZ

		// CMP divisor, #-1; B.EQ zero_case
		// CMN is ADD setting flags, so CMN Xn, #1 checks if Xn == -1
		encode_add_sub_imm(ctx, sf, 1, 1, 0, 1, pb->id, XZR);  // CMN divisor, #1
		int jneg1 = BUF_POS();
		encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ

		// Normal path: remainder = dividend - (quotient * divisor)
		encode_div(ctx, sf, 1, pb->id, pa->id, RTMP);  // RTMP = a / b (signed)
		encode_madd_msub(ctx, sf, 1, pb->id, pa->id, RTMP, pd->id);  // pd = a - (RTMP * b)
		int jend = BUF_POS();
		encode_branch_uncond(ctx, 0);  // B end

		// Zero case: return 0
		int zero_pos = BUF_POS();
		// MOV pd, #0 (using ORR with XZR)
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, pd->id);  // ORR pd, XZR, XZR

		patch_jump(ctx, jz, zero_pos);
		patch_jump(ctx, jneg1, zero_pos);
		patch_jump(ctx, jend, BUF_POS());
		break;
	}

	case OUMod: {
		// Unsigned modulo with special case:
		// - divisor == 0: return 0
		// CBZ divisor, zero_case
		int jz = BUF_POS();
		encode_cbz_cbnz(ctx, sf, 0, 0, pb->id);  // CBZ

		// Normal path
		encode_div(ctx, sf, 0, pb->id, pa->id, RTMP);  // RTMP = a / b (unsigned)
		encode_madd_msub(ctx, sf, 1, pb->id, pa->id, RTMP, pd->id);  // pd = a - (RTMP * b)
		int jend = BUF_POS();
		encode_branch_uncond(ctx, 0);  // B end

		// Zero case: return 0
		int zero_pos = BUF_POS();
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, pd->id);  // ORR pd, XZR, XZR

		patch_jump(ctx, jz, zero_pos);
		patch_jump(ctx, jend, BUF_POS());
		break;
	}

	case OAnd:
		// AND Xd, Xn, Xm
		encode_logical_reg(ctx, sf, 0x00, 0, 0, pb->id, 0, pa->id, pd->id);
		break;

	case OOr:
		// ORR Xd, Xn, Xm
		encode_logical_reg(ctx, sf, 0x01, 0, 0, pb->id, 0, pa->id, pd->id);
		break;

	case OXor:
		// EOR Xd, Xn, Xm
		encode_logical_reg(ctx, sf, 0x02, 0, 0, pb->id, 0, pa->id, pd->id);
		break;

	case OShl:
		// LSL Xd, Xn, Xm (logical shift left)
		encode_shift_reg(ctx, sf, 0x00, pb->id, pa->id, pd->id);
		break;

	case OUShr:
		// LSR Xd, Xn, Xm (logical shift right - unsigned)
		encode_shift_reg(ctx, sf, 0x01, pb->id, pa->id, pd->id);
		break;

	case OSShr:
		// ASR Xd, Xn, Xm (arithmetic shift right - signed)
		encode_shift_reg(ctx, sf, 0x02, pb->id, pa->id, pd->id);
		break;

	default:
		JIT_ASSERT(0);  // Unknown operation
	}

	// Mask result for sub-32-bit integer types (UI8, UI16)
	// AArch64 doesn't have 8/16-bit registers like x86, so we need explicit masking
	if (dst->t->kind == HUI8 || dst->t->kind == HBOOL) {
		// AND Wd, Wd, #0xFF (sf=0, opc=0, N=0, immr=0, imms=7)
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, pd->id, pd->id);
	} else if (dst->t->kind == HUI16) {
		// AND Wd, Wd, #0xFFFF (sf=0, opc=0, N=0, immr=0, imms=15)
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, pd->id, pd->id);
	}

	reg_bind(ctx, dst, pd);
	mark_dirty(ctx, dst);
}

/**
 * Unary negation (ONeg)
 */
static void op_neg(jit_ctx *ctx, vreg *dst, vreg *a) {
	if (IS_FLOAT(a)) {
		// FNEG Vd, Vn
		preg *pa = fetch(ctx, a);
		preg *pd = alloc_fpu(ctx);

		if (pd->holds != NULL)
			free_reg(ctx, pd);

		int type = (dst->t->kind == HF64) ? 0x01 : 0x00;
		encode_fp_1src(ctx, 0, 0, type, 0x02, pa->id, pd->id);

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
	} else {
		// NEG Xd, Xn  (implemented as SUB Xd, XZR, Xn)
		preg *pa = fetch(ctx, a);
		preg *pd = alloc_cpu(ctx, RCPU);

		if (pd->holds != NULL)
			free_reg(ctx, pd);

		int sf = (dst->size == 8) ? 1 : 0;
		encode_add_sub_reg(ctx, sf, 1, 0, 0, pa->id, 0, XZR, pd->id);

		// Mask result for sub-32-bit integer types
		if (dst->t->kind == HUI8 || dst->t->kind == HBOOL) {
			encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, pd->id, pd->id);
		} else if (dst->t->kind == HUI16) {
			encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, pd->id, pd->id);
		}

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
	}
}

/**
 * Logical NOT (ONot) - boolean negation
 */
static void op_not(jit_ctx *ctx, vreg *dst, vreg *a) {
	// XOR with 1 (boolean NOT)
	preg *pa = fetch(ctx, a);
	preg *pd = alloc_cpu(ctx, RCPU);

	if (pd->holds != NULL)
		free_reg(ctx, pd);

	// Load immediate 1
	load_immediate(ctx, 1, RTMP, false);

	// EOR Wd, Wn, Wtmp (32-bit XOR with 1)
	encode_logical_reg(ctx, 0, 0x02, 0, 0, RTMP, 0, pa->id, pd->id);

	reg_bind(ctx, dst, pd);
	mark_dirty(ctx, dst);
}

/**
 * Increment (OIncr)
 */
static void op_incr(jit_ctx *ctx, vreg *dst) {
	// ADD Xd, Xd, #1 with memory writeback
	preg *pd = fetch(ctx, dst);
	int sf = (dst->size == 8) ? 1 : 0;

	// ADD Xd, Xn, #1
	encode_add_sub_imm(ctx, sf, 0, 0, 0, 1, pd->id, pd->id);

	// Mask result for sub-32-bit integer types
	if (dst->t->kind == HUI8 || dst->t->kind == HBOOL) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, pd->id, pd->id);
	} else if (dst->t->kind == HUI16) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, pd->id, pd->id);
	}

	mark_dirty(ctx, dst);
}

/**
 * Decrement (ODecr)
 */
static void op_decr(jit_ctx *ctx, vreg *dst) {
	// SUB Xd, Xd, #1 with memory writeback
	preg *pd = fetch(ctx, dst);
	int sf = (dst->size == 8) ? 1 : 0;

	// SUB Xd, Xn, #1
	encode_add_sub_imm(ctx, sf, 1, 0, 0, 1, pd->id, pd->id);

	// Mask result for sub-32-bit integer types
	if (dst->t->kind == HUI8 || dst->t->kind == HBOOL) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, pd->id, pd->id);
	} else if (dst->t->kind == HUI16) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, pd->id, pd->id);
	}

	mark_dirty(ctx, dst);
}

// ============================================================================
// Type Conversion Operations
// ============================================================================

/**
 * Convert to integer (OToInt)
 * Handles: float->int, i32->i64 sign extension, and int->int copy
 */
static void op_toint(jit_ctx *ctx, vreg *dst, vreg *src) {
	// Same register optimization
	if (dst == src) return;

	// Case 1: Float to integer conversion
	if (IS_FLOAT(src)) {
		preg *ps = fetch(ctx, src);
		preg *pd = alloc_cpu(ctx, RCPU);

		if (pd->holds != NULL)
			free_reg(ctx, pd);

		int sf = (dst->size == 8) ? 1 : 0;
		int type = (src->t->kind == HF64) ? 0x01 : 0x00;

		// FCVTZS Xd, Vn (float to signed int, round toward zero)
		encode_fcvt_int(ctx, sf, 0, type, 0x03, 0x00, ps->id, pd->id);

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
		return;
	}

	// Case 2: i32 to i64 sign extension
	if (dst->size == 8 && src->size == 4) {
		preg *ps = fetch(ctx, src);
		preg *pd = alloc_cpu(ctx, RCPU);

		if (pd->holds != NULL)
			free_reg(ctx, pd);

		Arm64Reg src_r = (ps->kind == RCPU) ? (Arm64Reg)ps->id : RTMP;
		if (ps->kind == RCONST) {
			load_immediate(ctx, ps->id, src_r, false);
		} else if (ps->kind != RCPU) {
			ldr_stack(ctx, src_r, src->stackPos, src->size);
		}

		// SXTW Xd, Wn (sign extend word to doubleword)
		// Encoding: 0x93407c00 | (Rn << 5) | Rd
		EMIT32(ctx, 0x93407c00 | (src_r << 5) | pd->id);

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
		return;
	}

	// Case 3: Integer to integer copy (same size or truncation)
	preg *ps = fetch(ctx, src);
	preg *pd = alloc_cpu(ctx, RCPU);

	if (pd->holds != NULL)
		free_reg(ctx, pd);

	Arm64Reg src_r = (ps->kind == RCPU) ? (Arm64Reg)ps->id : RTMP;
	if (ps->kind == RCONST) {
		load_immediate(ctx, ps->id, src_r, src->size == 8);
	} else if (ps->kind != RCPU) {
		ldr_stack(ctx, src_r, src->stackPos, src->size);
	}

	// MOV Xd, Xn (or MOV Wd, Wn for 32-bit)
	int sf = (dst->size == 8) ? 1 : 0;
	mov_reg_reg(ctx, pd->id, src_r, sf);

	reg_bind(ctx, dst, pd);
	mark_dirty(ctx, dst);
}

/**
 * Convert signed integer to float, or convert between float precisions (OToSFloat)
 * Handles: integer -> float (SCVTF), F64 -> F32, F32 -> F64 (FCVT)
 */
static void op_tosfloat(jit_ctx *ctx, vreg *dst, vreg *src) {
	// Handle float-to-float precision conversions
	if (src->t->kind == HF64 && dst->t->kind == HF32) {
		// F64 -> F32: FCVT Sd, Dn
		preg *ps = fetch(ctx, src);
		preg *pd = alloc_fpu(ctx);
		if (pd->holds != NULL)
			free_reg(ctx, pd);

		Arm64FpReg src_r = (ps->kind == RFPU) ? (Arm64FpReg)ps->id : V16;
		if (ps->kind != RFPU) {
			ldr_stack_fp(ctx, src_r, src->stackPos, src->size);
		}

		// FCVT Sd, Dn: type=1 (double source), opcode=4 (convert to single)
		encode_fp_1src(ctx, 0, 0, 1, 4, src_r, pd->id);

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
		return;
	}

	if (src->t->kind == HF32 && dst->t->kind == HF64) {
		// F32 -> F64: FCVT Dd, Sn
		preg *ps = fetch(ctx, src);
		preg *pd = alloc_fpu(ctx);
		if (pd->holds != NULL)
			free_reg(ctx, pd);

		Arm64FpReg src_r = (ps->kind == RFPU) ? (Arm64FpReg)ps->id : V16;
		if (ps->kind != RFPU) {
			ldr_stack_fp(ctx, src_r, src->stackPos, src->size);
		}

		// FCVT Dd, Sn: type=0 (single source), opcode=5 (convert to double)
		encode_fp_1src(ctx, 0, 0, 0, 5, src_r, pd->id);

		reg_bind(ctx, dst, pd);
		mark_dirty(ctx, dst);
		return;
	}

	// Integer to float conversion (original behavior)
	preg *ps = fetch(ctx, src);
	preg *pd = alloc_fpu(ctx);

	if (pd->holds != NULL)
		free_reg(ctx, pd);

	int sf = (src->size == 8) ? 1 : 0;
	int type = (dst->t->kind == HF64) ? 0x01 : 0x00;

	// SCVTF Vd, Xn (signed int to float)
	encode_int_fcvt(ctx, sf, 0, type, 0x00, 0x02, ps->id, pd->id);

	reg_bind(ctx, dst, pd);
	mark_dirty(ctx, dst);
}

/**
 * Convert unsigned integer to float (OToUFloat)
 */
static void op_toufloat(jit_ctx *ctx, vreg *dst, vreg *src) {
	preg *ps = fetch(ctx, src);
	preg *pd = alloc_fpu(ctx);

	if (pd->holds != NULL)
		free_reg(ctx, pd);

	int sf = (src->size == 8) ? 1 : 0;
	int type = (dst->t->kind == HF64) ? 0x01 : 0x00;

	// UCVTF Vd, Xn (unsigned int to float)
	encode_int_fcvt(ctx, sf, 0, type, 0x00, 0x03, ps->id, pd->id);

	reg_bind(ctx, dst, pd);
	mark_dirty(ctx, dst);
}

// ============================================================================
// Jump Patching
// ============================================================================

/**
 * Add a jump to the patch list
 * Also mark the target opcode so we know to discard registers when we reach it
 */
static void register_jump(jit_ctx *ctx, int pos, int target) {
	jlist *j = (jlist*)malloc(sizeof(jlist));
	j->pos = pos;
	j->target = target;
	j->next = ctx->jumps;
	ctx->jumps = j;

	// Mark target as a jump destination (like x86 does)
	// This tells us to discard register bindings when we reach this opcode
	if (target > 0 && target < ctx->maxOps && ctx->opsPos[target] == 0)
		ctx->opsPos[target] = -1;
}

/**
 * Patch a jump instruction with the correct offset
 * AArch64 branches use instruction offsets (divide byte offset by 4)
 */
static void patch_jump(jit_ctx *ctx, int pos, int target_pos) {
	unsigned int *code = (unsigned int*)(ctx->startBuf + pos);
	int offset = target_pos - pos;  // Byte offset
	int insn_offset = offset / 4;   // Instruction offset

	// Check if this is a conditional branch (B.cond) or unconditional (B)
	unsigned int insn = *code;
	unsigned int opcode = (insn >> 24) & 0xFF;

	if (opcode == 0x54) {
		// B.cond - 19-bit signed offset
		// Range: ±1MB (±0x40000 instructions, ±0x100000 bytes)
		if (insn_offset < -0x40000 || insn_offset >= 0x40000) {
			printf("JIT Error: Conditional branch offset too large: %d\n", insn_offset);
			JIT_ASSERT(0);
		}
		// Clear old offset, set new offset (bits 5-23)
		*code = (insn & 0xFF00001F) | ((insn_offset & 0x7FFFF) << 5);
	} else if ((opcode & 0xFC) == 0x14) {
		// B or BL - 26-bit signed offset
		// Range: ±128MB (±0x2000000 instructions, ±0x8000000 bytes)
		if (insn_offset < -0x2000000 || insn_offset >= 0x2000000) {
			printf("JIT Error: Branch offset too large: %d\n", insn_offset);
			JIT_ASSERT(0);
		}
		// Clear old offset, set new offset (bits 0-25)
		*code = (insn & 0xFC000000) | (insn_offset & 0x3FFFFFF);
	} else if ((opcode & 0x7E) == 0x34) {
		// CBZ/CBNZ - 19-bit signed offset
		if (insn_offset < -0x40000 || insn_offset >= 0x40000) {
			printf("JIT Error: CBZ/CBNZ offset too large: %d\n", insn_offset);
			JIT_ASSERT(0);
		}
		*code = (insn & 0xFF00001F) | ((insn_offset & 0x7FFFF) << 5);
	} else {
		printf("JIT Error: Unknown branch instruction at %d: 0x%08X\n", pos, insn);
		JIT_ASSERT(0);
	}
}

// ============================================================================
// Control Flow & Comparisons
// ============================================================================

/**
 * Map HashLink condition to AArch64 condition code
 */
static ArmCondition hl_cond_to_arm(hl_op op, bool is_float) {
	switch (op) {
	case OJEq:     return COND_EQ;  // Equal
	case OJNotEq:  return COND_NE;  // Not equal
	case OJSLt:    return is_float ? COND_MI : COND_LT;  // Signed less than
	case OJSGte:   return is_float ? COND_PL : COND_GE;  // Signed greater or equal
	case OJSGt:    return COND_GT;  // Signed greater than
	case OJSLte:   return COND_LE;  // Signed less or equal
	case OJULt:    return COND_LO;  // Unsigned less than (carry clear)
	case OJUGte:   return COND_HS;  // Unsigned greater or equal (carry set)
	// Float NaN-aware comparisons (includes unordered case)
	case OJNotLt:  return COND_HS;  // Not less than (C=1: >=, or unordered)
	case OJNotGte: return COND_LT;  // Not greater/equal (N!=V: <, or unordered)
	default:
		JIT_ASSERT(0);
		return COND_AL;
	}
}

/**
 * Conditional and comparison jumps
 *
 * Handles special cases for dynamic types:
 * - HDYN/HFUN: Call hl_dyn_compare() to compare dynamic values
 * - HTYPE: Call hl_same_type() to compare type objects
 * - HNULL: Compare boxed values (Null<T>)
 * - HVIRTUAL: Compare virtual objects with underlying values
 */
static void op_jump(jit_ctx *ctx, vreg *a, vreg *b, hl_op op, int target_opcode) {
	// Spill all registers to stack BEFORE the branch.
	// Target label will use discard_regs() and expect values on stack.
	spill_regs(ctx);
	spill_callee_saved(ctx);  // Callee-saved must also be spilled at control flow merge

	// Handle dynamic and function type comparisons
	if (a->t->kind == HDYN || b->t->kind == HDYN || a->t->kind == HFUN || b->t->kind == HFUN) {
		// Call hl_dyn_compare(a, b) which returns:
		//   0 if equal
		//   negative if a < b
		//   positive if a > b
		//   hl_invalid_comparison (0xAABBCCDD) for incomparable types
		vreg *args[2] = { a, b };
		int stack_space = prepare_call_args(ctx, NULL, args, 2, true);

		// Load function pointer and call
		load_immediate(ctx, (int64_t)hl_dyn_compare, RTMP, true);
		EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

		// Clean up stack
		if (stack_space > 0) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
		}

		// Handle ordered comparisons (OJSLt/OJSGt/OJSLte/OJSGte) - need to check for hl_invalid_comparison
		if (op == OJSLt || op == OJSGt || op == OJSLte || op == OJSGte) {
			// Compare result with hl_invalid_comparison (0xAABBCCDD)
			// If equal, don't take the branch (skip the jump)
			load_immediate(ctx, hl_invalid_comparison, RTMP, false);
			encode_add_sub_reg(ctx, 0, 1, 1, 0, RTMP, 0, X0, XZR);  // CMP W0, WTMP
			int skip_pos = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ skip (if invalid comparison)

			// Valid comparison - compare result with 0 for sign flags
			encode_add_sub_imm(ctx, 0, 1, 1, 0, 0, X0, XZR);  // CMP W0, #0
			ArmCondition cond = hl_cond_to_arm(op, false);
			int jump_pos = BUF_POS();
			encode_branch_cond(ctx, 0, cond);
			register_jump(ctx, jump_pos, target_opcode);

			// Patch the skip branch to here
			int skip_offset = (BUF_POS() - skip_pos) / 4;
			*(int*)(ctx->startBuf + skip_pos) = (*(int*)(ctx->startBuf + skip_pos) & 0xFF00001F) | ((skip_offset & 0x7FFFF) << 5);
			return;
		}

		// For OJEq/OJNotEq: result == 0 means equal
		// TST W0, W0 (sets flags based on W0 & W0)
		encode_logical_reg(ctx, 0, 0x3, 0, 0, X0, 0, X0, XZR);  // ANDS WZR, W0, W0

		// Branch based on zero flag (only equality ops should reach here)
		ArmCondition cond = (op == OJEq) ? COND_EQ : COND_NE;
		int jump_pos = BUF_POS();
		encode_branch_cond(ctx, 0, cond);
		register_jump(ctx, jump_pos, target_opcode);
		return;
	}

	// Handle type comparisons
	if (a->t->kind == HTYPE) {
		// Call hl_same_type(a, b) which returns bool
		vreg *args[2] = { a, b };
		int stack_space = prepare_call_args(ctx, NULL, args, 2, true);

		load_immediate(ctx, (int64_t)hl_same_type, RTMP, true);
		EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

		if (stack_space > 0) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
		}

		// Compare result with 1 (true): CMP W0, #1 = SUBS WZR, W0, #1
		// Note: S=1 is required both to set flags AND to make Rd=31 mean XZR (not SP)
		encode_add_sub_imm(ctx, 0, 1, 1, 0, 1, X0, XZR);  // CMP W0, #1

		ArmCondition cond = (op == OJEq) ? COND_EQ : COND_NE;
		int jump_pos = BUF_POS();
		encode_branch_cond(ctx, 0, cond);
		register_jump(ctx, jump_pos, target_opcode);
		return;
	}

	// Handle HNULL (Null<T>) comparisons
	// HNULL values have their inner value at offset HDYN_VALUE (8)
	if (a->t->kind == HNULL) {
		preg *pa = fetch(ctx, a);
		preg *pb = fetch(ctx, b);
		Arm64Reg ra = (pa->kind == RCPU) ? (Arm64Reg)pa->id : RTMP;
		Arm64Reg rb = (pb->kind == RCPU) ? (Arm64Reg)pb->id : RTMP2;
		if (pa->kind != RCPU) ldr_stack(ctx, ra, a->stackPos, 8);
		if (pb->kind != RCPU) ldr_stack(ctx, rb, b->stackPos, 8);

		if (op == OJEq) {
			// if (a == b || (a && b && a->v == b->v)) goto target
			// First: CMP a, b - if equal, jump to target
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int jump_pos1 = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);
			register_jump(ctx, jump_pos1, target_opcode);

			// If a == NULL, skip (don't jump)
			int skip_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);  // CBZ ra, skip

			// If b == NULL, skip (don't jump)
			int skip_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);  // CBZ rb, skip

			// Load inner values: a->v and b->v (at offset HDYN_VALUE)
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HDYN_VALUE / 8, ra, ra);  // LDR ra, [ra, #8]
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HDYN_VALUE / 8, rb, rb);  // LDR rb, [rb, #8]

			// Compare inner values
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int jump_pos2 = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);
			register_jump(ctx, jump_pos2, target_opcode);

			// Patch skip branches to here
			int here = BUF_POS();
			int off_a = (here - skip_a) / 4;
			int off_b = (here - skip_b) / 4;
			*(int*)(ctx->startBuf + skip_a) = (*(int*)(ctx->startBuf + skip_a) & 0xFF00001F) | ((off_a & 0x7FFFF) << 5);
			*(int*)(ctx->startBuf + skip_b) = (*(int*)(ctx->startBuf + skip_b) & 0xFF00001F) | ((off_b & 0x7FFFF) << 5);
		} else if (op == OJNotEq) {
			// if (a != b && (!a || !b || a->v != b->v)) goto target
			// First: CMP a, b - if equal, skip entirely
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int skip_eq = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ skip (a == b means not-not-equal)

			// If a == NULL, goto target (NULL != non-NULL)
			int jump_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);  // CBZ ra, target
			register_jump(ctx, jump_a, target_opcode);

			// If b == NULL, goto target (non-NULL != NULL)
			int jump_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);  // CBZ rb, target
			register_jump(ctx, jump_b, target_opcode);

			// Load inner values
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HDYN_VALUE / 8, ra, ra);
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HDYN_VALUE / 8, rb, rb);

			// Compare inner values - if not equal, goto target
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int skip_cmp = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ skip (values equal, don't jump)

			// Values not equal - jump to target
			int jump_ne = BUF_POS();
			encode_branch_uncond(ctx, 0);
			register_jump(ctx, jump_ne, target_opcode);

			// Patch skip branches
			int here = BUF_POS();
			int off_eq = (here - skip_eq) / 4;
			int off_cmp = (here - skip_cmp) / 4;
			*(int*)(ctx->startBuf + skip_eq) = (*(int*)(ctx->startBuf + skip_eq) & 0xFF00001F) | ((off_eq & 0x7FFFF) << 5);
			*(int*)(ctx->startBuf + skip_cmp) = (*(int*)(ctx->startBuf + skip_cmp) & 0xFF00001F) | ((off_cmp & 0x7FFFF) << 5);
		} else {
			jit_error("Unsupported comparison op for HNULL");
		}
		return;
	}

	// Handle HVIRTUAL comparisons
	// Virtual objects have a 'value' pointer at offset HL_WSIZE (8)
	if (a->t->kind == HVIRTUAL) {
		preg *pa = fetch(ctx, a);
		preg *pb = fetch(ctx, b);
		Arm64Reg ra = (pa->kind == RCPU) ? (Arm64Reg)pa->id : RTMP;
		Arm64Reg rb = (pb->kind == RCPU) ? (Arm64Reg)pb->id : RTMP2;
		if (pa->kind != RCPU) ldr_stack(ctx, ra, a->stackPos, 8);
		if (pb->kind != RCPU) ldr_stack(ctx, rb, b->stackPos, 8);

		if (b->t->kind == HOBJ) {
			// Comparing virtual to object: compare a->value with b
			if (op == OJEq) {
				// if (a ? (b && a->value == b) : (b == NULL)) goto target
				int ja = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 0, 0, ra);  // CBZ ra, check_b_null

				// a != NULL: check if b != NULL and a->value == b
				int jb = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 0, 0, rb);  // CBZ rb, skip (a!=NULL, b==NULL: not equal)

				// Load a->value and compare with b
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, ra, ra);  // LDR ra, [ra, #8]
				encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
				int jvalue = BUF_POS();
				encode_branch_uncond(ctx, 0);  // B to_cmp

				// a == NULL: check if b == NULL
				int here_ja = BUF_POS();
				int off_ja = (here_ja - ja) / 4;
				*(int*)(ctx->startBuf + ja) = (*(int*)(ctx->startBuf + ja) & 0xFF00001F) | ((off_ja & 0x7FFFF) << 5);
				encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, XZR, XZR);  // CMP rb, #0 (TST rb)

				// Patch jvalue to here (to_cmp)
				int here_jv = BUF_POS();
				int off_jv = (here_jv - jvalue) / 4;
				*(int*)(ctx->startBuf + jvalue) = 0x14000000 | (off_jv & 0x3FFFFFF);

				// Now flags are set - branch if equal
				int jump_pos = BUF_POS();
				encode_branch_cond(ctx, 0, COND_EQ);
				register_jump(ctx, jump_pos, target_opcode);

				// Patch jb to skip
				int here_jb = BUF_POS();
				int off_jb = (here_jb - jb) / 4;
				*(int*)(ctx->startBuf + jb) = (*(int*)(ctx->startBuf + jb) & 0xFF00001F) | ((off_jb & 0x7FFFF) << 5);
			} else if (op == OJNotEq) {
				// if (a ? (b == NULL || a->value != b) : (b != NULL)) goto target
				int ja = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 0, 0, ra);  // CBZ ra, check_b_notnull

				// a != NULL: jump if b == NULL
				int jump_b_null = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 0, 0, rb);  // CBZ rb, target
				register_jump(ctx, jump_b_null, target_opcode);

				// Load a->value and compare with b
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, ra, ra);
				encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
				int jvalue = BUF_POS();
				encode_branch_uncond(ctx, 0);  // B to_cmp

				// a == NULL: check if b != NULL
				int here_ja = BUF_POS();
				int off_ja = (here_ja - ja) / 4;
				*(int*)(ctx->startBuf + ja) = (*(int*)(ctx->startBuf + ja) & 0xFF00001F) | ((off_ja & 0x7FFFF) << 5);
				encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, XZR, XZR);  // CMP rb, #0

				// Patch jvalue
				int here_jv = BUF_POS();
				int off_jv = (here_jv - jvalue) / 4;
				*(int*)(ctx->startBuf + jvalue) = 0x14000000 | (off_jv & 0x3FFFFFF);

				// Branch if not equal
				int jump_pos = BUF_POS();
				encode_branch_cond(ctx, 0, COND_NE);
				register_jump(ctx, jump_pos, target_opcode);
			} else {
				jit_error("Unsupported comparison op for HVIRTUAL vs HOBJ");
			}
			return;
		}

		// Both are HVIRTUAL - compare underlying values
		if (op == OJEq) {
			// if (a == b || (a && b && a->value && b->value && a->value == b->value)) goto
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int jump_eq = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);
			register_jump(ctx, jump_eq, target_opcode);

			// Check a != NULL
			int skip_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);
			// Check b != NULL
			int skip_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);

			// Load a->value
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, ra, ra);
			int skip_av = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);  // CBZ if a->value == NULL

			// Load b->value
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, rb, rb);
			int skip_bv = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);  // CBZ if b->value == NULL

			// Compare values
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int jump_val = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);
			register_jump(ctx, jump_val, target_opcode);

			// Patch all skips to here
			int here = BUF_POS();
			int patches[] = { skip_a, skip_b, skip_av, skip_bv };
			for (int i = 0; i < 4; i++) {
				int off = (here - patches[i]) / 4;
				*(int*)(ctx->startBuf + patches[i]) = (*(int*)(ctx->startBuf + patches[i]) & 0xFF00001F) | ((off & 0x7FFFF) << 5);
			}
		} else if (op == OJNotEq) {
			// if (a != b && (!a || !b || !a->value || !b->value || a->value != b->value)) goto
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int skip_eq = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);  // Skip if a == b

			// If a == NULL, jump
			int jump_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);
			register_jump(ctx, jump_a, target_opcode);

			// If b == NULL, jump
			int jump_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);
			register_jump(ctx, jump_b, target_opcode);

			// Load a->value
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, ra, ra);
			int jump_av = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);
			register_jump(ctx, jump_av, target_opcode);

			// Load b->value
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, HL_WSIZE / 8, rb, rb);
			int jump_bv = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);
			register_jump(ctx, jump_bv, target_opcode);

			// Compare - if not equal, jump
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);
			int skip_val = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);

			// Not equal - jump to target
			int jump_ne = BUF_POS();
			encode_branch_uncond(ctx, 0);
			register_jump(ctx, jump_ne, target_opcode);

			// Patch skips
			int here = BUF_POS();
			int off_eq = (here - skip_eq) / 4;
			int off_val = (here - skip_val) / 4;
			*(int*)(ctx->startBuf + skip_eq) = (*(int*)(ctx->startBuf + skip_eq) & 0xFF00001F) | ((off_eq & 0x7FFFF) << 5);
			*(int*)(ctx->startBuf + skip_val) = (*(int*)(ctx->startBuf + skip_val) & 0xFF00001F) | ((off_val & 0x7FFFF) << 5);
		} else {
			jit_error("Unsupported comparison op for HVIRTUAL");
		}
		return;
	}

	// Handle HOBJ/HSTRUCT vs HVIRTUAL (swap operands)
	if ((a->t->kind == HOBJ || a->t->kind == HSTRUCT) && b->t->kind == HVIRTUAL) {
		// Swap and recurse - the HVIRTUAL case handles HOBJ on the right
		op_jump(ctx, b, a, op, target_opcode);
		return;
	}

	// Handle String EQUALITY comparison (value-based per Haxe spec)
	// hl_str_cmp only returns 0 (equal) or 1 (not equal), so it can only be used
	// for OJEq/OJNotEq. For ordered comparisons, fall through to compareFun path.
	if ((op == OJEq || op == OJNotEq) && is_string_type(a->t) && is_string_type(b->t)) {
		// Spill before call
		spill_regs(ctx);
		spill_callee_saved(ctx);

		// Call hl_str_cmp(a, b) - returns 0 if equal, non-zero if not equal
		vreg *args[2] = { a, b };
		int stack_space = prepare_call_args(ctx, NULL, args, 2, true);
		load_immediate(ctx, (int64_t)hl_str_cmp, RTMP, true);
		EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
		if (stack_space > 0) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
		}

		// Result in X0: 0 = equal, non-zero = not equal
		// TST X0, X0 sets Z flag (Z=1 if X0==0)
		encode_logical_reg(ctx, 1, 0x3, 0, 0, X0, 0, X0, XZR);  // TST X0, X0

		// Branch based on op (only EQ or NE)
		ArmCondition cond = (op == OJEq) ? COND_EQ : COND_NE;
		int jump_pos = BUF_POS();
		encode_branch_cond(ctx, 0, cond);
		register_jump(ctx, jump_pos, target_opcode);
		return;
	}

	// Handle HOBJ/HSTRUCT with compareFun (e.g., String)
	// Use hl_get_obj_rt() to ensure runtime object is initialized (like x86 does)
	// NOTE: compareFun is a FUNCTION INDEX, not a function pointer!
	if ((a->t->kind == HOBJ || a->t->kind == HSTRUCT) && hl_get_obj_rt(a->t)->compareFun) {
		int compareFunIndex = (int)(int_val)hl_get_obj_rt(a->t)->compareFun;
		preg *pa = fetch(ctx, a);
		preg *pb = fetch(ctx, b);
		Arm64Reg ra = (pa->kind == RCPU) ? (Arm64Reg)pa->id : RTMP;
		Arm64Reg rb = (pb->kind == RCPU) ? (Arm64Reg)pb->id : RTMP2;
		if (pa->kind != RCPU) ldr_stack(ctx, ra, a->stackPos, 8);
		if (pb->kind != RCPU) ldr_stack(ctx, rb, b->stackPos, 8);

		if (op == OJEq) {
			// if (a == b || (a && b && cmp(a,b) == 0)) goto target
			// First check pointer equality
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);  // CMP ra, rb
			int jump_eq = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);
			register_jump(ctx, jump_eq, target_opcode);

			// If a == NULL, skip
			int skip_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);

			// If b == NULL, skip
			int skip_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);

			// Call compareFun(a, b) - compareFunIndex is a function index, not a pointer!
			vreg *args[2] = { a, b };
			int stack_space = prepare_call_args(ctx, NULL, args, 2, true);
			emit_call_findex(ctx, compareFunIndex, stack_space);

			// If result == 0, goto target
			encode_logical_reg(ctx, 0, 0x3, 0, 0, X0, 0, X0, XZR);  // TST W0, W0
			int skip_cmp = BUF_POS();
			encode_branch_cond(ctx, 0, COND_NE);  // Skip if result != 0

			// Jump to target
			int jump_target = BUF_POS();
			encode_branch_uncond(ctx, 0);
			register_jump(ctx, jump_target, target_opcode);

			// Patch all skips to here
			int here = BUF_POS();
			int patches[] = { skip_a, skip_b, skip_cmp };
			for (int i = 0; i < 3; i++) {
				int off = (here - patches[i]) / 4;
				*(int*)(ctx->startBuf + patches[i]) = (*(int*)(ctx->startBuf + patches[i]) & 0xFF00001F) | ((off & 0x7FFFF) << 5);
			}
		} else if (op == OJNotEq) {
			// if (a != b && (!a || !b || cmp(a,b) != 0)) goto target
			// First check pointer equality - if equal, skip entirely
			encode_add_sub_reg(ctx, 1, 1, 1, 0, rb, 0, ra, XZR);  // CMP ra, rb
			int skip_eq = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);

			// If a == NULL, goto target
			int jump_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);
			register_jump(ctx, jump_a, target_opcode);

			// If b == NULL, goto target
			int jump_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);
			register_jump(ctx, jump_b, target_opcode);

			// Call compareFun(a, b) - compareFunIndex is a function index, not a pointer!
			vreg *args[2] = { a, b };
			int stack_space = prepare_call_args(ctx, NULL, args, 2, true);
			emit_call_findex(ctx, compareFunIndex, stack_space);

			// If result != 0, goto target
			encode_logical_reg(ctx, 0, 0x3, 0, 0, X0, 0, X0, XZR);  // TST W0, W0
			int skip_cmp = BUF_POS();
			encode_branch_cond(ctx, 0, COND_EQ);  // Skip if result == 0

			// Jump to target
			int jump_target = BUF_POS();
			encode_branch_uncond(ctx, 0);
			register_jump(ctx, jump_target, target_opcode);

			// Patch skips to here
			int here = BUF_POS();
			int off_eq = (here - skip_eq) / 4;
			int off_cmp = (here - skip_cmp) / 4;
			*(int*)(ctx->startBuf + skip_eq) = (*(int*)(ctx->startBuf + skip_eq) & 0xFF00001F) | ((off_eq & 0x7FFFF) << 5);
			*(int*)(ctx->startBuf + skip_cmp) = (*(int*)(ctx->startBuf + skip_cmp) & 0xFF00001F) | ((off_cmp & 0x7FFFF) << 5);
		} else {
			// For OJSGt, OJSGte, OJSLt, OJSLte: if (a && b && cmp(a,b) ?? 0) goto
			int skip_a = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, ra);

			int skip_b = BUF_POS();
			encode_cbz_cbnz(ctx, 1, 0, 0, rb);

			// Call compareFun(a, b) - compareFunIndex is a function index, not a pointer!
			vreg *args[2] = { a, b };
			int stack_space = prepare_call_args(ctx, NULL, args, 2, true);
			emit_call_findex(ctx, compareFunIndex, stack_space);

			// Compare result with 0: CMP W0, #0
			encode_add_sub_imm(ctx, 0, 1, 1, 0, 0, X0, XZR);  // CMP W0, #0

			// Branch based on condition
			ArmCondition cond = hl_cond_to_arm(op, false);
			int jump_pos = BUF_POS();
			encode_branch_cond(ctx, 0, cond);
			register_jump(ctx, jump_pos, target_opcode);

			// Patch skips to here
			int here = BUF_POS();
			int off_a = (here - skip_a) / 4;
			int off_b = (here - skip_b) / 4;
			*(int*)(ctx->startBuf + skip_a) = (*(int*)(ctx->startBuf + skip_a) & 0xFF00001F) | ((off_a & 0x7FFFF) << 5);
			*(int*)(ctx->startBuf + skip_b) = (*(int*)(ctx->startBuf + skip_b) & 0xFF00001F) | ((off_b & 0x7FFFF) << 5);
		}
		return;
	}

	// Standard comparison for other types
	bool is_float = IS_FLOAT(a);
	preg *pa = fetch(ctx, a);
	preg *pb = fetch(ctx, b);

	if (is_float) {
		// Floating-point comparison: FCMP Vn, Vm
		int type = (a->t->kind == HF64) ? 0x01 : 0x00;
		encode_fp_compare(ctx, 0, 0, type, pb->id, 0, pa->id);
	} else {
		// Integer comparison: CMP Xn, Xm (implemented as SUBS XZR, Xn, Xm)
		int sf = (a->size == 8) ? 1 : 0;
		encode_add_sub_reg(ctx, sf, 1, 1, 0, pb->id, 0, pa->id, XZR);
	}

	// Emit conditional branch
	ArmCondition cond = hl_cond_to_arm(op, is_float);
	int jump_pos = BUF_POS();
	encode_branch_cond(ctx, 0, cond);  // Offset will be patched later

	// Register for patching
	register_jump(ctx, jump_pos, target_opcode);
}

/**
 * Simple conditional jumps (OJTrue, OJFalse, OJNull, OJNotNull)
 */
static void op_jcond(jit_ctx *ctx, vreg *a, hl_op op, int target_opcode) {
	// Spill all registers to stack BEFORE the branch.
	// Target label will use discard_regs() and expect values on stack.
	spill_regs(ctx);
	spill_callee_saved(ctx);  // Callee-saved must also be spilled at control flow merge

	preg *pa = fetch(ctx, a);
	int jump_pos = BUF_POS();

	// Determine which condition to test
	bool test_zero = (op == OJFalse || op == OJNull);

	// Use CBZ (compare and branch if zero) or CBNZ (compare and branch if non-zero)
	int sf = (a->size == 8) ? 1 : 0;
	int op_bit = test_zero ? 0 : 1;  // 0=CBZ, 1=CBNZ

	encode_cbz_cbnz(ctx, sf, op_bit, 0, pa->id);  // Offset will be patched

	// Register for patching
	register_jump(ctx, jump_pos, target_opcode);
}

/**
 * Unconditional jump (OJAlways)
 */
static void op_jalways(jit_ctx *ctx, int target_opcode) {
	// Spill all registers to stack BEFORE the branch.
	// Target label will use discard_regs() and expect values on stack.
	spill_regs(ctx);
	spill_callee_saved(ctx);  // Callee-saved must also be spilled at control flow merge

	int jump_pos = BUF_POS();
	encode_branch_uncond(ctx, 0);  // Offset will be patched

	// Register for patching
	register_jump(ctx, jump_pos, target_opcode);
}

/**
 * Discard all register bindings at merge points (labels).
 *
 * Used at labels where control flow can come from multiple paths.
 * Clears register↔vreg bindings so subsequent operations load from stack.
 *
 * With dirty tracking: If reached via fallthrough (not a jump), registers
 * might still be dirty and need to be spilled first. Registers reached via
 * jump are already clean because spill_regs() is called before all jumps.
 */
static void discard_regs(jit_ctx *ctx) {
	int i;
	// Handle CPU scratch registers (X0-X17)
	// NOTE: This function must NOT emit any code!
	// At labels, spill_regs() + spill_callee_saved() is called BEFORE this (for fallthrough).
	// We just clear bindings here - values are already on stack.
	for (i = 0; i < 18; i++) {
		preg *r = &ctx->pregs[i];
		if (r->holds) {
			r->holds->dirty = 0;
			r->holds->current = NULL;
			r->holds = NULL;
		}
	}
	// Handle callee-saved CPU registers (X19-X26)
	// At merge points, callee-saved must also be discarded for consistent state
	for (i = 0; i < RCPU_CALLEE_ALLOC_COUNT; i++) {
		preg *r = REG_AT(RCPU_CALLEE_ALLOC[i]);
		if (r->holds) {
			r->holds->dirty = 0;
			r->holds->current = NULL;
			r->holds = NULL;
		}
	}
	// Handle ALL FPU registers (V0-V31) at merge points
	// At labels, control flow may come from different paths with different allocations
	for (i = 0; i < RFPU_COUNT; i++) {
		preg *r = &ctx->pregs[RCPU_COUNT + i];
		if (r->holds) {
			r->holds->dirty = 0;
			r->holds->current = NULL;
			r->holds = NULL;
		}
	}
}

/**
 * Label marker (OLabel) - just records position for jump targets
 * At a label, control flow could come from multiple places,
 * so we must invalidate all register associations.
 *
 * IMPORTANT: No code is emitted here! The main loop calls spill_regs()
 * BEFORE this function for the fallthrough path. Jump paths have already
 * spilled before jumping. We just clear bindings so subsequent ops
 * load from stack.
 */
static void op_label(jit_ctx *ctx) {
	// Just clear bindings - spill_regs() was already called in main loop
	discard_regs(ctx);
}

// ============================================================================
// Memory Operations
// ============================================================================

/*
 * Load byte/halfword/word from memory
 * OGetI8/OGetI16/OGetI32: dst = *(type*)(base + offset)
 */
static void op_get_mem(jit_ctx *ctx, vreg *dst, vreg *base, int offset, int size) {
	preg *base_reg = fetch(ctx, base);
	preg *dst_reg = alloc_dst(ctx, dst);

	Arm64Reg base_r = (base_reg->kind == RCPU) ? (Arm64Reg)base_reg->id : RTMP;
	if (base_reg->kind != RCPU) {
		ldr_stack(ctx, base_r, base->stackPos, base->size);
	}

	// Handle float and integer cases separately
	if (IS_FLOAT(dst)) {
		// Float: load into FPU register
		Arm64FpReg dst_r = (dst_reg->kind == RFPU) ? (Arm64FpReg)dst_reg->id : V16;
		int size_bits = (size == 8) ? 0x03 : 0x02;  // D or S

		if (offset >= 0 && offset < (1 << 12) * size) {
			int imm12 = offset / size;
			encode_ldr_str_imm(ctx, size_bits, 1, 0x01, imm12, base_r, dst_r);  // V=1 for FP
		} else {
			load_immediate(ctx, offset, RTMP2, false);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, base_r, RTMP);
			encode_ldr_str_imm(ctx, size_bits, 1, 0x01, 0, RTMP, dst_r);  // V=1 for FP
		}

		str_stack_fp(ctx, dst_r, dst->stackPos, dst->size);
	} else {
		// Integer/pointer: load into CPU register
		// Use RTMP2 as temp (not RTMP) because str_stack's fallback uses RTMP internally.
		// If we loaded into RTMP, str_stack would clobber the value.
		Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP2;

		// Load with offset
		// LDR Xd, [Xn, #offset]  or  LDRB/LDRH for smaller sizes
		if (offset >= 0 && offset < (1 << 12) * size) {
			// Fits in immediate offset
			int imm12 = offset / size;
			// size: 1=LDRB, 2=LDRH, 4=LDR(W), 8=LDR(X)
			int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;
			encode_ldr_str_imm(ctx, size_bits, 0, 0x01, imm12, base_r, dst_r);
		} else {
			// Offset too large - compute effective address in RTMP, then load into dst_r
			load_immediate(ctx, offset, RTMP2, false);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, base_r, RTMP);
			// LDR dst_r, [RTMP]
			int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;
			encode_ldr_str_imm(ctx, size_bits, 0, 0x01, 0, RTMP, dst_r);
		}

		// Always store to stack - it's the source of truth for later loads
		// (registers may be clobbered by subsequent calls)
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	// Release the base register - discard() will store if dirty
	discard(ctx, base_reg);
}

/*
 * Store byte/halfword/word to memory
 * OSetI8/OSetI16/OSetI32: *(type*)(base + offset) = value
 */
static void op_set_mem(jit_ctx *ctx, vreg *base, int offset, vreg *value, int size) {
	preg *base_reg = fetch(ctx, base);
	preg *value_reg = fetch(ctx, value);

	/*
	 * IMPORTANT: Load value FIRST, then base.
	 * ldr_stack's fallback path uses RTMP internally, so if we load base into RTMP
	 * first, then load value from stack, RTMP would get clobbered.
	 * By loading value first (into RTMP2 or FPU reg), any RTMP usage is harmless.
	 * Then we load base into RTMP, which is safe since value is already loaded.
	 */

	// Handle float and integer cases separately
	if (IS_FLOAT(value)) {
		// Float: load value first into FPU register
		Arm64FpReg value_r = (value_reg->kind == RFPU) ? (Arm64FpReg)value_reg->id : V16;
		if (value_reg->kind != RFPU) {
			ldr_stack_fp(ctx, value_r, value->stackPos, value->size);
		}

		// Now load base (safe - value is already in FPU reg)
		Arm64Reg base_r = (base_reg->kind == RCPU) ? (Arm64Reg)base_reg->id : RTMP;
		if (base_reg->kind != RCPU) {
			ldr_stack(ctx, base_r, base->stackPos, base->size);
		}

		int size_bits = (size == 8) ? 0x03 : 0x02;  // D or S

		if (offset >= 0 && offset < (1 << 12) * size) {
			int imm12 = offset / size;
			encode_ldr_str_imm(ctx, size_bits, 1, 0x00, imm12, base_r, value_r);  // V=1 for FP
		} else {
			load_immediate(ctx, offset, RTMP2, false);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, base_r, RTMP);
			encode_ldr_str_imm(ctx, size_bits, 1, 0x00, 0, RTMP, value_r);  // V=1 for FP
		}
	} else {
		// Integer/pointer: load value first into CPU register
		Arm64Reg value_r = (value_reg->kind == RCPU) ? (Arm64Reg)value_reg->id : RTMP2;
		if (value_reg->kind == RCONST) {
			load_immediate(ctx, value_reg->id, value_r, value->size == 8);
		} else if (value_reg->kind != RCPU) {
			ldr_stack(ctx, value_r, value->stackPos, value->size);
		}

		// Now load base (safe - value is already in RTMP2 or CPU reg)
		Arm64Reg base_r = (base_reg->kind == RCPU) ? (Arm64Reg)base_reg->id : RTMP;
		if (base_reg->kind != RCPU) {
			ldr_stack(ctx, base_r, base->stackPos, base->size);
		}

		// Store with offset
		// STR Xd, [Xn, #offset]  or  STRB/STRH for smaller sizes
		if (offset >= 0 && offset < (1 << 12) * size) {
			// Fits in immediate offset
			int imm12 = offset / size;
			int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;
			encode_ldr_str_imm(ctx, size_bits, 0, 0x00, imm12, base_r, value_r);
		} else {
			// Offset too large - load offset to temp register
			if (value_r == RTMP2) {
				// Value is already in RTMP2, use a different temp
				load_immediate(ctx, offset, X9, false);
				encode_add_sub_reg(ctx, 1, 0, 0, 0, X9, 0, base_r, RTMP);
			} else {
				load_immediate(ctx, offset, RTMP2, false);
				encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, base_r, RTMP);
			}
			// STR value_r, [RTMP]
			int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;
			encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, RTMP, value_r);
		}
	}

	discard(ctx, base_reg);
	discard(ctx, value_reg);
}

/*
 * Load byte/halfword/word from memory with register offset
 * OGetI8/OGetI16/OGetMem: dst = *(type*)(base + offset_reg)
 * Unlike op_get_mem which takes an immediate offset, this takes an offset vreg
 */
/*
 * IMPORTANT: We must load offset BEFORE base when base uses RTMP,
 * because ldr_stack's fallback path uses RTMP as a temporary.
 * Order: offset -> base -> compute address -> load
 */
static void op_get_mem_reg(jit_ctx *ctx, vreg *dst, vreg *base, vreg *offset, int size) {
	preg *base_reg = fetch(ctx, base);
	preg *offset_reg = fetch(ctx, offset);
	preg *dst_reg = alloc_dst(ctx, dst);

	// Step 1: Load offset FIRST (may clobber RTMP in fallback, but we haven't used it yet)
	Arm64Reg offset_r = (offset_reg->kind == RCPU) ? (Arm64Reg)offset_reg->id : RTMP2;
	if (offset_reg->kind == RCONST) {
		load_immediate(ctx, offset_reg->id, offset_r, false);
	} else if (offset_reg->kind != RCPU) {
		ldr_stack(ctx, offset_r, offset->stackPos, offset->size);
	}

	// Step 2: Load base (if it needs RTMP, the value will stay in RTMP)
	Arm64Reg base_r = (base_reg->kind == RCPU) ? (Arm64Reg)base_reg->id : RTMP;
	if (base_reg->kind != RCPU) {
		ldr_stack(ctx, base_r, base->stackPos, base->size);
	}

	// Step 3: Compute effective address: RTMP = base + offset
	encode_add_sub_reg(ctx, 1, 0, 0, 0, offset_r, 0, base_r, RTMP);

	// Load from [RTMP] - handle float vs integer types
	int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;

	if (IS_FLOAT(dst)) {
		// Float load: use FPU register and V=1
		Arm64FpReg dst_fp = (dst_reg->kind == RFPU) ? (Arm64FpReg)dst_reg->id : V16;
		if (dst_fp == V16) {
			preg *pv16 = PVFPR(16);
			if (pv16->holds != NULL) free_reg(ctx, pv16);
		}
		encode_ldr_str_imm(ctx, size_bits, 1, 0x01, 0, RTMP, dst_fp);  // V=1 for FP
		str_stack_fp(ctx, dst_fp, dst->stackPos, dst->size);
	} else {
		// Integer load
		Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : X9;
		if (dst_r == X9) {
			preg *px9 = &ctx->pregs[X9];
			if (px9->holds != NULL) free_reg(ctx, px9);
		}
		encode_ldr_str_imm(ctx, size_bits, 0, 0x01, 0, RTMP, dst_r);
		// For byte/halfword loads, the result is zero-extended automatically by LDRB/LDRH
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	discard(ctx, base_reg);
	discard(ctx, offset_reg);
}

/*
 * Store byte/halfword/word to memory with register offset
 * OSetI8/OSetI16/OSetMem: *(type*)(base + offset_reg) = value
 * Unlike op_set_mem which takes an immediate offset, this takes an offset vreg
 *
 * IMPORTANT: We must load the value BEFORE computing the address in RTMP,
 * because ldr_stack's fallback path for large/unaligned offsets uses RTMP
 * as a temporary register.
 */
static void op_set_mem_reg(jit_ctx *ctx, vreg *base, vreg *offset, vreg *value, int size) {
	preg *base_reg = fetch(ctx, base);
	preg *offset_reg = fetch(ctx, offset);
	preg *value_reg = fetch(ctx, value);

	int size_bits = (size == 1) ? 0x00 : (size == 2) ? 0x01 : (size == 4) ? 0x02 : 0x03;

	// Step 1: Load value FIRST (before using RTMP for address computation)
	// ldr_stack's fallback path uses RTMP, so we must do this before RTMP holds the address
	Arm64FpReg value_fp = V16;
	Arm64Reg value_r = X9;

	if (IS_FLOAT(value)) {
		value_fp = (value_reg->kind == RFPU) ? (Arm64FpReg)value_reg->id : V16;
		if (value_reg->kind != RFPU) {
			// Ensure V16 is free before using it
			if (value_fp == V16) {
				preg *pv16 = PVFPR(16);
				if (pv16->holds != NULL) free_reg(ctx, pv16);
			}
			ldr_stack_fp(ctx, value_fp, value->stackPos, value->size);
		}
	} else {
		value_r = (value_reg->kind == RCPU) ? (Arm64Reg)value_reg->id : X9;
		if (value_reg->kind == RCONST) {
			// Ensure X9 is free if we are using it
			if (value_r == X9) {
				preg *px9 = &ctx->pregs[X9];
				if (px9->holds != NULL) free_reg(ctx, px9);
			}
			load_immediate(ctx, value_reg->id, value_r, value->size == 8);
		} else if (value_reg->kind != RCPU) {
			// Ensure X9 is free if we are using it
			if (value_r == X9) {
				preg *px9 = &ctx->pregs[X9];
				if (px9->holds != NULL) free_reg(ctx, px9);
			}
			ldr_stack(ctx, value_r, value->stackPos, value->size);
		}
	}

	// Step 2: Load base and offset (these may also use RTMP in fallback, but that's ok
	// since we compute the final address in RTMP at the end)
	Arm64Reg base_r = (base_reg->kind == RCPU) ? (Arm64Reg)base_reg->id : RTMP;
	if (base_reg->kind != RCPU) {
		ldr_stack(ctx, base_r, base->stackPos, base->size);
	}

	Arm64Reg offset_r = (offset_reg->kind == RCPU) ? (Arm64Reg)offset_reg->id : RTMP2;
	if (offset_reg->kind == RCONST) {
		load_immediate(ctx, offset_reg->id, offset_r, false);
	} else if (offset_reg->kind != RCPU) {
		ldr_stack(ctx, offset_r, offset->stackPos, offset->size);
	}

	// Step 3: Compute effective address: RTMP = base + offset
	encode_add_sub_reg(ctx, 1, 0, 0, 0, offset_r, 0, base_r, RTMP);

	// Step 4: Store to [RTMP]
	if (IS_FLOAT(value)) {
		encode_ldr_str_imm(ctx, size_bits, 1, 0x00, 0, RTMP, value_fp);  // V=1 for FP
	} else {
		encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, RTMP, value_r);
	}

	discard(ctx, base_reg);
	discard(ctx, offset_reg);
	discard(ctx, value_reg);
}

/*
 * Field access: dst = obj->field
 * OField: dst = *(obj + field_offset)
 *
 * Special handling for HPACKED -> HSTRUCT: return address of inline storage
 * instead of loading a value (LEA semantics).
 */
static void op_field(jit_ctx *ctx, vreg *dst, vreg *obj, int field_index) {
	hl_runtime_obj *rt = hl_get_obj_rt(obj->t);
	int offset = rt->fields_indexes[field_index];

	// Check for packed field -> struct destination (LEA semantics)
	if (dst->t->kind == HSTRUCT) {
		hl_type *ft = hl_obj_field_fetch(obj->t, field_index)->t;
		if (ft->kind == HPACKED) {
			// Return address of inline storage: dst = &obj->field
			preg *p_obj = fetch(ctx, obj);
			preg *p_dst = alloc_dst(ctx, dst);  // Allocates register, binds to dst, marks dirty

			Arm64Reg obj_r = (p_obj->kind == RCPU) ? (Arm64Reg)p_obj->id : RTMP;
			if (p_obj->kind != RCPU) {
				ldr_stack(ctx, obj_r, obj->stackPos, obj->size);
			}

			Arm64Reg dst_r = (Arm64Reg)p_dst->id;  // alloc_dst always returns RCPU for non-float

			// ADD dst, obj, #offset (equivalent to LEA)
			if (offset >= 0 && offset < 4096) {
				encode_add_sub_imm(ctx, 1, 0, 0, 0, offset, obj_r, dst_r);
			} else {
				load_immediate(ctx, offset, RTMP2, false);
				encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, obj_r, dst_r);
			}

			// Don't call store_result - alloc_dst already set up the binding
			// The value will be spilled when needed
			discard(ctx, p_obj);
			return;
		}
	}

	op_get_mem(ctx, dst, obj, offset, dst->size);
}

/*
 * Field assignment: obj->field = value
 * OSetField: *(obj + field_offset) = value
 *
 * Special handling for HSTRUCT -> HPACKED: must copy struct byte-by-byte
 * because HPACKED means the struct is stored inline, not as a pointer.
 */
static void op_set_field(jit_ctx *ctx, vreg *obj, int field_index, vreg *value) {
	hl_runtime_obj *rt = hl_get_obj_rt(obj->t);
	int field_offset = rt->fields_indexes[field_index];

	// Check for struct-to-packed-field assignment
	if (value->t->kind == HSTRUCT) {
		hl_type *ft = hl_obj_field_fetch(obj->t, field_index)->t;
		if (ft->kind == HPACKED) {
			// Copy struct byte-by-byte
			hl_runtime_obj *frt = hl_get_obj_rt(ft->tparam);

			// Load obj pointer into RTMP and value pointer into RTMP2.
			// This is simpler than trying to manage register allocation for the copy.
			preg *p_obj = fetch(ctx, obj);
			preg *p_val = fetch(ctx, value);

			// Always load to scratch registers to avoid conflicts with copy temp
			Arm64Reg obj_r = RTMP;
			Arm64Reg val_r = RTMP2;

			if (p_obj->kind == RCPU) {
				// Move from allocated register to RTMP: ORR RTMP, XZR, Rm
				encode_logical_reg(ctx, 1, 0x01, 0, 0, (Arm64Reg)p_obj->id, 0, XZR, obj_r);
			} else {
				ldr_stack(ctx, obj_r, obj->stackPos, obj->size);
			}

			if (p_val->kind == RCPU) {
				// Move from allocated register to RTMP2: ORR RTMP2, XZR, Rm
				encode_logical_reg(ctx, 1, 0x01, 0, 0, (Arm64Reg)p_val->id, 0, XZR, val_r);
			} else {
				ldr_stack(ctx, val_r, value->stackPos, value->size);
			}

			// Use X9 for data copy, X10 for large offset computation
			// Evict both if they're holding values
			preg *p_x9 = &ctx->pregs[X9];
			preg *p_x10 = &ctx->pregs[X10];
			if (p_x9->holds != NULL) {
				free_reg(ctx, p_x9);
			}
			if (p_x10->holds != NULL) {
				free_reg(ctx, p_x10);
			}

			Arm64Reg tmp = X9;
			int offset = 0;
			while (offset < frt->size) {
				int remain = frt->size - offset;
				int copy_size = remain >= HL_WSIZE ? HL_WSIZE : (remain >= 4 ? 4 : (remain >= 2 ? 2 : 1));
				int size_bits = (copy_size == 8) ? 0x03 : (copy_size == 4) ? 0x02 : (copy_size == 2) ? 0x01 : 0x00;

				// Load from source: LDR tmp, [val_r, #offset]
				// Source offset starts at 0 and increments by copy_size, so always aligned
				encode_ldr_str_imm(ctx, size_bits, 0, 0x01, offset / copy_size, val_r, tmp);

				// Store to dest: STR tmp, [obj_r + field_offset + offset]
				// Dest offset may not be aligned to copy_size, so compute address explicitly
				int dest_offset = field_offset + offset;
				if ((dest_offset % copy_size) == 0 && dest_offset >= 0 && dest_offset < (1 << 12) * copy_size) {
					// Aligned and fits in immediate - use scaled offset
					encode_ldr_str_imm(ctx, size_bits, 0, 0x00, dest_offset / copy_size, obj_r, tmp);
				} else {
					// Misaligned or large offset - compute address in X10
					load_immediate(ctx, dest_offset, X10, false);
					encode_add_sub_reg(ctx, 1, 0, 0, 0, X10, 0, obj_r, X10);
					encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, X10, tmp);
				}

				offset += copy_size;
			}

			discard(ctx, p_obj);
			discard(ctx, p_val);
			return;
		}
	}

	op_set_mem(ctx, obj, field_offset, value, value->size);
}

/*
 * Array element access: dst = array[index]
 * OGetArray: dst = hl_aptr(array)[index]
 *
 * varray layout: { hl_type *t, hl_type *at, int size, int __pad } = 24 bytes
 * Data is INLINE immediately after the header (not via a pointer!)
 * hl_aptr(a,t) = (t*)(((varray*)(a))+1) = array + sizeof(varray)
 *
 * CArray (HABSTRACT) layout: raw memory, no header
 * For HOBJ/HSTRUCT: return address of element (LEA)
 * For other types: load value (LDR)
 */
/*
 * IMPORTANT: We must load index BEFORE array when array uses RTMP,
 * because ldr_stack's fallback path uses RTMP as a temporary.
 * Order: index -> array -> compute address -> load
 */
static void op_get_array(jit_ctx *ctx, vreg *dst, vreg *array, vreg *index) {
	preg *array_reg = fetch(ctx, array);
	preg *index_reg = fetch(ctx, index);
	preg *dst_reg = alloc_dst(ctx, dst);

	// CArrays (HABSTRACT) have different layout - no header, and for HOBJ/HSTRUCT
	// we return the address (LEA) rather than loading the value
	bool is_carray = (array->t->kind == HABSTRACT);
	bool is_lea = is_carray && (dst->t->kind == HOBJ || dst->t->kind == HSTRUCT);

	int elem_size;
	if (is_carray) {
		if (is_lea) {
			// For HOBJ/HSTRUCT in CArray, element size is the runtime object size
			hl_runtime_obj *rt = hl_get_obj_rt(dst->t);
			elem_size = rt->size;
		} else {
			// For other types in CArray, element size is pointer size
			elem_size = sizeof(void*);
		}
	} else {
		elem_size = hl_type_size(dst->t);
	}

	// Step 1: Load index FIRST (may clobber RTMP in fallback, but we haven't used it yet)
	Arm64Reg index_r = (index_reg->kind == RCPU) ? (Arm64Reg)index_reg->id : RTMP2;
	if (index_reg->kind == RCONST) {
		load_immediate(ctx, index_reg->id, index_r, false);
	} else if (index_reg->kind != RCPU) {
		ldr_stack(ctx, index_r, index->stackPos, index->size);
	}

	// Step 2: Load array (if it needs RTMP, the value will stay in RTMP)
	Arm64Reg array_r = (array_reg->kind == RCPU) ? (Arm64Reg)array_reg->id : RTMP;
	if (array_reg->kind != RCPU) {
		ldr_stack(ctx, array_r, array->stackPos, array->size);
	}

	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : X9;
	if (dst_r == X9) {
		preg *px9 = &ctx->pregs[X9];
		if (px9->holds != NULL) free_reg(ctx, px9);
	}

	// Step 3: Calculate element address
	// For varray: array + sizeof(varray) + index * elem_size
	// For CArray: array + index * elem_size (no header)

	if (is_carray) {
		// CArray: no header offset, start from array_r directly
		// Scale index by elem_size
		if (elem_size == 1) {
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, 0, array_r, RTMP);
		} else if (elem_size == 2 || elem_size == 4 || elem_size == 8) {
			int shift = (elem_size == 2) ? 1 : (elem_size == 4) ? 2 : 3;
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, shift, array_r, RTMP);
		} else {
			// Non-power-of-2: compute index * elem_size in RTMP2, then add
			load_immediate(ctx, elem_size, RTMP2, false);
			encode_madd_msub(ctx, 1, 0, RTMP2, XZR, index_r, RTMP2);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, array_r, RTMP);
		}
	} else {
		// varray: add sizeof(varray) header offset first
		encode_add_sub_imm(ctx, 1, 0, 0, 0, sizeof(varray), array_r, RTMP);

		// Add scaled index offset: RTMP = RTMP + (index_r << shift)
		if (elem_size == 1) {
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, 0, RTMP, RTMP);
		} else if (elem_size == 2 || elem_size == 4 || elem_size == 8) {
			int shift = (elem_size == 2) ? 1 : (elem_size == 4) ? 2 : 3;
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, shift, RTMP, RTMP);
		} else {
			// Non-power-of-2: scale index into RTMP2, then add
			load_immediate(ctx, elem_size, RTMP2, false);
			encode_madd_msub(ctx, 1, 0, RTMP2, XZR, index_r, RTMP2);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, RTMP, RTMP);
		}
	}

	if (is_lea) {
		// LEA: just move the computed address to dst
		mov_reg_reg(ctx, dst_r, RTMP, true);
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	} else if (IS_FLOAT(dst)) {
		// Float load: use FP register with V=1
		preg *pv0 = PVFPR(0);
		if (pv0->holds != NULL && pv0->holds != dst) {
			free_reg(ctx, pv0);
		}
		int size_bits = (dst->size == 8) ? 0x03 : 0x02;  // F64 or F32
		encode_ldr_str_imm(ctx, size_bits, 1, 0x01, 0, RTMP, V0);  // V=1 for FP
		str_stack_fp(ctx, V0, dst->stackPos, dst->size);
		// Clear dst's old binding - value is now on stack, not in a register
		if (dst->current != NULL) {
			dst->current->holds = NULL;
			dst->current = NULL;
		}
	} else {
		// Integer load
		int size_bits = (elem_size == 1) ? 0x00 : (elem_size == 2) ? 0x01 : (elem_size == 4) ? 0x02 : 0x03;
		encode_ldr_str_imm(ctx, size_bits, 0, 0x01, 0, RTMP, dst_r);
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	discard(ctx, array_reg);
	discard(ctx, index_reg);
}

/*
 * Array element assignment: array[index] = value
 * OSetArray: hl_aptr(array)[index] = value
 *
 * varray layout: { hl_type *t, hl_type *at, int size, int __pad } = 24 bytes
 * Data is INLINE immediately after the header (not via a pointer!)
 *
 * CArray (HABSTRACT) layout: raw memory, no header
 * For HOBJ/HSTRUCT: copy entire struct from value (which is address from LEA)
 * For other types: store value directly
 */
/*
 * IMPORTANT: We must load value and index BEFORE array when array uses RTMP,
 * because ldr_stack's fallback path uses RTMP as a temporary.
 * Order: value -> index -> array -> compute address -> store
 */
static void op_set_array(jit_ctx *ctx, vreg *array, vreg *index, vreg *value) {
	preg *array_reg = fetch(ctx, array);
	preg *index_reg = fetch(ctx, index);
	preg *value_reg = fetch(ctx, value);

	// CArrays (HABSTRACT) have different semantics
	bool is_carray = (array->t->kind == HABSTRACT);
	bool is_struct_copy = is_carray && (value->t->kind == HOBJ || value->t->kind == HSTRUCT);

	int elem_size;
	if (is_carray) {
		if (is_struct_copy) {
			// For HOBJ/HSTRUCT in CArray, element size is the runtime object size
			hl_runtime_obj *rt = hl_get_obj_rt(value->t);
			elem_size = rt->size;
		} else {
			// For other types in CArray, element size is pointer size
			elem_size = sizeof(void*);
		}
	} else {
		elem_size = hl_type_size(value->t);
	}

	// Step 1: Load value FIRST (before using RTMP for address computation)
	// For struct copy, value is a pointer to the source struct
	// For floats, use FP register; for integers, use CPU register
	Arm64Reg value_r = X9;
	Arm64FpReg value_fp = V16;
	bool is_float_value = IS_FLOAT(value);

	if (is_float_value) {
		if (value_reg->kind == RFPU) {
			value_fp = (Arm64FpReg)value_reg->id;
		} else {
			// Ensure V16 is free before using it
			preg *pv16 = PVFPR(16);
			if (pv16->holds != NULL) free_reg(ctx, pv16);
			ldr_stack_fp(ctx, value_fp, value->stackPos, value->size);
		}
	} else {
		value_r = (value_reg->kind == RCPU) ? (Arm64Reg)value_reg->id : X9;
		if (value_reg->kind == RCONST) {
			// Ensure X9 is free if we are using it
			if (value_r == X9) {
				preg *px9 = &ctx->pregs[X9];
				if (px9->holds != NULL) free_reg(ctx, px9);
			}
			load_immediate(ctx, value_reg->id, value_r, value->size == 8);
		} else if (value_reg->kind != RCPU) {
			// Ensure X9 is free if we are using it
			if (value_r == X9) {
				preg *px9 = &ctx->pregs[X9];
				if (px9->holds != NULL) free_reg(ctx, px9);
			}
			ldr_stack(ctx, value_r, value->stackPos, value->size);
		}
	}

	// Step 2: Load index (may clobber RTMP in fallback, but we haven't used it yet)
	Arm64Reg index_r = (index_reg->kind == RCPU) ? (Arm64Reg)index_reg->id : RTMP2;
	if (index_reg->kind == RCONST) {
		load_immediate(ctx, index_reg->id, index_r, false);
	} else if (index_reg->kind != RCPU) {
		ldr_stack(ctx, index_r, index->stackPos, index->size);
	}

	// Step 3: Load array (if it needs RTMP, the value will stay in RTMP)
	Arm64Reg array_r = (array_reg->kind == RCPU) ? (Arm64Reg)array_reg->id : RTMP;
	if (array_reg->kind != RCPU) {
		ldr_stack(ctx, array_r, array->stackPos, array->size);
	}

	// Step 4: Calculate element address
	// For varray: array + sizeof(varray) + index * elem_size
	// For CArray: array + index * elem_size (no header)

	if (is_carray) {
		// CArray: no header offset
		if (elem_size == 1) {
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, 0, array_r, RTMP);
		} else if (elem_size == 2 || elem_size == 4 || elem_size == 8) {
			int shift = (elem_size == 2) ? 1 : (elem_size == 4) ? 2 : 3;
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, shift, array_r, RTMP);
		} else {
			// Non-power-of-2: compute index * elem_size
			load_immediate(ctx, elem_size, RTMP2, false);
			encode_madd_msub(ctx, 1, 0, RTMP2, XZR, index_r, RTMP2);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, array_r, RTMP);
		}
	} else {
		// varray: add sizeof(varray) header offset first
		encode_add_sub_imm(ctx, 1, 0, 0, 0, sizeof(varray), array_r, RTMP);

		// Add scaled index offset
		if (elem_size == 1) {
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, 0, RTMP, RTMP);
		} else if (elem_size == 2 || elem_size == 4 || elem_size == 8) {
			int shift = (elem_size == 2) ? 1 : (elem_size == 4) ? 2 : 3;
			encode_add_sub_reg(ctx, 1, 0, 0, SHIFT_LSL, index_r, shift, RTMP, RTMP);
		} else {
			load_immediate(ctx, elem_size, RTMP2, false);
			encode_madd_msub(ctx, 1, 0, RTMP2, XZR, index_r, RTMP2);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, RTMP, RTMP);
		}
	}

	if (is_struct_copy) {
		// Copy struct from value (pointer) to RTMP (destination)
		// value_r points to source struct, RTMP points to destination
		// Use X10 as temporary for copy (not value_r which we need as source base)
		int offset = 0;
		while (offset < elem_size) {
			int remain = elem_size - offset;
			int copy_size, size_bits;
			if (remain >= 8) {
				copy_size = 8;
				size_bits = 0x03;
			} else if (remain >= 4) {
				copy_size = 4;
				size_bits = 0x02;
			} else if (remain >= 2) {
				copy_size = 2;
				size_bits = 0x01;
			} else {
				copy_size = 1;
				size_bits = 0x00;
			}
			// Load from source: X10 = [value_r + offset]
			encode_ldur_stur(ctx, size_bits, 0, 0x01, offset, value_r, X10);
			// Store to dest: [RTMP + offset] = X10
			encode_ldur_stur(ctx, size_bits, 0, 0x00, offset, RTMP, X10);
			offset += copy_size;
		}
	} else if (is_float_value) {
		// Float store: STR Vn, [RTMP] with V=1
		int size_bits = (value->size == 8) ? 0x03 : 0x02;  // F64 or F32
		encode_ldr_str_imm(ctx, size_bits, 1, 0x00, 0, RTMP, value_fp);  // V=1 for FP
	} else {
		// Integer store: STR Xn, [RTMP]
		int size_bits = (elem_size == 1) ? 0x00 : (elem_size == 2) ? 0x01 : (elem_size == 4) ? 0x02 : 0x03;
		encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, RTMP, value_r);
	}

	discard(ctx, array_reg);
	discard(ctx, index_reg);
	discard(ctx, value_reg);
}

/*
 * Global variable access: dst = globals[index]
 * OGetGlobal: Use PC-relative addressing with ADRP + LDR
 */
static void op_get_global(jit_ctx *ctx, vreg *dst, int global_index) {
	preg *dst_reg = alloc_dst(ctx, dst);

	// Get global address from module
	void **globals = (void**)ctx->m->globals_data;
	void *global_addr = &globals[global_index];

	// Load global address to RTMP2
	load_immediate(ctx, (int64_t)global_addr, RTMP2, true);

	if (IS_FLOAT(dst)) {
		// Float: load into FPU register
		Arm64FpReg dst_r = (dst_reg->kind == RFPU) ? (Arm64FpReg)dst_reg->id : V16;
		// LDR Vn, [RTMP2] - floating point load
		// size: 0x02=32-bit (S), 0x03=64-bit (D)
		encode_ldr_str_imm(ctx, dst->size == 8 ? 0x03 : 0x02, 1, 0x01, 0, RTMP2, dst_r);
		// Store to stack
		str_stack_fp(ctx, dst_r, dst->stackPos, dst->size);
	} else {
		// Integer/pointer: load into CPU register
		Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;
		// LDR Xn, [RTMP2]
		encode_ldr_str_imm(ctx, dst->size == 8 ? 0x03 : 0x02, 0, 0x01, 0, RTMP2, dst_r);
		// Store to stack
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	}
}

/*
 * Global variable assignment: globals[index] = value
 * OSetGlobal
 */
static void op_set_global(jit_ctx *ctx, int global_index, vreg *value) {
	preg *value_reg = fetch(ctx, value);

	// Get global address from module
	void **globals = (void**)ctx->m->globals_data;
	void *global_addr = &globals[global_index];

	// Load global address to RTMP2
	load_immediate(ctx, (int64_t)global_addr, RTMP2, true);

	if (IS_FLOAT(value)) {
		// Float: store from FPU register
		Arm64FpReg value_r = (value_reg->kind == RFPU) ? (Arm64FpReg)value_reg->id : V16;
		if (value_reg->kind != RFPU) {
			// Load from stack into temp FPU register
			ldr_stack_fp(ctx, value_r, value->stackPos, value->size);
		}
		// STR Vn, [RTMP2] - floating point store
		encode_ldr_str_imm(ctx, value->size == 8 ? 0x03 : 0x02, 1, 0x00, 0, RTMP2, value_r);
	} else {
		// Integer/pointer: store from CPU register
		Arm64Reg value_r = (value_reg->kind == RCPU) ? (Arm64Reg)value_reg->id : RTMP;
		if (value_reg->kind == RCONST) {
			load_immediate(ctx, value_reg->id, value_r, value->size == 8);
		} else if (value_reg->kind != RCPU) {
			ldr_stack(ctx, value_r, value->stackPos, value->size);
		}
		// STR Xn, [RTMP2]
		encode_ldr_str_imm(ctx, value->size == 8 ? 0x03 : 0x02, 0, 0x00, 0, RTMP2, value_r);
	}

	discard(ctx, value_reg);
}

// ============================================================================
// Reference Operations
// ============================================================================

/*
 * Create reference: dst = &src
 * ORef: dst = address of vreg
 *
 * IMPORTANT: After taking a reference to a vreg, that vreg may be modified
 * through the reference (via OSetref). We must:
 * 1. Ensure src is spilled to stack (in case it's only in a register)
 * 2. Invalidate src's register binding so future reads go to stack
 */
static void op_ref(jit_ctx *ctx, vreg *dst, vreg *src) {
	// First, ensure src is on stack and invalidate its register binding
	// (like x86's scratch(ra->current))
	if (src->current != NULL) {
		// Spill to stack if in a register
		store(ctx, src, src->current);
		// Invalidate the binding so future reads go to stack
		src->current->holds = NULL;
		src->current = NULL;
	}

	preg *dst_reg = alloc_dst(ctx, dst);
	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;

	// Calculate stack address: FP + src->stackPos
	if (src->stackPos >= 0) {
		// ADD dst_r, FP, #stackPos
		encode_add_sub_imm(ctx, 1, 0, 0, 0, src->stackPos, FP, dst_r);
	} else {
		// SUB dst_r, FP, #(-stackPos)
		encode_add_sub_imm(ctx, 1, 1, 0, 0, -src->stackPos, FP, dst_r);
	}

	// Always store to stack - source of truth for later loads
	str_stack(ctx, dst_r, dst->stackPos, dst->size);
}

/*
 * Dereference: dst = *src
 * OUnref: Load value from pointer
 */
static void op_unref(jit_ctx *ctx, vreg *dst, vreg *src) {
	preg *src_reg = fetch(ctx, src);

	// Load the pointer (always integer register since it's an address)
	Arm64Reg src_r = (src_reg->kind == RCPU) ? (Arm64Reg)src_reg->id : RTMP;
	if (src_reg->kind != RCPU) {
		ldr_stack(ctx, src_r, src->stackPos, src->size);
	}

	int size_bits = (dst->size == 1) ? 0x00 : (dst->size == 2) ? 0x01 : (dst->size == 4) ? 0x02 : 0x03;

	if (IS_FLOAT(dst)) {
		// Float dereference: LDR Vd, [src_r]
		preg *dst_reg = alloc_dst(ctx, dst);
		Arm64FpReg dst_r = (dst_reg->kind == RFPU) ? (Arm64FpReg)dst_reg->id : V16;
		encode_ldr_str_imm(ctx, size_bits, 1, 0x01, 0, src_r, dst_r);
		str_stack_fp(ctx, dst_r, dst->stackPos, dst->size);
	} else {
		// Integer dereference: LDR Xd, [src_r]
		preg *dst_reg = alloc_dst(ctx, dst);
		Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP2;
		encode_ldr_str_imm(ctx, size_bits, 0, 0x01, 0, src_r, dst_r);
		str_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	discard(ctx, src_reg);
}

/*
 * Set reference: *dst = src
 * OSetref: Store value to pointer
 */
static void op_setref(jit_ctx *ctx, vreg *dst, vreg *src) {
	preg *dst_reg = fetch(ctx, dst);
	preg *src_reg = fetch(ctx, src);

	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;
	if (dst_reg->kind != RCPU) {
		ldr_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	Arm64Reg src_r = (src_reg->kind == RCPU) ? (Arm64Reg)src_reg->id : RTMP2;
	if (src_reg->kind == RCONST) {
		load_immediate(ctx, src_reg->id, src_r, src->size == 8);
	} else if (src_reg->kind != RCPU) {
		ldr_stack(ctx, src_r, src->stackPos, src->size);
	}

	// Store to pointer: STR src_r, [dst_r]
	int size_bits = (src->size == 1) ? 0x00 : (src->size == 2) ? 0x01 : (src->size == 4) ? 0x02 : 0x03;
	encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, dst_r, src_r);

	discard(ctx, dst_reg);
	discard(ctx, src_reg);
}

// ============================================================================
// Comparison Operations (result stored, not branching)
// ============================================================================

/*
 * Equality comparison: dst = (a == b)
 * OEq/ONeq/OLt/OGte/etc: Store comparison result as boolean
 */
static void op_compare(jit_ctx *ctx, vreg *dst, vreg *a, vreg *b, hl_op op) {
	preg *a_reg = fetch(ctx, a);
	preg *b_reg = fetch(ctx, b);
	preg *dst_reg = alloc_dst(ctx, dst);

	Arm64Reg a_r = (a_reg->kind == RCPU) ? (Arm64Reg)a_reg->id : RTMP;
	if (a_reg->kind == RCONST) {
		load_immediate(ctx, a_reg->id, a_r, a->size == 8);
	} else if (a_reg->kind != RCPU) {
		ldr_stack(ctx, a_r, a->stackPos, a->size);
	}

	Arm64Reg b_r = (b_reg->kind == RCPU) ? (Arm64Reg)b_reg->id : RTMP2;
	if (b_reg->kind == RCONST) {
		load_immediate(ctx, b_reg->id, b_r, b->size == 8);
	} else if (b_reg->kind != RCPU) {
		ldr_stack(ctx, b_r, b->stackPos, b->size);
	}

	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : X9;
	if (dst_r == X9) {
		preg *px9 = &ctx->pregs[X9];
		if (px9->holds != NULL) free_reg(ctx, px9);
	}

	bool is_float = IS_FLOAT(a);

	if (is_float) {
		// Floating-point comparison
		Arm64FpReg fa_r = (a_reg->kind == RFPU) ? (Arm64FpReg)a_reg->id : V16;
		Arm64FpReg fb_r = (b_reg->kind == RFPU) ? (Arm64FpReg)b_reg->id : V17;

		if (fa_r == V16) {
			preg *pv16 = PVFPR(16);
			if (pv16->holds != NULL) free_reg(ctx, pv16);
		}
		if (fb_r == V17) {
			preg *pv17 = PVFPR(17);
			if (pv17->holds != NULL) free_reg(ctx, pv17);
		}

		if (a_reg->kind != RFPU) {
			// Load from stack to FP register
			ldr_stack_fp(ctx, fa_r, a->stackPos, a->size);
		}
		if (b_reg->kind != RFPU) {
			ldr_stack_fp(ctx, fb_r, b->stackPos, b->size);
		}

		// FCMP fa_r, fb_r
		int is_double = a->size == 8 ? 1 : 0;
		encode_fp_compare(ctx, 0, is_double, is_double, fb_r, 0, fa_r);
	} else {
		// Integer comparison: CMP a_r, b_r
		encode_add_sub_reg(ctx, a->size == 8 ? 1 : 0, 1, 1, 0, b_r, 0, a_r, XZR);
	}

	// Get condition code for this operation
	ArmCondition cond = hl_cond_to_arm(op, is_float);

	// CSET dst_r, cond  (Set register to 1 if condition true, 0 otherwise)
	// Encoding: CSINC dst, XZR, XZR, !cond
	// This sets dst = (cond) ? 1 : 0
	int inv_cond = cond ^ 1;  // Invert condition
	// CSINC: sf=0, op=0, S=0, Rm=XZR, cond=inv_cond, o2=1, Rn=XZR, Rd=dst_r
	EMIT32(ctx,(0 << 31) | (0 << 30) | (0xD4 << 21) | (XZR << 16) | (inv_cond << 12) | (1 << 10) | (XZR << 5) | dst_r);

	// Always store to stack - source of truth for later loads
	str_stack(ctx, dst_r, dst->stackPos, dst->size);

	discard(ctx, a_reg);
	discard(ctx, b_reg);
}

// ============================================================================
// Type and Object Operations
// ============================================================================

/*
 * Get object type: dst = obj->type
 * OType: Load type pointer from object
 */
static void op_type(jit_ctx *ctx, vreg *dst, vreg *obj) {
	preg *obj_reg = fetch(ctx, obj);
	preg *dst_reg = alloc_dst(ctx, dst);

	Arm64Reg obj_r = (obj_reg->kind == RCPU) ? (Arm64Reg)obj_reg->id : RTMP;
	if (obj_reg->kind != RCPU) {
		ldr_stack(ctx, obj_r, obj->stackPos, obj->size);
	}

	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP2;

	// Load type pointer from object header (first field at offset 0)
	// LDR dst_r, [obj_r]
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, obj_r, dst_r);

	// Always store to stack - source of truth for later loads
	str_stack(ctx, dst_r, dst->stackPos, dst->size);

	discard(ctx, obj_reg);
}

/*
 * OGetThis: Load a field from the "this" object (R(0))
 * Equivalent to OField but implicitly uses R(0) as the object
 */
static void op_get_this(jit_ctx *ctx, vreg *dst, int field_idx) {
	vreg *this_vreg = R(0);
	op_field(ctx, dst, this_vreg, field_idx);
}

/*
 * Get the dynamic cast function for a given type
 */
static void *get_dyncast(hl_type *t) {
	switch (t->kind) {
	case HF32:
		return hl_dyn_castf;
	case HF64:
		return hl_dyn_castd;
	case HI64:
	case HGUID:
		return hl_dyn_casti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_casti;
	default:
		return hl_dyn_castp;
	}
}

/*
 * Cast operation (safe cast with runtime check)
 * OSafeCast: dst = (target_type)obj or NULL if cast fails
 */
static void op_safe_cast(jit_ctx *ctx, vreg *dst, vreg *obj, hl_type *target_type) {
	// Special case: Null<T> to T - unbox with null check
	if (obj->t->kind == HNULL && obj->t->tparam->kind == dst->t->kind) {
		int jnull, jend;

		switch (dst->t->kind) {
		case HUI8:
		case HUI16:
		case HI32:
		case HBOOL:
		case HI64:
		case HGUID:
			{
				preg *tmp = fetch(ctx, obj);
				Arm64Reg r = (tmp->kind == RCPU) ? tmp->id : RTMP;
				if (tmp->kind != RCPU) {
					ldr_stack(ctx, r, obj->stackPos, obj->size);
				}
				// Test for null
				encode_add_sub_imm(ctx, 1, 1, 1, 0, 0, r, XZR);  // CMP r, #0
				jnull = BUF_POS();
				encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ null_path

				// Non-null: load value from offset 8 with correct size
				// Size determines scale: 0x00=1, 0x01=2, 0x02=4, 0x03=8
				// So offset = 8 / scale to get byte offset 8
				int size_code;
				int scaled_offset;
				switch (dst->size) {
					case 1: size_code = 0x00; scaled_offset = 8; break;  // LDRB [r, #8]
					case 2: size_code = 0x01; scaled_offset = 4; break;  // LDRH [r, #8]
					case 4: size_code = 0x02; scaled_offset = 2; break;  // LDR W [r, #8]
					default: size_code = 0x03; scaled_offset = 1; break; // LDR X [r, #8]
				}
				// The LDR below clobbers r. If obj is dirty in r, save it to stack first.
				// This preserves obj's value (the dynamic pointer) for later use.
				if (obj->dirty && obj->current == tmp) {
					str_stack(ctx, r, obj->stackPos, obj->size);
					obj->dirty = 0;
				}
				encode_ldr_str_imm(ctx, size_code, 0, 0x01, scaled_offset, r, r);
				jend = BUF_POS();
				encode_branch_uncond(ctx, 0);  // B end

				// Null path: set to zero
				patch_jump(ctx, jnull, BUF_POS());
				load_immediate(ctx, 0, r, dst->size == 8);

				// End
				patch_jump(ctx, jend, BUF_POS());
				str_stack(ctx, r, dst->stackPos, dst->size);
				// Clear binding - register no longer holds obj's original value
				discard(ctx, tmp);
				// Invalidate dst's old binding since we wrote directly to stack
				if (dst->current) {
					dst->current->holds = NULL;
					dst->current = NULL;
				}
			}
			return;

		case HF32:
		case HF64:
			{
				preg *tmp = fetch(ctx, obj);
				Arm64Reg r = (tmp->kind == RCPU) ? tmp->id : RTMP;
				if (tmp->kind != RCPU) {
					ldr_stack(ctx, r, obj->stackPos, obj->size);
				}
				// Evict any vreg currently bound to V0 before using it
				preg *pv0 = PVFPR(0);
				if (pv0->holds != NULL && pv0->holds != dst) {
					free_reg(ctx, pv0);
				}
				// Test for null
				encode_add_sub_imm(ctx, 1, 1, 1, 0, 0, r, XZR);  // CMP r, #0
				jnull = BUF_POS();
				encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ null_path

				// Non-null: load float from offset 8
				encode_ldr_str_imm(ctx, (dst->size == 8) ? 0x03 : 0x02, 1, 0x01, 8 / dst->size, r, V0);
				jend = BUF_POS();
				encode_branch_uncond(ctx, 0);  // B end

				// Null path: set to zero
				patch_jump(ctx, jnull, BUF_POS());
				// FMOV Vd, XZR
				EMIT32(ctx, (1 << 31) | (0 << 29) | (0x1E << 24) | (1 << 22) | (1 << 21) | (7 << 16) | (31 << 5) | V0);

				// End
				patch_jump(ctx, jend, BUF_POS());
				str_stack_fp(ctx, V0, dst->stackPos, dst->size);
				// Clear binding - register no longer holds obj's original value
				discard(ctx, tmp);
				// Invalidate dst's old binding since we wrote directly to stack
				if (dst->current) {
					dst->current->holds = NULL;
					dst->current = NULL;
				}
			}
			return;

		default:
			break;
		}
	}

	// General case: call runtime cast function
	spill_regs(ctx);

	// Get stack address of obj
	// LEA X0, [FP, #obj->stackPos] or similar
	if (obj->stackPos >= 0) {
		encode_add_sub_imm(ctx, 1, 0, 0, 0, obj->stackPos, FP, X0);
	} else {
		encode_add_sub_imm(ctx, 1, 1, 0, 0, -obj->stackPos, FP, X0);
	}

	// Set up arguments based on destination type
	void *cast_func = get_dyncast(dst->t);
	switch (dst->t->kind) {
	case HF32:
	case HF64:
	case HI64:
		// 2 args: ptr, src_type
		load_immediate(ctx, (int64_t)obj->t, X1, true);
		break;
	default:
		// 3 args: ptr, src_type, dst_type
		load_immediate(ctx, (int64_t)obj->t, X1, true);
		load_immediate(ctx, (int64_t)dst->t, X2, true);
		break;
	}

	// Call cast function
	load_immediate(ctx, (int64_t)cast_func, RTMP, true);
	EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

	// Store result and clear stale binding
	if (IS_FLOAT(dst)) {
		str_stack_fp(ctx, V0, dst->stackPos, dst->size);
	} else {
		str_stack(ctx, X0, dst->stackPos, dst->size);
	}
	store_result(ctx, dst);
}

/*
 * Null coalescing: dst = (a != null) ? a : b
 * OCoalesce/ONullCheck
 */
static void op_null_check(jit_ctx *ctx, vreg *dst, int hashed_name) {
	// Check if dst is null and call hl_null_access if so
	preg *dst_reg = fetch(ctx, dst);

	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;
	if (dst_reg->kind != RCPU) {
		ldr_stack(ctx, dst_r, dst->stackPos, dst->size);
	}

	// Compare with zero: CMP dst_r, #0 (actually SUBS XZR, dst_r, #0)
	encode_add_sub_imm(ctx, 1, 1, 1, 0, 0, dst_r, XZR);

	// If not zero (not null), skip error handling: B.NE skip
	int bne_pos = BUF_POS();
	encode_branch_cond(ctx, 0, COND_NE);  // B.NE (will patch offset)

	// Null path: call hl_null_access or jit_null_fail
	// NOTE: Do NOT call spill_regs() here! hl_null_access never returns (it throws),
	// and spill_regs() would corrupt compile-time register bindings for the non-null path.
	if (hashed_name) {
		load_immediate(ctx, hashed_name, X0, true);
		load_immediate(ctx, (int64_t)jit_null_fail, RTMP, true);
	} else {
		load_immediate(ctx, (int64_t)hl_null_access, RTMP, true);
	}
	EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
	// hl_null_access doesn't return (it throws), but we don't emit anything after

	// Patch the B.NE to skip here
	int skip_pos = BUF_POS();
	int bne_offset = (skip_pos - bne_pos) / 4;
	ctx->buf.b = ctx->startBuf + bne_pos;
	encode_branch_cond(ctx, bne_offset, COND_NE);
	ctx->buf.b = ctx->startBuf + skip_pos;

	discard(ctx, dst_reg);
}

/*
 * Object/memory allocation operations
 * These typically call into the runtime allocator
 */
static void op_new(jit_ctx *ctx, vreg *dst, hl_type *type) {
	// Call runtime allocator based on type kind
	// Different type kinds require different allocation functions:
	// - HOBJ/HSTRUCT: hl_alloc_obj(type)
	// - HDYNOBJ: hl_alloc_dynobj() - no arguments!
	// - HVIRTUAL: hl_alloc_virtual(type)

	// Spill all caller-saved registers BEFORE the call
	spill_regs(ctx);

	void *alloc_func;
	int has_type_arg = 1;

	switch (type->kind) {
	case HOBJ:
	case HSTRUCT:
		alloc_func = (void*)hl_alloc_obj;
		break;
	case HDYNOBJ:
		alloc_func = (void*)hl_alloc_dynobj;
		has_type_arg = 0;  // hl_alloc_dynobj takes no arguments
		break;
	case HVIRTUAL:
		alloc_func = (void*)hl_alloc_virtual;
		break;
	default:
		// Unsupported type for ONew
		printf("op_new: unsupported type kind %d\n", type->kind);
		return;
	}

	// Load type address to X0 (first argument) if needed
	if (has_type_arg) {
		load_immediate(ctx, (int64_t)type, X0, true);
	}

	// Load function pointer and call
	load_immediate(ctx, (int64_t)alloc_func, RTMP, true);

	// Call allocator: BLR RTMP
	EMIT32(ctx, (0xD63F0000) | (RTMP << 5));

	// Result is in X0 - always store to stack first (source of truth for later loads)
	str_stack(ctx, X0, dst->stackPos, dst->size);

	// Also keep in a register if allocated
	preg *dst_reg = alloc_dst(ctx, dst);
	if (dst_reg->kind == RCPU && (Arm64Reg)dst_reg->id != X0) {
		mov_reg_reg(ctx, (Arm64Reg)dst_reg->id, X0, 8);
	}
}

/*
 * String/bytes operations
 */
static void op_string(jit_ctx *ctx, vreg *dst, int string_index) {
	// Load UTF-16 string from module string table
	preg *dst_reg = alloc_dst(ctx, dst);
	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;

	// Get UTF-16 string pointer (hl_get_ustring converts from UTF-8 and caches)
	const uchar *string_ptr = hl_get_ustring(ctx->m->code, string_index);

	// Load string address
	load_immediate(ctx, (int64_t)string_ptr, dst_r, true);

	// Always store to stack - source of truth for later loads
	str_stack(ctx, dst_r, dst->stackPos, dst->size);
}

static void op_bytes(jit_ctx *ctx, vreg *dst, int bytes_index) {
	// Load bytes from module bytes table
	preg *dst_reg = alloc_dst(ctx, dst);
	Arm64Reg dst_r = (dst_reg->kind == RCPU) ? (Arm64Reg)dst_reg->id : RTMP;

	// Get bytes pointer from module - use bytes_pos lookup for version >= 5
	char *bytes_ptr;
	if (ctx->m->code->version >= 5)
		bytes_ptr = ctx->m->code->bytes + ctx->m->code->bytes_pos[bytes_index];
	else
		bytes_ptr = ctx->m->code->strings[bytes_index];

	// Load bytes address
	load_immediate(ctx, (int64_t)bytes_ptr, dst_r, true);

	// Always store to stack - source of truth for later loads
	str_stack(ctx, dst_r, dst->stackPos, dst->size);
}

// Forward declaration for prepare_call_args (defined later)
static int prepare_call_args(jit_ctx *ctx, hl_type **arg_types, vreg **args, int nargs, bool is_native);

/*
 * Virtual/method calls
 * OCallMethod/OCallThis/OCallClosure
 */
// ============================================================================
// Dynamic Object Helpers
// ============================================================================

/**
 * Get the appropriate dynamic set function for a type
 */
static void *get_dynset(hl_type *t) {
	switch (t->kind) {
	case HF32:
		return hl_dyn_setf;
	case HF64:
		return hl_dyn_setd;
	case HI64:
	case HGUID:
		return hl_dyn_seti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_seti;
	default:
		return hl_dyn_setp;
	}
}

/**
 * Get the appropriate dynamic get function for a type
 */
static void *get_dynget(hl_type *t) {
	switch (t->kind) {
	case HF32:
		return hl_dyn_getf;
	case HF64:
		return hl_dyn_getd;
	case HI64:
	case HGUID:
		return hl_dyn_geti64;
	case HI32:
	case HUI16:
	case HUI8:
	case HBOOL:
		return hl_dyn_geti;
	default:
		return hl_dyn_getp;
	}
}

// ============================================================================
// Method and Function Calls
// ============================================================================

static void op_call_method_obj(jit_ctx *ctx, vreg *dst, vreg *obj, int method_index, vreg **args, int nargs) {
	// HOBJ method call: obj->type->vobj_proto[method_index](obj, args...)

	// Spill all caller-saved registers BEFORE the call
	spill_regs(ctx);

	// Now fetch obj (will load from stack since we just spilled)
	preg *obj_reg = fetch(ctx, obj);

	Arm64Reg obj_r = (obj_reg->kind == RCPU) ? (Arm64Reg)obj_reg->id : RTMP;
	if (obj_reg->kind != RCPU) {
		ldr_stack(ctx, obj_r, obj->stackPos, obj->size);
	}

	// Load type from obj[0]
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, obj_r, RTMP);  // RTMP = obj->type
	// Load vobj_proto from type[16] (HL_WSIZE*2 = offset index 2)
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 2, RTMP, RTMP);   // RTMP = type->vobj_proto
	// Load method pointer from proto[method_index] into RTMP2
	// NOTE: We use RTMP2 here because prepare_call_args uses RTMP for stack calculations
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, method_index, RTMP, RTMP2);

	discard(ctx, obj_reg);

	// Prepare call with obj as first argument
	vreg **full_args = (vreg**)malloc(sizeof(vreg*) * (nargs + 1));
	full_args[0] = obj;
	for (int i = 0; i < nargs; i++) {
		full_args[i + 1] = args[i];
	}

	// Prepare arguments (this uses RTMP, but method pointer is safe in RTMP2)
	int stack_space = prepare_call_args(ctx, NULL, full_args, nargs + 1, false);
	free(full_args);

	// Call method: BLR RTMP2
	EMIT32(ctx,(0xD63F0000) | (RTMP2 << 5));

	// Clean up stack
	if (stack_space > 0) {
		encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
	}

	// Store return value
	if (dst && dst->t->kind != HVOID) {
		preg *p = alloc_dst(ctx, dst);
		if (IS_FLOAT(dst)) {
			if (p->kind == RFPU && (Arm64FpReg)p->id != V0) {
				fmov_reg_reg(ctx, (Arm64FpReg)p->id, V0, dst->size);
			} else if (p->kind == RSTACK) {
				str_stack_fp(ctx, V0, dst->stackPos, dst->size);
			}
		} else {
			if (p->kind == RCPU && (Arm64Reg)p->id != X0) {
				mov_reg_reg(ctx, (Arm64Reg)p->id, X0, dst->size);
			} else if (p->kind == RSTACK) {
				str_stack(ctx, X0, dst->stackPos, dst->size);
			}
		}
	}
}

// ============================================================================
// Function Calls
// ============================================================================

/*
 * Prepare arguments for a function call according to AAPCS64:
 * - First 8 integer/pointer args in X0-X7
 * - First 8 floating-point args in V0-V7
 * - Additional args on stack (16-byte aligned)
 * - Returns the total stack space needed for overflow args
 */
static int prepare_call_args(jit_ctx *ctx, hl_type **arg_types, vreg **args, int nargs, bool is_native) {
	int int_reg_count = 0;
	int fp_reg_count = 0;
	int stack_offset = 0;

	// First pass: count args and calculate stack space needed
	for (int i = 0; i < nargs; i++) {
		bool is_fp = IS_FLOAT(args[i]);
		int *reg_count = is_fp ? &fp_reg_count : &int_reg_count;

		if (*reg_count >= CALL_NREGS) {
			// Arg goes on stack
			stack_offset += 8;  // Each stack arg takes 8 bytes (aligned)
		}
		(*reg_count)++;
	}

	// Align stack to 16 bytes
	if (stack_offset & 15)
		stack_offset = (stack_offset + 15) & ~15;

	// Allocate stack space for overflow args if needed
	if (stack_offset > 0) {
		// SUB SP, SP, #stack_offset
		encode_add_sub_imm(ctx, 1, 1, 0, 0, stack_offset, SP_REG, SP_REG);
	}

	// Second pass: move arguments to their locations
	int_reg_count = 0;
	fp_reg_count = 0;
	int current_stack_offset = 0;

	// After spill_regs(), all values are on stack.
	// Load arguments directly to their destination registers to avoid
	// the register allocation problem where fetch() reuses registers.
	for (int i = 0; i < nargs; i++) {
		vreg *arg = args[i];
		bool is_fp = IS_FLOAT(arg);

		if (is_fp) {
			if (fp_reg_count < CALL_NREGS) {
				// Load directly to FP argument register
				Arm64FpReg dest_reg = FP_CALL_REGS[fp_reg_count];
				ldr_stack_fp(ctx, dest_reg, arg->stackPos, arg->size);
				fp_reg_count++;
			} else {
				// Overflow: load to temp, then store to stack
				ldr_stack_fp(ctx, V16, arg->stackPos, arg->size);
				encode_ldr_str_imm(ctx, arg->size == 4 ? 0x02 : 0x03, 1, 0x00,
				                   current_stack_offset / (arg->size == 4 ? 4 : 8),
				                   SP_REG, V16);
				current_stack_offset += 8;
			}
		} else {
			// Integer/pointer argument
			if (int_reg_count < CALL_NREGS) {
				// Load directly to integer argument register
				Arm64Reg dest_reg = CALL_REGS[int_reg_count];
				ldr_stack(ctx, dest_reg, arg->stackPos, arg->size);
				int_reg_count++;
			} else {
				// Overflow: load to temp, then store to stack
				ldr_stack(ctx, RTMP, arg->stackPos, arg->size);
				encode_ldr_str_imm(ctx, arg->size == 8 ? 0x03 : 0x02, 0, 0x00,
				                   current_stack_offset / (arg->size == 8 ? 8 : 4),
				                   SP_REG, RTMP);
				current_stack_offset += 8;
			}
		}
	}

	return stack_offset;
}

/*
 * Call a native C function
 */
static void op_call_native(jit_ctx *ctx, vreg *dst, hl_type *ftype, void *func_ptr, vreg **args, int nargs) {
	// Spill all caller-saved registers BEFORE the call
	spill_regs(ctx);

	// Prepare arguments (arg_types not actually used by prepare_call_args)
	int stack_space = prepare_call_args(ctx, NULL, args, nargs, true);

	// Load function pointer to RTMP
	load_immediate(ctx, (int64_t)func_ptr, RTMP, true);

	// BLR RTMP  (Branch with Link to Register)
	// Encoding: 1101 0110 0011 1111 0000 00rr rrr0 0000
	// where rrrrr = RTMP register number
	EMIT32(ctx,(0xD63F0000) | (RTMP << 5));

	// Clean up stack if we allocated space for args
	if (stack_space > 0) {
		// ADD SP, SP, #stack_space
		encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
	}

	// Store return value if needed
	if (dst && dst->t->kind != HVOID) {
		// Always store to stack first (source of truth for later loads)
		if (IS_FLOAT(dst)) {
			str_stack_fp(ctx, V0, dst->stackPos, dst->size);
		} else {
			str_stack(ctx, X0, dst->stackPos, dst->size);
		}

		// Also keep in a register if allocated to a different one
		preg *p = alloc_dst(ctx, dst);
		if (IS_FLOAT(dst)) {
			if (p->kind == RFPU && (Arm64FpReg)p->id != V0) {
				fmov_reg_reg(ctx, (Arm64FpReg)p->id, V0, dst->size);
			}
		} else {
			if (p->kind == RCPU && (Arm64Reg)p->id != X0) {
				mov_reg_reg(ctx, (Arm64Reg)p->id, X0, dst->size);
			}
		}
	}
}

/*
 * Call a native function with a known absolute address
 * The address is embedded directly in the instruction stream (no patching needed)
 */
static void call_native(jit_ctx *ctx, void *nativeFun, int stack_space) {
	// Emit indirect call sequence with the address embedded inline:
	//   LDR X17, #12     ; load target address from PC+12
	//   BLR X17          ; call
	//   B #12            ; skip over the literal
	//   .quad addr       ; 8-byte absolute address (embedded now, not patched later)

	EMIT32(ctx, 0x58000071);  // LDR X17, #12
	EMIT32(ctx, 0xD63F0220);  // BLR X17
	EMIT32(ctx, 0x14000003);  // B #12 (skip 3 instructions = 12 bytes)

	// Embed the native function address directly
	uint64_t addr = (uint64_t)nativeFun;
	EMIT32(ctx, (uint32_t)(addr & 0xFFFFFFFF));        // Low 32 bits
	EMIT32(ctx, (uint32_t)((addr >> 32) & 0xFFFFFFFF)); // High 32 bits

	// Clean up stack if we allocated space for args
	if (stack_space > 0) {
		encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
	}
}

/*
 * Emit a call to a function by its index (without spill/prepare - for use when those are already done)
 * Used by compareFun and other places that set up args manually
 */
static void emit_call_findex(jit_ctx *ctx, int findex, int stack_space) {
	int fid = findex < 0 ? -1 : ctx->m->functions_indexes[findex];
	bool isNative = fid >= ctx->m->code->nfunctions;

	if (fid < 0) {
		jit_error("Invalid function index");
	} else if (isNative) {
		// Native function - address is already resolved
		call_native(ctx, ctx->m->functions_ptrs[findex], stack_space);
	} else {
		// JIT function - use indirect call via literal pool (patched later)
		EMIT32(ctx, 0x58000071);  // LDR X17, #12
		EMIT32(ctx, 0xD63F0220);  // BLR X17

		// Register literal position for patching
		jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
		j->pos = BUF_POS() + 4;   // Position of the 8-byte literal (after B instruction)
		j->target = findex;
		j->next = ctx->calls;
		ctx->calls = j;

		EMIT32(ctx, 0x14000003);  // B #12 (skip 3 instructions = 12 bytes)
		EMIT32(ctx, 0);           // Low 32 bits placeholder
		EMIT32(ctx, 0);           // High 32 bits placeholder

		// Clean up stack if we allocated space for args
		if (stack_space > 0) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
		}
	}
}

/*
 * Call a HashLink function (native or JIT-compiled)
 * For OCall0-OCall4, OCallN
 */
static void op_call_hl(jit_ctx *ctx, vreg *dst, int findex, vreg **args, int nargs) {
	// Spill all caller-saved registers BEFORE the call
	// This must happen before prepare_call_args to save values that might be clobbered
	spill_regs(ctx);

	// Prepare arguments
	int stack_space = prepare_call_args(ctx, NULL, args, nargs, false);

	// Check if this is a native function or JIT function
	int fid = findex < 0 ? -1 : ctx->m->functions_indexes[findex];
	bool isNative = fid >= ctx->m->code->nfunctions;

	if (fid < 0) {
		// Invalid function index
		jit_error("Invalid function index");
	} else if (isNative) {
		// Native function - address is already resolved, call directly
		call_native(ctx, ctx->m->functions_ptrs[findex], stack_space);
	} else {
		// JIT function - use indirect call via literal pool (patched later)
		// During JIT compilation, functions_ptrs contains CODE OFFSETS.
		// The conversion to absolute addresses happens in hl_jit_code.
		//
		// Sequence:
		//   LDR X17, #12     ; load target address from PC+12
		//   BLR X17          ; call
		//   B #12            ; skip over the literal
		//   .quad addr       ; 8-byte address placeholder (patched later)

		EMIT32(ctx, 0x58000071);  // LDR X17, #12
		EMIT32(ctx, 0xD63F0220);  // BLR X17

		// Register literal position for patching
		jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
		j->pos = BUF_POS() + 4;   // Position of the 8-byte literal (after B instruction)
		j->target = findex;
		j->next = ctx->calls;
		ctx->calls = j;

		EMIT32(ctx, 0x14000003);  // B #12 (skip 3 instructions = 12 bytes)
		EMIT32(ctx, 0);           // Low 32 bits placeholder
		EMIT32(ctx, 0);           // High 32 bits placeholder

		// Clean up stack if we allocated space for args
		if (stack_space > 0) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, stack_space, SP_REG, SP_REG);
		}
	}

	// Note: spill_regs was already called before prepare_call_args

	// Store return value if needed
	if (dst && dst->t->kind != HVOID) {
		// Always store to stack first (source of truth for later loads)
		if (IS_FLOAT(dst)) {
			str_stack_fp(ctx, V0, dst->stackPos, dst->size);
		} else {
			str_stack(ctx, X0, dst->stackPos, dst->size);
		}

		// Also keep in a register if allocated to a different one
		preg *p = alloc_dst(ctx, dst);
		if (IS_FLOAT(dst)) {
			if (p->kind == RFPU && (Arm64FpReg)p->id != V0) {
				fmov_reg_reg(ctx, (Arm64FpReg)p->id, V0, dst->size);
			}
		} else {
			if (p->kind == RCPU && (Arm64Reg)p->id != X0) {
				mov_reg_reg(ctx, (Arm64Reg)p->id, X0, dst->size);
			}
		}
	}
}

// ============================================================================
// C↔HL Trampolines
// ============================================================================

static void *call_jit_c2hl = NULL;
static void *call_jit_hl2c = NULL;

// Maximum args for dynamic calls
#define MAX_ARGS 64

/**
 * Wrapper function for HL->C calls - unpacks arguments and calls the wrapped function.
 * Called from jit_hl2c trampoline.
 */
static vdynamic *jit_wrapper_call(vclosure_wrapper *c, char *stack_args, void **regs) {
	vdynamic *args[MAX_ARGS];
	int i;
	int nargs = c->cl.t->fun->nargs;
	int nextCpu = 1;  // Skip X0 which holds the closure pointer
	int nextFpu = 0;

	if (nargs > MAX_ARGS)
		hl_error("Too many arguments for wrapped call");

	for (i = 0; i < nargs; i++) {
		hl_type *t = c->cl.t->fun->args[i];

		if (t->kind == HF32 || t->kind == HF64) {
			// Float argument
			if (nextFpu < CALL_NREGS) {
				// In FP register - regs[CALL_NREGS + fpu_index]
				args[i] = hl_make_dyn(regs + CALL_NREGS + nextFpu, &hlt_f64);
				nextFpu++;
			} else {
				// On stack
				args[i] = hl_make_dyn(stack_args, &hlt_f64);
				stack_args += 8;
			}
		} else {
			// Integer/pointer argument
			if (nextCpu < CALL_NREGS) {
				// In CPU register
				if (hl_is_dynamic(t)) {
					args[i] = *(vdynamic**)(regs + nextCpu);
				} else {
					args[i] = hl_make_dyn(regs + nextCpu, t);
				}
				nextCpu++;
			} else {
				// On stack
				if (hl_is_dynamic(t)) {
					args[i] = *(vdynamic**)stack_args;
				} else {
					args[i] = hl_make_dyn(stack_args, t);
				}
				stack_args += 8;
			}
		}
	}
	return hl_dyn_call(c->wrappedFun, args, nargs);
}

/**
 * Wrapper for pointer-returning HL->C calls
 */
static void *jit_wrapper_ptr(vclosure_wrapper *c, char *stack_args, void **regs) {
	vdynamic *ret = jit_wrapper_call(c, stack_args, regs);
	hl_type *tret = c->cl.t->fun->ret;
	switch (tret->kind) {
	case HVOID:
		return NULL;
	case HUI8:
	case HUI16:
	case HI32:
	case HBOOL:
		return (void*)(int_val)hl_dyn_casti(&ret, &hlt_dyn, tret);
	case HI64:
	case HGUID:
		return (void*)(int_val)hl_dyn_casti64(&ret, &hlt_dyn);
	default:
		return hl_dyn_castp(&ret, &hlt_dyn, tret);
	}
}

/**
 * Wrapper for float-returning HL->C calls
 */
static double jit_wrapper_d(vclosure_wrapper *c, char *stack_args, void **regs) {
	vdynamic *ret = jit_wrapper_call(c, stack_args, regs);
	return hl_dyn_castd(&ret, &hlt_dyn);
}

/**
 * Select which register to use for an argument based on type and position.
 * Returns register ID or -1 if should go on stack.
 */
static int select_call_reg_c2hl(int *nextCpu, int *nextFpu, hl_type *t) {
	if (t->kind == HF32 || t->kind == HF64) {
		if (*nextFpu < CALL_NREGS)
			return RCPU_COUNT + (*nextFpu)++;  // FPU register
		return -1;  // Stack
	} else {
		if (*nextCpu < CALL_NREGS)
			return (*nextCpu)++;  // CPU register
		return -1;  // Stack
	}
}

/**
 * Get the stack size for a type
 */
static int stack_size_c2hl(hl_type *t) {
	switch (t->kind) {
	case HUI8:
	case HBOOL:
		return 1;
	case HUI16:
		return 2;
	case HI32:
	case HF32:
		return 4;
	default:
		return 8;
	}
}

/**
 * Callback function that prepares arguments and calls the JIT trampoline.
 * Called from C code to invoke JIT-compiled functions.
 */
static void *callback_c2hl(void *_f, hl_type *t, void **args, vdynamic *ret) {
	void **f = (void**)_f;
	// Stack layout:
	// [0..size) = stack args (pushed in reverse)
	// [size..size+CALL_NREGS*8) = integer register args (X0-X7)
	// [size+CALL_NREGS*8..size+CALL_NREGS*16) = FP register args (V0-V7)
	unsigned char stack[MAX_ARGS * 16];
	int nextCpu = 0, nextFpu = 0;
	int mappedRegs[MAX_ARGS];

	// Zero-initialize the stack to avoid passing garbage to unused registers
	// The jit_c2hl trampoline loads ALL 8 int + 8 FP registers unconditionally
	memset(stack, 0, sizeof(stack));

	if (t->fun->nargs > MAX_ARGS)
		hl_error("Too many arguments for dynamic call");

	// First pass: determine register assignments and stack size
	int i, size = 0;
	for (i = 0; i < t->fun->nargs; i++) {
		hl_type *at = t->fun->args[i];
		int creg = select_call_reg_c2hl(&nextCpu, &nextFpu, at);
		mappedRegs[i] = creg;
		if (creg < 0) {
			int tsize = stack_size_c2hl(at);
			if (tsize < 8) tsize = 8;  // Align to 8 bytes on stack
			size += tsize;
		}
	}

	// Align stack size to 16 bytes
	int pad = (-size) & 15;
	size += pad;

	// Second pass: copy arguments to appropriate locations
	int pos = 0;
	for (i = 0; i < t->fun->nargs; i++) {
		hl_type *at = t->fun->args[i];
		void *v = args[i];
		int creg = mappedRegs[i];
		void *store;

		if (creg >= 0) {
			if (creg >= RCPU_COUNT) {
				// FP register - stored after integer registers
				store = stack + size + CALL_NREGS * 8 + (creg - RCPU_COUNT) * 8;
			} else {
				// Integer register
				store = stack + size + creg * 8;
			}
			switch (at->kind) {
			case HBOOL:
			case HUI8:
				*(int64*)store = *(unsigned char*)v;
				break;
			case HUI16:
				*(int64*)store = *(unsigned short*)v;
				break;
			case HI32:
				*(int64*)store = *(int*)v;
				break;
			case HF32:
				*(double*)store = *(float*)v;
				break;
			case HF64:
				*(double*)store = *(double*)v;
				break;
			case HI64:
			case HGUID:
				*(int64*)store = *(int64*)v;
				break;
			default:
				*(void**)store = v;
				break;
			}
		} else {
			// Stack argument
			store = stack + pos;
			int tsize = 8;
			switch (at->kind) {
			case HBOOL:
			case HUI8:
				*(int64*)store = *(unsigned char*)v;
				break;
			case HUI16:
				*(int64*)store = *(unsigned short*)v;
				break;
			case HI32:
			case HF32:
				*(int64*)store = *(int*)v;
				break;
			case HF64:
				*(double*)store = *(double*)v;
				break;
			case HI64:
			case HGUID:
				*(int64*)store = *(int64*)v;
				break;
			default:
				*(void**)store = v;
				break;
			}
			pos += tsize;
		}
	}

	pos += pad;
	pos >>= 3;  // Convert to 64-bit units

	// Call the trampoline with: function pointer, reg args pointer, stack args end
	switch (t->fun->ret->kind) {
	case HUI8:
	case HUI16:
	case HI32:
	case HBOOL:
		ret->v.i = ((int (*)(void *, void *, void *))call_jit_c2hl)(*f, (void**)stack + pos, stack);
		return &ret->v.i;
	case HI64:
	case HGUID:
		ret->v.i64 = ((int64 (*)(void *, void *, void *))call_jit_c2hl)(*f, (void**)stack + pos, stack);
		return &ret->v.i64;
	case HF32:
		ret->v.f = ((float (*)(void *, void *, void *))call_jit_c2hl)(*f, (void**)stack + pos, stack);
		return &ret->v.f;
	case HF64:
		ret->v.d = ((double (*)(void *, void *, void *))call_jit_c2hl)(*f, (void**)stack + pos, stack);
		return &ret->v.d;
	default:
		return ((void *(*)(void *, void *, void *))call_jit_c2hl)(*f, (void**)stack + pos, stack);
	}
}

/**
 * Generate the HL-to-C trampoline.
 * Called from C code with a vclosure_wrapper* in X0 and native args in X1-X7, V0-V7.
 * Saves registers and calls jit_wrapper_ptr or jit_wrapper_d based on return type.
 */
static void jit_hl2c(jit_ctx *ctx) {
	hl_type_fun *ft = NULL;

	// Function prologue - save frame
	// STP X29, X30, [SP, #-16]!
	encode_ldp_stp(ctx, 0x02, 0, 0x03, -2, LR, SP_REG, FP);
	// MOV X29, SP
	mov_reg_reg(ctx, FP, SP_REG, true);

	// Allocate space for saved registers: 8 CPU regs + 8 FP regs = 16 * 8 = 128 bytes
	// SUB SP, SP, #128
	encode_add_sub_imm(ctx, 1, 1, 0, 0, 128, SP_REG, SP_REG);

	// Trampoline marker: MOV W17, #0xE001 (HL2C trampoline)
	EMIT32(ctx, 0x52800011 | (0xE001 << 5));

	// Save integer argument registers X0-X7 at [SP, #0..63]
	for (int i = 0; i < CALL_NREGS; i++) {
		encode_ldr_str_imm(ctx, 0x03, 0, 0x00, i, SP_REG, i);  // STR Xi, [SP, #i*8]
	}

	// Save FP argument registers V0-V7 at [SP, #64..127]
	for (int i = 0; i < CALL_NREGS; i++) {
		encode_ldr_str_imm(ctx, 0x03, 1, 0x00, 8 + i, SP_REG, i);  // STR Di, [SP, #(8+i)*8]
	}

	// X0 = closure pointer (vclosure_wrapper*)
	// Check return type: closure->t->fun->ret->kind
	// X9 = X0->t
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, X0, X9);
	// X9 = X9->fun (hl_type->fun is at offset 8 on 64-bit)
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 1, X9, X9);
	// X9 = X9->ret (hl_type_fun->ret offset)
	int ret_offset = (int)(int_val)&ft->ret;
	if (ret_offset < 4096 && (ret_offset % 8) == 0) {
		encode_ldr_str_imm(ctx, 0x03, 0, 0x01, ret_offset / 8, X9, X9);
	} else {
		load_immediate(ctx, ret_offset, RTMP, true);
		encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X9, X9);
	}
	// W9 = X9->kind (hl_type->kind is at offset 0, 32-bit)
	encode_ldr_str_imm(ctx, 0x02, 0, 0x01, 0, X9, X9);

	// Compare with HF64 and HF32
	// CMP W9, #HF64
	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF64, X9, XZR);
	int jfloat1 = BUF_POS();
	encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ float_path

	// CMP W9, #HF32
	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF32, X9, XZR);
	int jfloat2 = BUF_POS();
	encode_branch_cond(ctx, 0, COND_EQ);  // B.EQ float_path

	// Integer/pointer path: call jit_wrapper_ptr(closure, stack_args, regs)
	// X0 = closure (reload from saved regs)
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, SP_REG, X0);
	// X1 = stack_args (FP + 16 is return address area, args start after saved frame)
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, FP, X1);
	// X2 = regs pointer (SP)
	mov_reg_reg(ctx, X2, SP_REG, true);

	load_immediate(ctx, (int64_t)jit_wrapper_ptr, RTMP, true);
	EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

	// Result in X0, jump to exit
	int jexit = BUF_POS();
	encode_branch_uncond(ctx, 0);  // B exit

	// Float path
	int float_pos = BUF_POS();
	patch_jump(ctx, jfloat1, float_pos);
	patch_jump(ctx, jfloat2, float_pos);

	// X0 = closure (reload)
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, SP_REG, X0);
	// X1 = stack_args
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, FP, X1);
	// X2 = regs pointer
	mov_reg_reg(ctx, X2, SP_REG, true);

	load_immediate(ctx, (int64_t)jit_wrapper_d, RTMP, true);
	EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
	// Result in V0

	// Exit path
	int exit_pos = BUF_POS();
	patch_jump(ctx, jexit, exit_pos);

	// Restore frame and return
	// MOV SP, X29
	mov_reg_reg(ctx, SP_REG, FP, true);
	// LDP X29, X30, [SP], #16
	encode_ldp_stp(ctx, 0x02, 0, 0x01, 2, LR, SP_REG, FP);
	// RET
	encode_branch_reg(ctx, 0x02, LR);
}

/**
 * Generate the C-to-HL trampoline.
 * Input: X0 = function pointer, X1 = reg args pointer, X2 = stack args end
 * The trampoline loads arguments from the prepared stack and calls the function.
 */
static void jit_c2hl(jit_ctx *ctx) {
	// Save callee-saved registers and set up frame
	// STP X29, X30, [SP, #-16]!
	encode_ldp_stp(ctx, 0x02, 0, 0x03, -2, LR, SP_REG, FP);
	// MOV X29, SP
	mov_reg_reg(ctx, FP, SP_REG, true);

	// Trampoline marker: MOV W17, #0xE002 (C2HL trampoline)
	EMIT32(ctx, 0x52800011 | (0xE002 << 5));

	// Save function pointer to X9 (caller-saved, will survive loads)
	// MOV X9, X0
	mov_reg_reg(ctx, X9, X0, true);

	// Save stack args pointers to X10, X11
	// MOV X10, X1  (reg args pointer)
	// MOV X11, X2  (stack args end)
	mov_reg_reg(ctx, X10, X1, true);
	mov_reg_reg(ctx, X11, X2, true);

	// Load integer register arguments X0-X7 from [X10]
	for (int i = 0; i < CALL_NREGS; i++) {
		// LDR Xi, [X10, #i*8]
		encode_ldr_str_imm(ctx, 0x03, 0, 0x01, i, X10, CALL_REGS[i]);
	}

	// Load FP register arguments V0-V7 from [X10 + CALL_NREGS*8]
	for (int i = 0; i < CALL_NREGS; i++) {
		// LDR Di, [X10, #(CALL_NREGS + i)*8]
		// Using 64-bit FP load: size=11, opc=01
		EMIT32(ctx,0xFD400000 | (((CALL_NREGS + i) & 0x1FF) << 10) | (X10 << 5) | FP_CALL_REGS[i]);
	}

	// Push stack args: loop from X11 to X10, pushing each 8-byte value
	// Calculate how many stack args: (X10 - X11) / 8
	// Compare X10 and X11
	int loop_start = BUF_POS();
	// CMP X10, X11
	encode_add_sub_reg(ctx, 1, 1, 1, 0, X11, 0, X10, XZR);

	// B.EQ done (if X10 == X11, no more stack args)
	int beq_pos = BUF_POS();
	EMIT32(ctx,0x54000000 | (COND_EQ & 0xF));  // B.EQ (will patch)

	// SUB X10, X10, #8
	encode_add_sub_imm(ctx, 1, 1, 0, 0, 8, X10, X10);

	// LDR X12, [X10]
	encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, X10, X12);

	// STR X12, [SP, #-16]! (push with pre-decrement, keeping 16-byte alignment)
	// We'll push pairs to maintain alignment - but for simplicity, push 16 at a time
	// SUB SP, SP, #16
	encode_add_sub_imm(ctx, 1, 1, 0, 0, 16, SP_REG, SP_REG);
	// STR X12, [SP]
	encode_ldr_str_imm(ctx, 0x03, 0, 0x00, 0, SP_REG, X12);

	// B loop_start
	int b_offset = (loop_start - BUF_POS()) / 4;
	EMIT32(ctx,0x14000000 | (b_offset & 0x3FFFFFF));

	// Patch the B.EQ to jump here
	int done_pos = BUF_POS();
	int beq_offset = (done_pos - beq_pos) / 4;
	ctx->buf.w = (unsigned int*)(ctx->startBuf + beq_pos);
	EMIT32(ctx,0x54000000 | ((beq_offset & 0x7FFFF) << 5) | (COND_EQ & 0xF));
	ctx->buf.w = (unsigned int*)(ctx->startBuf + done_pos);

	// Call the function: BLR X9
	EMIT32(ctx,0xD63F0000 | (X9 << 5));

	// Restore frame and return
	// MOV SP, X29
	mov_reg_reg(ctx, SP_REG, FP, true);
	// LDP X29, X30, [SP], #16
	encode_ldp_stp(ctx, 0x02, 0, 0x01, 2, LR, SP_REG, FP);
	// RET
	encode_branch_reg(ctx, 0x02, LR);
}

/**
 * Get wrapper function for HL-to-C calls.
 * This is used for callbacks from C code back into HashLink.
 * Returns the jit_hl2c trampoline address.
 */
static void *get_wrapper(hl_type *t) {
	return call_jit_hl2c;
}

// ============================================================================
// JIT API Implementation
// ============================================================================

// Forward declaration
static void hl_jit_init_module(jit_ctx *ctx, hl_module *m);

jit_ctx *hl_jit_alloc() {
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	if (ctx == NULL)
		return NULL;
	memset(ctx, 0, sizeof(jit_ctx));
	return ctx;
}

void hl_jit_free(jit_ctx *ctx, h_bool can_reset) {
	if (ctx == NULL || ctx->freed)
		return;

	// Mark as freed immediately to prevent double-free
	ctx->freed = true;

	// Free and NULL each pointer atomically to prevent use-after-free window
	if (ctx->startBuf) {
		void *tmp = ctx->startBuf;
		ctx->startBuf = NULL;
		free(tmp);
	}
	if (ctx->vregs) {
		void *tmp = ctx->vregs;
		ctx->vregs = NULL;
		free(tmp);
	}
	if (ctx->opsPos) {
		void *tmp = ctx->opsPos;
		ctx->opsPos = NULL;
		free(tmp);
	}
	if (ctx->debug) {
		void *tmp = ctx->debug;
		ctx->debug = NULL;
		free(tmp);
	}

	// Clear remaining fields
	ctx->buf.b = NULL;
	ctx->bufSize = 0;
	ctx->maxRegs = 0;
	ctx->maxOps = 0;
	ctx->calls = NULL;
	// closure_list is managed by GC (allocated in falloc/galloc)

	// Free allocators before freeing ctx
	hl_free(&ctx->falloc);
	hl_free(&ctx->galloc);

	if (!can_reset) {
#ifdef GC_DEBUG
		// Poison memory to catch use-after-free in debug builds
		memset(ctx, 0xDD, sizeof(jit_ctx));
#endif
		free(ctx);
	}
}

void hl_jit_reset(jit_ctx *ctx, hl_module *m) {
	ctx->freed = false;  // Allow reuse after reset
	ctx->debug = NULL;
	hl_jit_init_module(ctx, m);
}

/**
 * Build a JIT helper function, ensuring buffer is allocated.
 * Returns the position in the buffer where the function starts.
 */
static int jit_build(jit_ctx *ctx, void (*fbuild)(jit_ctx *)) {
	int pos;
	jit_buf(ctx);  // Ensure buffer is allocated
	pos = BUF_POS();
	fbuild(ctx);
	return pos;
}

/**
 * Initialize module-specific data in JIT context.
 */
static void hl_jit_init_module(jit_ctx *ctx, hl_module *m) {
	int i;
	ctx->m = m;
	ctx->closure_list = NULL;

	// Allocate debug info array if bytecode has debug info
	if (m->code->hasdebug && m->code->nfunctions > 0) {
		ctx->debug = (hl_debug_infos*)malloc(sizeof(hl_debug_infos) * m->code->nfunctions);
		if (ctx->debug)
			memset(ctx->debug, 0, sizeof(hl_debug_infos) * m->code->nfunctions);
	}

	// Store float constants in the code buffer (like x86 does)
	for (i = 0; i < m->code->nfloats; i++) {
		jit_buf(ctx);
		*ctx->buf.d++ = m->code->floats[i];
	}
}

void hl_jit_init(jit_ctx *ctx, hl_module *m) {
	hl_jit_init_module(ctx, m);

	// Generate C↔HL trampolines
	ctx->c2hl = jit_build(ctx, jit_c2hl);
	ctx->hl2c = jit_build(ctx, jit_hl2c);
}

/**
 * Allocate a static closure object.
 * For native functions, the function pointer is set immediately.
 * For JIT functions, the function pointer is stored temporarily as the findex
 * and the closure is added to closure_list for later patching.
 */
static vclosure *alloc_static_closure(jit_ctx *ctx, int fid) {
	hl_module *m = ctx->m;
	vclosure *c = hl_malloc(&m->ctx.alloc, sizeof(vclosure));
	int fidx = m->functions_indexes[fid];
	c->hasValue = 0;
	if (fidx >= m->code->nfunctions) {
		// Native function - pointer is already resolved
		c->t = m->code->natives[fidx - m->code->nfunctions].t;
		c->fun = m->functions_ptrs[fid];
		c->value = NULL;
	} else {
		// JIT function - store fid temporarily, add to closure_list for patching
		c->t = m->code->functions[fidx].type;
		c->fun = (void*)(int_val)fid;
		c->value = ctx->closure_list;
		ctx->closure_list = c;
	}
	return c;
}

int hl_jit_function(jit_ctx *ctx, hl_module *m, hl_function *f) {
	int i, size = 0, opCount;
	int codePos = BUF_POS();
	int nargs = f->type->fun->nargs;
	unsigned short *debug16 = NULL;
	int *debug32 = NULL;

	ctx->f = f;
	ctx->m = m;
	ctx->allocOffset = 0;

	// Allocate virtual register array if needed
	if (f->nregs > ctx->maxRegs) {
		free(ctx->vregs);
		ctx->vregs = (vreg*)calloc(f->nregs + 1, sizeof(vreg));
		if (ctx->vregs == NULL) {
			ctx->maxRegs = 0;
			return -1;
		}
		ctx->maxRegs = f->nregs;
	}

	// Allocate opcode position array if needed
	if (f->nops > ctx->maxOps) {
		free(ctx->opsPos);
		ctx->opsPos = (int*)malloc(sizeof(int) * (f->nops + 1));
		if (ctx->opsPos == NULL) {
			ctx->maxOps = 0;
			return -1;
		}
		ctx->maxOps = f->nops;
	}

	memset(ctx->opsPos, 0, (f->nops + 1) * sizeof(int));

	// Clear/initialize physical registers
	for (i = 0; i < RCPU_COUNT; i++) {
		preg *p = &ctx->pregs[i];
		p->kind = RCPU;
		p->id = i;
		p->holds = NULL;
		p->lock = 0;
	}
	for (i = 0; i < RFPU_COUNT; i++) {
		preg *p = &ctx->pregs[RCPU_COUNT + i];
		p->kind = RFPU;
		p->id = i;
		p->holds = NULL;
		p->lock = 0;
	}

	// Initialize virtual registers
	for (i = 0; i < f->nregs; i++) {
		vreg *r = R(i);
		r->t = f->regs[i];
		r->size = hl_type_size(r->t);
        r->stackPos = 0;
		r->current = NULL;
		r->stack.holds = NULL;
		r->stack.id = i;
		r->stack.kind = RSTACK;
		r->stack.lock = 0;
	}

	// Calculate stack layout
	// Arguments: first 8 integer args in X0-X7, first 8 FP args in V0-V7
	// Additional args on stack
	size = 0;
	int argsSize = 0;
	int int_arg_count = 0;
	int fp_arg_count = 0;

	for (i = 0; i < nargs; i++) {
		vreg *r = R(i);
		bool is_fp = IS_FLOAT(r);
		int *arg_count = is_fp ? &fp_arg_count : &int_arg_count;

		if (*arg_count < CALL_NREGS) {
			// Argument is in register - allocate stack space for it
			size += r->size;
			size += hl_pad_size(size, r->t);
			r->stackPos = -size;
			(*arg_count)++;
		} else {
			// Argument is on stack (caller's frame)
			// +96 for saved callee-saved (64 bytes) + RTMP/RTMP2 (16 bytes) + FP/LR (16 bytes)
			// Each stack arg occupies 8 bytes (matching caller's prepare_call_args)
			r->stackPos = argsSize + 96;
			argsSize += 8;
		}
	}

	// Local variables
	for (i = nargs; i < f->nregs; i++) {
		vreg *r = R(i);
		size += r->size;
		size += hl_pad_size(size, r->t);
		r->stackPos = -size;
	}

	// Align stack to 16 bytes
	size += (-size) & 15;
	ctx->totalRegsSize = size;

	jit_buf(ctx);
	ctx->functionPos = BUF_POS();
	ctx->currentPos = 1;

	// Initialize Phase 2 callee-saved tracking
	ctx->callee_saved_used = 0;
	memset(ctx->stp_positions, 0, sizeof(ctx->stp_positions));
	memset(ctx->ldp_positions, 0, sizeof(ctx->ldp_positions));

	// Function prologue - offset-based for selective NOP patching (Phase 2)
	// Reserve space for callee-saved (64 bytes) + RTMP/RTMP2 (16 bytes) + FP/LR (16 bytes) = 96 bytes
	encode_add_sub_imm(ctx, 1, 1, 0, 0, 96, SP_REG, SP_REG);  // SUB SP, SP, #96

	// Save RTMP/RTMP2 (X27, X28) - NOT NOPpable as they are used internally by JIT
	stp_offset(ctx, RTMP, RTMP2, SP_REG, 80); // STP X27, X28, [SP, #80]

	// Save callee-saved at fixed offsets (NOPpable) - positions recorded for backpatching
	ctx->stp_positions[0] = BUF_POS();
	stp_offset(ctx, X25, X26, SP_REG, 64);  // STP X25, X26, [SP, #64]

	ctx->stp_positions[1] = BUF_POS();
	stp_offset(ctx, X23, X24, SP_REG, 48);  // STP X23, X24, [SP, #48]

	ctx->stp_positions[2] = BUF_POS();
	stp_offset(ctx, X21, X22, SP_REG, 32);  // STP X21, X22, [SP, #32]

	ctx->stp_positions[3] = BUF_POS();
	stp_offset(ctx, X19, X20, SP_REG, 16);  // STP X19, X20, [SP, #16]

	// Save FP/LR at bottom (NOT NOPpable - always needed)
	stp_offset(ctx, FP, LR, SP_REG, 0);     // STP X29, X30, [SP, #0]

	// MOV X29, SP  ; Set frame pointer (points to saved FP/LR)
	mov_reg_reg(ctx, FP, SP_REG, true);

	// SUB SP, SP, #size  ; Allocate stack space
	if (size > 0) {
		if (size < 4096) {
			encode_add_sub_imm(ctx, 1, 1, 0, 0, size, SP_REG, SP_REG);
		} else {
			// Large stack frame - use multiple instructions
			// Must use extended register form (UXTX) for SP, not shifted register
			load_immediate(ctx, size, RTMP, true);
			encode_add_sub_ext(ctx, 1, 1, 0, RTMP, 3, 0, SP_REG, SP_REG);  // SUB SP, SP, RTMP, UXTX
		}
	}

	// Function marker: MOV W17, #(0xF000 | (findex & 0xFFF)) ; MOVK W17, #(findex >> 12), LSL #16
	// This encodes as 0xFnnnnnnn where nnnnnnn is the function index
	// Distinguishes from opcode markers which are smaller numbers
	{
		int findex = f->findex;
		int low12 = 0xF000 | (findex & 0xFFF);
		int high = (findex >> 12) & 0xFFFF;
		// MOV W17, #low12
		EMIT32(ctx, 0x52800011 | (low12 << 5));
		if (high != 0) {
			// MOVK W17, #high, LSL #16
			EMIT32(ctx, 0x72A00011 | (high << 5));
		}
	}

	// Store register arguments to their stack locations FIRST
	// (before we clobber the argument registers with zero-init)
	int_arg_count = 0;
	fp_arg_count = 0;
	for (i = 0; i < nargs && i < f->nregs; i++) {
		vreg *r = R(i);
		bool is_fp = IS_FLOAT(r);
		int *arg_count = is_fp ? &fp_arg_count : &int_arg_count;

		if (*arg_count < CALL_NREGS) {
			// This arg was in a register - store it to stack
			// Skip void arguments (size 0) - they don't need storage
			// but still consume a call register slot
			if (r->size > 0) {
				if (is_fp) {
					str_stack_fp(ctx, FP_CALL_REGS[fp_arg_count], r->stackPos, r->size);
				} else {
					str_stack(ctx, CALL_REGS[int_arg_count], r->stackPos, r->size);
				}
			}
			(*arg_count)++;
		}
	}

	// Zero-initialize local variables on stack (not arguments)
	// This ensures reading unassigned locals returns null/0
	if (f->nregs > nargs) {
		// Store zeros to each local variable slot using XZR
		for (i = nargs; i < f->nregs; i++) {
			vreg *r = R(i);
			if (r->size > 0 && r->stackPos < 0) {
				// Use str_stack with XZR as source - efficient and handles all offsets
				if (r->size != 1 && r->size != 2 && r->size != 4 && r->size != 8) {
					JIT_ASSERT(0);
				}
				str_stack(ctx, XZR, r->stackPos, r->size);
			}
		}
	}

	ctx->opsPos[0] = BUF_POS();

	// Initialize debug offset tracking
	if (ctx->m->code->hasdebug) {
		debug16 = (unsigned short*)malloc(sizeof(unsigned short) * (f->nops + 1));
		debug16[0] = (unsigned short)(BUF_POS() - codePos);
	}

	// Main opcode translation loop
	for (opCount = 0; opCount < f->nops; opCount++) {
		hl_opcode *o = f->ops + opCount;
		vreg *dst = R(o->p1);
		vreg *ra = R(o->p2);
		vreg *rb = R(o->p3);

		ctx->currentPos = opCount + 1;
		jit_buf(ctx);

		// Emit opcode marker for debugging: MOV W17, #(opcode | (opCount << 8))
		// W17 is IP1, a scratch register. This encodes both the opcode type and index.
		{
			int marker = (o->op & 0xFF) | ((opCount & 0xFF) << 8);
			EMIT32(ctx, 0x52800011 | ((marker & 0xFFFF) << 5));  // MOV W17, #marker
		}

		// Before a label (merge point), spill dirty registers for fallthrough path.
		// After spilling, update the label position so jumps bypass the spill code.
		// discard_regs() in op_label just clears bindings (no code).
		if (o->op == OLabel) {
			spill_regs(ctx);
			// Update label position AFTER spill - jumps should target here,
			// not before the spill (which is only for fallthrough path)
			ctx->opsPos[opCount] = BUF_POS();
		}

		// Emit code based on opcode
		switch (o->op) {
		case OMov:
		case OUnsafeCast:
			op_mov(ctx, dst, ra);
			break;

		case OInt:
			store_const(ctx, dst, m->code->ints[o->p2]);
			break;

		case OBool:
			store_const(ctx, dst, o->p2);
			break;

		case ONull:
			// Set register to NULL (0)
			store_const(ctx, dst, 0);
			break;

		case OFloat: {
			// Load float constant from module
			// Float constants are stored at the start of the code buffer (offset o->p2 * 8)
			double float_val = m->code->floats[o->p2];
			preg *dst_reg = alloc_fpu(ctx);

			if (float_val == 0.0) {
				// Zero out FP register: FMOV Dd, XZR
				// FMOV Dd, XZR: sf=1, S=0, type=01, rmode=00, opcode=000111, Rn=31, Rd
				EMIT32(ctx, (1 << 31) | (0 << 29) | (0x1E << 24) | (1 << 22) | (1 << 21) | (7 << 16) | (31 << 5) | dst_reg->id);
			} else {
				// Float constants are at the start of the code buffer
				// Calculate PC-relative offset from current position to the float data
				int float_offset = o->p2 * 8;  // Offset from start of code buffer
				int cur_pos = BUF_POS();       // Current position in code buffer
				int pc_offset = float_offset - cur_pos;  // PC-relative offset

				// LDR Dt, <label> - PC-relative load for 64-bit float
				// Encoding: opc=01 V=1 imm19 Rt
				// imm19 = offset / 4 (must be aligned)
				int imm19 = pc_offset / 4;
				if (imm19 >= -(1 << 18) && imm19 < (1 << 18)) {
					// LDR Dt, #imm19 - load 64-bit from PC + imm19*4
					EMIT32(ctx, (0x5C << 24) | ((imm19 & 0x7FFFF) << 5) | dst_reg->id);
				} else {
					// Offset too large for PC-relative load - use absolute address
					// Load the address of the float constant from the module's float array
					load_immediate(ctx, (int64_t)&m->code->floats[o->p2], RTMP, true);
					// LDR Dt, [Xn] - load 64-bit float from address in RTMP
					// Encoding: 11 111101 01 imm12 Rn Rt (size=11, opc=01 for 64-bit load)
					EMIT32(ctx, (0xFD4 << 20) | (0 << 10) | (RTMP << 5) | dst_reg->id);
				}

				// If destination is HF32, convert from double to float
				if (dst->t->kind == HF32) {
					// FCVT Sd, Dn - convert double to single
					// 0001 1110 0110 0010 0100 00nn nnnd dddd
					EMIT32(ctx, (0x1E624000) | (dst_reg->id << 5) | dst_reg->id);
				}
			}

			reg_bind(ctx, dst, dst_reg);
			mark_dirty(ctx, dst);
			break;
		}

		case ORet: {
			// Return from function - move return value to appropriate register
			if (dst->t->kind == HF32 || dst->t->kind == HF64) {
				// Float return in V0
				preg *p = fetch(ctx, dst);
				if (p->kind == RFPU && (Arm64FpReg)p->id != V0) {
					fmov_reg_reg(ctx, V0, (Arm64FpReg)p->id, dst->size);
				} else if (p->kind == RSTACK) {
					ldr_stack_fp(ctx, V0, dst->stackPos, dst->size);
				}
			} else {
				// Integer/pointer return in X0
				preg *p = fetch(ctx, dst);
				if (p->kind == RCPU && (Arm64Reg)p->id != X0) {
					mov_reg_reg(ctx, X0, (Arm64Reg)p->id, dst->size);
				} else if (p->kind == RSTACK) {
					ldr_stack(ctx, X0, dst->stackPos, dst->size);
				}
			}
			// Jump to main epilogue (at position f->nops) instead of inlining.
			// This ensures callee-saved register restores match the NOPped saves.
			jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
			j->pos = BUF_POS();
			j->target = f->nops;  // Main epilogue position
			j->next = ctx->jumps;
			ctx->jumps = j;
			// Emit placeholder branch (will be patched later)
			EMIT32(ctx, 0x14000000);  // B with offset 0
			break;
		}

		case OSetThis: {
			// Set field on 'this' object (R(0))
			// Use op_set_field to handle HPACKED fields correctly
			vreg *this_obj = R(0);
			op_set_field(ctx, this_obj, o->p1, ra);
			break;
		}

		// Arithmetic operations
		case OAdd:
		case OSub:
		case OMul:
		case OSDiv:
		case OUDiv:
		case OSMod:
		case OUMod:
		case OAnd:
		case OOr:
		case OXor:
		case OShl:
		case OSShr:
		case OUShr:
			op_binop(ctx, dst, ra, rb, o->op);
			break;

		case ONeg:
			op_neg(ctx, dst, ra);
			break;

		case ONot:
			op_not(ctx, dst, ra);
			break;

		case OIncr:
			op_incr(ctx, dst);
			break;

		case ODecr:
			op_decr(ctx, dst);
			break;

		// Type conversions
		case OToInt:
			op_toint(ctx, dst, ra);
			break;

		case OToSFloat:
			op_tosfloat(ctx, dst, ra);
			break;

		case OToUFloat:
			op_toufloat(ctx, dst, ra);
			break;

		case OToDyn:
			// Convert to dynamic type
			if (ra->t->kind == HBOOL) {
				// Boolean: call hl_alloc_dynbool(value)
				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// Load value from stack to X0 (first argument)
				ldr_stack(ctx, X0, ra->stackPos, ra->size);

				load_immediate(ctx, (int64_t)hl_alloc_dynbool, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				preg *p_dst = alloc_dst(ctx, dst);
				if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
					mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, true);
				} else if (p_dst->kind == RSTACK) {
					str_stack(ctx, X0, dst->stackPos, dst->size);
				}
			} else {
				int jump_skip = 0;

				// Spill caller-saved registers before any branch or call
				spill_regs(ctx);

				// If pointer type, check for NULL
				if (hl_is_ptr(ra->t)) {
					preg *r_val = fetch(ctx, ra);
					// CBZ - if NULL, skip allocation and copying
					jump_skip = BUF_POS();
					encode_cbz_cbnz(ctx, 1, 0, 0, (Arm64Reg)r_val->id);  // CBZ
				}

				// Call hl_alloc_dynamic(type)
				load_immediate(ctx, (int64_t)ra->t, X0, true);
				load_immediate(ctx, (int64_t)hl_alloc_dynamic, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Copy value to dynamic object at offset HDYN_VALUE
				// After native call, load directly from stack (don't use fetch!)
				// Result of hl_alloc_dynamic is in X0
				if (IS_FLOAT(ra)) {
					// Float: load from stack and store to [X0 + HDYN_VALUE]
					ldr_stack_fp(ctx, V16, ra->stackPos, ra->size);
					// STR Vn, [X0, #HDYN_VALUE]
					encode_ldr_str_imm(ctx, (ra->size == 8) ? 0x03 : 0x02, 1, 0x00,
					                   HDYN_VALUE / ((ra->size == 8) ? 8 : 4), X0, V16);
				} else {
					// Integer/pointer: load from stack and store to [X0 + HDYN_VALUE]
					ldr_stack(ctx, X16, ra->stackPos, ra->size);
					// STR Xn, [X0, #HDYN_VALUE]
					encode_ldr_str_imm(ctx, (ra->size == 8) ? 0x03 : 0x02, 0, 0x00,
					                   HDYN_VALUE / ((ra->size == 8) ? 8 : 4), X0, X16);
				}

				// Patch NULL skip if needed
				if (hl_is_ptr(ra->t)) {
					int pos_end = BUF_POS();
					patch_jump(ctx, jump_skip, pos_end);
				}

				// Store result
				preg *p_dst = alloc_dst(ctx, dst);
				if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
					mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, true);
				} else if (p_dst->kind == RSTACK) {
					str_stack(ctx, X0, dst->stackPos, dst->size);
				}
			}
			break;

		// Control flow operations
		case OLabel:
			op_label(ctx);
			break;

		case OThrow:
			// Throw exception: call hl_throw(exception)
			{
				// Spill registers before the call (ensures consistent stack state)
				spill_regs(ctx);

				// Get exception value
				vreg *exception = R(o->p1);

				// Handle HVOID (null/dynamic exception) specially
				if (exception->size == 0) {
					// X0 = NULL for HVOID exceptions
					load_immediate(ctx, 0, X0, true);
				} else {
					preg *r_exc = fetch(ctx, exception);

					// X0 = exception
					if (r_exc->kind == RCPU && (Arm64Reg)r_exc->id != X0) {
						mov_reg_reg(ctx, X0, (Arm64Reg)r_exc->id, true);
					} else if (r_exc->kind == RSTACK) {
						ldr_stack(ctx, X0, exception->stackPos, exception->size);
					}
					// If r_exc is already X0 (RCPU with id==0), nothing to do
				}

				// Call hl_throw - this function does not return
				load_immediate(ctx, (int64_t)hl_throw, RTMP, true);
				EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			}
			break;

		case OJTrue:
		case OJFalse:
		case OJNull:
		case OJNotNull:
			op_jcond(ctx, dst, o->op, (opCount + 1) + o->p2);
			break;

		case OJEq:
		case OJNotEq:
		case OJSLt:
		case OJSGte:
		case OJSLte:
		case OJSGt:
		case OJULt:
		case OJUGte:
		case OJNotLt:
		case OJNotGte:
			op_jump(ctx, dst, ra, o->op, (opCount + 1) + o->p3);
			break;

		case OJAlways:
			op_jalways(ctx, (opCount + 1) + o->p1);
			break;

		// Function calls
		case OCall0: {
			op_call_hl(ctx, dst, o->p2, NULL, 0);
			break;
		}

		case OCall1: {
			vreg *args[1] = { rb };
			op_call_hl(ctx, dst, o->p2, args, 1);
			break;
		}

		case OCall2: {
			// Note: o->extra is a pointer cast to int, not an array
			int arg1_idx = (int)(int_val)o->extra;
			vreg *args[2] = { rb, R(arg1_idx) };
			op_call_hl(ctx, dst, o->p2, args, 2);
			break;
		}

		case OCall3: {
			vreg *args[3] = { rb, R(o->extra[0]), R(o->extra[1]) };
			op_call_hl(ctx, dst, o->p2, args, 3);
			break;
		}

		case OCall4: {
			vreg *args[4] = { rb, R(o->extra[0]), R(o->extra[1]), R(o->extra[2]) };
			op_call_hl(ctx, dst, o->p2, args, 4);
			break;
		}

		case OCallN: {
			int nargs = o->p3;
			vreg **args = (vreg**)malloc(sizeof(vreg*) * nargs);
			for (int i = 0; i < nargs; i++) {
				args[i] = R(o->extra[i]);
			}
			op_call_hl(ctx, dst, o->p2, args, nargs);
			free(args);
			break;
		}

		// Memory operations with register offset
		// OGetI8/OGetI16/OGetMem: dst = *(type*)(ra + rb)
		//   p1 = dst, p2 = base (ra), p3 = offset (rb)
		// OSetI8/OSetI16/OSetMem: *(type*)(dst + ra) = rb
		//   p1 = base (dst), p2 = offset (ra), p3 = value (rb)
		case OGetI8:
			op_get_mem_reg(ctx, dst, ra, rb, 1);
			break;

		case OGetI16:
			op_get_mem_reg(ctx, dst, ra, rb, 2);
			break;

		case OGetMem:
			op_get_mem_reg(ctx, dst, ra, rb, dst->size);
			break;

		case OSetI8:
			op_set_mem_reg(ctx, dst, ra, rb, 1);
			break;

		case OSetI16:
			op_set_mem_reg(ctx, dst, ra, rb, 2);
			break;

		case OSetMem:
			op_set_mem_reg(ctx, dst, ra, rb, rb->size);
			break;

		// Field access
		case OField:
			switch (ra->t->kind) {
			case HOBJ:
			case HSTRUCT:
				op_field(ctx, dst, ra, o->p3);
				break;
			case HVIRTUAL:
				// if( hl_vfields(o)[f] ) r = *hl_vfields(o)[f]; else r = hl_dyn_get(o,hash,type)
				{
					// Spill dirty registers FIRST - before loading vfield pointer into RTMP.
					// str_stack can use RTMP for large stack offsets, which would clobber
					// the vfield pointer if we spill after loading it.
					spill_regs(ctx);

					preg *r_obj = fetch(ctx, ra);
					preg *r_vfield = alloc_dst(ctx, dst);
					int vfield_offset = sizeof(vvirtual) + HL_WSIZE * o->p3;
					Arm64Reg obj_r = (r_obj->kind == RCPU) ? r_obj->id : RTMP;

					// Load vfield pointer: RTMP = obj[vfield_offset]
					if (vfield_offset < 4096)
						encode_ldr_str_imm(ctx, 0x03, 0, 0x01, vfield_offset / 8, obj_r, RTMP);
					else {
						load_immediate(ctx, vfield_offset, RTMP2, true);
						encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP2, 0x03, 0, obj_r, RTMP);
					}

					// Test and branch if NULL
					int jnull = BUF_POS();
					encode_cbz_cbnz(ctx, 1, 0, 0, RTMP);  // CBZ -> dyn_get path

					// Has vfield: load from it
					if (IS_FLOAT(dst)) {
						// Float: load into FPU register
						Arm64FpReg dst_r = (r_vfield->kind == RFPU) ? (Arm64FpReg)r_vfield->id : V16;
						int size_bits = (dst->size == 8) ? 0x03 : 0x02;
						encode_ldr_str_imm(ctx, size_bits, 1, 0x01, 0, RTMP, dst_r);  // V=1 for FP
						str_stack_fp(ctx, dst_r, dst->stackPos, dst->size);
					} else {
						// Integer/pointer: load into CPU register
						Arm64Reg dst_r = (r_vfield->kind == RCPU) ? (Arm64Reg)r_vfield->id : RTMP2;
						// Size bits: 0x00=8-bit, 0x01=16-bit, 0x02=32-bit, 0x03=64-bit
						int size_bits = (dst->size == 1) ? 0x00 : (dst->size == 2) ? 0x01 : (dst->size == 4) ? 0x02 : 0x03;
						encode_ldr_str_imm(ctx, size_bits, 0, 0x01, 0, RTMP, dst_r);
						str_stack(ctx, dst_r, dst->stackPos, dst->size);
					}
					int jend = BUF_POS();
					encode_branch_uncond(ctx, 0);  // B end

					// NULL path: call dyn_get
					int null_pos = BUF_POS();
					// Spill caller-saved registers before the call
					spill_regs(ctx);
					// Load arguments (obj was spilled, load from stack)
					ldr_stack(ctx, X0, ra->stackPos, ra->size);
					load_immediate(ctx, (int64_t)ra->t->virt->fields[o->p3].hashed_name, X1, true);
					if (!IS_FLOAT(dst) && dst->t->kind != HI64)
						load_immediate(ctx, (int64_t)dst->t, X2, true);
					load_immediate(ctx, (int64_t)get_dynget(dst->t), RTMP, true);
					EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR
					// Result in X0 (or V0 for floats), store to dst
					if (IS_FLOAT(dst)) {
						str_stack_fp(ctx, V0, dst->stackPos, dst->size);
					} else {
						str_stack(ctx, X0, dst->stackPos, dst->size);
					}
					store_result(ctx, dst);

					patch_jump(ctx, jnull, null_pos);
					patch_jump(ctx, jend, BUF_POS());
				}
				break;
			default:
				printf("JIT Error: OField with unsupported type %d\n", ra->t->kind);
				break;
			}
			break;

		case OSetField:
			// OSetField: p1=object, p2=field_index (NOT a register!), p3=value
			// So we use dst=R(p1) for object, o->p2 directly for field index, rb=R(p3) for value
			switch (dst->t->kind) {
			case HOBJ:
			case HSTRUCT:
				op_set_field(ctx, dst, o->p2, rb);
				break;
			case HVIRTUAL:
				// if( hl_vfields(o)[f] ) *hl_vfields(o)[f] = v; else hl_dyn_set(o,hash,type,v)
				{
					// Spill dirty registers FIRST - before loading vfield pointer into RTMP.
					// str_stack can use RTMP for large stack offsets, which would clobber
					// the vfield pointer if we spill after loading it.
					spill_regs(ctx);

					// Now fetch operands and load vfield pointer
					preg *r_obj = fetch(ctx, dst);
					preg *r_val = fetch(ctx, rb);
					int vfield_offset = sizeof(vvirtual) + HL_WSIZE * o->p2;
					Arm64Reg obj_r = (r_obj->kind == RCPU) ? r_obj->id : RTMP2;

					// Load vfield pointer
					if (vfield_offset < 4096)
						encode_ldr_str_imm(ctx, 0x03, 0, 0x01, vfield_offset / 8, obj_r, RTMP);
					else {
						load_immediate(ctx, vfield_offset, RTMP, true);
						encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, obj_r, RTMP);
					}

					// Test and branch
					int jnull = BUF_POS();
					encode_cbz_cbnz(ctx, 1, 0, 0, RTMP);

					// Has vfield: store to it
					if (IS_FLOAT(rb)) {
						// Float: store from FPU register
						Arm64FpReg val_r = (r_val->kind == RFPU) ? (Arm64FpReg)r_val->id : V16;
						if (r_val->kind != RFPU)
							ldr_stack_fp(ctx, val_r, rb->stackPos, rb->size);
						int size_bits = (rb->size == 8) ? 0x03 : 0x02;
						encode_ldr_str_imm(ctx, size_bits, 1, 0x00, 0, RTMP, val_r);  // V=1 for FP store
					} else {
						// Integer/pointer: store from CPU register
						Arm64Reg val_r = (r_val->kind == RCPU) ? r_val->id : RTMP2;
						if (r_val->kind != RCPU)
							ldr_stack(ctx, val_r, rb->stackPos, rb->size);
						// Size bits: 0x00=8-bit, 0x01=16-bit, 0x02=32-bit, 0x03=64-bit
						int size_bits = (rb->size == 1) ? 0x00 : (rb->size == 2) ? 0x01 : (rb->size == 4) ? 0x02 : 0x03;
						encode_ldr_str_imm(ctx, size_bits, 0, 0x00, 0, RTMP, val_r);  // STR with correct size
					}
					int jend = BUF_POS();
					encode_branch_uncond(ctx, 0);

					// NULL path: call dyn_set
					int null_pos = BUF_POS();
					// Spill caller-saved registers before the call
					spill_regs(ctx);
					// Load arguments from stack
					ldr_stack(ctx, X0, dst->stackPos, dst->size);  // obj
					load_immediate(ctx, (int64_t)dst->t->virt->fields[o->p2].hashed_name, X1, true);
					
					if (IS_FLOAT(rb)) {
						ldr_stack_fp(ctx, V0, rb->stackPos, rb->size);
					} else if (rb->t->kind == HI64) {
						// hl_dyn_seti64(obj, field, value) - value in X2
						ldr_stack(ctx, X2, rb->stackPos, rb->size);
					} else {
						// hl_dyn_setp/i(obj, field, type, value) - type in X2, value in X3
						load_immediate(ctx, (int64_t)rb->t, X2, true);
						ldr_stack(ctx, X3, rb->stackPos, rb->size);
					}
					load_immediate(ctx, (int64_t)get_dynset(rb->t), RTMP, true);
					EMIT32(ctx,(0xD63F0000) | (RTMP << 5));

					patch_jump(ctx, jnull, null_pos);
					patch_jump(ctx, jend, BUF_POS());
				}
				break;
			default:
				printf("JIT Error: OSetField with unsupported type %d\n", dst->t->kind);
				break;
			}
			break;

		// Array operations
		case OGetArray:
			op_get_array(ctx, dst, ra, rb);
			break;

		case OSetArray:
			op_set_array(ctx, dst, ra, rb);
			break;

		// Global variables
		case OGetGlobal:
			op_get_global(ctx, dst, o->p2);
			break;

		case OSetGlobal:
			op_set_global(ctx, o->p1, ra);
			break;

		// Reference operations
		case ORef:
			op_ref(ctx, dst, ra);
			break;

		case OUnref:
			op_unref(ctx, dst, ra);
			break;

		case OSetref:
			op_setref(ctx, dst, ra);
			break;

		// Type operations
		case OType:
			// Load constant type pointer from types array: dst = &m->code->types[p2]
			{
				hl_type *type_ptr = m->code->types + o->p2;
				preg *r_dst = alloc_dst(ctx, dst);
				Arm64Reg dst_r = (r_dst->kind == RCPU) ? (Arm64Reg)r_dst->id : RTMP;

				// Load address of type
				load_immediate(ctx, (int64_t)type_ptr, dst_r, true);

				if (r_dst->kind == RSTACK) {
					str_stack(ctx, dst_r, dst->stackPos, dst->size);
				} else {
					mark_dirty(ctx, dst);
				}
			}
			break;

		case OGetThis:
			// Load field from "this" object: dst = R(0).fields[p2]
			op_get_this(ctx, dst, o->p2);
			break;

		case OSafeCast:
			op_safe_cast(ctx, dst, ra, (hl_type*)(uintptr_t)o->p3);
			break;

		case ONullCheck:
			{
				int hashed_name = 0;
				// Look ahead to find the field access
				hl_opcode *next = f->ops + opCount + 1;
				// Skip const and basic operations
				while( (next < f->ops + f->nops - 1) && (next->op >= OInt && next->op <= ODecr) ) {
					next++;
				}
				if( (next->op == OField && next->p2 == o->p1) || (next->op == OSetField && next->p1 == o->p1) ) {
					int fid = next->op == OField ? next->p3 : next->p2;
					hl_obj_field *field = NULL;
					if( dst->t->kind == HOBJ || dst->t->kind == HSTRUCT ) {
						field = hl_obj_field_fetch(dst->t, fid);
					} else if( dst->t->kind == HVIRTUAL ) {
						field = dst->t->virt->fields + fid;
					}
					if( field ) hashed_name = field->hashed_name;
				} else if( (next->op >= OCall1 && next->op <= OCallN) && next->p3 == o->p1 ) {
					// Method call
					int fid = next->p2 < 0 ? -1 : ctx->m->functions_indexes[next->p2];
					if( fid >= 0 && fid < ctx->m->code->nfunctions ) {
						hl_function *cf = ctx->m->code->functions + fid;
						const uchar *name = fun_field_name(cf);
						if( name ) hashed_name = hl_hash_gen(name, true);
					}
				}
				op_null_check(ctx, dst, hashed_name);
			}
			break;

		// Object allocation
		case ONew:
			op_new(ctx, dst, dst->t);
			break;

		// String and bytes
		case OString:
			op_string(ctx, dst, o->p2);
			break;

		case OBytes:
			op_bytes(ctx, dst, o->p2);
			break;

		// Method calls
		case OCallMethod: {
			// obj.method(args...) - object is in o->extra[0], method index is p2, arg count is p3
			// o->extra contains all p3 arguments, with extra[0] being the object
			int method_index = o->p2;
			int nargs = o->p3;  // Total args including object
			vreg *obj = R(o->extra[0]);

			// Additional args (not including object)
			int extra_arg_count = nargs - 1;
			vreg **extra_args = NULL;
			if (extra_arg_count > 0) {
				extra_args = (vreg**)malloc(sizeof(vreg*) * extra_arg_count);
				for (int i = 0; i < extra_arg_count; i++) {
					extra_args[i] = R(o->extra[i + 1]);
				}
			}

			switch (obj->t->kind) {
			case HOBJ:
				op_call_method_obj(ctx, dst, obj, method_index, extra_args, extra_arg_count);
				break;

			case HVIRTUAL: {
				// HVIRTUAL method call:
				// if (hl_vfields(obj)[method_index])
				//     dst = vfield(obj->value, args...);
				// else
				//     dst = hl_dyn_call_obj(obj->value, field_type, field_hash, args, &ret);

				spill_regs(ctx);

				// Load obj pointer
				Arm64Reg obj_r = X9;  // Use X9 as temp for obj pointer
				ldr_stack(ctx, obj_r, obj->stackPos, obj->size);

				// Load vfield pointer: obj + sizeof(vvirtual) + method_index * HL_WSIZE
				// sizeof(vvirtual) = 24 (3 pointers: t, value, next)
				int vfield_offset = 24 + method_index * HL_WSIZE;
				if (vfield_offset < 32760) {
					// Can use scaled immediate
					encode_ldr_str_imm(ctx, 0x03, 0, 0x01, vfield_offset / 8, obj_r, RTMP);
				} else {
					// Need to use register offset for large offset
					load_immediate(ctx, vfield_offset, X10, false);
					// ADD X10, obj_r, X10
					encode_add_sub_reg(ctx, 1, 0, 0, 0, X10, 0, obj_r, X10);
					// LDR RTMP, [X10]
					encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, X10, RTMP);
				}

				// Test if vfield is NULL: CBNZ RTMP, has_vfield
				int jump_has_vfield = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 1, 0, RTMP);  // CBNZ = op bit 1

				// ---- NULL path: call hl_dyn_call_obj ----
				// hl_dyn_call_obj(vdynamic *obj, hl_type *ft, int hfield, void **args, vdynamic *ret)

				// For non-pointer non-void return types, we need stack space for ret value
				bool need_ret = !hl_is_ptr(dst->t) && dst->t->kind != HVOID;
				int ret_stack_offset = 0;

				if (need_ret) {
					// Allocate stack space for vdynamic return value (aligned)
					int vdyn_size = (sizeof(vdynamic) + 15) & ~15;
					encode_add_sub_imm(ctx, 1, 1, 0, 0, vdyn_size, SP_REG, SP_REG);  // SUB SP, SP, #vdyn_size
					ret_stack_offset = vdyn_size;
				}

				// Build args array on stack for extra args
				int args_size = extra_arg_count * HL_WSIZE;
				if (args_size & 15) args_size = (args_size + 15) & ~15;
				if (args_size > 0) {
					encode_add_sub_imm(ctx, 1, 1, 0, 0, args_size, SP_REG, SP_REG);
				}

				// Fill args array with pointers to extra args
				for (int i = 0; i < extra_arg_count; i++) {
					vreg *arg = extra_args[i];
					if (hl_is_ptr(arg->t)) {
						// Pointer: store pointer value directly
						ldr_stack(ctx, X10, arg->stackPos, arg->size);
						encode_ldr_str_imm(ctx, 0x03, 0, 0x00, i, SP_REG, X10);
					} else {
						// Non-pointer: store pointer to stack location
						// ADD X10, FP, #stackPos (or SUB for negative offset)
						int offset = arg->stackPos;
						if (offset >= 0) {
							encode_add_sub_imm(ctx, 1, 0, 0, 0, offset, FP, X10);
						} else {
							encode_add_sub_imm(ctx, 1, 1, 0, 0, -offset, FP, X10);
						}
						encode_ldr_str_imm(ctx, 0x03, 0, 0x00, i, SP_REG, X10);
					}
				}

				// X0 = obj->value (at offset 8 from obj)
				ldr_stack(ctx, obj_r, obj->stackPos, obj->size);  // Reload obj
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 1, obj_r, X0);  // X0 = obj->value

				// X1 = field type
				load_immediate(ctx, (int64_t)obj->t->virt->fields[method_index].t, X1, true);

				// X2 = hashed field name
				load_immediate(ctx, obj->t->virt->fields[method_index].hashed_name, X2, false);

				// X3 = args array (current SP if we have args)
				if (extra_arg_count > 0) {
					mov_reg_reg(ctx, X3, SP_REG, true);
				} else {
					mov_reg_reg(ctx, X3, XZR, true);  // NULL
				}

				// X4 = ret pointer (NULL for void/pointer, stack buffer otherwise)
				if (need_ret) {
					// Point to the vdynamic buffer we allocated
					encode_add_sub_imm(ctx, 1, 0, 0, 0, args_size, SP_REG, X4);
				} else {
					mov_reg_reg(ctx, X4, XZR, true);  // NULL
				}

				// Call hl_dyn_call_obj
				load_immediate(ctx, (int64_t)hl_dyn_call_obj, RTMP, true);
				EMIT32(ctx, (0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Store result
				if (dst->t->kind != HVOID) {
					if (need_ret) {
						// Load result from vdynamic buffer
						// X4 was clobbered by call, recompute: ret buffer is at SP + args_size
						encode_add_sub_imm(ctx, 1, 0, 0, 0, args_size + HDYN_VALUE, SP_REG, X10);
						if (IS_FLOAT(dst)) {
							encode_ldr_str_imm(ctx, dst->size == 8 ? 0x03 : 0x02, 1, 0x01, 0, X10, V0);
							str_stack_fp(ctx, V0, dst->stackPos, dst->size);
						} else {
							encode_ldr_str_imm(ctx, dst->size == 8 ? 0x03 : 0x02, 0, 0x01, 0, X10, X0);
							str_stack(ctx, X0, dst->stackPos, dst->size);
						}
					} else {
						// Pointer result in X0
						str_stack(ctx, X0, dst->stackPos, dst->size);
					}
				}

				// Clean up stack
				int total_cleanup = args_size + ret_stack_offset;
				if (total_cleanup > 0) {
					encode_add_sub_imm(ctx, 1, 0, 0, 0, total_cleanup, SP_REG, SP_REG);
				}

				// Jump to end
				int jump_end = BUF_POS();
				encode_branch_uncond(ctx, 0);  // Will be patched

				// ---- has_vfield path: direct call ----
				int pos_has_vfield = BUF_POS();
				patch_jump(ctx, jump_has_vfield, pos_has_vfield);

				// RTMP has vfield pointer, but ldr_stack can clobber RTMP for large offsets!
				// Save vfield pointer to X8 (indirect result register, safe to use as temp)
				mov_reg_reg(ctx, X8, RTMP, true);

				// Reload obj and get obj->value
				ldr_stack(ctx, obj_r, obj->stackPos, obj->size);
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 1, obj_r, X0);  // X0 = obj->value

				// Load extra args into X1-X7 / V0-V7
				// Note: X0 is already set to obj->value
				int int_reg = 1;  // Start from X1 (X0 is obj->value)
				int fp_reg = 0;

				for (int i = 0; i < extra_arg_count; i++) {
					vreg *arg = extra_args[i];
					if (IS_FLOAT(arg)) {
						if (fp_reg < 8) {
							ldr_stack_fp(ctx, FP_CALL_REGS[fp_reg], arg->stackPos, arg->size);
							fp_reg++;
						}
						// Stack overflow args not implemented for HVIRTUAL
					} else {
						if (int_reg < 8) {
							ldr_stack(ctx, CALL_REGS[int_reg], arg->stackPos, arg->size);
							int_reg++;
						}
						// Stack overflow args not implemented for HVIRTUAL
					}
				}

				// Call vfield: BLR X8 (saved vfield pointer)
				EMIT32(ctx, (0xD63F0000) | (X8 << 5));

				// Store result
				if (dst->t->kind != HVOID) {
					if (IS_FLOAT(dst)) {
						str_stack_fp(ctx, V0, dst->stackPos, dst->size);
					} else {
						str_stack(ctx, X0, dst->stackPos, dst->size);
					}
				}

				// Patch end jump
				int pos_end = BUF_POS();
				patch_jump(ctx, jump_end, pos_end);

				// Allocate dst register
				if (dst->t->kind != HVOID) {
					preg *p = alloc_dst(ctx, dst);
					if (IS_FLOAT(dst)) {
						if (p->kind == RFPU) {
							ldr_stack_fp(ctx, (Arm64FpReg)p->id, dst->stackPos, dst->size);
						}
					} else {
						if (p->kind == RCPU) {
							ldr_stack(ctx, (Arm64Reg)p->id, dst->stackPos, dst->size);
						}
					}
				}
				break;
			}

			default:
				printf("JIT ERROR: OCallMethod on unsupported type kind %d\n", obj->t->kind);
				jit_exit();
			}

			if (extra_args) free(extra_args);
			break;
		}

		case OCallThis: {
			// Call method on 'this' (register 0): this.method(extra_args...)
			// p2 = method index, p3 = number of extra args (not including 'this')
			// 'this' is always HOBJ type, so use op_call_method_obj directly
			int method_index = o->p2;
			int nargs = o->p3 + 1;  // +1 for 'this'
			vreg **args = (vreg**)malloc(sizeof(vreg*) * nargs);
			args[0] = R(0);  // 'this' is always register 0
			for (int i = 1; i < nargs; i++) {
				int reg_id = o->extra[i - 1];
				if (reg_id < 0 || reg_id >= f->nregs) {
					printf("JIT ERROR: OCallThis: invalid register index %d at position %d (nregs=%d, p3=%d)\n",
						reg_id, i-1, f->nregs, o->p3);
					jit_exit();
				}
				args[i] = R(reg_id);
			}
			// Debug: check for corrupt vreg pointers
			for (int i = 0; i < nargs; i++) {
				if ((unsigned long)args[i] < 0x1000 || (unsigned long)args[i] > 0x7fffffffffff) {
					printf("JIT ERROR: OCallThis: corrupt vreg pointer at args[%d] = %p\n", i, args[i]);
					jit_exit();
				}
			}
			op_call_method_obj(ctx, dst, args[0], method_index, args + 1, nargs - 1);
			free(args);
			break;
		}

		case OGetTID:
			// Get type ID (first 4 bytes of object)
			op_get_mem(ctx, dst, ra, 0, 4);
			break;

		case OArraySize:
			// Get array size
			{
				// Array size offset depends on type
				int offset = (ra->t->kind == HABSTRACT) ? (HL_WSIZE + 4) : (HL_WSIZE * 2);
				op_get_mem(ctx, dst, ra, offset, 4);
			}
			break;

		case OGetType:
			// Get type of object (NULL check required)
			{
				preg *r_obj = fetch(ctx, ra);
				preg *r_dst = alloc_dst(ctx, dst);

				// CBZ - branch if object is NULL
				int jump_null = BUF_POS();
				encode_cbz_cbnz(ctx, 1, 0, 0, (Arm64Reg)r_obj->id);  // CBZ Xn, null_case

				// Not NULL: Load type from object (first pointer at offset 0)
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, (Arm64Reg)r_obj->id, (Arm64Reg)r_dst->id);

				// Jump over NULL case
				int jump_end = BUF_POS();
				encode_branch_uncond(ctx, 0);  // B end

				// NULL case: Load &hlt_void
				int pos_null = BUF_POS();
				load_immediate(ctx, (int64_t)&hlt_void, (Arm64Reg)r_dst->id, true);

				// Patch jumps
				int pos_end = BUF_POS();
				patch_jump(ctx, jump_null, pos_null);
				patch_jump(ctx, jump_end, pos_end);

				mark_dirty(ctx, dst);
			}
			break;

		case OToVirtual:
			// Convert to virtual type - call hl_to_virtual(type, value)
			{
				// Ensure runtime obj is initialized
				if (ra->t->kind == HOBJ) {
					hl_get_obj_rt(ra->t);
				}

				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// X0 = dst->t
				load_immediate(ctx, (int64_t)dst->t, X0, true);

				// X1 = value (load from stack since we spilled)
				ldr_stack(ctx, X1, ra->stackPos, ra->size);

				// Call hl_to_virtual
				load_immediate(ctx, (int64_t)hl_to_virtual, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Store result from X0
				preg *p_dst = alloc_dst(ctx, dst);
				if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
					mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, true);
				} else if (p_dst->kind == RSTACK) {
					str_stack(ctx, X0, dst->stackPos, dst->size);
				}
			}
			break;

		case OInstanceClosure:
			// Create closure with captured instance: dst = closure(rb, function[p2])
			// hl_alloc_closure_ptr(hl_type *fullt, void *fvalue, void *v)
			{
				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// X0 = function type (first arg)
				hl_type *ftype = m->code->functions[m->functions_indexes[o->p2]].type;
				load_immediate(ctx, (int64_t)ftype, X0, true);

				// X1 = function pointer - will be patched later
				// Emit: B skip; .quad literal; skip: LDR X1, [PC-8]
				EMIT32(ctx,0x14000003);  // B +3 instructions (skip over literal)

				// Store position for patching
				jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
				j->pos = BUF_POS();
				j->target = o->p2;
				j->next = ctx->calls;
				ctx->calls = j;

				// Emit placeholder 64-bit literal
				EMIT32(ctx,0xDEADBEEF);
				EMIT32(ctx,0xDEADBEEF);

				// LDR X1, [PC, #-8] - load the 64-bit literal
				// Layout: B(+12) | lit_lo | lit_hi | LDR <- we are here
				// LDR needs to load from lit_lo which is at PC-8
				EMIT32(ctx,0x58ffffc1);  // LDR X1, [PC, #-8]

				// X2 = captured value (instance) - load from stack since we spilled
				ldr_stack(ctx, X2, rb->stackPos, rb->size);

				// Call hl_alloc_closure_ptr(type, fun_ptr, value)
				load_immediate(ctx, (int64_t)hl_alloc_closure_ptr, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Store result to stack (source of truth)
				str_stack(ctx, X0, dst->stackPos, dst->size);
				// Clear any stale register binding for dst
				if (dst->current != NULL) {
					dst->current->holds = NULL;
					dst->current = NULL;
				}
			}
			break;

		case OStaticClosure:
			// Create a static closure (no captured value)
			{
				vclosure *c = alloc_static_closure(ctx, o->p2);
				// Load pointer to closure into temp register and store to stack
				load_immediate(ctx, (int64_t)c, RTMP2, true);
				str_stack(ctx, RTMP2, dst->stackPos, dst->size);
				// Clear any stale register binding for dst
				if (dst->current != NULL) {
					dst->current->holds = NULL;
					dst->current = NULL;
				}
			}
			break;

		case OCallClosure:
			// Call a closure
			if (ra->t->kind == HDYN) {
				// Dynamic closure: call hl_dyn_call
				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// Allocate stack space for args array
				int offset = o->p3 * HL_WSIZE;
				if (offset & 15) offset += 16 - (offset & 15);  // Align to 16

				if (offset > 0) {
					// SUB SP, SP, #offset
					if (offset < 4096) {
						encode_add_sub_imm(ctx, 1, 1, 0, 0, offset, SP_REG, SP_REG);
					} else {
						load_immediate(ctx, offset, RTMP, true);
						encode_add_sub_ext(ctx, 1, 1, 0, RTMP, 3, 0, SP_REG, SP_REG);  // SUB SP, SP, RTMP, UXTX
					}

					// Store args to stack (load from vreg stack positions since we spilled)
					for (int i = 0; i < o->p3; i++) {
						vreg *a = R(o->extra[i]);
						ldr_stack(ctx, RTMP, a->stackPos, a->size);
						// STR Xn, [SP, #i*8]
						encode_ldr_str_imm(ctx, 0x03, 0, 0x00, i, SP_REG, RTMP);
					}
				}

				// Call hl_dyn_call(closure, args, nargs)
				// X0 = closure (load from stack since we spilled)
				ldr_stack(ctx, X0, ra->stackPos, ra->size);
				// X1 = SP (args array)
				mov_reg_reg(ctx, X1, SP_REG, true);
				// X2 = nargs
				load_immediate(ctx, o->p3, X2, false);

				load_immediate(ctx, (int64_t)hl_dyn_call, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Clean up stack
				if (offset > 0) {
					if (offset < 4096) {
						encode_add_sub_imm(ctx, 1, 0, 0, 0, offset, SP_REG, SP_REG);  // ADD
					} else {
						load_immediate(ctx, offset, RTMP, true);
						encode_add_sub_ext(ctx, 1, 0, 0, RTMP, 3, 0, SP_REG, SP_REG);  // ADD SP, SP, RTMP, UXTX
					}
				}

				if (dst->t->kind != HVOID) {
					// Always store to stack first (source of truth for later loads)
					if (IS_FLOAT(dst)) {
						str_stack_fp(ctx, V0, dst->stackPos, dst->size);
					} else {
						str_stack(ctx, X0, dst->stackPos, dst->size);
					}
					// Also keep in a register if allocated to a different one
					preg *p_dst = alloc_dst(ctx, dst);
					if (IS_FLOAT(dst)) {
						if (p_dst->kind == RFPU && (Arm64FpReg)p_dst->id != V0) {
							fmov_reg_reg(ctx, (Arm64FpReg)p_dst->id, V0, dst->size);
						}
					} else {
						if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
							mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, dst->size);
						}
					}
				}
			} else {
				// Static closure: check hasValue and call appropriately
				// Structure: vclosure { t:8, fun:8, hasValue:4, padding:4, value:8 }
				// Offsets: t=0, fun=8, hasValue=16, value=24

				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// Load closure from stack since we spilled
				Arm64Reg closure_r = RTMP;
				ldr_stack(ctx, closure_r, ra->stackPos, ra->size);

				// Load hasValue field at offset HL_WSIZE*2 (16 for 64-bit)
				// LDR W_RTMP2, [closure_r, #16]
				// For 32-bit load, imm12 is scaled by 4, so imm12 = 16/4 = 4
				encode_ldr_str_imm(ctx, 0x02, 0, 0x01, 4, closure_r, RTMP2);  // 32-bit load

				// Test if hasValue is zero
				// CBZ W_RTMP2, no_value_label
				int no_value_pos = BUF_POS();
				EMIT32(ctx,0x34000000 | (RTMP2 & 0x1F));  // CBZ (will patch offset later)

				// Has-value path: prepare args with value as first argument
				// Load value from offset HL_WSIZE*3 (24 bytes)
				// LDR X_RTMP2, [closure_r, #24]
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 3, closure_r, RTMP2);

				// The bound value always goes to X0 as a GP register value.
				// This matches x86 behavior which treats it as hlt_dyn (dynamic/pointer).
				// The closure's exposed type (ra->t->fun) hides the first arg, so args[0]
				// is actually the first PASSED arg, not the bound value type.
				// For methods, the bound value is always 'this' (an object pointer).
				int gp_arg_idx = 1;  // X0 is used for bound value
				int fp_arg_idx = 0;  // Start from V0

				// X0 = value (bound first arg)
				mov_reg_reg(ctx, X0, RTMP2, true);

				// Manually place remaining args (load from stack since we spilled)
				for (int i = 0; i < o->p3; i++) {
					vreg *arg = R(o->extra[i]);
					if (IS_FLOAT(arg)) {
						if (fp_arg_idx >= CALL_NREGS) break;
						ldr_stack_fp(ctx, FP_CALL_REGS[fp_arg_idx], arg->stackPos, arg->size);
						fp_arg_idx++;
					} else {
						if (gp_arg_idx >= CALL_NREGS) break;
						ldr_stack(ctx, CALL_REGS[gp_arg_idx], arg->stackPos, arg->size);
						gp_arg_idx++;
					}
				}

				// Reload closure pointer from stack
				ldr_stack(ctx, closure_r, ra->stackPos, ra->size);

				// Load function pointer from offset 8 (HL_WSIZE)
				// LDR X_RTMP, [closure_r, #8]
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 1, closure_r, RTMP);

				// Call function
				// BLR RTMP
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));

				// Jump to end
				int end_jmp_pos = BUF_POS();
				EMIT32(ctx,0x14000000);  // B (will patch offset later)

				// No-value path
				int no_value_label = BUF_POS();

				// Patch CBZ offset
				int cbz_offset = (no_value_label - no_value_pos) / 4;
				ctx->buf.b = ctx->startBuf + no_value_pos;
				EMIT32(ctx,0x34000000 | ((cbz_offset & 0x7FFFF) << 5) | (RTMP2 & 0x1F));
				ctx->buf.b = ctx->startBuf + no_value_label;  // Restore to continue after no-value label

				// Manually place args (load from stack since we spilled)
				{
					int gp_idx = 0, fp_idx = 0;
					for (int i = 0; i < o->p3; i++) {
						vreg *arg = R(o->extra[i]);
						if (IS_FLOAT(arg)) {
							if (fp_idx >= CALL_NREGS) break;
							ldr_stack_fp(ctx, FP_CALL_REGS[fp_idx], arg->stackPos, arg->size);
							fp_idx++;
						} else {
							if (gp_idx >= CALL_NREGS) break;
							ldr_stack(ctx, CALL_REGS[gp_idx], arg->stackPos, arg->size);
							gp_idx++;
						}
					}
				}

				// Reload closure pointer from stack
				ldr_stack(ctx, closure_r, ra->stackPos, ra->size);

				// Load function pointer from offset 8 (HL_WSIZE)
				// LDR X_RTMP, [closure_r, #8]
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 1, closure_r, RTMP);

				// Call function
				// BLR RTMP
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));

				// End label
				int end_label = BUF_POS();

				// Patch jump to end
				int b_offset = (end_label - end_jmp_pos) / 4;
				ctx->buf.b = ctx->startBuf + end_jmp_pos;
				EMIT32(ctx,0x14000000 | (b_offset & 0x3FFFFFF));
				ctx->buf.b = ctx->startBuf + end_label;

				// Store result if needed
				if (dst->t->kind != HVOID) {
					// Always store to stack first (source of truth for later loads)
					if (IS_FLOAT(dst)) {
						str_stack_fp(ctx, V0, dst->stackPos, dst->size);
					} else {
						str_stack(ctx, X0, dst->stackPos, dst->size);
					}
					// Also keep in a register if allocated to a different one
					preg *p_dst = alloc_dst(ctx, dst);
					if (IS_FLOAT(dst)) {
						if (p_dst->kind == RFPU && (Arm64FpReg)p_dst->id != V0) {
							fmov_reg_reg(ctx, (Arm64FpReg)p_dst->id, V0, dst->size);
						}
					} else {
						if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
							mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, dst->size);
						}
					}
				}
			}
			break;

		case ODynGet:
			// Dynamic field get - call get_dynget function
			{
				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// Get field name hash
				int_val field_hash = (int_val)hl_hash_utf8(m->code->strings[o->p3]);

				// X0 = object (load from stack since we spilled)
				ldr_stack(ctx, X0, ra->stackPos, ra->size);
				// X1 = field hash
				load_immediate(ctx, field_hash, X1, true);

				// X2 = type (for non-float, non-i64)
				if (!IS_FLOAT(dst) && dst->t->kind != HI64) {
					load_immediate(ctx, (int_val)dst->t, X2, true);
				}

				// Call appropriate get_dynget function
				void *fn = get_dynget(dst->t);
				load_immediate(ctx, (int64_t)fn, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP

				// Store result
				if (dst->t->kind != HVOID) {
					preg *p_dst = alloc_dst(ctx, dst);
					if (IS_FLOAT(dst)) {
						if (p_dst->kind == RFPU && (Arm64FpReg)p_dst->id != V0) {
							fmov_reg_reg(ctx, (Arm64FpReg)p_dst->id, V0, dst->size);
						} else if (p_dst->kind == RSTACK) {
							str_stack_fp(ctx, V0, dst->stackPos, dst->size);
						}
					} else {
						if (p_dst->kind == RCPU && (Arm64Reg)p_dst->id != X0) {
							mov_reg_reg(ctx, (Arm64Reg)p_dst->id, X0, true);
						} else if (p_dst->kind == RSTACK) {
							str_stack(ctx, X0, dst->stackPos, dst->size);
						}
					}
				}
			}
			break;

		case ODynSet:
			// Dynamic field set - call get_dynset function
			{
				// Spill caller-saved registers before the call
				spill_regs(ctx);

				// Get field name hash
				int_val field_hash = hl_hash_gen(hl_get_ustring(m->code, o->p2), true);

				// X0 = object (load from stack since we spilled)
				ldr_stack(ctx, X0, dst->stackPos, dst->size);
				// X1 = field hash
				load_immediate(ctx, field_hash, X1, true);

				// Prepare value arg based on type (load from stack since we spilled)
				switch (rb->t->kind) {
				case HF32:
				case HF64:
					// V0 = value (float)
					ldr_stack_fp(ctx, V0, rb->stackPos, rb->size);
					break;
				case HI64:
					// X2 = value (i64)
					ldr_stack(ctx, X2, rb->stackPos, rb->size);
					break;
				default:
					// X2 = type, X3 = value (matches hl_dyn_seti signature)
					load_immediate(ctx, (int_val)rb->t, X2, true);
					ldr_stack(ctx, X3, rb->stackPos, rb->size);
					break;
				}

				// Call appropriate get_dynset function
				void *fn = get_dynset(rb->t);
				load_immediate(ctx, (int64_t)fn, RTMP, true);
				EMIT32(ctx,(0xD63F0000) | (RTMP << 5));  // BLR RTMP
			}
			break;

		case OSwitch:
			// Switch statement - optimized using branch table
			{
				spill_regs(ctx);
				preg *r = fetch(ctx, dst);
				Arm64Reg r_val = (Arm64Reg)r->id;

				// Ensure value is in a CPU register (not stack)
				if (r->kind != RCPU) {
					ldr_stack(ctx, RTMP2, dst->stackPos, dst->size);
					r_val = RTMP2;
				}

				// CMP r_val, #count
				if (o->p2 < 4096) {
					encode_add_sub_imm(ctx, (dst->size == 8) ? 1 : 0, 1, 1, 0, o->p2, r_val, XZR);
				} else {
					load_immediate(ctx, o->p2, RTMP, false); // Use RTMP here since r_val might be RTMP2
					encode_add_sub_reg(ctx, (dst->size == 8) ? 1 : 0, 1, 1, 0, RTMP, 0, r_val, XZR);
				}

				// B.HS default (index >= count)
				int jdefault = BUF_POS();
				encode_branch_cond(ctx, 0, COND_HS);

				// Compute target address: target = table_start + (index * 4)
				// ADR RTMP, table_start (PC + 12 bytes)
				// Offset 12: immhi=3, immlo=0
				encode_adr(ctx, 0, 3, RTMP);

				// ADD RTMP, RTMP, r_val, LSL #2 (or UXTW #2 for 32-bit index)
				// sf=1 (64-bit result), op=0 (ADD), S=0, Rm=r_val, option=3(64)/2(32), imm3=2, Rn=RTMP, Rd=RTMP
				int option = (dst->size == 8) ? 3 : 2; // 3=UXTX(64), 2=UXTW(32)
				encode_add_sub_ext(ctx, 1, 0, 0, r_val, option, 2, RTMP, RTMP);

				// BR RTMP
				// 0xD61F0000 | (Rn << 5)
				EMIT32(ctx, 0xD61F0000 | (RTMP << 5));

				// table_start:
				// Emit jumps
				for (int i = 0; i < o->p2; i++) {
					int jump_pos = BUF_POS();
					// Emit B 0 (unconditional branch, to be patched)
					EMIT32(ctx, 0x14000000);
					register_jump(ctx, jump_pos, (opCount + 1) + o->extra[i]);
				}

				// Patch jdefault to here (fallthrough)
				patch_jump(ctx, jdefault, BUF_POS());
			}
			break;


	// Exception handling
	case ORethrow:
		// Call hl_rethrow(exception)
		{
			vreg *arg = R(o->p1);
			op_call_native(ctx, NULL, NULL, (void*)hl_rethrow, &arg, 1);
		}
		break;

	case OTrap:
		// Exception handling with setjmp/longjmp
		// OTrap dst, jump_offset
		//   dst = register to store caught exception value
		//   o->p2 = opcode offset to catch handler (relative to next opcode)
		{
			vreg *dst = R(o->p1);

			// Calculate trap context size (16-byte aligned)
			// hl_trap_ctx contains: jmp_buf, prev pointer, tcheck pointer
			int trap_size = (sizeof(hl_trap_ctx) + 15) & ~15;

			// For offset calculations using NULL pointer trick (like x86)
			hl_trap_ctx *t = NULL;
			hl_thread_info *tinf = NULL;
			int offset_trap_current = (int)(int_val)&tinf->trap_current;
			int offset_exc_value = (int)(int_val)&tinf->exc_value;
			int offset_prev = (int)(int_val)&t->prev;
			int offset_tcheck = (int)(int_val)&t->tcheck;

			// Spill caller-saved registers before any calls
			spill_regs(ctx);

			// Step 1: Allocate trap context on stack
			// SUB SP, SP, #trap_size
			if (trap_size < 4096) {
				encode_add_sub_imm(ctx, 1, 1, 0, 0, trap_size, SP_REG, SP_REG);
			} else {
				load_immediate(ctx, trap_size, RTMP, true);
				encode_add_sub_ext(ctx, 1, 1, 0, RTMP, 3, 0, SP_REG, SP_REG);  // SUB SP, SP, RTMP, UXTX
			}

			// Step 2: Call hl_get_thread() to get thread info pointer
			// Result will be in X0
			load_immediate(ctx, (int64_t)hl_get_thread, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

			// X0 now contains thread info pointer
			// Save it to a callee-saved register (X19) for later use
			// But we need to be careful - after setjmp/longjmp X19 may not be preserved
			// So we'll re-call hl_get_thread in the exception path (like x86 does)

			// Step 3: Link to trap chain
			// Load old trap: X9 = tinf->trap_current
			if (offset_trap_current < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, offset_trap_current / 8, X0, X9);
			} else {
				load_immediate(ctx, offset_trap_current, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X0, X9);
			}

			// Store trap->prev = old trap
			// STR X9, [SP, #offset_prev]
			// Need to copy SP to a GPR first since SP can't be used as Rt in STR
			mov_reg_reg(ctx, X10, SP_REG, true);  // X10 = SP
			if (offset_prev < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x00, offset_prev / 8, X10, X9);
			} else {
				load_immediate(ctx, offset_prev, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x00, RTMP, 0x03, 0, X10, X9);
			}

			// Store tinf->trap_current = SP (new trap context)
			// STR X10, [X0, #offset_trap_current]
			if (offset_trap_current < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x00, offset_trap_current / 8, X0, X10);
			} else {
				load_immediate(ctx, offset_trap_current, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x00, RTMP, 0x03, 0, X0, X10);
			}

			// Step 4: Set tcheck (type filtering)
			hl_opcode *cat = f->ops + opCount + 1;
			hl_opcode *next = f->ops + opCount + 1 + o->p2;
			hl_opcode *next2 = f->ops + opCount + 2 + o->p2;
			int gindex = -1;

			if (cat->op == OCatch) {
				gindex = cat->p1;
			} else if (next->op == OGetGlobal && next2->op == OCall2 && next2->p3 == next->p1 && dst->stack.id == (int)(int_val)next2->extra) {
				gindex = next->p2;
			}

			bool has_type_check = false;
			if (gindex >= 0) {
				hl_type *gt = m->code->globals[gindex];
				while (gt->kind == HOBJ && gt->obj->super) gt = gt->obj->super;
				if (gt->kind == HOBJ && gt->obj->nfields && gt->obj->fields[0].t->kind == HTYPE) {
					// Load global address
					void *addr = m->globals_data + m->globals_indexes[gindex];
					load_immediate(ctx, (int64_t)addr, RTMP, true);
					
					// STR RTMP, [SP, #offset_tcheck]
					// We use X10 (which holds SP) from previous step
					if (offset_tcheck < 4096) {
						encode_ldr_str_imm(ctx, 0x03, 0, 0x00, offset_tcheck / 8, X10, RTMP);
					} else {
						// Use RTMP2 for offset since RTMP holds the value
						load_immediate(ctx, offset_tcheck, RTMP2, true);
						encode_ldr_str_reg(ctx, 0x03, 0, 0x00, RTMP2, 0x03, 0, X10, RTMP);
					}
					has_type_check = true;
				}
			}

			if (!has_type_check) {
				// STR XZR, [SP, #offset_tcheck]
				if (offset_tcheck < 4096) {
					encode_ldr_str_imm(ctx, 0x03, 0, 0x00, offset_tcheck / 8, X10, XZR);
				} else {
					load_immediate(ctx, offset_tcheck, RTMP, true);
					encode_ldr_str_reg(ctx, 0x03, 0, 0x00, RTMP, 0x03, 0, X10, XZR);
				}
			}

			// Step 5: Call setjmp(trap_ctx)
			// X0 = pointer to trap context (SP, which starts with jmp_buf)
			mov_reg_reg(ctx, X0, SP_REG, true);
			load_immediate(ctx, (int64_t)setjmp, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			// Note: spill_regs was already called at the start of OTrap

			// X0 now contains setjmp return value:
			//   0 = normal path (just called setjmp)
			//   non-zero = exception path (returned via longjmp)

			// Step 6: Branch on setjmp return value
			// CBZ X0, normal_path
			int jnormal = BUF_POS();
			EMIT32(ctx, 0xB4000000 | X0);  // CBZ X0, #0 (will be patched)

			// --- Exception path (reached via longjmp) ---
			// After longjmp, all registers have been restored to setjmp state.
			// The vreg/preg bindings in ctx are stale and invalid.
			// We must NOT spill (would write garbage to stack).
			// Just clear all register bindings.
			for (int i = 0; i < REG_COUNT; i++) {
				preg *r = &ctx->pregs[i];
				if (r->holds) {
					r->holds->current = NULL;
					r->holds = NULL;
				}
			}

			// Deallocate trap context
			if (trap_size < 4096) {
				encode_add_sub_imm(ctx, 1, 0, 0, 0, trap_size, SP_REG, SP_REG);
			} else {
				load_immediate(ctx, trap_size, RTMP, true);
				encode_add_sub_ext(ctx, 1, 0, 0, RTMP, 3, 0, SP_REG, SP_REG);  // ADD SP, SP, RTMP, UXTX
			}

			// Call hl_get_thread() again (can't trust registers after longjmp)
			load_immediate(ctx, (int64_t)hl_get_thread, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP

			// Load exception value: X9 = tinf->exc_value
			if (offset_exc_value < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, offset_exc_value / 8, X0, X9);
			} else {
				load_immediate(ctx, offset_exc_value, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X0, X9);
			}

			// Store exception value to destination vreg's stack slot.
			// IMPORTANT: Must always store to stack because the catch handler
			// was compiled separately and will load from the stack slot.
			// Register bindings are cleared after longjmp, so we can't rely
			// on the value being in any register.
			str_stack(ctx, X9, dst->stackPos, 8);

			// Jump to catch handler (opCount + 1 + o->p2)
			int jcatch = BUF_POS();
			EMIT32(ctx, 0x14000000);  // B #0 (will be patched by register_jump)
			register_jump(ctx, jcatch, (opCount + 1) + o->p2);

			// --- Normal path ---
			// Patch the CBZ to jump here
			patch_jump(ctx, jnormal, BUF_POS());
		}
		break;

	case OEndTrap:
		// End of try block - unlink trap from chain and deallocate
		{
			// Spill caller-saved registers before the call
			spill_regs(ctx);

			// Calculate trap context size (must match OTrap)
			int trap_size = (sizeof(hl_trap_ctx) + 15) & ~15;

			// Offset calculations
			hl_trap_ctx *t = NULL;
			hl_thread_info *tinf = NULL;
			int offset_trap_current = (int)(int_val)&tinf->trap_current;
			int offset_prev = (int)(int_val)&t->prev;

			// Call hl_get_thread() to get thread info pointer
			load_immediate(ctx, (int64_t)hl_get_thread, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			// X0 = thread info pointer

			// Load current trap: X9 = tinf->trap_current
			if (offset_trap_current < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, offset_trap_current / 8, X0, X9);
			} else {
				load_immediate(ctx, offset_trap_current, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X0, X9);
			}

			// Load previous trap: X9 = current->prev
			if (offset_prev < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, offset_prev / 8, X9, X9);
			} else {
				load_immediate(ctx, offset_prev, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X9, X9);
			}

			// Store tinf->trap_current = prev (restore old trap)
			if (offset_trap_current < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x00, offset_trap_current / 8, X0, X9);
			} else {
				load_immediate(ctx, offset_trap_current, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x00, RTMP, 0x03, 0, X0, X9);
			}

			// Deallocate trap context from stack
			if (trap_size < 4096) {
				encode_add_sub_imm(ctx, 1, 0, 0, 0, trap_size, SP_REG, SP_REG);
			} else {
				load_immediate(ctx, trap_size, RTMP, true);
				encode_add_sub_ext(ctx, 1, 0, 0, RTMP, 3, 0, SP_REG, SP_REG);  // ADD SP, SP, RTMP, UXTX
			}
		}
		break;

	case ONop:
		// No operation - just continue
		break;

	case OCatch:
		// Exception catch marker - discard register bindings since this is a jump
		// target from the OTrap exception path (reached via longjmp).
		// Like OLabel, we need to clear bindings so subsequent code loads from stack.
		discard_regs(ctx);
		break;

	case OAssert:
		// Call the assertion helper (static_functions[1])
		{
			// Emit indirect call sequence:
			//   LDR X17, [PC, #12]
			//   BLR X17
			//   B #12
			//   .quad address (patched later)

			EMIT32(ctx, 0x58000071);  // LDR X17, #12
			EMIT32(ctx, 0xD63F0220);  // BLR X17

			// Register literal position for patching
			jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
			j->pos = BUF_POS() + 4;   // Position of the 8-byte literal (after B instruction)
			j->target = -2;  // Special marker for assert function
			j->next = ctx->calls;
			ctx->calls = j;

			EMIT32(ctx, 0x14000003);  // B #12 (skip 3 instructions = 12 bytes)
			EMIT32(ctx, 0);           // Low 32 bits placeholder
			EMIT32(ctx, 0);           // High 32 bits placeholder
		}
		break;

	case OPrefetch:
		// Memory prefetch hint - fetch address into register, then PRFM
		{
			preg *r = fetch(ctx, dst);
			Arm64Reg base = (r->kind == RCPU) ? r->id : RTMP;
			if (r->kind != RCPU) {
				ldr_stack(ctx, base, dst->stackPos, dst->size);
			}

			if (o->p2 > 0) {
				// Prefetch field offset
				switch (dst->t->kind) {
				case HOBJ:
				case HSTRUCT:
					{
						hl_runtime_obj *rt = hl_get_obj_rt(dst->t);
						int offset = rt->fields_indexes[o->p2 - 1];
						// ADD base, base, #offset
						if (offset < 4096) {
							encode_add_sub_imm(ctx, 1, 0, 0, 0, offset, base, base);
						} else {
							load_immediate(ctx, offset, RTMP2, true);
							encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, base, base);
						}
					}
					break;
				default:
					break;
				}
			}

			// PRFM PLDL1KEEP, [base] - prefetch for load, L1 cache, keep
			// Encoding: 11111001 10 imm12 Rn 00000 (PRFM with imm12=0, prfop=0)
			EMIT32(ctx, 0xF9800000 | (base << 5));
		}
		break;

	// Enum operations
	case OEnumAlloc:
		// Allocate enum value: dst = hl_alloc_enum(type, construct_index)
		{
			spill_regs(ctx);
			// Load arguments: X0 = dst->t (type), X1 = o->p2 (construct index)
			load_immediate(ctx, (int64_t)dst->t, X0, true);
			load_immediate(ctx, o->p2, X1, true);
			// Call hl_alloc_enum
			load_immediate(ctx, (int64_t)hl_alloc_enum, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			// Store result
			str_stack(ctx, X0, dst->stackPos, dst->size);
			// Clear dst's old register binding - value is now on stack
			if (dst->current != NULL) {
				dst->current->holds = NULL;
				dst->current = NULL;
			}
		}
		break;

	case OEnumIndex:
		// Get enum constructor index: dst = ((venum*)obj)->index
		// Index is at offset 8 (after type pointer)
		// NOTE: We use RTMP2 for the index value because str_stack uses RTMP for address calculation
		{
			preg *r = fetch(ctx, ra);
			Arm64Reg src = (r->kind == RCPU) ? r->id : RTMP;
			if (r->kind != RCPU) {
				ldr_stack(ctx, src, ra->stackPos, ra->size);
			}
			// Load index from offset 8 (32-bit value) into RTMP2
			encode_ldr_str_imm(ctx, 0x02, 0, 0x01, 8 / 4, src, RTMP2);  // LDR W, [src, #8]
			str_stack(ctx, RTMP2, dst->stackPos, 4);
			// Release source register
			discard(ctx, r);
			// Clear dst's old register binding - value is now on stack
			if (dst->current != NULL) {
				dst->current->holds = NULL;
				dst->current = NULL;
			}
		}
		break;

	case OEnumField:
		// Get enum field: dst = enum->construct->fields[extra[0]]
		// NOTE: We use RTMP2 for the field value because str_stack uses RTMP for address calculation
		{
			hl_enum_construct *c = &ra->t->tenum->constructs[o->p3];
			int field_offset = c->offsets[(int)(int_val)o->extra];
			preg *r = fetch(ctx, ra);
			Arm64Reg src = (r->kind == RCPU) ? r->id : RTMP;
			if (r->kind != RCPU) {
				ldr_stack(ctx, src, ra->stackPos, ra->size);
			}
			// Load field value
			if (IS_FLOAT(dst)) {
				// Evict any vreg currently bound to V0 before using it
				preg *pv0 = PVFPR(0);
				if (pv0->holds != NULL && pv0->holds != dst) {
					free_reg(ctx, pv0);
				}
				if (field_offset < 4096 && (field_offset % (dst->size)) == 0) {
					int scale = (dst->size == 8) ? 3 : 2;
					encode_ldr_str_imm(ctx, scale, 1, 0x01, field_offset >> scale, src, V0);
				} else {
					load_immediate(ctx, field_offset, RTMP2, true);
					encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, src, RTMP2);
					encode_ldr_str_imm(ctx, (dst->size == 8) ? 3 : 2, 1, 0x01, 0, RTMP2, V0);
				}
				str_stack_fp(ctx, V0, dst->stackPos, dst->size);
			} else {
				int size_code = (dst->size == 8) ? 0x03 : (dst->size == 4) ? 0x02 : (dst->size == 2) ? 0x01 : 0x00;
				if (field_offset < 4096 && (field_offset % dst->size) == 0) {
					// Use RTMP2 to avoid conflict with str_stack's use of RTMP
					encode_ldr_str_imm(ctx, size_code, 0, 0x01, field_offset / dst->size, src, RTMP2);
				} else {
					load_immediate(ctx, field_offset, RTMP2, true);
					encode_ldr_str_reg(ctx, size_code, 0, 0x01, RTMP2, 0x03, 0, src, RTMP2);
				}
				str_stack(ctx, RTMP2, dst->stackPos, dst->size);
			}
			// Release source register
			discard(ctx, r);
			// Clear dst's old register binding - value is now on stack
			if (dst->current != NULL) {
				dst->current->holds = NULL;
				dst->current = NULL;
			}
		}
		break;

	case OSetEnumField:
		// Set enum field: enum->construct->fields[p2] = rb
		{
			hl_enum_construct *c = &dst->t->tenum->constructs[0];  // Always construct 0 for set
			int field_offset = c->offsets[o->p2];
			preg *r_enum = fetch(ctx, dst);
			preg *r_val = fetch(ctx, rb);
			Arm64Reg enum_r = (r_enum->kind == RCPU) ? r_enum->id : RTMP;
			if (r_enum->kind != RCPU) {
				ldr_stack(ctx, enum_r, dst->stackPos, dst->size);
			}

			if (IS_FLOAT(rb)) {
				Arm64FpReg val_r = (r_val->kind == RFPU) ? (Arm64FpReg)r_val->id : V0;
				if (r_val->kind != RFPU) {
					ldr_stack_fp(ctx, val_r, rb->stackPos, rb->size);
				}
				if (field_offset < 4096 && (field_offset % rb->size) == 0) {
					int scale = (rb->size == 8) ? 3 : 2;
					encode_ldr_str_imm(ctx, scale, 1, 0x00, field_offset >> scale, enum_r, val_r);
				} else {
					load_immediate(ctx, field_offset, RTMP2, true);
					encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP2, 0, enum_r, RTMP2);
					encode_ldr_str_imm(ctx, (rb->size == 8) ? 3 : 2, 1, 0x00, 0, RTMP2, val_r);
				}
			} else {
				Arm64Reg val_r = (r_val->kind == RCPU) ? r_val->id : RTMP2;
				if (r_val->kind != RCPU) {
					ldr_stack(ctx, val_r, rb->stackPos, rb->size);
				}
				int size_code = (rb->size == 8) ? 0x03 : (rb->size == 4) ? 0x02 : (rb->size == 2) ? 0x01 : 0x00;
				if (field_offset < 4096 && (field_offset % rb->size) == 0) {
					encode_ldr_str_imm(ctx, size_code, 0, 0x00, field_offset / rb->size, enum_r, val_r);
				} else {
					if (val_r == RTMP2) {
						// Need to use a different temp
						preg *px9 = &ctx->pregs[X9];
						if (px9->holds != NULL) free_reg(ctx, px9);
						ldr_stack(ctx, X9, rb->stackPos, rb->size);
						val_r = X9;
					}
					load_immediate(ctx, field_offset, RTMP2, true);
					encode_ldr_str_reg(ctx, size_code, 0, 0x00, RTMP2, 0x03, 0, enum_r, val_r);
				}
			}
		}
		break;

	case OMakeEnum:
		// Make enum with all field values
		{
			hl_enum_construct *c = &dst->t->tenum->constructs[o->p2];
			spill_regs(ctx);
			// Allocate enum: X0 = hl_alloc_enum(type, construct_index)
			load_immediate(ctx, (int64_t)dst->t, X0, true);
			load_immediate(ctx, o->p2, X1, true);
			load_immediate(ctx, (int64_t)hl_alloc_enum, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			// X0 = allocated enum pointer

			// Fill in fields
			for (int i = 0; i < c->nparams; i++) {
				vreg *field_val = R(o->extra[i]);
				int field_offset = c->offsets[i];

				if (IS_FLOAT(field_val)) {
					ldr_stack_fp(ctx, V0, field_val->stackPos, field_val->size);
					if (field_offset < 4096 && (field_offset % field_val->size) == 0) {
						int scale = (field_val->size == 8) ? 3 : 2;
						encode_ldr_str_imm(ctx, scale, 1, 0x00, field_offset >> scale, X0, V0);
					} else {
						load_immediate(ctx, field_offset, RTMP, true);
						encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, X0, RTMP);
						encode_ldr_str_imm(ctx, (field_val->size == 8) ? 3 : 2, 1, 0x00, 0, RTMP, V0);
					}
				} else {
					ldr_stack(ctx, RTMP, field_val->stackPos, field_val->size);
					int size_code = (field_val->size == 8) ? 0x03 : (field_val->size == 4) ? 0x02 : (field_val->size == 2) ? 0x01 : 0x00;
					if (field_offset < 4096 && (field_offset % field_val->size) == 0) {
						encode_ldr_str_imm(ctx, size_code, 0, 0x00, field_offset / field_val->size, X0, RTMP);
					} else {
						load_immediate(ctx, field_offset, RTMP2, true);
						encode_ldr_str_reg(ctx, size_code, 0, 0x00, RTMP2, 0x03, 0, X0, RTMP);
					}
				}
				if ((i & 15) == 0) jit_buf(ctx);  // Ensure buffer space periodically
			}
			// Store result
			str_stack(ctx, X0, dst->stackPos, dst->size);
		}
		break;

	// Reference operations
	case ORefData:
		// Get pointer to array/bytes data
		switch (ra->t->kind) {
		case HARRAY:
			{
				// Array data starts after varray header
				preg *r = fetch(ctx, ra);
				Arm64Reg src = (r->kind == RCPU) ? r->id : RTMP2;
				if (r->kind != RCPU) {
					ldr_stack(ctx, src, ra->stackPos, ra->size);
				}
				// ADD dst, src, #sizeof(varray)
				// Use RTMP2 for result since str_stack may clobber RTMP for large offsets
				int offset = sizeof(varray);
				if (offset < 4096) {
					encode_add_sub_imm(ctx, 1, 0, 0, 0, offset, src, RTMP2);
				} else {
					load_immediate(ctx, offset, RTMP, true);
					encode_add_sub_reg(ctx, 1, 0, 0, 0, RTMP, 0, src, RTMP2);
				}
				str_stack(ctx, RTMP2, dst->stackPos, dst->size);
				// Clear any stale binding on dst - value is now on stack
				if (dst->current != NULL) {
					dst->current->holds = NULL;
					dst->current = NULL;
				}
				discard(ctx, r);
			}
			break;
		case HBYTES:
			// Bytes is just the pointer itself
			op_mov(ctx, dst, ra);
			break;
		default:
			JIT_ASSERT(ra->t->kind);
			break;
		}
		break;

	case ORefOffset:
		// Offset a reference: dst = ptr + offset * element_size
		{
			preg *r_ptr = fetch(ctx, ra);
			preg *r_off = fetch(ctx, rb);
			Arm64Reg ptr_r = (r_ptr->kind == RCPU) ? r_ptr->id : RTMP;
			Arm64Reg off_r = (r_off->kind == RCPU) ? r_off->id : RTMP2;
			if (r_ptr->kind != RCPU) {
				ldr_stack(ctx, ptr_r, ra->stackPos, ra->size);
			}
			if (r_off->kind != RCPU) {
				ldr_stack(ctx, off_r, rb->stackPos, rb->size);
			}
			// Get element size from ref type
			// Use RTMP2 for result since str_stack may clobber RTMP for large offsets
			int elem_size = hl_type_size(dst->t->tparam);
			if (elem_size == 1) {
				// ADD dst, ptr, offset
				encode_add_sub_reg(ctx, 1, 0, 0, 0, off_r, 0, ptr_r, RTMP2);
			} else {
				// Ensure X9 is free
				preg *px9 = &ctx->pregs[X9];
				if (px9->holds != NULL) free_reg(ctx, px9);

				// Multiply offset by element size, then add
				load_immediate(ctx, elem_size, X9, true);
				encode_madd_msub(ctx, 1, 0, X9, ptr_r, off_r, RTMP2);  // RTMP2 = ptr + off * elem_size
			}
			str_stack(ctx, RTMP2, dst->stackPos, dst->size);
			// Clear any stale binding on dst - value is now on stack
			if (dst->current != NULL) {
				dst->current->holds = NULL;
				dst->current = NULL;
			}
			discard(ctx, r_ptr);
			discard(ctx, r_off);
		}
		break;

	case OVirtualClosure:
		// Create closure from virtual method
		{
			preg *r = fetch(ctx, ra);
			Arm64Reg obj_r = (r->kind == RCPU) ? r->id : RTMP;
			if (r->kind != RCPU) {
				ldr_stack(ctx, obj_r, ra->stackPos, ra->size);
			}

			// Find the method type by walking the prototype chain
			hl_type *ot = ra->t;
			hl_type *fun_type = NULL;
			while (fun_type == NULL && ot != NULL) {
				for (int i = 0; i < ot->obj->nproto; i++) {
					hl_obj_proto *pp = ot->obj->proto + i;
					if (pp->pindex == o->p3) {
						fun_type = m->code->functions[m->functions_indexes[pp->findex]].type;
						break;
					}
				}
				ot = ot->obj->super;
			}

			spill_regs(ctx);

			// Allocate closure: hl_alloc_closure_ptr(type, func, obj)
			// First get the function pointer from vtable
			ldr_stack(ctx, X2, ra->stackPos, ra->size);  // obj -> X2
			// Load vtable: X9 = obj->t (first field)
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, 0, X2, X9);
			// Load runtime obj: X9 = ((hl_type*)X9)->obj->rt
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, FIELD_OFFSET_SCALED(hl_type, obj), X9, X9);
			encode_ldr_str_imm(ctx, 0x03, 0, 0x01, FIELD_OFFSET_SCALED(hl_type_obj, rt), X9, X9);
			// Load method from vtable: X1 = rt->methods[pindex]
			int method_offset = HL_WSIZE * 2 + o->p3 * HL_WSIZE;  // Skip hasPtr and nFields
			if (method_offset < 4096) {
				encode_ldr_str_imm(ctx, 0x03, 0, 0x01, method_offset / 8, X9, X1);
			} else {
				load_immediate(ctx, method_offset, RTMP, true);
				encode_ldr_str_reg(ctx, 0x03, 0, 0x01, RTMP, 0x03, 0, X9, X1);
			}

			// Load type and call hl_alloc_closure_ptr
			load_immediate(ctx, (int64_t)fun_type, X0, true);
			// X1 = func ptr (already loaded), X2 = obj (already loaded)
			load_immediate(ctx, (int64_t)hl_alloc_closure_ptr, RTMP, true);
			EMIT32(ctx, 0xD63F0000 | (RTMP << 5));  // BLR RTMP
			str_stack(ctx, X0, dst->stackPos, dst->size);
			// Clear any stale register binding for dst
			if (dst->current != NULL) {
				dst->current->holds = NULL;
				dst->current = NULL;
			}
		}
		break;

	default:
		printf("JIT Warning: Unimplemented opcode %d (%s) at position %d\n",
		       o->op, hl_op_name(o->op), opCount);
		break;
		}

		// If the next opcode is a jump target, spill dirty registers and clear bindings.
		// The jump path already spilled before jumping, but the fallthrough path
		// may still have dirty registers that need to be saved before the merge point.
		if (opCount + 1 < f->nops && ctx->opsPos[opCount + 1] == -1) {
			spill_regs(ctx);
			spill_callee_saved(ctx);  // Callee-saved must also be spilled at merge points
		}

		// Record position for this opcode (for debug info and jump patching)
		if (opCount + 1 < f->nops)
			ctx->opsPos[opCount + 1] = BUF_POS();

		// Record debug offset for this opcode
		if (debug16 || debug32) {
			int dbg_size = BUF_POS() - codePos;
			if (debug16 && dbg_size > 0xFF00) {
				// Upgrade to 32-bit offsets
				int dbg_i;
				debug32 = (int*)malloc(sizeof(int) * (f->nops + 1));
				for (dbg_i = 0; dbg_i <= opCount; dbg_i++)
					debug32[dbg_i] = debug16[dbg_i];
				free(debug16);
				debug16 = NULL;
			}
			if (debug16)
				debug16[opCount + 1] = (unsigned short)dbg_size;
			else if (debug32)
				debug32[opCount + 1] = dbg_size;
		}
	}

	// Record epilogue position BEFORE emitting it (for jumps past last opcode)
	ctx->opsPos[f->nops] = BUF_POS();

	// Function epilogue - offset-based for selective NOP patching (Phase 2)
	// MOV SP, X29  ; Restore stack pointer to frame pointer
	mov_reg_reg(ctx, SP_REG, FP, true);

	// Restore FP/LR from bottom (NOT NOPpable - always needed)
	ldp_offset(ctx, FP, LR, SP_REG, 0);  // LDP X29, X30, [SP, #0]

	// Restore callee-saved - record positions for potential NOPping
	ctx->ldp_positions[3] = BUF_POS();
	ldp_offset(ctx, X19, X20, SP_REG, 16);  // LDP X19, X20, [SP, #16]

	ctx->ldp_positions[2] = BUF_POS();
	ldp_offset(ctx, X21, X22, SP_REG, 32);  // LDP X21, X22, [SP, #32]

	ctx->ldp_positions[1] = BUF_POS();
	ldp_offset(ctx, X23, X24, SP_REG, 48);  // LDP X23, X24, [SP, #48]

	ctx->ldp_positions[0] = BUF_POS();
	ldp_offset(ctx, X25, X26, SP_REG, 64);  // LDP X25, X26, [SP, #64]

	// Restore RTMP/RTMP2 (X27, X28)
	ldp_offset(ctx, RTMP, RTMP2, SP_REG, 80); // LDP X27, X28, [SP, #80]

	// Deallocate callee-saved frame
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 96, SP_REG, SP_REG);  // ADD SP, SP, #96

	// RET  ; Return (BR X30)
	encode_branch_reg(ctx, 0x02, LR);

	// Patch jumps for this function NOW, while ctx->opsPos is still valid
	// (x86 does the same at the end of each function)
	{
		jlist *j = ctx->jumps;
		while (j != NULL) {
			int target_pos = ctx->opsPos[j->target];
			patch_jump(ctx, j->pos, target_pos);
			j = j->next;
		}
		ctx->jumps = NULL;
	}

	// Phase 2: Backpatch unused callee-saved register saves/restores to NOPs
	// Each STP/LDP handles a pair: [0]=X25,X26  [1]=X23,X24  [2]=X21,X22  [3]=X19,X20
	// Bitmap bits: 0,1=X19,X20  2,3=X21,X22  4,5=X23,X24  6,7=X25,X26
	{
		int i;
		for (i = 0; i < 4; i++) {
			int pair_mask = 3 << ((3-i) * 2);  // stp[0]->bits 6,7, stp[3]->bits 0,1
			if (!(ctx->callee_saved_used & pair_mask)) {
				// Neither register in pair was used - NOP both save and restore
				unsigned int *stp_code = (unsigned int*)(ctx->startBuf + ctx->stp_positions[i]);
				unsigned int *ldp_code = (unsigned int*)(ctx->startBuf + ctx->ldp_positions[i]);
				*stp_code = 0xD503201F;  // NOP
				*ldp_code = 0xD503201F;  // NOP
			}
		}
	}

	// Save debug info for this function
	if (ctx->debug) {
		int fid = (int)(f - m->code->functions);
		ctx->debug[fid].start = codePos;
		ctx->debug[fid].offsets = debug32 ? (void*)debug32 : (void*)debug16;
		ctx->debug[fid].large = debug32 != NULL;
	}

	return codePos;
}

static void missing_closure() {
	hl_error("Missing static closure");
}

/**
 * Write debug information to an external file for use with GDB.
 * Triggered by HL_JIT_DEBUG_FILE environment variable.
 * Set to "1" for default path (/tmp/hl-debug/jit-debug.txt) or a custom path.
 */
static void write_jit_debug_file(jit_ctx *ctx, hl_module *m, void *code, int code_size) {
	const char *path = getenv("HL_JIT_DEBUG_FILE");
	if (!path) return;

	// Use default path if set to "1"
	char default_path[256];
	if (strcmp(path, "1") == 0) {
		snprintf(default_path, sizeof(default_path), "/tmp/hl-debug/jit-debug.txt");
		path = default_path;
	}

	FILE *fp = fopen(path, "w");
	if (!fp) {
		fprintf(stderr, "Warning: Could not open JIT debug file: %s\n", path);
		return;
	}

	fprintf(fp, "# HashLink JIT Debug Map\n");
	fprintf(fp, "# JIT Base: %p  Size: 0x%x\n\n", code, code_size);

	for (int i = 0; i < m->code->nfunctions; i++) {
		hl_function *f = &m->code->functions[i];
		hl_debug_infos *dbg = &ctx->debug[i];

		// Skip if no debug info for this function
		if (!dbg->offsets) continue;

		// Function header
		fprintf(fp, "# F%d\n", f->findex);

		unsigned char *func_base = (unsigned char*)code + dbg->start;
		int end_offset = dbg->large ?
			((int*)dbg->offsets)[f->nops] :
			((unsigned short*)dbg->offsets)[f->nops];
		fprintf(fp, "# Range: %p - %p  Opcodes: %d\n",
				(void*)func_base, (void*)(func_base + end_offset), f->nops);

		// Per-opcode entries
		for (int op = 0; op < f->nops; op++) {
			int offset = dbg->large ?
				((int*)dbg->offsets)[op] :
				((unsigned short*)dbg->offsets)[op];
			void *addr = func_base + offset;

			// Get source location if available
			const char *file = "?";
			int line = 0;
			if (f->debug && m->code->debugfiles) {
				int file_idx = f->debug[op * 2];
				line = f->debug[op * 2 + 1];
				if (file_idx >= 0 && file_idx < m->code->ndebugfiles)
					file = m->code->debugfiles[file_idx];
			}

			fprintf(fp, "%p  F%d:%-4d %-16s %s:%d\n",
					addr, f->findex, op,
					hl_op_name(f->ops[op].op),
					file, line);
		}
		fprintf(fp, "\n");
	}

	fclose(fp);
	fprintf(stderr, "JIT debug info written to: %s\n", path);
}

void *hl_jit_code(jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous) {
	int code_size = BUF_POS();
	unsigned char *code;
	jlist *j;
    unsigned int *insn_ptr;
    unsigned int insn;

	// Round up code size to page boundary for memory allocation
	int alloc_size = (code_size + 4095) & ~4095;

	// Note: Jump patching is now done at the end of each function in jit_function()
	// This ensures ctx->opsPos contains the correct positions for each function's jumps

	// Allocate executable memory
	code = (unsigned char*)hl_alloc_executable_memory(alloc_size);
	if (code == NULL) {
		printf("JIT Error: Failed to allocate executable memory (%d bytes)\n", alloc_size);
		return NULL;
	}

	// Make JIT memory writable (Apple Silicon W^X)
	hl_jit_write_protect(false);

	// Copy generated code to executable memory (with jumps already patched)
	memcpy(code, ctx->startBuf, code_size);

	// Set up C↔HL trampolines and callbacks
	if (!call_jit_c2hl) {
		call_jit_c2hl = code + ctx->c2hl;
		call_jit_hl2c = code + ctx->hl2c;
		hl_setup.get_wrapper = get_wrapper;
		hl_setup.static_call = callback_c2hl;
		hl_setup.static_call_ref = true;
	}

	// Patch function calls and closures
	j = ctx->calls;
	while (j != NULL) {
		int findex = j->target;
		void *fabs;

		// Handle special markers
		if (findex == -2) {
			// OAssert: patch to hl_assert runtime function
			fabs = (void*)hl_assert;
			goto do_patch;
		}

		void *target_addr = ctx->m->functions_ptrs[findex];

		// Resolve function address
		// Only JIT functions should be in ctx->calls (native functions are handled at compile time)
		// For JIT functions, functions_ptrs contains a relative offset into the code
		if (target_addr == NULL) {
			// Try to read absolute address from previous module (hot reload)
			if (previous != NULL && previous->code != NULL) {
				int old_idx = m->hash->functions_hashes[m->functions_indexes[findex]];
				if (old_idx < 0) {
					printf("JIT Error: NULL function pointer at index %d - compilation failed\n", findex);
					return NULL;
				}
				fabs = previous->functions_ptrs[(previous->code->functions + old_idx)->findex];
			} else {
				printf("JIT Error: NULL function pointer at index %d - compilation failed\n", findex);
				return NULL;
			}
		} else {
			// JIT function - target_addr is a relative offset, convert to absolute
			fabs = (unsigned char*)code + (int_val)target_addr;
		}

	do_patch:
		// Check what kind of patching we need to do
		insn_ptr = (unsigned int*)(code + j->pos);
		insn = *insn_ptr;

		if ((insn & 0xFC000000) == 0x94000000) {
			// BL instruction - patch with relative offset
			if (fabs != NULL) {
				int64_t call_addr = (int64_t)(code + j->pos);
				int64_t target = (int64_t)fabs;
				int64_t offset = target - call_addr;
				int insn_offset = offset / 4;

				// Check if offset fits in 26 bits
				if (insn_offset >= -(1 << 25) && insn_offset < (1 << 25)) {
					*insn_ptr = 0x94000000 | (insn_offset & 0x03FFFFFF);
				} else {
					printf("JIT Warning: Call offset too large for direct BL at position %d\n", j->pos);
				}
			}
		} else {
			// 64-bit literal - patch with absolute address
			uint64_t *literal_ptr = (uint64_t*)(code + j->pos);
			*literal_ptr = (uint64_t)fabs;
		}

		j = j->next;
	}
	ctx->calls = NULL;

	// Patch closures
	{
		vclosure *c = ctx->closure_list;
		while (c != NULL) {
			vclosure *next;
			int fidx = (int)(int_val)c->fun;
			void *fabs = m->functions_ptrs[fidx];
			if (fabs == NULL) {
				// Try to read from previous module (hot reload)
				int old_idx = m->hash->functions_hashes[m->functions_indexes[fidx]];
				if (old_idx < 0) {
					// No previous version - set to error function
					fabs = missing_closure;
				} else if (previous != NULL) {
					fabs = previous->functions_ptrs[(previous->code->functions + old_idx)->findex];
				} else {
					fabs = missing_closure;
				}
			} else {
				// Convert relative offset to absolute address
				fabs = (unsigned char*)code + (int)(int_val)fabs;
			}
			c->fun = fabs;
			next = (vclosure*)c->value;
			c->value = NULL;
			c = next;
		}
		ctx->closure_list = NULL;
	}

	// CRITICAL: Flush instruction cache on ARM64
	// This ensures the CPU sees the newly written instructions
	// Without this, the CPU might execute stale cached instructions
#if defined(__GNUC__) || defined(__clang__)
	// Use GCC/Clang built-in for instruction cache flush
	__builtin___clear_cache((char*)code, (char*)(code + code_size));
#else
	// Fallback: manual cache flush (may not be available on all platforms)
	#warning "Instruction cache flush not implemented for this compiler"
#endif

	// Write debug file if requested via environment variable
	if (ctx->debug && m->code->hasdebug) {
		write_jit_debug_file(ctx, m, code, code_size);
	}

	// Flush instruction cache and make executable (Apple Silicon W^X)
	hl_jit_flush_cache(code, code_size);
	hl_jit_write_protect(true);

	// Set return values
	if (codesize)
		*codesize = code_size;

	if (debug) {
		*debug = ctx->debug;
	}

	return code;
}

void hl_jit_patch_method(void *old_fun, void **new_fun_table) {
	// Runtime method patching for hot reload
	// Overwrites the beginning of old_fun with a trampoline that:
	// 1. Loads the address of new_fun_table into X16
	// 2. Loads the function pointer from new_fun_table
	// 3. Branches to the new function
	//
	// Uses X16 (IP0) as it's designated for veneers/trampolines in AAPCS64
	// Sequence: MOVZ/MOVK to load address, LDR to dereference, BR to jump
	//
	// Total: 6 instructions = 24 bytes

	unsigned int *insn = (unsigned int *)old_fun;
	unsigned long long addr = (unsigned long long)(int_val)new_fun_table;

	// Make JIT memory writable (Apple Silicon W^X)
	hl_jit_write_protect(false);

	// MOVZ X16, #imm16 (bits 0-15)
	*insn++ = 0xD2800000 | (16) | (((addr >> 0) & 0xFFFF) << 5);

	// MOVK X16, #imm16, LSL #16 (bits 16-31)
	*insn++ = 0xF2A00000 | (16) | (((addr >> 16) & 0xFFFF) << 5);

	// MOVK X16, #imm16, LSL #32 (bits 32-47)
	*insn++ = 0xF2C00000 | (16) | (((addr >> 32) & 0xFFFF) << 5);

	// MOVK X16, #imm16, LSL #48 (bits 48-63)
	*insn++ = 0xF2E00000 | (16) | (((addr >> 48) & 0xFFFF) << 5);

	// LDR X16, [X16] - load function pointer from table
	*insn++ = 0xF9400210;  // LDR X16, [X16]

	// BR X16 - branch to function
	*insn++ = 0xD61F0200;  // BR X16

	// Flush instruction cache and make executable (Apple Silicon W^X)
	hl_jit_flush_cache(old_fun, 24);
	hl_jit_write_protect(true);
}
