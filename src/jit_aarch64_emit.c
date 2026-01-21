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
 * AArch64 Instruction Encoding
 *
 * This file provides low-level instruction encoding functions for the AArch64
 * architecture. All instructions are 32-bit fixed width.
 *
 * References:
 * - ARM Architecture Reference Manual ARMv8 (ARM ARM)
 * - AArch64 Instruction Set Architecture
 */

#if !defined(__aarch64__) && !defined(_M_ARM64)
#  error "This file is for AArch64 architecture only."
#endif

#include "jit_aarch64_emit.h"

/*
 * Helper macros for bit field manipulation
 */
#define BITS(val, start, len) (((unsigned int)(val) & ((1u << (len)) - 1)) << (start))
#define BIT(val, pos) (((unsigned int)(val) & 1) << (pos))

// EMIT32 is defined in jit_common.h

// ============================================================================
// ADD/SUB Instructions
// ============================================================================

/**
 * Encode ADD/SUB (immediate) instruction
 * Format: ADD/SUB Xd, Xn, #imm12 [, LSL #shift]
 *
 * @param sf     1=64-bit, 0=32-bit
 * @param op     0=ADD, 1=SUB
 * @param S      1=set flags (ADDS/SUBS), 0=don't set flags
 * @param shift  0=LSL #0, 1=LSL #12
 * @param imm12  12-bit unsigned immediate
 * @param Rn     Source register (0-31, 31=SP)
 * @param Rd     Destination register (0-31, 31=SP)
 */
void encode_add_sub_imm(jit_ctx *ctx, int sf, int op, int S, int shift, int imm12, Arm64Reg Rn, Arm64Reg Rd) {
	// ADD/SUB (immediate) encoding:
	// [31] = sf, [30] = op (0=ADD, 1=SUB), [29] = S, [28:23] = 100010, [22] = sh
	// [21:10] = imm12, [9:5] = Rn, [4:0] = Rd
	unsigned int insn = BIT(sf, 31) |         // [31] = sf
	                    BIT(op, 30) |         // [30] = op
	                    BIT(S, 29) |          // [29] = S
	                    BITS(0x22, 23, 6) |   // [28:23] = 100010
	                    BIT(shift, 22) |      // [22] = sh
	                    BITS(imm12, 10, 12) | // [21:10] = imm12
	                    BITS(Rn, 5, 5) |      // [9:5] = Rn
	                    BITS(Rd, 0, 5);       // [4:0] = Rd
	EMIT32(ctx, insn);
}

/**
 * Encode ADD/SUB (shifted register) instruction
 * Format: ADD/SUB Xd, Xn, Xm [, shift #amount]
 *
 * @param sf     1=64-bit, 0=32-bit
 * @param op     0=ADD, 1=SUB
 * @param S      1=set flags, 0=don't set flags
 * @param shift  00=LSL, 01=LSR, 10=ASR
 * @param Rm     Second source register
 * @param imm6   Shift amount (0-63)
 * @param Rn     First source register
 * @param Rd     Destination register
 */
void encode_add_sub_reg(jit_ctx *ctx, int sf, int op, int S, int shift, Arm64Reg Rm,
                        int imm6, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(op, 30, 1) | BIT(S, 29) | BITS(0x0B, 24, 5) |
	                    BITS(shift, 22, 2) | BITS(Rm, 16, 5) | BITS(imm6, 10, 6) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode ADD/SUB (extended register) instruction
 * Format: ADD/SUB Xd, Xn, Wm, extend [#amount]
 *
 * @param sf      1=64-bit, 0=32-bit
 * @param op      0=ADD, 1=SUB
 * @param S       1=set flags, 0=don't set flags
 * @param Rm      Second source register
 * @param option  Extend type (UXTB=000, UXTH=001, UXTW=010, UXTX=011, SXTB=100, SXTH=101, SXTW=110, SXTX=111)
 * @param imm3    Shift amount (0-4)
 * @param Rn      First source register
 * @param Rd      Destination register
 */
void encode_add_sub_ext(jit_ctx *ctx, int sf, int op, int S, Arm64Reg Rm,
                        int option, int imm3, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(op, 30, 1) | BIT(S, 29) | BITS(0x0B, 24, 5) |
	                    BITS(1, 21, 2) | BITS(Rm, 16, 5) | BITS(option, 13, 3) |
	                    BITS(imm3, 10, 3) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Logical Instructions
// ============================================================================

/**
 * Encode Logical (immediate) instruction
 * Format: AND/ORR/EOR/ANDS Xd, Xn, #imm
 *
 * @param sf    1=64-bit, 0=32-bit
 * @param opc   00=AND, 01=ORR, 10=EOR, 11=ANDS
 * @param N     Immediate encoding parameter
 * @param immr  Immediate encoding parameter (rotation)
 * @param imms  Immediate encoding parameter (size)
 * @param Rn    Source register
 * @param Rd    Destination register
 */
void encode_logical_imm(jit_ctx *ctx, int sf, int opc, int N, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x24, 23, 6) | BIT(N, 22) |
	                    BITS(immr, 16, 6) | BITS(imms, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode Logical (shifted register) instruction
 * Format: AND/ORR/EOR/ANDS Xd, Xn, Xm [, shift #amount]
 *
 * @param sf     1=64-bit, 0=32-bit
 * @param opc    00=AND, 01=ORR, 10=EOR, 11=ANDS
 * @param shift  00=LSL, 01=LSR, 10=ASR, 11=ROR
 * @param N      Must be 0 for regular logical ops
 * @param Rm     Second source register
 * @param imm6   Shift amount
 * @param Rn     First source register
 * @param Rd     Destination register
 */
void encode_logical_reg(jit_ctx *ctx, int sf, int opc, int shift, int N, Arm64Reg Rm,
                        int imm6, Arm64Reg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x0A, 24, 5) | BITS(shift, 22, 2) |
	                    BIT(N, 21) | BITS(Rm, 16, 5) | BITS(imm6, 10, 6) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Move Wide (immediate) Instructions
// ============================================================================

/**
 * Encode MOVZ/MOVN/MOVK instruction
 * Format: MOVZ/MOVN/MOVK Xd, #imm16 [, LSL #shift]
 *
 * @param sf    1=64-bit, 0=32-bit
 * @param opc   10=MOVZ, 00=MOVN, 11=MOVK
 * @param hw    Hardware position (0-3 for 64-bit, 0-1 for 32-bit) - selects 16-bit field
 * @param imm16 16-bit immediate value
 * @param Rd    Destination register
 */
void encode_mov_wide_imm(jit_ctx *ctx, int sf, int opc, int hw, int imm16, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BITS(opc, 29, 2) | BITS(0x25, 23, 6) |
	                    BITS(hw, 21, 2) | BITS(imm16, 5, 16) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Multiply Instructions
// ============================================================================

/**
 * Encode MADD/MSUB instruction (multiply-add/subtract)
 * Format: MADD Xd, Xn, Xm, Xa  (Xd = Xa + Xn*Xm)
 *         MSUB Xd, Xn, Xm, Xa  (Xd = Xa - Xn*Xm)
 *
 * @param sf  1=64-bit, 0=32-bit
 * @param op  0=MADD, 1=MSUB
 * @param Rm  Second multiplicand
 * @param Ra  Addend/subtrahend (use XZR for simple MUL)
 * @param Rn  First multiplicand
 * @param Rd  Destination
 */
void encode_madd_msub(jit_ctx *ctx, int sf, int op, Arm64Reg Rm, Arm64Reg Ra, Arm64Reg Rn, Arm64Reg Rd) {
	// MADD/MSUB encoding: [31]=sf, [30:29]=00, [28:24]=11011, [23:21]=000, [20:16]=Rm
	// [15]=op (0=MADD, 1=MSUB), [14:10]=Ra, [9:5]=Rn, [4:0]=Rd
	unsigned int insn = BIT(sf, 31) | BITS(0xD8, 21, 8) | BITS(Rm, 16, 5) |
	                    BIT(op, 15) | BITS(Ra, 10, 5) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode SDIV/UDIV instruction
 * Format: SDIV/UDIV Xd, Xn, Xm
 *
 * @param sf  1=64-bit, 0=32-bit
 * @param U   0=SDIV (signed), 1=UDIV (unsigned)
 * @param Rm  Divisor
 * @param Rn  Dividend
 * @param Rd  Destination (quotient)
 */
void encode_div(jit_ctx *ctx, int sf, int U, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd) {
	// SDIV/UDIV encoding: [31]=sf, [30:29]=00, [28:21]=11010110, [20:16]=Rm
	// [15:11]=00001, [10]=U (1=SDIV, 0=UDIV), [9:5]=Rn, [4:0]=Rd
	unsigned int insn = BIT(sf, 31) | BITS(0xD6, 21, 8) | BITS(Rm, 16, 5) |
	                    BITS(0x2 | U, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Shift Instructions
// ============================================================================

/**
 * Encode variable shift (LSLV/LSRV/ASRV/RORV)
 * Format: LSL/LSR/ASR/ROR Xd, Xn, Xm
 *
 * @param sf   1=64-bit, 0=32-bit
 * @param op2  00=LSLV, 01=LSRV, 10=ASRV, 11=RORV
 * @param Rm   Shift amount register
 * @param Rn   Source register
 * @param Rd   Destination register
 */
void encode_shift_reg(jit_ctx *ctx, int sf, int op2, Arm64Reg Rm, Arm64Reg Rn, Arm64Reg Rd) {
	// LSLV/LSRV/ASRV/RORV encoding: [31]=sf, [30:29]=00, [28:21]=11010110, [20:16]=Rm
	// [15:12]=0010, [11:10]=op2, [9:5]=Rn, [4:0]=Rd
	unsigned int insn = BIT(sf, 31) | BITS(0xD6, 21, 8) | BITS(Rm, 16, 5) |
	                    BITS(0x08 | op2, 10, 6) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Load/Store Instructions
// ============================================================================

/**
 * Encode LDR/STR (unsigned immediate offset)
 * Format: LDR/STR Xt, [Xn, #imm]
 *
 * @param size  00=8-bit, 01=16-bit, 10=32-bit, 11=64-bit
 * @param V     0=GPR, 1=FP/SIMD
 * @param opc   For V=0: 01=LDR, 00=STR, 10=LDRSW, 11=prfm
 * @param imm12 Unsigned 12-bit offset (scaled by size)
 * @param Rn    Base register
 * @param Rt    Source/destination register
 */
void encode_ldr_str_imm(jit_ctx *ctx, int size, int V, int opc, int imm12, Arm64Reg Rn, Arm64Reg Rt) {
	// LDR/STR (unsigned offset) encoding:
	// [31:30] = size, [29:27] = 111, [26] = V, [25:24] = 01, [23:22] = opc
	// [21:10] = imm12, [9:5] = Rn, [4:0] = Rt
	unsigned int insn = BITS(size, 30, 2) |   // [31:30] = size
	                    BITS(7, 27, 3) |      // [29:27] = 111
	                    BIT(V, 26) |          // [26] = V
	                    BITS(1, 24, 2) |      // [25:24] = 01 (unsigned offset)
	                    BITS(opc, 22, 2) |    // [23:22] = opc
	                    BITS(imm12, 10, 12) | // [21:10] = imm12
	                    BITS(Rn, 5, 5) |      // [9:5] = Rn
	                    BITS(Rt, 0, 5);       // [4:0] = Rt
	EMIT32(ctx, insn);
}

/**
 * Encode LDR/STR (register offset)
 * Format: LDR/STR Xt, [Xn, Xm{, extend {#amount}}]
 *
 * @param size    00=8-bit, 01=16-bit, 10=32-bit, 11=64-bit
 * @param V       0=GPR, 1=FP/SIMD
 * @param opc     For V=0: 01=LDR, 00=STR
 * @param Rm      Offset register
 * @param option  Extend type (010=UXTW, 011=LSL, 110=SXTW, 111=SXTX)
 * @param S       1=scale offset by size, 0=no scaling
 * @param Rn      Base register
 * @param Rt      Source/destination register
 */
void encode_ldr_str_reg(jit_ctx *ctx, int size, int V, int opc, Arm64Reg Rm,
                        int option, int S, Arm64Reg Rn, Arm64Reg Rt) {
	// LDR/STR (register offset) encoding:
	// [31:30] = size, [29:27] = 111, [26] = V, [25:24] = 00, [23:22] = opc
	// [21] = 1, [20:16] = Rm, [15:13] = option, [12] = S, [11:10] = 10
	// [9:5] = Rn, [4:0] = Rt
	unsigned int insn = BITS(size, 30, 2) |   // [31:30] = size
	                    BITS(7, 27, 3) |      // [29:27] = 111
	                    BIT(V, 26) |          // [26] = V
	                    BITS(0, 24, 2) |      // [25:24] = 00 (register offset)
	                    BITS(opc, 22, 2) |    // [23:22] = opc
	                    BIT(1, 21) |          // [21] = 1
	                    BITS(Rm, 16, 5) |     // [20:16] = Rm
	                    BITS(option, 13, 3) | // [15:13] = option
	                    BIT(S, 12) |          // [12] = S
	                    BITS(2, 10, 2) |      // [11:10] = 10
	                    BITS(Rn, 5, 5) |      // [9:5] = Rn
	                    BITS(Rt, 0, 5);       // [4:0] = Rt
	EMIT32(ctx, insn);
}

/**
 * Encode LDUR/STUR (unscaled signed offset)
 * Format: LDUR/STUR Rt, [Xn, #simm9]
 *
 * This instruction uses a signed 9-bit immediate offset (-256 to +255) that is
 * NOT scaled by the access size. This is ideal for accessing stack locals at
 * negative offsets from the frame pointer.
 *
 * @param size  00=8-bit, 01=16-bit, 10=32-bit, 11=64-bit
 * @param V     0=GPR, 1=FP/SIMD
 * @param opc   00=STUR, 01=LDUR
 * @param imm9  Signed 9-bit offset (-256 to +255), unscaled
 * @param Rn    Base register
 * @param Rt    Source/destination register
 */
void encode_ldur_stur(jit_ctx *ctx, int size, int V, int opc, int imm9, Arm64Reg Rn, Arm64Reg Rt) {
	// LDUR/STUR (unscaled offset) encoding:
	// [31:30] = size, [29:27] = 111, [26] = V, [25:24] = 00, [23:22] = opc
	// [21] = 0, [20:12] = imm9, [11:10] = 00, [9:5] = Rn, [4:0] = Rt
	unsigned int insn = BITS(size, 30, 2) |        // [31:30] = size
	                    BITS(7, 27, 3) |           // [29:27] = 111
	                    BIT(V, 26) |               // [26] = V
	                    BITS(0, 24, 2) |           // [25:24] = 00 (unscaled offset)
	                    BITS(opc, 22, 2) |         // [23:22] = opc
	                    BIT(0, 21) |               // [21] = 0
	                    BITS(imm9 & 0x1FF, 12, 9) | // [20:12] = imm9 (masked to 9 bits)
	                    BITS(0, 10, 2) |           // [11:10] = 00
	                    BITS(Rn, 5, 5) |           // [9:5] = Rn
	                    BITS(Rt, 0, 5);            // [4:0] = Rt
	EMIT32(ctx, insn);
}

/**
 * Encode LDP/STP (Load/Store Pair)
 * Format: LDP/STP Xt1, Xt2, [Xn, #imm]  (various addressing modes)
 *
 * @param opc   Size: 00=32-bit, 10=64-bit
 * @param V     0=GPR, 1=FP/SIMD registers
 * @param mode  Addressing mode + load/store:
 *              0x01 = post-indexed load  (LDP Xt1, Xt2, [Xn], #imm)
 *              0x03 = pre-indexed store  (STP Xt1, Xt2, [Xn, #imm]!)
 *              Other values: mode & 3 = addressing mode (1=post, 2=offset, 3=pre), L=1
 * @param imm7  Signed 7-bit offset (scaled by register size: *4 for 32-bit, *8 for 64-bit)
 * @param Rt2   Second register
 * @param Rn    Base register
 * @param Rt    First register
 *
 * ARM64 encoding:
 *   [31:30] = opc (size)
 *   [29:27] = 101 (fixed)
 *   [26]    = V
 *   [25:24] = addressing mode (01=post, 10=offset, 11=pre)
 *   [23]    = 0 (reserved)
 *   [22]    = L (0=store, 1=load)
 *   [21:15] = imm7
 *   [14:10] = Rt2
 *   [9:5]   = Rn
 *   [4:0]   = Rt
 */
void encode_ldp_stp(jit_ctx *ctx, int opc, int V, int mode, int imm7,
                    Arm64Reg Rt2, Arm64Reg Rn, Arm64Reg Rt) {
	int addr_mode, L;

	// Decode mode parameter to get addressing mode and load/store bit
	if (mode == 0x03) {
		// Pre-indexed store: STP Xt1, Xt2, [Xn, #imm]!
		addr_mode = 3;
		L = 0;
	} else if (mode == 0x01) {
		// Post-indexed load: LDP Xt1, Xt2, [Xn], #imm
		addr_mode = 1;
		L = 1;
	} else {
		// Default: use mode as addressing mode, assume load
		addr_mode = mode & 3;
		L = 1;
	}

	unsigned int insn = BITS(opc, 30, 2) |       // [31:30] = opc
	                    BITS(5, 27, 3) |         // [29:27] = 101
	                    BIT(V, 26) |             // [26] = V
	                    BITS(addr_mode, 23, 2) | // [24:23] = addressing mode
	                    BIT(L, 22) |             // [22] = L
	                    BITS(imm7, 15, 7) |      // [21:15] = imm7
	                    BITS(Rt2, 10, 5) |       // [14:10] = Rt2
	                    BITS(Rn, 5, 5) |         // [9:5] = Rn
	                    BITS(Rt, 0, 5);          // [4:0] = Rt
	EMIT32(ctx, insn);
}

// ============================================================================
// PC-Relative Addressing
// ============================================================================

/**
 * Encode ADRP instruction
 * Format: ADRP Xd, label  (load PC-relative page address)
 *
 * @param immlo  Low 2 bits of 21-bit offset (bits 0-1)
 * @param immhi  High 19 bits of 21-bit offset (bits 2-20)
 * @param Rd     Destination register
 *
 * Note: offset is in pages (4KB), so actual byte offset = imm21 << 12
 */
void encode_adrp(jit_ctx *ctx, int immlo, int immhi, Arm64Reg Rd) {
	unsigned int insn = BITS(1, 31, 1) | BITS(immlo, 29, 2) | BITS(0x10, 24, 5) |
	                    BITS(immhi, 5, 19) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode ADR instruction
 * Format: ADR Xd, label  (load PC-relative address)
 *
 * @param immlo  Low 2 bits of 21-bit offset
 * @param immhi  High 19 bits of 21-bit offset
 * @param Rd     Destination register
 */
void encode_adr(jit_ctx *ctx, int immlo, int immhi, Arm64Reg Rd) {
	unsigned int insn = BITS(0, 31, 1) | BITS(immlo, 29, 2) | BITS(0x10, 24, 5) |
	                    BITS(immhi, 5, 19) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Branch Instructions
// ============================================================================

/**
 * Encode conditional branch
 * Format: B.cond label
 *
 * @param imm19  Signed 19-bit offset (in instructions, i.e., offset/4)
 * @param cond   Condition code (0000=EQ, 0001=NE, 1010=GE, 1011=LT, etc.)
 */
void encode_branch_cond(jit_ctx *ctx, int imm19, ArmCondition cond) {
	unsigned int insn = BITS(0x54, 24, 8) | BITS(imm19, 5, 19) | BITS(cond, 0, 4);
	EMIT32(ctx, insn);
}

/**
 * Encode unconditional branch
 * Format: B label
 *
 * @param imm26  Signed 26-bit offset (in instructions, i.e., offset/4)
 */
void encode_branch_uncond(jit_ctx *ctx, int imm26) {
	unsigned int insn = BITS(0x05, 26, 6) | BITS(imm26, 0, 26);
	EMIT32(ctx, insn);
}

/**
 * Encode branch with link
 * Format: BL label
 *
 * @param imm26  Signed 26-bit offset (in instructions)
 */
void encode_branch_link(jit_ctx *ctx, int imm26) {
	unsigned int insn = BITS(0x25, 26, 6) | BITS(imm26, 0, 26);
	EMIT32(ctx, insn);
}

/**
 * Encode register branch instructions
 * Format: BR/BLR/RET Xn
 *
 * @param opc  00=BR, 01=BLR, 10=RET
 * @param Rn   Register containing target address (X30/LR for RET)
 */
void encode_branch_reg(jit_ctx *ctx, int opc, Arm64Reg Rn) {
	unsigned int insn = BITS(0x6B0, 21, 11) | BITS(opc, 21, 2) |
	                    BITS(0x1F, 16, 5) | BITS(Rn, 5, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode CBZ/CBNZ (compare and branch if zero/non-zero)
 * Format: CBZ/CBNZ Xt, label
 *
 * @param sf     1=64-bit, 0=32-bit
 * @param op     0=CBZ, 1=CBNZ
 * @param imm19  Signed 19-bit offset (in instructions)
 * @param Rt     Register to test
 */
void encode_cbz_cbnz(jit_ctx *ctx, int sf, int op, int imm19, Arm64Reg Rt) {
	unsigned int insn = BIT(sf, 31) | BITS(0x1A, 25, 6) | BIT(op, 24) |
	                    BITS(imm19, 5, 19) | BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode TBZ/TBNZ (test bit and branch if zero/non-zero)
 * Format: TBZ/TBNZ Xt, #bit, label
 *
 * @param b5     Bit 5 of bit position (0-63)
 * @param op     0=TBZ, 1=TBNZ
 * @param b40    Bits 4-0 of bit position
 * @param imm14  Signed 14-bit offset (in instructions)
 * @param Rt     Register to test
 */
void encode_tbz_tbnz(jit_ctx *ctx, int b5, int op, int b40, int imm14, Arm64Reg Rt) {
	unsigned int insn = BIT(b5, 31) | BITS(0x1B, 25, 6) | BIT(op, 24) |
	                    BITS(b40, 19, 5) | BITS(imm14, 5, 14) | BITS(Rt, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Floating-Point Instructions
// ============================================================================

/**
 * Encode floating-point arithmetic (2-source)
 * Format: FADD/FSUB/FMUL/FDIV/FMAX/FMIN Vd, Vn, Vm
 *
 * @param M       0=scalar, 1=vector
 * @param S       0=single precision, 1=double precision
 * @param type    00=single, 01=double
 * @param Rm      Second source register
 * @param opcode  0000=FMUL, 0001=FDIV, 0010=FADD, 0011=FSUB, 0100=FMAX, 0101=FMIN
 * @param Rn      First source register
 * @param Rd      Destination register
 */
void encode_fp_arith(jit_ctx *ctx, int M, int S, int type, Arm64FpReg Rm,
                     int opcode, Arm64FpReg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) | BITS(Rm, 16, 5) |
	                    BITS(opcode, 12, 4) | BITS(2, 10, 2) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode floating-point negate/abs/sqrt (1-source)
 * Format: FNEG/FABS/FSQRT Vd, Vn
 *
 * @param M       0=scalar, 1=vector
 * @param S       0=single precision, 1=double precision
 * @param type    00=single, 01=double
 * @param opcode  000000=FMOV, 000001=FABS, 000010=FNEG, 000011=FSQRT
 * @param Rn      Source register
 * @param Rd      Destination register
 */
void encode_fp_1src(jit_ctx *ctx, int M, int S, int type, int opcode, Arm64FpReg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(opcode, 15, 6) | BITS(0x10, 10, 5) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode floating-point compare
 * Format: FCMP/FCMPE Vn, Vm
 *
 * @param M     0=scalar
 * @param S     0=single precision, 1=double precision
 * @param type  00=single, 01=double
 * @param Rm    Second source register (or 0 for comparison with zero)
 * @param op    00=FCMP, 10=FCMPE (signal exception on qNaN)
 * @param Rn    First source register
 */
void encode_fp_compare(jit_ctx *ctx, int M, int S, int type, Arm64FpReg Rm, int op, Arm64FpReg Rn) {
	unsigned int insn = BIT(M, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) | BITS(Rm, 16, 5) |
	                    BITS(op, 14, 2) | BITS(8, 10, 4) | BITS(Rn, 5, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode floating-point conversion to integer
 * Format: FCVTZS/FCVTZU Xd, Vn
 *
 * @param sf    1=64-bit int, 0=32-bit int
 * @param S     0=single precision, 1=double precision
 * @param type  00=single, 01=double, 10/11=half
 * @param rmode 00=round to nearest, 01=round towards +inf, 10=round towards -inf, 11=round towards zero
 * @param opc   000=FCVTNS, 001=FCVTNU, 010=SCVTF, 011=UCVTF, 110=FMOV, 111=FMOV
 * @param Rn    Source FP register
 * @param Rd    Destination integer register
 */
void encode_fcvt_int(jit_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64FpReg Rn, Arm64Reg Rd) {
	unsigned int insn = BIT(sf, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(rmode, 19, 2) | BITS(opc, 16, 3) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

/**
 * Encode integer conversion to floating-point
 * Format: SCVTF/UCVTF Vd, Xn
 *
 * @param sf    1=64-bit int, 0=32-bit int
 * @param S     0=single precision, 1=double precision
 * @param type  00=single, 01=double
 * @param rmode 00 for conversions
 * @param opc   010=SCVTF, 011=UCVTF
 * @param Rn    Source integer register
 * @param Rd    Destination FP register
 */
void encode_int_fcvt(jit_ctx *ctx, int sf, int S, int type, int rmode, int opc, Arm64Reg Rn, Arm64FpReg Rd) {
	unsigned int insn = BIT(sf, 31) | BIT(S, 29) | BITS(0x1E, 24, 5) |
	                    BITS(type, 22, 2) | BITS(1, 21, 1) |
	                    BITS(rmode, 19, 2) | BITS(opc, 16, 3) |
	                    BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// Conditional Select
// ============================================================================

/**
 * Encode CSEL/CSINC/CSINV/CSNEG
 * Format: CSEL Xd, Xn, Xm, cond
 *
 * @param sf    1=64-bit, 0=32-bit
 * @param op    0=CSEL, 1=CSINC/CSINV/CSNEG (depends on op2)
 * @param Rm    Second source register
 * @param cond  Condition code
 * @param op2   00=CSEL, 01=CSINC, 10=CSINV, 11=CSNEG
 * @param Rn    First source register
 * @param Rd    Destination register
 */
void encode_cond_select(jit_ctx *ctx, int sf, int op, Arm64Reg Rm, ArmCondition cond,
                        int op2, Arm64Reg Rn, Arm64Reg Rd) {
	// CSEL/CSINC/CSINV/CSNEG encoding: [31]=sf, [30]=op, [29]=S=0, [28:21]=11010100
	// [20:16]=Rm, [15:12]=cond, [11:10]=op2, [9:5]=Rn, [4:0]=Rd
	unsigned int insn = BIT(sf, 31) | BIT(op, 30) | BITS(0xD4, 21, 8) |
	                    BITS(Rm, 16, 5) | BITS(cond, 12, 4) |
	                    BITS(op2, 10, 2) | BITS(Rn, 5, 5) | BITS(Rd, 0, 5);
	EMIT32(ctx, insn);
}

// ============================================================================
// High-Level Helper Functions
// ============================================================================

/**
 * Load an immediate value into a register
 * Uses MOVZ/MOVK sequence for multi-halfword values
 *
 * @param val       64-bit immediate value
 * @param dst       Destination register
 * @param is_64bit  true=64-bit register, false=32-bit register
 */
void load_immediate(jit_ctx *ctx, int64_t val, Arm64Reg dst, bool is_64bit) {
	int sf = is_64bit ? 1 : 0;

	// Special case: zero
	if (val == 0) {
		// MOV Xd, XZR (using ORR with XZR)
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, dst);
		return;
	}

	// Special case: all ones (for 32-bit: 0xFFFFFFFF, for 64-bit: 0xFFFFFFFFFFFFFFFF)
	if ((!is_64bit && val == 0xFFFFFFFF) || (is_64bit && val == -1LL)) {
		// MOVN Xd, #0
		encode_mov_wide_imm(ctx, sf, 0x00, 0, 0, dst);
		return;
	}

	// Special case: small negative values that fit in a single MOVN instruction
	// MOVN Xd, #imm16 produces ~imm16, which equals -(imm16+1)
	// So for values in range [-65536, -1], we can use a single MOVN
	// For 32-bit mode, sign extension is automatic
	if (val < 0 && val >= -65536) {
		// ~val gives us the immediate to use with MOVN
		// e.g., for val=-8: ~(-8) = 7, and MOVN Xd, #7 produces ~7 = -8
		encode_mov_wide_imm(ctx, sf, 0x00, 0, (int)(~val) & 0xFFFF, dst);
		return;
	}

	// Special case: small positive values that fit in a single MOVZ instruction
	if (val > 0 && val <= 65535) {
		encode_mov_wide_imm(ctx, sf, 0x02, 0, (int)val, dst);
		return;
	}

	// Count which halfwords are non-zero
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

	// Try MOVN (move inverted) if more halfwords are 0xFFFF than not
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
		// Use MOVN (inverted) + MOVK
		int first = 1;
		for (int i = 0; i < total_hw; i++) {
			int hw_val = (uval >> (i * 16)) & 0xFFFF;
			if (hw_val != 0xFFFF) {
				if (first) {
					// MOVN Xd, #(~hw_val & 0xFFFF), LSL #(i*16)
					encode_mov_wide_imm(ctx, sf, 0x00, i, (~hw_val) & 0xFFFF, dst);
					first = 0;
				} else {
					// MOVK Xd, #hw_val, LSL #(i*16)
					encode_mov_wide_imm(ctx, sf, 0x03, i, hw_val, dst);
				}
			}
		}
	} else {
		// Use MOVZ + MOVK
		int first = 1;
		for (int i = 0; i < total_hw; i++) {
			int hw_val = (uval >> (i * 16)) & 0xFFFF;
			if (hw_val != 0) {
				if (first) {
					// MOVZ Xd, #hw_val, LSL #(i*16)
					encode_mov_wide_imm(ctx, sf, 0x02, i, hw_val, dst);
					first = 0;
				} else {
					// MOVK Xd, #hw_val, LSL #(i*16)
					encode_mov_wide_imm(ctx, sf, 0x03, i, hw_val, dst);
				}
			}
		}
	}
}
