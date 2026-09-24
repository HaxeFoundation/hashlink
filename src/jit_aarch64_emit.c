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

#if !defined(__aarch64__) && !defined(_M_ARM64)
#  error "This file is for AArch64 architecture only."
#endif

#include "jit_aarch64_emit.h"

#define BITS(val, start, len) (((unsigned int)(val) & ((1u << (len)) - 1)) << (start))
#define BIT(val, pos) (((unsigned int)(val) & 1) << (pos))

// ADD/SUB{S} Rd, Rn, #imm12{, LSL #12} ; Rn/Rd 31 = SP
void encode_add_sub_imm(code_ctx *ctx, int sf, int op, int S, int shift, int imm12, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) |
	                    BIT(op, 30) |
	                    BIT(S, 29) |
	                    BITS(0x22, 23, 6) |
	                    BIT(shift, 22) |
	                    BITS(imm12, 10, 12) |
	                    BITS(Rn, 5, 5) |
	                    BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ADD/SUB{S} Rd, Rn, Rm{, shift #imm6} ; reg 31 = ZR
void encode_add_sub_reg(code_ctx *ctx, int sf, int op, int S, int shift, Arm64Reg Rm,
                        int imm6, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(op, 30, 1) | BIT(S, 29) | BITS(0x0B, 24, 5) |
	                    BITS(shift, 22, 2) | BITS(Rm, 16, 5) | BITS(imm6, 10, 6) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ADD/SUB{S} Rd, Rn, Rm, extend {#imm3} ; Rn/Rd 31 = SP
void encode_add_sub_ext(code_ctx *ctx, int sf, int op, int S, Arm64Reg Rm,
                        int option, int imm3, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(op, 30, 1) | BIT(S, 29) | BITS(0x0B, 24, 5) |
	                    BITS(1, 21, 2) | BITS(Rm, 16, 5) | BITS(option, 13, 3) |
	                    BITS(imm3, 10, 3) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// AND/ORR/EOR/ANDS Rd, Rn, #bitmask (opc 0-3)
void encode_logical_imm(code_ctx *ctx, int sf, int opc, int N, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x24, 23, 6) | BIT(N, 22) |
	                    BITS(immr, 16, 6) | BITS(imms, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// AND/ORR/EOR/ANDS Rd, Rn, Rm{, shift #imm6} ; N=1 inverts Rm (BIC/ORN/EON/BICS)
void encode_logical_reg(code_ctx *ctx, int sf, int opc, int shift, int N, Arm64Reg Rm,
                        int imm6, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x0A, 24, 5) | BITS(shift, 22, 2) |
	                    BIT(N, 21) | BITS(Rm, 16, 5) | BITS(imm6, 10, 6) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// MOVN/MOVZ/MOVK Rd, #imm16, LSL #(hw*16) (opc 0/2/3)
void encode_mov_wide_imm(code_ctx *ctx, int sf, int opc, int hw, int imm16, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x25, 23, 6) |
	                    BITS(hw, 21, 2) | BITS(imm16, 5, 16) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// MADD/MSUB Rd, Rn, Rm, Ra : Rd = Ra +/- Rn*Rm (MUL with Ra=XZR)
void encode_madd_msub(code_ctx *ctx, int sf, int op, Arm64Reg Rm, Arm64Reg Ra, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(0xD8, 21, 8) | BITS(Rm, 16, 5) |
	                    BIT(op, 15) | BITS(Ra, 10, 5) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// UDIV/SDIV Rd, Rn, Rm (U=0 : UDIV, U=1 : SDIV)
void encode_div(code_ctx *ctx, int sf, int U, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(0xD6, 21, 8) | BITS(Rm, 16, 5) |
	                    BITS(0x2 | U, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// LSLV/LSRV/ASRV/RORV Rd, Rn, Rm (op2 0-3)
void encode_shift_reg(code_ctx *ctx, int sf, int op2, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(0xD6, 21, 8) | BITS(Rm, 16, 5) |
	                    BITS(0x08 | op2, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// LDR/STR Rt, [Rn, #imm12] ; imm12 is scaled by the access size
void encode_ldr_str_imm(code_ctx *ctx, int size, int V, int opc, int imm12, Arm64Reg Rn, Arm64Reg Rt) {
	unsigned int insn = BITS(size, 30, 2) |
	                    BITS(7, 27, 3) |
	                    BIT(V, 26) |
	                    BITS(1, 24, 2) |
	                    BITS(opc, 22, 2) |
	                    BITS(imm12, 10, 12) |
	                    BITS(Rn, 5, 5) |
	                    BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// LDR/STR Rt, [Rn, Rm{, extend {#amount}}] ; S=1 scales Rm by the access size
void encode_ldr_str_reg(code_ctx *ctx, int size, int V, int opc, Arm64Reg Rm,
                        int option, int S, Arm64Reg Rn, Arm64Reg Rt) {
	unsigned int insn = BITS(size, 30, 2) |
	                    BITS(7, 27, 3) |
	                    BIT(V, 26) |
	                    BITS(0, 24, 2) |
	                    BITS(opc, 22, 2) |
	                    BIT(1, 21) |
	                    BITS(Rm, 16, 5) |
	                    BITS(option, 13, 3) |
	                    BIT(S, 12) |
	                    BITS(2, 10, 2) |
	                    BITS(Rn, 5, 5) |
	                    BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// LDUR/STUR Rt, [Rn, #simm9] ; unscaled, -256..255
void encode_ldur_stur(code_ctx *ctx, int size, int V, int opc, int imm9, Arm64Reg Rn, Arm64Reg Rt) {
	unsigned int insn = BITS(size, 30, 2) |
	                    BITS(7, 27, 3) |
	                    BIT(V, 26) |
	                    BITS(0, 24, 2) |
	                    BITS(opc, 22, 2) |
	                    BIT(0, 21) |
	                    BITS(imm9 & 0x1FF, 12, 9) |
	                    BITS(0, 10, 2) |
	                    BITS(Rn, 5, 5) |
	                    BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// LDP/STP Rt, Rt2, [Rn, #imm7] ; imm7 is signed and scaled by the register size
// mode: 0x01 post-indexed load, 0x02 offset load, 0x03 pre-indexed store,
//       0x11 post-indexed store, 0x12 offset store, 0x13 pre-indexed store
void encode_ldp_stp(code_ctx *ctx, int opc, int V, int mode, int imm7,
                    Arm64Reg Rt2, Arm64Reg Rn, Arm64Reg Rt) {
	int addr_mode, L;

	if (mode & 0x10) {
		addr_mode = mode & 3;
		L = 0;
	} else if (mode == 0x03) {
		addr_mode = 3;
		L = 0;
	} else if (mode == 0x01) {
		addr_mode = 1;
		L = 1;
	} else {
		addr_mode = mode & 3;
		L = 1;
	}

	unsigned int insn = BITS(opc, 30, 2) |
	                    BITS(5, 27, 3) |
	                    BIT(V, 26) |
	                    BITS(addr_mode, 23, 2) |
	                    BIT(L, 22) |
	                    BITS(imm7, 15, 7) |
	                    BITS(Rt2, 10, 5) |
	                    BITS(Rn, 5, 5) |
	                    BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// ADRP Xd, #(imm21 << 12) ; imm21 = immhi:immlo, in 4KB pages
void encode_adrp(code_ctx *ctx, int immlo, int immhi, Arm64Reg Rd) {
	unsigned int insn = BITS(1, 31, 1) | BITS(immlo, 29, 2) | BITS(0x10, 24, 5) |
	                    BITS(immhi, 5, 19) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// B.cond #(imm19*4)
void encode_branch_cond(code_ctx *ctx, int imm19, ArmCondition cond) {
	unsigned int insn = BITS(0x54, 24, 8) | BITS(imm19, 5, 19) | BITS(cond, 0, 4);
	EMIT32(ctx, insn);
}

// B #(imm26*4)
void encode_branch_uncond(code_ctx *ctx, int imm26) {
	unsigned int insn = BITS(0x05, 26, 6) | BITS(imm26, 0, 26);
	EMIT32(ctx, insn);
}

// BL #(imm26*4)
void encode_branch_link(code_ctx *ctx, int imm26) {
	unsigned int insn = BITS(0x25, 26, 6) | BITS(imm26, 0, 26);
	EMIT32(ctx, insn);
}

// BR/BLR/RET Xn (opc 0/1/2)
void encode_branch_reg(code_ctx *ctx, int opc, Arm64Reg Rn) {
	unsigned int insn = BITS(0x6B0, 21, 11) | BITS(opc, 21, 2) |
	                    BITS(0x1F, 16, 5) | BITS(Rn, 5, 5);
	EMIT32(ctx, insn);
}

// CBZ/CBNZ Rt, #(imm19*4)
void encode_cbz_cbnz(code_ctx *ctx, int sf, int op, int imm19, Arm64Reg Rt) {
	unsigned int insn = BIT(sf, 31) | BITS(0x1A, 25, 6) | BIT(op, 24) |
	                    BITS(imm19, 5, 19) | BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// FMUL/FDIV/FADD/FSUB/FMAX/FMIN Vd, Vn, Vm (opcode 0-5) ; type 0=single, 1=double
void encode_fp_arith(code_ctx *ctx, int M, int S, int type, Arm64FpReg Rm,
                     int opcode, Arm64FpReg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) | BITS(Rm, 16, 5) |
	                    BITS(opcode, 12, 4) | BITS(2, 10, 2) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// FMOV/FABS/FNEG/FSQRT Vd, Vn (opcode 0-3)
void encode_fp_1src(code_ctx *ctx, int M, int S, int type, int opcode, Arm64FpReg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(opcode, 15, 6) | BITS(0x10, 10, 5) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// FCMP/FCMPE Vn, Vm (op 0/2)
void encode_fp_compare(code_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int op, Arm64FpReg Rn) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) | BITS(Rm, 16, 5) |
	                    BITS(op, 14, 2) | BITS(8, 10, 4) | BITS(Rn, 5, 5);
	EMIT32(ctx, insn);
}

// FCVT*/FMOV Rd, Vn ; rmode/opc select the conversion (FCVTZS: rmode=3, opc=0)
void encode_fcvt_int(code_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64FpReg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(rmode, 19, 2) | BITS(opc, 16, 3) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// SCVTF/UCVTF Vd, Rn (opc 2/3)
void encode_int_fcvt(code_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64Reg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(sf, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(rmode, 19, 2) | BITS(opc, 16, 3) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// FCSEL Vd, Vn, Vm, cond
void encode_fp_cond_select(code_ctx *ctx, int type, Arm64FpReg Rm, ArmCondition cond,
						   Arm64FpReg Rn, Arm64FpReg Rd) {
	unsigned int insn = BITS(0x1E, 24, 5) | BITS(type, 22, 2) | BIT(1, 21) |
	                    BITS(Rm, 16, 5) | BITS(cond, 12, 4) | BITS(3, 10, 2) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// CSEL/CSINC/CSINV/CSNEG Rd, Rn, Rm, cond
void encode_cond_select(code_ctx *ctx, int sf, int op, Arm64Reg Rm, ArmCondition cond,
                        int op2, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BIT(op, 30) | BITS(0xD4, 21, 8) |
	                    BITS(Rm, 16, 5) | BITS(cond, 12, 4) |
	                    BITS(op2, 10, 2) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

static inline uint64_t rotate_right_64(uint64_t val, int rotation) {
	return (val >> (rotation & 63)) | (val << ((-rotation) & 63));
}

// Bitmask immediates are a rotated run of ones replicated over a 2/4/8/16/32/64-bit
// element. Algorithm from https://dougallj.wordpress.com/2021/10/30/
static bool is_logical_immediate_64(uint64_t val, int *N, int *immr, int *imms) {
	if (val == 0 || ~val == 0)
		return false;

	// rotate the trailing ones away (ctzll(0) is undefined)
	uint64_t tmp = val & (val + 1);
	int rotation = (tmp == 0) ? 0 : __builtin_ctzll(tmp);
	uint64_t normalized = rotate_right_64(val, rotation);

	int zeroes = __builtin_clzll(normalized);
	int ones = __builtin_ctzll(~normalized);
	int size = zeroes + ones;

	// also rejects a size that is not a power of 2
	if (rotate_right_64(val, size) != val)
		return false;

	*immr = (-rotation) & (size - 1);
	*imms = ((-(size << 1)) | (ones - 1)) & 0x3f;
	*N = (size >> 6);

	return true;
}

// 32-bit ops require N=0
static bool is_logical_immediate_32(uint32_t val, int *N, int *immr, int *imms) {
	if (val == 0 || val == 0xFFFFFFFF)
		return false;

	uint64_t val64 = ((uint64_t)val << 32) | val;

	if (!is_logical_immediate_64(val64, N, immr, imms))
		return false;

	if (*N != 0)
		return false;

	return true;
}

void load_immediate(code_ctx *ctx, int64_t val, Arm64Reg dst, bool is_64bit) {
	int sf = is_64bit ? 1 : 0;

	if (val == 0) {
		// MOV Xd, XZR
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, dst);
		return;
	}

	if ((!is_64bit && val == 0xFFFFFFFF) || (is_64bit && val == -1LL)) {
		// MOVN Xd, #0
		encode_mov_wide_imm(ctx, sf, 0x00, 0, 0, dst);
		return;
	}

	if (val < 0 && val >= -65536) {
		// MOVN Xd, #~val
		encode_mov_wide_imm(ctx, sf, 0x00, 0, (int)(~val) & 0xFFFF, dst);
		return;
	}

	if (val > 0 && val <= 65535) {
		encode_mov_wide_imm(ctx, sf, 0x02, 0, (int)val, dst);
		return;
	}

	{
		int N, immr, imms;
		bool can_encode = is_64bit
			? is_logical_immediate_64((uint64_t)val, &N, &immr, &imms)
			: is_logical_immediate_32((uint32_t)val, &N, &immr, &imms);

		if (can_encode) {
			// ORR Xd, XZR, #imm
			encode_logical_imm(ctx, sf, 0x01, N, immr, imms, XZR, dst);
			return;
		}
	}

	uint64_t uval = (uint64_t)val;
	int hw0 = uval & 0xFFFF;
	int hw1 = (uval >> 16) & 0xFFFF;
	int hw2 = (uval >> 32) & 0xFFFF;
	int hw3 = (uval >> 48) & 0xFFFF;

	int nonzero_count = 0;
	if (hw0) nonzero_count++;
	if (hw1) nonzero_count++;
	if (is_64bit) {
		if (hw2) nonzero_count++;
		if (hw3) nonzero_count++;
	}

	// start with MOVN if more halfwords are 0xFFFF than non-zero
	int ones_count = 0;
	if (hw0 == 0xFFFF) ones_count++;
	if (hw1 == 0xFFFF) ones_count++;
	if (is_64bit) {
		if (hw2 == 0xFFFF) ones_count++;
		if (hw3 == 0xFFFF) ones_count++;
	}

	int total_hw = is_64bit ? 4 : 2;
	bool use_movn = (ones_count > nonzero_count);

	if (use_movn) {
		int first = 1;
		for (int i = 0; i < total_hw; i++) {
			int hw_val = (uval >> (i * 16)) & 0xFFFF;
			if (hw_val != 0xFFFF) {
				if (first) {
					encode_mov_wide_imm(ctx, sf, 0x00, i, (~hw_val) & 0xFFFF, dst);
					first = 0;
				} else {
					encode_mov_wide_imm(ctx, sf, 0x03, i, hw_val, dst);
				}
			}
		}
	} else {
		int first = 1;
		for (int i = 0; i < total_hw; i++) {
			int hw_val = (uval >> (i * 16)) & 0xFFFF;
			if (hw_val != 0) {
				if (first) {
					encode_mov_wide_imm(ctx, sf, 0x02, i, hw_val, dst);
					first = 0;
				} else {
					encode_mov_wide_imm(ctx, sf, 0x03, i, hw_val, dst);
				}
			}
		}
	}
}
