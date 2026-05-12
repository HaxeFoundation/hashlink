/*
 * Copyright (C)2026 Haxe Foundation
 *
 * AArch64 instruction encoder for the HashLink JIT backend.
 *
 * All emitters write a single 32-bit little-endian instruction word into the
 * jit_ctx buffer via the W() macro inherited from jit_arm64.c. They never
 * touch the buffer pointer outside of W(), so each call advances by exactly
 * 4 bytes — which the rest of the backend relies on for patch offsets.
 *
 * Register naming convention used here:
 *   - x0..x30 are the 64-bit GPRs; w0..w30 are their 32-bit views.
 *   - xzr / wzr (encoding 31) is the read-as-zero / discard register; it
 *     aliases sp in some instruction forms — every emitter that depends on
 *     the distinction documents it.
 *   - v0..v31 are SIMD/FP registers; d0..d31 / s0..s31 are 64/32-bit views.
 *   - sp is encoding 31 in the addressing forms that accept it.
 *
 * References: ARM ARM (DDI 0487), Apple aarch64 ABI, AAPCS64.
 */

#ifndef JIT_ARM64_H
#define JIT_ARM64_H

#include <stdint.h>

// Reserved registers in our codegen convention:
//   x16, x17 — IP0/IP1, free for the JIT to use as scratch for veneers/patches
//   x18      — platform register, never touch on Darwin (reserved by the OS)
//   x29 (fp) — frame pointer, set up in prologue
//   x30 (lr) — link register, saved across calls
//   sp       — 16-byte aligned at every public boundary
//
// We pick x16/x17 as our internal scratches and x9..x15 as caller-saved
// temporaries that the register allocator can hand out freely.

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

// Emitters take the jit context as opaque to keep this header free of the
// full jit.c machinery; jit_arm64.c uses the W() macro internally.
struct _jit_ctx;
typedef struct _jit_ctx jit_ctx;

static inline void a64_emit( jit_ctx *ctx, uint32_t ins );

// ------------ Data-processing immediate -------------
// MOVZ Xd, #imm16, LSL #(shift*16) — zero-extend a 16-bit chunk.
void a64_movz( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
// MOVK Xd, #imm16, LSL #(shift*16) — keep other bits, overwrite the chunk.
void a64_movk( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
// MOVN Xd, #imm16, LSL #(shift*16) — bitwise-NOT of the chunk.
void a64_movn( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 );
// Materialise an arbitrary 64-bit constant in rd using a sequence of
// MOVZ + up-to-3 MOVK (or a single MOVN when that's shorter). Always 1-4
// instructions; never emits a literal-pool load.
void a64_mov_imm64( jit_ctx *ctx, a64_greg rd, int64_t value );
void a64_mov_imm32( jit_ctx *ctx, a64_greg rd, int32_t value );

// ADD/SUB (immediate). imm12 must fit unsigned 12 bits; the shift form
// (LSL #12) is selected automatically by a64_add_imm() when needed.
void a64_add_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 );
void a64_sub_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 );

// ADD/SUB (shifted register). shift_amt is 0..63 (resp. 0..31 for sf=0).
void a64_add_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_sub_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

// MUL Xd, Xn, Xm  (alias of MADD Xd, Xn, Xm, XZR).
void a64_mul( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
// SDIV / UDIV.
void a64_sdiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_udiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

// Logical (shifted register): AND/ORR/EOR.
void a64_and_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_orr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_eor_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

// MOV (register) Xd, Xm  — encoded as ORR Xd, XZR, Xm.
void a64_mov_reg( jit_ctx *ctx, a64_greg rd, a64_greg rm, int sf64 );

// Shifts (register).
void a64_lsl_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_lsr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );
void a64_asr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

// CMP Xn, Xm   = SUBS XZR, Xn, Xm   (sets NZCV).
void a64_cmp_reg( jit_ctx *ctx, a64_greg rn, a64_greg rm, int sf64 );
// CMP Xn, #imm12.
void a64_cmp_imm( jit_ctx *ctx, a64_greg rn, int32_t imm, int sf64 );

// ------------ Loads / stores -------------
// LDR/STR (unsigned offset). offset is in bytes; must be a multiple of the
// access size (1/2/4/8) and fit 12 bits scaled. Use a64_ldr_off64() helper
// to emit ADD + LDR when out of range.
void a64_ldr_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/, int sign_extend );
void a64_str_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/ );
// LDP/STP (pre/post-indexed) — used in prologue/epilogue for FP/LR save.
void a64_stp_pre( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm );
void a64_ldp_post( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm );

// LDUR/STUR — signed unscaled imm9 [-256, +255]. Cleaner than LDR for the
// negative offsets we use to address vreg slots from FP. Caller falls back
// to "sub tmp, fp, #|imm|" + LDR when the offset escapes imm9 range.
void a64_ldur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/, int sign_extend );
void a64_stur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size /*1/2/4/8*/ );
void a64_ldur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );
void a64_stur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double );

// ------------ Branches -------------
// B / BL (unconditional, ±128 MiB range). offset_words is signed in
// instruction units (i.e. bytes >> 2). Returns the buffer position so the
// caller can patch it later via a64_patch_branch().
int a64_b( jit_ctx *ctx, int32_t offset_words );
int a64_bl( jit_ctx *ctx, int32_t offset_words );
// B.cond (±1 MiB range).
int a64_bcond( jit_ctx *ctx, a64_cond cond, int32_t offset_words );
// BR Xn / BLR Xn / RET (uses LR by convention).
void a64_br( jit_ctx *ctx, a64_greg rn );
void a64_blr( jit_ctx *ctx, a64_greg rn );
void a64_ret( jit_ctx *ctx );

// Patch a previously emitted B / BL / B.cond at `pos` (byte offset within
// the buffer) so it targets `target` (byte offset within the buffer).
// Fails (returns 0) if the offset overflows the instruction's reach; the
// caller is expected to insert a veneer in that case.
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
