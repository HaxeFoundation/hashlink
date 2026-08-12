/*
 * Copyright (C)2026 Haxe Foundation
 *
 * AArch64 instruction encoder for the HashLink JIT backend. Each emitter
 * writes exactly one 32-bit instruction word via W(); reg 31 is xzr/wzr or
 * sp depending on the form.
 */

#ifndef JIT_ARM64_H
#define JIT_ARM64_H

#include <stdint.h>

// Reserved: x16/x17 (IP0/IP1) JIT scratch, x18 platform reg (never touch on
// Darwin), x29 fp, x30 lr, sp 16-byte aligned at public boundaries.

typedef enum {
	A64_X0 = 0,  A64_X1, A64_X2, A64_X3, A64_X4, A64_X5, A64_X6, A64_X7,
	A64_X8,      A64_X9, A64_X10, A64_X11, A64_X12, A64_X13, A64_X14, A64_X15,
	A64_X16,     A64_X17, A64_X18, A64_X19, A64_X20, A64_X21, A64_X22, A64_X23,
	A64_X24,     A64_X25, A64_X26, A64_X27, A64_X28, A64_FP /*x29*/,
	A64_LR /*x30*/, A64_SP_OR_ZR = 31
} a64_greg;

typedef enum {
	A64_V0 = 0,  A64_V1, A64_V2, A64_V3, A64_V4, A64_V5, A64_V6, A64_V7,
	A64_V8,      A64_V9, A64_V10, A64_V11, A64_V12, A64_V13, A64_V14, A64_V15,
	A64_V16,     A64_V17, A64_V18, A64_V19, A64_V20, A64_V21, A64_V22, A64_V23,
	A64_V24,     A64_V25, A64_V26, A64_V27, A64_V28, A64_V29, A64_V30, A64_V31
} a64_vreg;

// Condition codes (used by B.cond and CSEL family).
typedef enum {
	A64_EQ = 0x0, A64_NE = 0x1,
	A64_CS = 0x2, A64_CC = 0x3,  // unsigned >= / <
	A64_MI = 0x4, A64_PL = 0x5,
	A64_VS = 0x6, A64_VC = 0x7,
	A64_HI = 0x8, A64_LS = 0x9,  // unsigned > / <=
	A64_GE = 0xA, A64_LT = 0xB,  // signed
	A64_GT = 0xC, A64_LE = 0xD,
	A64_AL = 0xE  // always
} a64_cond;

struct _jit_ctx;
typedef struct _jit_ctx jit_ctx;

static inline void a64_emit( jit_ctx *ctx, uint32_t ins );

// ------------ Data-processing immediate -------------
void a64_movz( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
void a64_movk( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
void a64_movn( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
// materialise a constant via MOVZ+MOVK (or MOVN); 1-4 instrs, no literal pool
void a64_mov_imm64( jit_ctx *ctx, a64_greg rd, int64_t value );
void a64_mov_imm32( jit_ctx *ctx, a64_greg rd, int32_t value );

// imm12; LSL #12 form selected automatically
void a64_add_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 );
void a64_sub_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 );

void a64_add_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_sub_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

void a64_mul( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_sdiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_udiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

void a64_and_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_orr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_eor_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

void a64_mov_reg( jit_ctx *ctx, a64_greg rd, a64_greg rm, int sf64 );

void a64_lsl_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_lsr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_asr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

void a64_cmp_reg( jit_ctx *ctx, a64_greg rn, a64_greg rm, int sf64 );
void a64_cmp_imm( jit_ctx *ctx, a64_greg rn, int32_t imm, int sf64 );

// ------------ Loads / stores -------------
// LDR/STR unsigned offset: bytes, multiple of size, 12-bit scaled range
void a64_ldr_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/, int sign_extend );
void a64_str_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/ );
void a64_stp_pre( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm );
void a64_ldp_post( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm );

// LDUR/STUR: signed unscaled imm9 [-256,255], used for FP-relative vreg slots
void a64_ldur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/, int sign_extend );
void a64_stur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/ );
void a64_ldur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );
void a64_stur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );

// ------------ Branches -------------
// offset_words = bytes>>2; returns buffer pos for later a64_patch_branch()
int a64_b( jit_ctx *ctx, int32_t offset_words );
int a64_bl( jit_ctx *ctx, int32_t offset_words );
int a64_bcond( jit_ctx *ctx, a64_cond cond, int32_t offset_words );
void a64_br( jit_ctx *ctx, a64_greg rn );
void a64_blr( jit_ctx *ctx, a64_greg rn );
void a64_ret( jit_ctx *ctx );

// patch B/BL/B.cond at pos to target (both byte offsets); returns 0 if out of
// reach, caller must then insert a veneer
int a64_patch_branch( jit_ctx *ctx, int pos, int target );

// ------------ Floating-point -------------
void a64_fmov_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn );
void a64_fadd_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm );
void a64_fsub_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm );
void a64_fmul_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm );
void a64_fdiv_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm );
void a64_fcmp_d( jit_ctx *ctx, a64_vreg vn, a64_vreg vm );
void a64_scvtf_d( jit_ctx *ctx, a64_vreg vd, a64_greg rn, int sf64 );
void a64_fcvtzs_d( jit_ctx *ctx, a64_greg rd, a64_vreg vn, int sf64 );
void a64_ldr_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );
void a64_str_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );

#endif // JIT_ARM64_H
