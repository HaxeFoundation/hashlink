/*
 * Copyright (C)2015-2026 Haxe Foundation
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
#include <hlmodule.h>
#include <jit.h>
#include "data_struct.h"

#define S_TYPE			byte_arr
#define S_NAME(name)	byte_##name
#define S_VALUE			unsigned char
#include "data_struct.c"
#define byte_reserve(set,count)	byte_reserve_impl(DEF_ALLOC,&set,count)

// constant table lookup
#define S_SORTED
#define S_MAP
#define S_TYPE			value_map
#define S_NAME(name)	value_map_##name
#define S_KEY			uint64
#define S_VALUE			int
#define S_DEFVAL		-1
#include "data_struct.c"
#undef S_MAP
#undef S_SORTED

struct _code_ctx {
	jit_ctx *jit;
	byte_arr code;
	int_arr branch_fixups; // (code_pos, target_op, is_cond)
	int_arr addr_fixups; // (adrp_pos, target_op) for PUSH_ADDR
	int *pos_map;
	int cur_op;
	bool flushed;
	int_arr funs; // (code_pos, fid, kind) kind 0 = BL, 1 = ADRP+ADD
	value_map const_table_lookup;
	byte_arr const_table;
	int_arr const_refs; // (adrp_pos, const_offset)
	int_arr const_addr; // (table_offs, code_pos) absolute addresses for jump tables
	int const_table_pos;
	int null_access_pos;
	int null_field_pos;
	bool long_cond; // emit B.cond as B.!cond +8 ; B target
	bool cond_overflow;
};

// caller must byte_reserve first
#define EMIT32(ctx, val) do { \
	*(unsigned int*)&(ctx)->code.values[(ctx)->code.cur] = (unsigned int)(val); \
	(ctx)->code.cur += 4; \
} while(0)

typedef enum {
	X0  = 0,  X1  = 1,  X2  = 2,  X3  = 3,
	X4  = 4,  X5  = 5,  X6  = 6,  X7  = 7,
	X8  = 8,  X9  = 9,  X10 = 10, X11 = 11,
	X12 = 12, X13 = 13, X14 = 14, X15 = 15,
	X16 = 16, X17 = 17, X18 = 18, X19 = 19,
	X20 = 20, X21 = 21, X22 = 22, X23 = 23,
	X24 = 24, X25 = 25, X26 = 26, X27 = 27,
	X28 = 28, X29 = 29, X30 = 30,

	FP = 29,
	LR = 30,
	SP_REG = 31,  // SP or ZR depending on the instruction
	XZR = 31
} Arm64Reg;

typedef enum {
	W0  = 0,  W1  = 1,  W2  = 2,  W3  = 3,
	W4  = 4,  W5  = 5,  W6  = 6,  W7  = 7,
	W8  = 8,  W9  = 9,  W10 = 10, W11 = 11,
	W12 = 12, W13 = 13, W14 = 14, W15 = 15,
	W16 = 16, W17 = 17, W18 = 18, W19 = 19,
	W20 = 20, W21 = 21, W22 = 22, W23 = 23,
	W24 = 24, W25 = 25, W26 = 26, W27 = 27,
	W28 = 28, W29 = 29, W30 = 30,
	WZR = 31
} Arm64Reg32;

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

typedef enum {
	COND_EQ = 0x0,
	COND_NE = 0x1,
	COND_CS = 0x2,  // HS
	COND_CC = 0x3,  // LO
	COND_MI = 0x4,
	COND_PL = 0x5,
	COND_VS = 0x6,
	COND_VC = 0x7,
	COND_HI = 0x8,  // unsigned >
	COND_LS = 0x9,  // unsigned <=
	COND_GE = 0xA,
	COND_LT = 0xB,
	COND_GT = 0xC,
	COND_LE = 0xD,
	COND_AL = 0xE,
	COND_NV = 0xF  // behaves as AL
} ArmCondition;

#define COND_HS COND_CS
#define COND_LO COND_CC

typedef enum {
	EXTEND_UXTB = 0,
	EXTEND_UXTH = 1,
	EXTEND_UXTW = 2,
	EXTEND_UXTX = 3,
	EXTEND_SXTB = 4,
	EXTEND_SXTH = 5,
	EXTEND_SXTW = 6,
	EXTEND_SXTX = 7
} ArmExtend;

typedef enum {
	SHIFT_LSL = 0,
	SHIFT_LSR = 1,
	SHIFT_ASR = 2,
	SHIFT_ROR = 3
} ArmShift;

void encode_add_sub_imm(code_ctx *ctx, int sf, int op, int S, int shift, int imm12, Arm64Reg Rn, Arm64Reg Rd);
void encode_add_sub_reg(code_ctx *ctx, int sf, int op, int S, int shift, Arm64Reg Rm, int imm6, Arm64Reg Rn, Arm64Reg Rd);
void encode_add_sub_ext(code_ctx *ctx, int sf, int op, int S, Arm64Reg Rm, int option, int imm3, Arm64Reg Rn, Arm64Reg Rd);

void encode_logical_imm(code_ctx *ctx, int sf, int opc, int N, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd);
void encode_logical_reg(code_ctx *ctx, int sf, int opc, int shift, int N, Arm64Reg Rm, int imm6, Arm64Reg Rn, Arm64Reg Rd);

void encode_mov_wide_imm(code_ctx *ctx, int sf, int opc, int hw, int imm16, Arm64Reg Rd);

void encode_madd_msub(code_ctx *ctx, int sf, int op, Arm64Reg Rm, Arm64Reg Ra, Arm64Reg Rn, Arm64Reg Rd);
void encode_div(code_ctx *ctx, int sf, int U, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd);

void encode_shift_reg(code_ctx *ctx, int sf, int op2, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd);

void encode_ldr_str_imm(code_ctx *ctx, int size, int V, int opc, int imm12, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldr_str_reg(code_ctx *ctx, int size, int V, int opc, Arm64Reg Rm, int option, int S, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldur_stur(code_ctx *ctx, int size, int V, int opc, int imm9, Arm64Reg Rn, Arm64Reg Rt);
void encode_ldp_stp(code_ctx *ctx, int opc, int V, int mode, int imm7, Arm64Reg Rt2, Arm64Reg Rn, Arm64Reg Rt);

void encode_adrp(code_ctx *ctx, int immlo, int immhi, Arm64Reg Rd);

void encode_branch_cond(code_ctx *ctx, int imm19, ArmCondition cond);
void encode_branch_uncond(code_ctx *ctx, int imm26);
void encode_branch_link(code_ctx *ctx, int imm26);
void encode_branch_reg(code_ctx *ctx, int opc, Arm64Reg Rn);
void encode_cbz_cbnz(code_ctx *ctx, int sf, int op, int imm19, Arm64Reg Rt);

void encode_fp_arith(code_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int opcode, Arm64FpReg Rn, Arm64FpReg Rd);
void encode_fp_1src(code_ctx *ctx, int M, int S, int type, int opcode, Arm64FpReg Rn, Arm64FpReg Rd);
void encode_fp_compare(code_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int op, Arm64FpReg Rn);
void encode_fcvt_int(code_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64FpReg Rn, Arm64Reg Rd);
void encode_int_fcvt(code_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64Reg Rn, Arm64FpReg Rd);
void encode_fp_cond_select(code_ctx *ctx, int type, Arm64FpReg Rm, ArmCondition cond, Arm64FpReg Rn, Arm64FpReg Rd);

void encode_cond_select(code_ctx *ctx, int sf, int op, Arm64Reg Rm, ArmCondition cond, int op2, Arm64Reg Rn, Arm64Reg Rd);

void load_immediate(code_ctx *ctx, int64_t val, Arm64Reg dst, bool is_64bit);

#endif // JIT_AARCH64_EMIT_H
