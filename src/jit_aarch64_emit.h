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
#ifndef JIT_AARCH64_EMIT_H
#define JIT_AARCH64_EMIT_H

#include <stdbool.h>
#include <stdint.h>
#include "jit_common.h"

/*
 * AArch64 Register Definitions
 */

// General Purpose Registers (64-bit: X0-X30, 32-bit: W0-W30)
typedef enum {
	X0  = 0,  X1  = 1,  X2  = 2,  X3  = 3,
	X4  = 4,  X5  = 5,  X6  = 6,  X7  = 7,
	X8  = 8,  X9  = 9,  X10 = 10, X11 = 11,
	X12 = 12, X13 = 13, X14 = 14, X15 = 15,
	X16 = 16, X17 = 17, X18 = 18, X19 = 19,
	X20 = 20, X21 = 21, X22 = 22, X23 = 23,
	X24 = 24, X25 = 25, X26 = 26, X27 = 27,
	X28 = 28, X29 = 29, X30 = 30,

	// Special register names
	FP = 29,      // Frame Pointer (X29)
	LR = 30,      // Link Register (X30)
	SP_REG = 31,  // Stack Pointer (encoding value, context-dependent)
	XZR = 31      // Zero Register (encoding value, context-dependent)
} Arm64Reg;

// 32-bit register names (W registers)
typedef enum {
	W0  = 0,  W1  = 1,  W2  = 2,  W3  = 3,
	W4  = 4,  W5  = 5,  W6  = 6,  W7  = 7,
	W8  = 8,  W9  = 9,  W10 = 10, W11 = 11,
	W12 = 12, W13 = 13, W14 = 14, W15 = 15,
	W16 = 16, W17 = 17, W18 = 18, W19 = 19,
	W20 = 20, W21 = 21, W22 = 22, W23 = 23,
	W24 = 24, W25 = 25, W26 = 26, W27 = 27,
	W28 = 28, W29 = 29, W30 = 30,
	WZR = 31  // 32-bit zero register
} Arm64Reg32;

// Floating-Point/SIMD Registers
typedef enum {
	V0  = 0,  V1  = 1,  V2  = 2,  V3  = 3,
	V4  = 4,  V5  = 5,  V6  = 6,  V7  = 7,
	V8  = 8,  V9  = 9,  V10 = 10, V11 = 11,
	V12 = 12, V13 = 13, V14 = 14, V15 = 15,
	V16 = 16, V17 = 17, V18 = 18, V19 = 19,
	V20 = 20, V21 = 21, V22 = 22, V23 = 23,
	V24 = 24, V25 = 25, V26 = 26, V27 = 27,
	V28 = 28, V29 = 29, V30 = 30, V31 = 31
} Arm64FpReg;

// Aliases for specific precision
// D0-D31 = 64-bit (double precision) - same encoding as V0-V31
// S0-S31 = 32-bit (single precision) - same encoding as V0-V31
// H0-H31 = 16-bit (half precision) - same encoding as V0-V31

/*
 * Condition Codes for Conditional Branches and Selects
 */
typedef enum {
	COND_EQ = 0x0,  // Equal (Z == 1)
	COND_NE = 0x1,  // Not equal (Z == 0)
	COND_CS = 0x2,  // Carry set (C == 1), also HS (unsigned higher or same)
	COND_CC = 0x3,  // Carry clear (C == 0), also LO (unsigned lower)
	COND_MI = 0x4,  // Minus/negative (N == 1)
	COND_PL = 0x5,  // Plus/positive or zero (N == 0)
	COND_VS = 0x6,  // Overflow set (V == 1)
	COND_VC = 0x7,  // Overflow clear (V == 0)
	COND_HI = 0x8,  // Unsigned higher (C == 1 && Z == 0)
	COND_LS = 0x9,  // Unsigned lower or same (C == 0 || Z == 1)
	COND_GE = 0xA,  // Signed greater than or equal (N == V)
	COND_LT = 0xB,  // Signed less than (N != V)
	COND_GT = 0xC,  // Signed greater than (Z == 0 && N == V)
	COND_LE = 0xD,  // Signed less than or equal (Z == 1 || N != V)
	COND_AL = 0xE,  // Always (unconditional)
	COND_NV = 0xF   // Never (reserved, don't use)
} ArmCondition;

// Aliases
#define COND_HS COND_CS  // Unsigned higher or same
#define COND_LO COND_CC  // Unsigned lower

/*
 * Extend/Shift Types
 */
typedef enum {
	EXTEND_UXTB = 0,  // Unsigned extend byte
	EXTEND_UXTH = 1,  // Unsigned extend halfword
	EXTEND_UXTW = 2,  // Unsigned extend word
	EXTEND_UXTX = 3,  // Unsigned extend doubleword (64-bit, same as LSL)
	EXTEND_SXTB = 4,  // Signed extend byte
	EXTEND_SXTH = 5,  // Signed extend halfword
	EXTEND_SXTW = 6,  // Signed extend word
	EXTEND_SXTX = 7   // Signed extend doubleword
} ArmExtend;

typedef enum {
	SHIFT_LSL = 0,  // Logical shift left
	SHIFT_LSR = 1,  // Logical shift right
	SHIFT_ASR = 2,  // Arithmetic shift right
	SHIFT_ROR = 3   // Rotate right
} ArmShift;

/*
 * Function Declarations
 */

// ADD/SUB instructions
void encode_add_sub_imm(jit_ctx *ctx, int sf, int op, int S, int shift, int imm12, Arm64Reg Rn, Arm64Reg Rd);
void encode_add_sub_reg(jit_ctx *ctx, int sf, int op, int S, int shift, Arm64Reg Rm, int imm6, Arm64Reg Rn, Arm64Reg Rd);
void encode_add_sub_ext(jit_ctx *ctx, int sf, int op, int S, Arm64Reg Rm, int option, int imm3, Arm64Reg Rn, Arm64Reg Rd);

// Logical instructions
void encode_logical_imm(jit_ctx *ctx, int sf, int opc, int N, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd);
void encode_logical_reg(jit_ctx *ctx, int sf, int opc, int shift, int N, Arm64Reg Rm, int imm6, Arm64Reg Rn, Arm64Reg Rd);

// Move wide immediate
void encode_mov_wide_imm(jit_ctx *ctx, int sf, int opc, int hw, int imm16, Arm64Reg Rd);

// Multiply/divide
void encode_madd_msub(jit_ctx *ctx, int sf, int op, Arm64Reg Rm, Arm64Reg Ra, Arm64Reg Rn, Arm64Reg Rd);
void encode_div(jit_ctx *ctx, int sf, int U, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd);

// Shift instructions
void encode_shift_reg(jit_ctx *ctx, int sf, int op2, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd);

// Load/store instructions
void encode_ldr_str_imm(jit_ctx *ctx, int size, int V, int opc, int imm12, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldr_str_reg(jit_ctx *ctx, int size, int V, int opc, Arm64Reg Rm, int option, int S, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldur_stur(jit_ctx *ctx, int size, int V, int opc, int imm9, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldp_stp(jit_ctx *ctx, int opc, int V, int mode, int imm7, Arm64Reg Rt2, Arm64Reg Rn, Arm64Reg Rt);

// PC-relative addressing
void encode_adrp(jit_ctx *ctx, int immlo, int immhi, Arm64Reg Rd);
void encode_adr(jit_ctx *ctx, int immlo, int immhi, Arm64Reg Rd);

// Branch instructions
void encode_branch_cond(jit_ctx *ctx, int imm19, ArmCondition cond);
void encode_branch_uncond(jit_ctx *ctx, int imm26);
void encode_branch_link(jit_ctx *ctx, int imm26);
void encode_branch_reg(jit_ctx *ctx, int opc, Arm64Reg Rn);
void encode_cbz_cbnz(jit_ctx *ctx, int sf, int op, int imm19, Arm64Reg Rt);
void encode_tbz_tbnz(jit_ctx *ctx, int b5, int op, int b40, int imm14, Arm64Reg Rt);

// Floating-point instructions
void encode_fp_arith(jit_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int opcode, Arm64FpReg Rn, Arm64FpReg Rd);
void encode_fp_1src(jit_ctx *ctx, int M, int S, int type, int opcode, Arm64FpReg Rn, Arm64FpReg Rd);
void encode_fp_compare(jit_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int op, Arm64FpReg Rn);
void encode_fcvt_int(jit_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64FpReg Rn, Arm64Reg Rd);
void encode_int_fcvt(jit_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64Reg Rn, Arm64FpReg Rd);

// Conditional select
void encode_cond_select(jit_ctx *ctx, int sf, int op, Arm64Reg Rm, ArmCondition cond, int op2, Arm64Reg Rn, Arm64Reg Rd);

// High-level helpers
void load_immediate(jit_ctx *ctx, int64_t val, Arm64Reg dst, bool is_64bit);

#endif // JIT_AARCH64_EMIT_H
