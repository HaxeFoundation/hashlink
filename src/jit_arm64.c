/*
 * Copyright (C)2026 Haxe Foundation
 *
 * AArch64 JIT backend for HashLink.
 *
 * This file is the AArch64 counterpart of src/jit.c. It exports the same
 * public API (hl_jit_alloc/init/reset/function/code/free, hl_jit_patch_method)
 * but emits AArch64 instructions instead of x86. CMake selects exactly one of
 * jit.c or jit_arm64.c based on CMAKE_SYSTEM_PROCESSOR.
 *
 * Status: bring-up. Implemented opcodes: ONop, OLabel, OMov, OInt, ORet.
 * Everything else hits hl_jit_function()'s default branch and emits a BRK
 * (debug trap) followed by an error message — so unsupported programs crash
 * loudly instead of silently corrupting state.
 *
 * Convention de pile (frame layout, all offsets from FP=x29 after prologue):
 *   [FP +16]  caller's frame  (above us)
 *   [FP + 8]  saved LR
 *   [FP + 0]  saved FP   <- x29 points here
 *   [FP - 8]  vreg #0
 *   [FP - 16] vreg #1
 *   ...
 *   sp        16-byte aligned at every call boundary
 *
 * Convention d'appel: AAPCS64 (Apple variant on macOS — affects varargs
 * stack layout only, which we do not currently use for hl-to-c calls).
 *   - First 8 GPR args in x0..x7, first 8 FP args in d0..d7.
 *   - Return: x0 (int/ptr), d0 (float/double).
 *   - x9..x15, x16, x17 caller-saved temporaries.
 *   - x19..x28 callee-saved — we don't use them yet (single-pass codegen).
 */

#include <math.h>
#include <setjmp.h>
#include <stddef.h>
#include <hlmodule.h>
#include "hlsystem.h"
#include "jit_arm64.h"

#if !defined(__aarch64__)
#error "jit_arm64.c built on a non-AArch64 target; check the CMake gate."
#endif

// -----------------------------------------------------------------------
//  Forward declarations for the shared jit_ctx structure.
//
//  For the bring-up phase we keep the structure layout intentionally
//  simpler than the x86 backend's: a single byte buffer + per-function
//  metadata. Once register allocation lands we'll grow this to mirror
//  the vreg/preg machinery from jit.c.
// -----------------------------------------------------------------------

typedef struct jlist jlist;
struct jlist {
	int pos;      // buffer offset of the instruction to patch (bytes)
	int target;   // HL opcode index this branch targets
	jlist *next;
};

// Targets for staged HL-function references that need patching once
// functions_ptrs is populated. Stored in jlist.target with these tags:
//   - target >= 0           → BL @ jlist.pos  needs its imm26 patched
//   - target == -1000-fid   → 4xMOVZ/MOVK sequence @ jlist.pos materialises
//                              the absolute address of function `fid`
//                              (used by OInstanceClosure for x1).
#define CALL_TARGET_IS_BL(t)      ((t) >= 0)
#define CALL_TARGET_IS_IMM64(t)   ((t) <= -1000)
#define IMM64_FINDEX(t)           (-1000 - (t))
#define IMM64_TAG(fid)            (-1000 - (fid))

struct _jit_ctx {
	union {
		unsigned char *b;
		uint32_t *w;     // every aarch64 instruction is one 32-bit word
		int      *i;
		double   *d;
	} buf;
	unsigned char *startBuf;
	int bufSize;

	hl_module *m;
	hl_function *f;

	// Per-function state.
	int functionPos;     // buffer offset where the current function starts
	int totalRegsSize;   // size of the local frame in bytes (16-byte aligned)
	int *opsPos;         // opsPos[i] = buffer offset of HL opcode i

	jlist *jumps;        // pending intra-function branches
	jlist *calls;        // pending HL function calls (BL patches + imm64 patches)
	vclosure *closure_list; // OStaticClosure objects to patch in hl_jit_code

	hl_alloc falloc;     // per-function arena (reset between functions)
	hl_alloc galloc;     // global arena, lives the whole module
	hl_debug_infos *debug;

	// Trampoline offsets — recorded the first time we emit them so the
	// module setup can patch hl_setup.{static_call,get_wrapper}.
	int c2hl;
	int hl2c;
};

#define BUF_POS()  ((int)(ctx->buf.b - ctx->startBuf))
#define W(v)       (*ctx->buf.w++ = (uint32_t)(v))

static void jit_buf( jit_ctx *ctx ) {
	// Reserve room for at least 64 bytes (16 instructions); grow x2 when
	// running low. This mirrors the x86 backend's strategy.
	if( BUF_POS() + 64 < ctx->bufSize ) return;
	int newSize = ctx->bufSize ? ctx->bufSize * 2 : 4096;
	unsigned char *newBuf = (unsigned char*)malloc(newSize);
	memcpy(newBuf, ctx->startBuf, BUF_POS());
	int pos = BUF_POS();
	if( ctx->startBuf ) free(ctx->startBuf);
	ctx->startBuf = newBuf;
	ctx->buf.b = newBuf + pos;
	ctx->bufSize = newSize;
}

// -----------------------------------------------------------------------
//  Peephole register cache: skip redundant load_vreg/store_vreg around
//  consecutive opcodes that exchange data through the same physical reg.
//
//  Two single-slot caches (one GPR, one FP). They remember which HL vreg
//  is currently held in which physical reg. Every other emit clears the
//  cache (because we don't know what it wrote); store_vreg / load_vreg
//  re-populate the relevant slot after their own emit.
//
//  Correctness rests on: a64_emit clears the cache, so any instruction
//  that writes a register makes the cache miss next time. Jumps, calls,
//  labels, branches all go through a64_emit (or have explicit clears at
//  jump-target opcodes — see hl_jit_function's is_jump_target prescan).
// -----------------------------------------------------------------------

// Per-register ownership cache. reg_owner_gpr[r] is the vreg currently
// known to live in physical register r, or -1 if unknown/dead. Same for
// reg_owner_fp[v]. Replaces the old single-slot cache so multiple values
// can stay live in registers across encoders within a basic block.
//
// Updated by:
//   - load_vreg / store_vreg (set the owner after the LDR/STR)
//   - kill_gpr / kill_fp (called by encoders BEFORE their write so the
//     stale ownership doesn't survive)
//   - cache_reset (full wipe, called at branch targets and after calls)
static int8_t reg_owner_gpr[32];
static int8_t reg_owner_fp[32];

// Set HL_JIT_NO_CACHE=1 at runtime to disable the peephole cache —
// useful for A/B perf measurement and for confirming correctness when
// chasing a regression.
//
static int cache_disabled = -1;
static int is_cache_disabled( void ) {
	if( cache_disabled < 0 ) cache_disabled = (getenv("HL_JIT_NO_CACHE") != NULL) ? 1 : 0;
	return cache_disabled;
}

static inline void cache_reset( void ) {
	for( int i = 0; i < 32; i++ ) { reg_owner_gpr[i] = -1; reg_owner_fp[i] = -1; }
}
// Backwards-compat alias used by older code paths.
static inline void cache_clear( void ) { cache_reset(); }

// Mark a single GPR/FP as no longer holding any vreg. Called by encoders
// BEFORE they overwrite the register so stale ownership doesn't linger.
static inline void kill_gpr( a64_greg r ) {
	reg_owner_gpr[r & 0x1f] = -1;
}
static inline void kill_fp( a64_vreg v ) {
	reg_owner_fp[v & 0x1f] = -1;
}
// After a function call, every caller-saved register is clobbered.
// AAPCS64: x0-x17 are caller-saved. v0-v7 and v16-v31 are caller-saved
// (low 64 bits of v8-v15 are callee-saved, but we never touch them).
static inline void kill_caller_saved( void ) {
	for( int i = 0; i <= 17; i++ ) reg_owner_gpr[i] = -1;
	for( int i = 0; i <= 7; i++ )  reg_owner_fp[i]  = -1;
	for( int i = 16; i < 32; i++ ) reg_owner_fp[i]  = -1;
}
// After a write that updates memory[vi] OR a register holding vi's value,
// invalidate ALL OTHER registers (both banks) that claimed vi: their cached
// value is stale. Then mark the new owner.
//
// The cross-bank invalidation matters because some opcodes write a vreg via
// the integer path (e.g. OFloat stores the bit-pattern through X9) while
// other opcodes read it via the FP path. Without clearing the FP cache, a
// stale FP-side claim would skip the LDUR and use the old value.
static inline void claim_gpr( a64_greg r, int vi ) {
	if( vi < 0 ) return;
	for( int i = 0; i < 32; i++ ) {
		if( reg_owner_gpr[i] == vi ) reg_owner_gpr[i] = -1;
		if( reg_owner_fp[i]  == vi ) reg_owner_fp[i]  = -1;
	}
	reg_owner_gpr[r & 0x1f] = (int8_t)vi;
}
static inline void claim_fp( a64_vreg v, int vi ) {
	if( vi < 0 ) return;
	for( int i = 0; i < 32; i++ ) {
		if( reg_owner_gpr[i] == vi ) reg_owner_gpr[i] = -1;
		if( reg_owner_fp[i]  == vi ) reg_owner_fp[i]  = -1;
	}
	reg_owner_fp[v & 0x1f] = (int8_t)vi;
}
// Look up which physical register holds vi (or -1 if none).
static inline int find_gpr( int vi ) {
	if( vi < 0 || is_cache_disabled() ) return -1;
	for( int i = 0; i < 32; i++ ) if( reg_owner_gpr[i] == vi ) return i;
	return -1;
}
static inline int find_fp( int vi ) {
	if( vi < 0 || is_cache_disabled() ) return -1;
	for( int i = 0; i < 32; i++ ) if( reg_owner_fp[i] == vi ) return i;
	return -1;
}

// -----------------------------------------------------------------------
//  Encoder primitives — all 32-bit words, all written via W().
//  Naming and bit layouts follow the ARM ARM (DDI 0487) — comments cite
//  the encoding section so future readers can verify by hand.
// -----------------------------------------------------------------------

static inline void a64_emit( jit_ctx *ctx, uint32_t ins ) {
	jit_buf(ctx);
	W(ins);
	// No wholesale cache wipe here. Encoders that write a register call
	// kill_gpr/kill_fp(dst) BEFORE this emit so stale ownership is dropped.
	// Encoders that don't write any register (str_imm, branches, cmp) leave
	// the cache untouched.
}

// MOVZ / MOVK / MOVN — Move wide (immediate). C6.2.190/188/189.
// sf=1: 64-bit, sf=0: 32-bit. opc selects the variant.
static void mov_wide( jit_ctx *ctx, int opc, a64_greg rd, uint16_t imm16, int shift, int sf64 ) {
	// hw is the shift amount /16 (0..3 for 64-bit, 0..1 for 32-bit).
	int hw = shift & 3;
	uint32_t ins =
		((sf64 & 1) << 31) | ((opc & 3) << 29) | (0x25 << 23) |
		(hw << 21) | ((uint32_t)imm16 << 5) | (rd & 0x1f);
	// MOVK preserves bits, MOVZ/MOVN replace — but in either case the prior
	// owner of rd is gone after this insn. Only kill on the FIRST emit of a
	// chain (caller responsibility for chained MOVK).
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_movz( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 ) { mov_wide(ctx, 0x2, rd, imm16, shift, sf64); }
void a64_movk( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 ) { mov_wide(ctx, 0x3, rd, imm16, shift, sf64); }
void a64_movn( jit_ctx *ctx, a64_greg rd, uint16_t imm16, int shift, int sf64 ) { mov_wide(ctx, 0x0, rd, imm16, shift, sf64); }

void a64_mov_imm64( jit_ctx *ctx, a64_greg rd, int64_t value ) {
	// Strategy: pick the shorter of MOVZ+MOVK* and MOVN+MOVK*. MOVN wins
	// when v has more 0xFFFF chunks than v has 0 chunks (each 0xFFFF chunk
	// becomes a free instruction).
	uint64_t v = (uint64_t)value;
	int zeros = 0, ones = 0;
	for( int i = 0; i < 4; i++ ) {
		uint16_t c = (uint16_t)((v >> (i*16)) & 0xffff);
		if( c == 0 )      zeros++;
		if( c == 0xffff ) ones++;
	}
	int use_movn = (ones > zeros);
	int first = 1;
	for( int i = 0; i < 4; i++ ) {
		uint16_t chunk = (uint16_t)((v >> (i*16)) & 0xffff);
		if( use_movn ) {
			// Skip chunks where v=0xFFFF (already implicitly set by MOVN's
			// upper-bits-all-1 behavior), EXCEPT we still must emit the
			// MOVN itself somewhere. Pick the first such "non-trivial"
			// chunk; if none exist (value == -1) we emit MOVN #0 at i=0.
			if( first ) {
				a64_movn(ctx, rd, chunk ^ 0xffff, i, 1);
				first = 0;
			} else if( chunk != 0xffff ) {
				a64_movk(ctx, rd, chunk, i, 1);
			}
		} else {
			if( chunk == 0 ) continue;
			if( first ) { a64_movz(ctx, rd, chunk, i, 1); first = 0; }
			else        { a64_movk(ctx, rd, chunk, i, 1); }
		}
	}
	// All-zero or all-0xFFFF degenerate cases.
	if( first ) {
		if( use_movn ) a64_movn(ctx, rd, 0, 0, 1); // gives -1
		else           a64_movz(ctx, rd, 0, 0, 1); // gives 0
	}
}
void a64_mov_imm32( jit_ctx *ctx, a64_greg rd, int32_t value ) {
	// Same algorithm, restricted to two chunks.
	uint32_t v = (uint32_t)value;
	uint16_t lo = (uint16_t)(v & 0xffff);
	uint16_t hi = (uint16_t)((v >> 16) & 0xffff);
	if( lo == 0 && hi == 0 ) { a64_movz(ctx, rd, 0, 0, 0); return; }
	if( hi == 0 )            { a64_movz(ctx, rd, lo, 0, 0); return; }
	if( lo == 0 )            { a64_movz(ctx, rd, hi, 1, 0); return; }
	a64_movz(ctx, rd, lo, 0, 0);
	a64_movk(ctx, rd, hi, 1, 0);
}

// ADD/SUB (shifted register), shift = LSL #0. C6.2.5 / C6.2.343.
// Forward-declared so addsub_imm() can fall back to the register form when the
// immediate is too large for any 12-bit encoding.
static void addsub_reg( jit_ctx *ctx, int is_sub, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 );

// ADD/SUB (immediate). C6.2.4 / C6.2.342.
// Emit a single ADD/SUB (immediate) form, ARMv8 C6.2.4 / C6.2.342.
// imm MUST fit in 12 bits when sh=0, or be a 12-bit value shifted left 12 (sh=1).
// This is the raw encoder — addsub_imm() below splits large values for you.
static void addsub_imm_one( jit_ctx *ctx, int is_sub, a64_greg rd, a64_greg rn, uint32_t imm, int sh, int sf64 ) {
	uint32_t ins =
		((sf64 & 1) << 31) | ((is_sub & 1) << 30) | (0 << 29) | (0x11 << 24) |
		((sh & 1) << 22) | ((imm & 0xfff) << 10) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}

static void addsub_imm( jit_ctx *ctx, int is_sub, a64_greg rd, a64_greg rn, int32_t imm, int sf64 ) {
	if( imm < 0 ) { is_sub ^= 1; imm = -imm; }
	// Fast path: fits in 12 bits as-is.
	if( (imm & ~0xfff) == 0 ) {
		addsub_imm_one(ctx, is_sub, rd, rn, (uint32_t)imm, 0, sf64);
		return;
	}
	// Fast path: only the upper 12 bits are set (low 12 zero).
	if( (imm & 0xfff) == 0 && (((uint32_t)imm >> 12) & ~0xfff) == 0 ) {
		addsub_imm_one(ctx, is_sub, rd, rn, (uint32_t)imm >> 12, 1, sf64);
		return;
	}
	// Mixed bits: emit upper part with sh=1, then lower part with sh=0.
	// This handles any imm up to (4095<<12)+4095 = 0xFFFFFF (~16MB).
	if( (((uint32_t)imm >> 12) & ~0xfff) == 0 ) {
		addsub_imm_one(ctx, is_sub, rd, rn, (uint32_t)imm >> 12, 1, sf64);
		addsub_imm_one(ctx, is_sub, rd, rd, (uint32_t)imm & 0xfff, 0, sf64);
		return;
	}
	// Imm too large for the two-instruction sequence (>16MB). Materialise it
	// via MOVZ/MOVK into x16 then use ADD/SUB (shifted register, shift=0).
	// We deliberately clobber x16 (IP0); callers using x16 as a live value
	// must spill first — this is consistent with the rest of the backend.
	a64_mov_imm64(ctx, A64_X16, (int64_t)(uint32_t)imm);
	addsub_reg(ctx, is_sub, rd, rn, A64_X16, sf64);
}
void a64_add_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 ) { addsub_imm(ctx, 0, rd, rn, imm, sf64); }
void a64_sub_imm( jit_ctx *ctx, a64_greg rd, a64_greg rn, int32_t imm, int sf64 ) { addsub_imm(ctx, 1, rd, rn, imm, sf64); }

// ADD/SUB (shifted register), shift = LSL #0. C6.2.5 / C6.2.343.
static void addsub_reg( jit_ctx *ctx, int is_sub, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) {
	uint32_t ins =
		((sf64 & 1) << 31) | ((is_sub & 1) << 30) | (0 << 29) | (0x0b << 24) |
		((rm & 0x1f) << 16) | (0 << 10) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_add_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { addsub_reg(ctx, 0, rd, rn, rm, sf64); }
void a64_sub_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { addsub_reg(ctx, 1, rd, rn, rm, sf64); }

// MUL = MADD with Ra=XZR. C6.2.182.
void a64_mul( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) {
	uint32_t ins =
		((sf64 & 1) << 31) | (0x0d8 << 21) | ((rm & 0x1f) << 16) |
		(0 << 15) | (0x1f << 10) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
// SDIV / UDIV. C6.2.296 / C6.2.371.
static void divreg( jit_ctx *ctx, int is_signed, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) {
	uint32_t ins =
		((sf64 & 1) << 31) | (0x0d6 << 21) | ((rm & 0x1f) << 16) |
		(0x2 << 10) | ((is_signed & 1) << 10) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_sdiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { divreg(ctx, 1, rd, rn, rm, sf64); }
void a64_udiv( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { divreg(ctx, 0, rd, rn, rm, sf64); }

// Logical (shifted register), shift=LSL #0. C6.2.14 / C6.2.234 / C6.2.95.
static void logical_reg( jit_ctx *ctx, int opc, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) {
	// opc: AND=0, ORR=1, EOR=2, ANDS=3.
	uint32_t ins =
		((sf64 & 1) << 31) | ((opc & 3) << 29) | (0x0a << 24) |
		((rm & 0x1f) << 16) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	// ANDS (opc=3) writes flags AND rd. The other 3 only write rd.
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_and_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { logical_reg(ctx, 0, rd, rn, rm, sf64); }
void a64_orr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { logical_reg(ctx, 1, rd, rn, rm, sf64); }
void a64_eor_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { logical_reg(ctx, 2, rd, rn, rm, sf64); }
// MOV (reg) = ORR Xd, XZR, Xm.
void a64_mov_reg( jit_ctx *ctx, a64_greg rd, a64_greg rm, int sf64 ) { a64_orr_reg(ctx, rd, A64_SP_OR_ZR, rm, sf64); }

// Variable shifts (LSL/LSR/ASR register form), encoded as data-processing-2-source.
static void shift_reg( jit_ctx *ctx, int opc, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) {
	// opc selects 0x8=LSL, 0x9=LSR, 0xA=ASR (low 4 bits of the opcode field).
	uint32_t ins =
		((sf64 & 1) << 31) | (0x0d6 << 21) | ((rm & 0x1f) << 16) |
		((opc & 0xf) << 10) | ((rn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_lsl_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { shift_reg(ctx, 0x8, rd, rn, rm, sf64); }
void a64_lsr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { shift_reg(ctx, 0x9, rd, rn, rm, sf64); }
void a64_asr_reg( jit_ctx *ctx, a64_greg rd, a64_greg rn, a64_greg rm, int sf64 ) { shift_reg(ctx, 0xA, rd, rn, rm, sf64); }

// CMP = SUBS XZR, Xn, Xm (or imm).
void a64_cmp_reg( jit_ctx *ctx, a64_greg rn, a64_greg rm, int sf64 ) {
	// SUBS shifted-reg: opc=11 in addsub_reg layout (we open-code it here).
	uint32_t ins =
		((sf64 & 1) << 31) | (1u << 30) | (1u << 29) | (0x0b << 24) |
		((rm & 0x1f) << 16) | ((rn & 0x1f) << 5) | A64_SP_OR_ZR;
	a64_emit(ctx, ins);
}
void a64_cmp_imm( jit_ctx *ctx, a64_greg rn, int32_t imm, int sf64 ) {
	int sh = 0;
	int is_sub = 1;
	if( imm < 0 ) { is_sub = 0; imm = -imm; }
	if( (imm & ~0xfff) && !(imm & 0xfff) ) { sh = 1; imm >>= 12; }
	uint32_t ins =
		((sf64 & 1) << 31) | ((is_sub & 1) << 30) | (1u << 29) | (0x11 << 24) |
		((sh & 1) << 22) | ((imm & 0xfff) << 10) | ((rn & 0x1f) << 5) | A64_SP_OR_ZR;
	a64_emit(ctx, ins);
}

// LDR/STR (unsigned offset). Size encoding: 0=byte, 1=half, 2=word, 3=double.
// LDRSB/LDRSH/LDRSW selected via opc when sign_extend != 0.
void a64_ldr_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size, int sign_extend ) {
	int sz_log = (size == 1) ? 0 : (size == 2) ? 1 : (size == 4) ? 2 : 3;
	int scaled = imm >> sz_log;
	// If imm isn't representable as a 12-bit scaled offset (must be a multiple
	// of `size` and within [0, 4095*size]), fall back to materialising the
	// effective address in x16 then doing a zero-offset load. Silently
	// truncating to the low 12 bits — which the original code did — produced
	// reads from the wrong offset and shipped corrupted GC objects.
	if( (imm & ((1<<sz_log)-1)) || scaled < 0 || scaled > 0xfff ) {
		kill_gpr(A64_X16);
		a64_add_imm(ctx, A64_X16, rn, imm, 1);
		int opc_fb = sign_extend ? 0x2 : 0x1;
		uint32_t ins_fb =
			((sz_log & 3) << 30) | (0x39 << 24) | ((opc_fb & 3) << 22) |
			((0 & 0xfff) << 10) | ((A64_X16 & 0x1f) << 5) | (rt & 0x1f);
		kill_gpr(rt);
		a64_emit(ctx, ins_fb);
		return;
	}
	int opc = sign_extend ? 0x2 : 0x1; // LDR=01, LDRS*=10
	uint32_t ins =
		((sz_log & 3) << 30) | (0x39 << 24) | ((opc & 3) << 22) |
		((scaled & 0xfff) << 10) | ((rn & 0x1f) << 5) | (rt & 0x1f);
	kill_gpr(rt);
	a64_emit(ctx, ins);
}
void a64_str_imm( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size ) {
	int sz_log = (size == 1) ? 0 : (size == 2) ? 1 : (size == 4) ? 2 : 3;
	int scaled = imm >> sz_log;
	// Same fallback as a64_ldr_imm — out-of-range offsets must compute the
	// effective address in a scratch instead of silently truncating.
	if( (imm & ((1<<sz_log)-1)) || scaled < 0 || scaled > 0xfff ) {
		kill_gpr(A64_X16);
		a64_add_imm(ctx, A64_X16, rn, imm, 1);
		uint32_t ins_fb =
			((sz_log & 3) << 30) | (0x39 << 24) | (0 << 22) |
			((0 & 0xfff) << 10) | ((A64_X16 & 0x1f) << 5) | (rt & 0x1f);
		a64_emit(ctx, ins_fb);
		return;
	}
	uint32_t ins =
		((sz_log & 3) << 30) | (0x39 << 24) | (0 << 22) |
		((scaled & 0xfff) << 10) | ((rn & 0x1f) << 5) | (rt & 0x1f);
	a64_emit(ctx, ins);
}

// LDUR / STUR (unscaled, signed imm9). C6.2.165 / C6.2.302.
// Layout: size:2 | 111000 | opc:2 | 0 | imm9:9 | 00 | Rn:5 | Rt:5.
//   opc=00: STUR, opc=01: LDR (zero-extend), opc=10: LDRS (64-bit dst),
//   opc=11: LDRSW (32-bit dst, 32-bit access). For 64-bit access size=11.
static void ldst_unscaled( jit_ctx *ctx, int opc, a64_greg rt, a64_greg rn, int32_t imm9, int size_bytes ) {
	int sz_log = (size_bytes == 1) ? 0 : (size_bytes == 2) ? 1 : (size_bytes == 4) ? 2 : 3;
	uint32_t i9 = (uint32_t)(imm9 & 0x1ff);
	uint32_t ins =
		((sz_log & 3) << 30) | (0x38 << 24) | ((opc & 3) << 22) |
		(i9 << 12) | ((rn & 0x1f) << 5) | (rt & 0x1f);
	// opc=0 is STUR (mem write only), 1/2 are LDUR variants (kill rt).
	if( opc != 0 ) kill_gpr(rt);
	a64_emit(ctx, ins);
}
void a64_ldur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size, int sign_extend ) {
	ldst_unscaled(ctx, sign_extend ? 0x2 : 0x1, rt, rn, imm, size);
}
void a64_stur( jit_ctx *ctx, a64_greg rt, a64_greg rn, int32_t imm, int size ) {
	ldst_unscaled(ctx, 0x0, rt, rn, imm, size);
}
// LDUR/STUR for SIMD/FP regs — same layout but in the 0x3c row.
static void ldst_unscaled_fp( jit_ctx *ctx, int opc, a64_vreg vt, a64_greg rn, int32_t imm9, int is_double ) {
	int sz_log = is_double ? 3 : 2;
	uint32_t i9 = (uint32_t)(imm9 & 0x1ff);
	uint32_t ins =
		((sz_log & 3) << 30) | (0x3c << 24) | ((opc & 3) << 22) |
		(i9 << 12) | ((rn & 0x1f) << 5) | (vt & 0x1f);
	if( opc != 0 ) kill_fp(vt); // STUR=0 (write-only mem); LDUR kills vt
	a64_emit(ctx, ins);
}
void a64_ldur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double ) { ldst_unscaled_fp(ctx, 0x1, vt, rn, imm, is_double); }
void a64_stur_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double ) { ldst_unscaled_fp(ctx, 0x0, vt, rn, imm, is_double); }

// STP/LDP pre-indexed and post-indexed (64-bit form only — what we need for FP/LR).
// C6.2.273 (STP) / C6.2.135 (LDP).
void a64_stp_pre( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm ) {
	int scaled = (imm >> 3) & 0x7f; // 7-bit signed, scaled by 8
	uint32_t ins =
		(0x2 << 30) | (0xa6 << 22) | ((scaled & 0x7f) << 15) |
		((rt2 & 0x1f) << 10) | ((base & 0x1f) << 5) | (rt1 & 0x1f);
	a64_emit(ctx, ins);
}
void a64_ldp_post( jit_ctx *ctx, a64_greg rt1, a64_greg rt2, a64_greg base, int32_t imm ) {
	int scaled = (imm >> 3) & 0x7f;
	uint32_t ins =
		(0x2 << 30) | (0xa3 << 22) | ((scaled & 0x7f) << 15) |
		((rt2 & 0x1f) << 10) | ((base & 0x1f) << 5) | (rt1 & 0x1f);
	kill_gpr(rt1);
	kill_gpr(rt2);
	a64_emit(ctx, ins);
}

// Branches.
int a64_b( jit_ctx *ctx, int32_t offset_words ) {
	int pos = BUF_POS();
	uint32_t ins = (0x5 << 26) | (offset_words & 0x03ffffff);
	a64_emit(ctx, ins);
	return pos;
}
int a64_bl( jit_ctx *ctx, int32_t offset_words ) {
	int pos = BUF_POS();
	uint32_t ins = (1u << 31) | (0x5 << 26) | (offset_words & 0x03ffffff);
	a64_emit(ctx, ins);
	// Like BLR: AAPCS64 says the callee may clobber every caller-saved reg.
	kill_caller_saved();
	return pos;
}
int a64_bcond( jit_ctx *ctx, a64_cond cond, int32_t offset_words ) {
	int pos = BUF_POS();
	uint32_t ins = (0x54 << 24) | ((offset_words & 0x7ffff) << 5) | (cond & 0xf);
	a64_emit(ctx, ins);
	return pos;
}
void a64_br( jit_ctx *ctx, a64_greg rn ) {
	a64_emit(ctx, (0xd61f << 16) | ((rn & 0x1f) << 5));
}
void a64_blr( jit_ctx *ctx, a64_greg rn ) {
	a64_emit(ctx, (0xd63f << 16) | ((rn & 0x1f) << 5));
	// AAPCS64: every caller-saved reg is clobbered across a call.
	kill_caller_saved();
}
void a64_ret( jit_ctx *ctx ) {
	a64_emit(ctx, (0xd65f << 16) | ((A64_LR & 0x1f) << 5));
}

int a64_patch_branch( jit_ctx *ctx, int pos, int target ) {
	// A patched branch makes `target` reachable from `pos`. If the
	// current BUF_POS *is* the target (the usual case when we patch a
	// forward branch right where it lands), then this point is a join
	// and the reg cache state from "the fall-through path that emitted
	// the most recent store_vreg" no longer matches "the path that took
	// the branch and skipped that store". Clear the cache so any
	// subsequent load_vreg falls back to a real LDUR.
	if( target == BUF_POS() ) cache_clear();
	int delta_words = (target - pos) >> 2;
	uint32_t *slot = (uint32_t*)(ctx->startBuf + pos);
	uint32_t ins = *slot;
	uint32_t top = ins >> 26;
	if( top == 0x05 || top == 0x25 ) {
		// B or BL: 26-bit signed imm.
		if( delta_words < -(1<<25) || delta_words >= (1<<25) ) return 0;
		*slot = (ins & 0xfc000000) | ((uint32_t)delta_words & 0x03ffffff);
		return 1;
	}
	if( (ins >> 24) == 0x54 ) {
		// B.cond: 19-bit signed imm at bits [23:5].
		if( delta_words < -(1<<18) || delta_words >= (1<<18) ) return 0;
		*slot = (ins & 0xff00001f) | ((uint32_t)(delta_words & 0x7ffff) << 5);
		return 1;
	}
	return 0;
}

// ------------ FP scalars (double precision only for the bring-up) -------------
static void fp_dp2( jit_ctx *ctx, int op, a64_vreg vd, a64_vreg vn, a64_vreg vm ) {
	// Floating-point data-processing (2 source), double precision (type=01).
	// op: FMUL=0, FDIV=1, FADD=2, FSUB=3 (in our minimal coverage).
	uint32_t ins =
		(0x1e << 24) | (0x1 << 22) | (1u << 21) | ((vm & 0x1f) << 16) |
		((op & 0xf) << 12) | (0x2 << 10) | ((vn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
void a64_fadd_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm ) { fp_dp2(ctx, 0x2, vd, vn, vm); }
void a64_fsub_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm ) { fp_dp2(ctx, 0x3, vd, vn, vm); }
void a64_fmul_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm ) { fp_dp2(ctx, 0x0, vd, vn, vm); }
void a64_fdiv_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn, a64_vreg vm ) { fp_dp2(ctx, 0x1, vd, vn, vm); }

void a64_fmov_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn ) {
	// FMOV (register), double. C6.2.122.
	uint32_t ins = (0x1e << 24) | (0x1 << 22) | (1u << 21) | (0x10 << 10) |
		((vn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
void a64_fcmp_d( jit_ctx *ctx, a64_vreg vn, a64_vreg vm ) {
	uint32_t ins = (0x1e << 24) | (0x1 << 22) | (1u << 21) |
		((vm & 0x1f) << 16) | (0x8 << 10) | ((vn & 0x1f) << 5);
	a64_emit(ctx, ins);
}
void a64_scvtf_d( jit_ctx *ctx, a64_vreg vd, a64_greg rn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x1 << 22) | (1u << 21) |
		(0x2 << 16) | ((rn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
void a64_fcvtzs_d( jit_ctx *ctx, a64_greg rd, a64_vreg vn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x1 << 22) | (1u << 21) |
		(0x18 << 16) | ((vn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
// SCVTF single-precision (Sd <- Wn or Xn). ftype=00.
static void a64_scvtf_s( jit_ctx *ctx, a64_vreg vd, a64_greg rn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x0 << 22) | (1u << 21) |
		(0x2 << 16) | ((rn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
// UCVTF double / single (unsigned int → fp).
static void a64_ucvtf_d( jit_ctx *ctx, a64_vreg vd, a64_greg rn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x1 << 22) | (1u << 21) |
		(0x3 << 16) | ((rn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
static void a64_ucvtf_s( jit_ctx *ctx, a64_vreg vd, a64_greg rn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x0 << 22) | (1u << 21) |
		(0x3 << 16) | ((rn & 0x1f) << 5) | (vd & 0x1f);
	kill_fp(vd);
	a64_emit(ctx, ins);
}
// FCVT between precisions. opc selects target type:
//   D→S: ftype=01, opc=00 → 0x1E624000
//   S→D: ftype=00, opc=01 → 0x1E22C000
//   D→H/H→D etc not needed yet.
static void a64_fcvt_d_to_s( jit_ctx *ctx, a64_vreg vd, a64_vreg vn ) {
	kill_fp(vd);
	a64_emit(ctx, 0x1E624000 | ((vn & 0x1f) << 5) | (vd & 0x1f));
}
static void a64_fcvt_s_to_d( jit_ctx *ctx, a64_vreg vd, a64_vreg vn ) {
	kill_fp(vd);
	a64_emit(ctx, 0x1E22C000 | ((vn & 0x1f) << 5) | (vd & 0x1f));
}
// FCVTZS single-precision (Wd <- Sn or Xd <- Sn). ftype=00.
static void a64_fcvtzs_s( jit_ctx *ctx, a64_greg rd, a64_vreg vn, int sf64 ) {
	uint32_t ins = ((sf64 & 1) << 31) | (0x1e << 24) | (0x0 << 22) | (1u << 21) |
		(0x18 << 16) | ((vn & 0x1f) << 5) | (rd & 0x1f);
	kill_gpr(rd);
	a64_emit(ctx, ins);
}
void a64_ldr_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double ) {
	int sz_log = is_double ? 3 : 2;
	int scaled = imm >> sz_log;
	uint32_t ins =
		((sz_log & 3) << 30) | (0x3d << 24) | (1u << 22) |
		((scaled & 0xfff) << 10) | ((rn & 0x1f) << 5) | (vt & 0x1f);
	kill_fp(vt);
	a64_emit(ctx, ins);
}
void a64_str_fp( jit_ctx *ctx, a64_vreg vt, a64_greg rn, int32_t imm, int is_double ) {
	int sz_log = is_double ? 3 : 2;
	int scaled = imm >> sz_log;
	uint32_t ins =
		((sz_log & 3) << 30) | (0x3d << 24) | (0 << 22) |
		((scaled & 0xfff) << 10) | ((rn & 0x1f) << 5) | (vt & 0x1f);
	a64_emit(ctx, ins);
}

// BRK #imm16 — debug breakpoint for unimplemented paths. C6.2.42.
static void a64_brk( jit_ctx *ctx, uint16_t imm16 ) {
	a64_emit(ctx, (0xd4 << 24) | (0x1 << 21) | ((uint32_t)imm16 << 5));
}

// -----------------------------------------------------------------------
//  C2HL trampoline.
//
//  hl_call_method (libhl/fun.c) invokes a JIT'd HL function via
//  hl_setup.static_call. We expose callback_c2hl_arm64 there. It packs the
//  arguments described by hl_type into a fixed buffer and then jumps to a
//  JIT-emitted trampoline that loads the buffer back into x0..x7 / d0..d7
//  and BLR's the target.
//
//  Bring-up restrictions:
//    - No stack overflow handling. Functions with > 8 GPR or > 8 FPR args
//      are not callable via callback_c2hl_arm64 yet (BRK on the path).
//    - hl_setup.static_call_ref = 1 (we receive a void** to the closure).
//
//  Buffer layout passed from C to the JIT trampoline:
//    buf[0..7]   : 8 × int64_t — values for x0..x7
//    buf[8..15]  : 8 × double  — values for d0..d7
//
//  Trampoline signature (called as):
//    void *trampoline(void *fun_ptr, void *buf);
//    // result lands in x0 or d0 depending on caller-side cast.
// -----------------------------------------------------------------------

static void *call_jit_c2hl_native = NULL;

// Stub get_wrapper: libhl calls hl_setup.get_wrapper(ft) from
// hl_dyn_call_obj and hl_make_fun_wrapper. The result is stored into
// vclosure_wrapper.cl.fun and is only actually invoked when the wrapper
// closure is itself called as a function (rare path: passing the
// wrapper outside HL and back in). Returning NULL here is safe for the
// inline hl_wrapper_call path that hl_dyn_call_obj uses to dispatch
// virtual-fallback calls — that path reads wrappedFun->fun, not cl.fun.
// A proper HL2C trampoline can replace this later for full coverage.
static void *get_wrapper_arm64( hl_type *t ) {
	(void)t;
	return NULL;
}

static void *callback_c2hl_arm64( void *_f, hl_type *t, void **args, vdynamic *ret ) {
	void **f = (void**)_f;
	int nargs = t->fun->nargs;
	if( nargs > 8 ) hl_error("c2hl: > 8 args not supported in bring-up");
	uint64_t buf[16] = {0};
	int ngpr = 0, nfpr = 0;
	for( int i = 0; i < nargs; i++ ) {
		hl_type *at = t->fun->args[i];
		void *v = args[i];
		int is_fp = (at->kind == HF32 || at->kind == HF64);
		if( is_fp ) {
			if( nfpr >= 8 ) hl_error("c2hl: > 8 FP args not supported");
			if( at->kind == HF32 ) {
				float fv = *(float*)v;
				memcpy(&buf[8 + nfpr], &fv, sizeof(float));
			} else {
				double dv = *(double*)v;
				memcpy(&buf[8 + nfpr], &dv, sizeof(double));
			}
			nfpr++;
		} else {
			if( ngpr >= 8 ) hl_error("c2hl: > 8 GPR args not supported");
			switch( at->kind ) {
			case HBOOL: case HUI8:  buf[ngpr] = *(unsigned char*)v; break;
			case HUI16:             buf[ngpr] = *(unsigned short*)v; break;
			case HI32:              buf[ngpr] = (uint64_t)(int64_t)*(int32_t*)v; break;
			case HI64: case HGUID:  buf[ngpr] = *(uint64_t*)v; break;
			default:                buf[ngpr] = (uint64_t)(intptr_t)v; break;
			}
			ngpr++;
		}
	}
	switch( t->fun->ret->kind ) {
	case HVOID:
		((void(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
		return NULL;
	case HUI8: case HUI16: case HI32: case HBOOL:
		ret->v.i = ((int(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
		return &ret->v.i;
	case HI64: case HGUID:
		ret->v.i64 = ((int64(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
		return &ret->v.i64;
	case HF32:
		ret->v.f = ((float(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
		return &ret->v.f;
	case HF64:
		ret->v.d = ((double(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
		return &ret->v.d;
	default:
		return ((void*(*)(void*, void*))call_jit_c2hl_native)(*f, buf);
	}
}

// Emit the JIT trampoline. Entry: x0 = fun_ptr, x1 = buf.
// Returns the offset within the buffer.
static int emit_c2hl_trampoline( jit_ctx *ctx ) {
	jit_buf(ctx);
	int pos = BUF_POS();
	// Prologue.
	a64_stp_pre(ctx, A64_FP, A64_LR, A64_SP_OR_ZR, -16);
	a64_add_imm(ctx, A64_FP, A64_SP_OR_ZR, 0, 1);
	// Move fun_ptr / buf into scratches that won't be clobbered by the
	// upcoming x0/x1 reload.
	a64_mov_reg(ctx, A64_X16, A64_X0, 1); // x16 = fun ptr
	a64_mov_reg(ctx, A64_X17, A64_X1, 1); // x17 = buf
	// Load x0..x7 from buf[0..7].
	for( int i = 0; i < 8; i++ ) {
		a64_ldr_imm(ctx, (a64_greg)(A64_X0 + i), A64_X17, i * 8, 8, 0);
	}
	// Load d0..d7 from buf[8..15] (FP regs reuse the same buffer).
	for( int i = 0; i < 8; i++ ) {
		a64_ldr_fp(ctx, (a64_vreg)(A64_V0 + i), A64_X17, 64 + i * 8, 1);
	}
	// Call the target.
	a64_blr(ctx, A64_X16);
	// Epilogue.
	a64_add_imm(ctx, A64_SP_OR_ZR, A64_FP, 0, 1);
	a64_ldp_post(ctx, A64_FP, A64_LR, A64_SP_OR_ZR, 16);
	a64_ret(ctx);
	return pos;
}

// -----------------------------------------------------------------------
//  Prologue / epilogue.
//  We currently spill every HL vreg to the stack, no register allocator.
//  The frame is laid out so that vreg #i lives at [FP - 8*(i+1)] (8 bytes
//  per slot regardless of HL type — wasteful but correct for bring-up).
// -----------------------------------------------------------------------

// Forward decls for helpers defined further down the file — needed because
// emit_prologue spills incoming x0..x7 / d0..d7 into vreg slots.
static void store_vreg( jit_ctx *ctx, a64_greg src, int vi );
static void store_vreg_fp( jit_ctx *ctx, a64_vreg src, int vi );
static int  vreg_size( hl_function *f, int i );

static void emit_prologue( jit_ctx *ctx, int frameSize ) {
	if( frameSize & 0xf ) frameSize += 16 - (frameSize & 0xf);
	ctx->totalRegsSize = frameSize;
	// stp x29, x30, [sp, #-16]!  ;  mov x29, sp  ;  sub sp, sp, #frame
	a64_stp_pre(ctx, A64_FP, A64_LR, A64_SP_OR_ZR, -16);
	a64_add_imm(ctx, A64_FP, A64_SP_OR_ZR, 0, 1);
	if( frameSize ) a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, frameSize, 1);
	// Spill incoming arguments to their vreg slots. AAPCS64 passes the
	// first 8 non-FP arguments in x0..x7 and the first 8 FP arguments
	// in d0..d7. Beyond that, args come on the caller's stack — which
	// the spill-everything codegen does not yet reach into.
	hl_function *f = ctx->f;
	int ngpr = 0, nfpr = 0;
	int nargs = f->type->fun->nargs;
	// AAPCS64 stack-arg layout: after our prologue, FP = caller_sp - 16.
	// The first stack-passed arg (the 9th GPR or 9th FP, in declaration order)
	// sits at [FP + 16], then +24, +32, ... Each arg is 8-byte slot regardless
	// of its actual size (fixed-arity AAPCS64; we don't generate varargs).
	int stack_off = 16;
	for( int i = 0; i < nargs; i++ ) {
		hl_type *at = f->type->fun->args[i];
		int is_fp = (at->kind == HF32 || at->kind == HF64);
		if( is_fp ) {
			if( nfpr < 8 ) {
				store_vreg_fp(ctx, (a64_vreg)(A64_V0 + nfpr), i);
				nfpr++;
			} else {
				// Load FP arg from caller's stack into V16 then spill to vreg slot.
				a64_ldur_fp(ctx, A64_V16, A64_FP, stack_off, at->kind == HF64);
				store_vreg_fp(ctx, A64_V16, i);
				stack_off += 8;
			}
		} else {
			if( ngpr < 8 ) {
				store_vreg(ctx, (a64_greg)(A64_X0 + ngpr), i);
				ngpr++;
			} else {
				int sz = vreg_size(f, i);
				a64_ldur(ctx, A64_X9, A64_FP, stack_off, sz, 0);
				store_vreg(ctx, A64_X9, i);
				stack_off += 8;
			}
		}
	}
}

static void emit_epilogue( jit_ctx *ctx ) {
	// mov sp, x29
	a64_add_imm(ctx, A64_SP_OR_ZR, A64_FP, 0, 1);
	// ldp x29, x30, [sp], #16
	a64_ldp_post(ctx, A64_FP, A64_LR, A64_SP_OR_ZR, 16);
	a64_ret(ctx);
}

// Slot address for vreg #i, returned as offset from FP (negative).
//   - Negative imm9 forms (LDUR/STUR) handle -256..-1 directly.
//   - For larger frames we materialise the address in a scratch reg.
static int vreg_offset( int i ) {
	return -8 * (i + 1);
}

// Returns 1 if the vreg's type lives in an FP register (HF32/HF64).
static int vreg_is_fp( hl_function *f, int i ) {
	hl_type *t = f->regs[i];
	return t->kind == HF32 || t->kind == HF64;
}

// Size in bytes used for a vreg's HL type. We always reserve 8 bytes per slot
// in the frame; this only affects how we load/store (sign extension, etc.).
static int vreg_size( hl_function *f, int i ) {
	switch( f->regs[i]->kind ) {
	case HUI8: case HBOOL: return 1;
	case HUI16: return 2;
	case HI32: case HF32: return 4;
	default: return 8;
	}
}

// Emit "load vreg #vi into Xreg". For FP-typed vregs the caller is expected
// to call load_vreg_fp() instead.
static void load_vreg( jit_ctx *ctx, a64_greg dst, int vi ) {
	// Only the "already in dst" shortcut is taken — the cross-reg MOV form
	// (find_gpr returns a different holder → MOV dst, holder) used to live
	// here but introduced a stale-value crash on Heaps h2d.RenderContext
	// rendering. The Android Heaps demo (HeapDemo, 80 sprites) reproduced
	// it as a SIGSEGV in JIT'd hxd.BufferFormat.resolveMapping with a
	// dereferenced address pattern ~0x240xxx060. Same-reg shortcut still
	// catches the very common store-then-load-same-reg case from the
	// emit_prologue arg spills and from sequential opcode patterns.
	int holder = find_gpr(vi);
	if( holder == (int)dst ) return;
	int off = vreg_offset(vi);
	int sz  = vreg_size(ctx->f, vi);
	kill_gpr(dst);
	if( off >= -256 ) {
		a64_ldur(ctx, dst, A64_FP, off, sz, 0);
	} else {
		kill_gpr(A64_X16);
		a64_sub_imm(ctx, A64_X16, A64_FP, -off, 1);
		a64_ldr_imm(ctx, dst, A64_X16, 0, sz, 0);
	}
	claim_gpr(dst, vi);
}
static void store_vreg( jit_ctx *ctx, a64_greg src, int vi ) {
	int off = vreg_offset(vi);
	int sz  = vreg_size(ctx->f, vi);
	if( off >= -256 ) {
		a64_stur(ctx, src, A64_FP, off, sz);
	} else {
		kill_gpr(A64_X16);
		a64_sub_imm(ctx, A64_X16, A64_FP, -off, 1);
		a64_str_imm(ctx, src, A64_X16, 0, sz);
	}
	// `src` still holds the value just written.
	claim_gpr(src, vi);
}
static void load_vreg_fp( jit_ctx *ctx, a64_vreg dst, int vi ) {
	// See note in load_vreg about why we only take the same-reg shortcut.
	int holder = find_fp(vi);
	if( holder == (int)dst ) return;
	int off = vreg_offset(vi);
	int is_double = ctx->f->regs[vi]->kind == HF64;
	kill_fp(dst);
	if( off >= -256 ) {
		a64_ldur_fp(ctx, dst, A64_FP, off, is_double);
	} else {
		kill_gpr(A64_X16);
		a64_sub_imm(ctx, A64_X16, A64_FP, -off, 1);
		a64_ldr_fp(ctx, dst, A64_X16, 0, is_double);
	}
	claim_fp(dst, vi);
}
static void store_vreg_fp( jit_ctx *ctx, a64_vreg src, int vi ) {
	int off = vreg_offset(vi);
	int is_double = ctx->f->regs[vi]->kind == HF64;
	if( off >= -256 ) {
		a64_stur_fp(ctx, src, A64_FP, off, is_double);
	} else {
		kill_gpr(A64_X16);
		a64_sub_imm(ctx, A64_X16, A64_FP, -off, 1);
		a64_str_fp(ctx, src, A64_X16, 0, is_double);
	}
	claim_fp(src, vi);
}

// Register a jump to be patched once we know the target opcode's buffer
// position. opIdx is the HL opcode index (not byte position).
static void register_jump( jit_ctx *ctx, int pos, int opIdx ) {
	jlist *j = (jlist*)hl_malloc(&ctx->falloc, sizeof(jlist));
	j->pos = pos;
	j->target = opIdx;
	j->next = ctx->jumps;
	ctx->jumps = j;
}

// AAPCS64 argument layout — used by op_call_fun for native calls.
//   - First 8 GPR args in x0..x7
//   - First 8 FP args  in d0..d7
//   - Any further args spill to stack (16-byte aligned)
// The Apple variant only differs for varargs (stack layout), which we do not
// generate here. HL function entries are all of fixed arity.
//
// Returns the additional stack space consumed (bytes), always 16-byte aligned.
// Caller is responsible for emitting "add sp, sp, #size" after the BLR.
static int prepare_call_args( jit_ctx *ctx, int count, int *args ) {
	if( count == 0 ) return 0;
	hl_function *f = ctx->f;
	int ngpr = 0, nfpr = 0;
	int stack_bytes = 0;
	// Pass 1: count stack args.
	for( int i = 0; i < count; i++ ) {
		hl_type *t = f->regs[args[i]];
		int is_fp = (t->kind == HF32 || t->kind == HF64);
		if( is_fp ) {
			if( nfpr < 8 ) nfpr++;
			else stack_bytes += 8;
		} else {
			if( ngpr < 8 ) ngpr++;
			else stack_bytes += 8;
		}
	}
	// Round stack to 16 bytes.
	if( stack_bytes & 0xf ) stack_bytes += 16 - (stack_bytes & 0xf);
	if( stack_bytes ) a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, stack_bytes, 1);
	// Pass 2: place args. We do GPR first then FP first then stack — but
	// because each arg can only land in one place, we just walk args[].
	ngpr = nfpr = 0;
	int stack_off = 0;
	for( int i = 0; i < count; i++ ) {
		hl_type *t = f->regs[args[i]];
		int is_fp = (t->kind == HF32 || t->kind == HF64);
		if( is_fp ) {
			if( nfpr < 8 ) {
				load_vreg_fp(ctx, (a64_vreg)(A64_V0 + nfpr), args[i]);
				nfpr++;
			} else {
				load_vreg_fp(ctx, A64_V16, args[i]);
				a64_str_fp(ctx, A64_V16, A64_SP_OR_ZR, stack_off, t->kind == HF64);
				stack_off += 8;
			}
		} else {
			if( ngpr < 8 ) {
				load_vreg(ctx, (a64_greg)(A64_X0 + ngpr), args[i]);
				ngpr++;
			} else {
				load_vreg(ctx, A64_X9, args[i]);
				a64_str_imm(ctx, A64_X9, A64_SP_OR_ZR, stack_off, 8);
				stack_off += 8;
			}
		}
	}
	return stack_bytes;
}

static void op_call_fun( jit_ctx *ctx, int dst, int findex, int count, int *args ) {
	int fid = ctx->m->functions_indexes[findex];
	int is_native = fid >= ctx->m->code->nfunctions;
	int stack_bytes = prepare_call_args(ctx, count, args);
	if( is_native ) {
		// Resolved function pointer — already in m->functions_ptrs[findex].
		void *fp = ctx->m->functions_ptrs[findex];
		a64_mov_imm64(ctx, A64_X16, (int64_t)(intptr_t)fp);
		a64_blr(ctx, A64_X16);
	} else if( ctx->m->functions_ptrs[findex] != NULL ) {
		// Already compiled — emit BL to the absolute offset (patched in hl_jit_code).
		// We still stage in ctx->calls because we don't yet know the final code
		// base address; BL is relative so patching is needed anyway.
		int pos = a64_bl(ctx, 0);
		jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
		j->pos = pos;
		j->target = findex;
		j->next = ctx->calls;
		ctx->calls = j;
	} else {
		// Forward reference — same staging.
		int pos = a64_bl(ctx, 0);
		jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
		j->pos = pos;
		j->target = findex;
		j->next = ctx->calls;
		ctx->calls = j;
	}
	if( stack_bytes ) a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, stack_bytes, 1);
	// Store result in dst slot — return value is in x0 (or d0 for FP).
	if( dst >= 0 && ctx->f->regs[dst]->kind != HVOID ) {
		if( vreg_is_fp(ctx->f, dst) ) store_vreg_fp(ctx, A64_V0, dst);
		else store_vreg(ctx, A64_X0, dst);
	}
}

// Call an external C function whose address is known at JIT time.
// Arguments must already be in x0..x7 / d0..d7. Clobbers x16.
static void emit_call_native_ptr( jit_ctx *ctx, void *fn ) {
	a64_mov_imm64(ctx, A64_X16, (int64_t)(intptr_t)fn);
	a64_blr(ctx, A64_X16);
}

// Integer binary op. Loads operands from their slots into x9/x10, emits the
// AArch64 form, stores back. is64 controls 32 vs 64-bit operation width.
static void op_binop_int( jit_ctx *ctx, int dst, int a, int b, hl_op bop ) {
	int is64 = (ctx->f->regs[dst]->kind == HI64);
	load_vreg(ctx, A64_X9, a);
	load_vreg(ctx, A64_X10, b);
	switch( bop ) {
	case OAdd:  a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OSub:  a64_sub_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OMul:  a64_mul   (ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OSDiv: a64_sdiv  (ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OUDiv: a64_udiv  (ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OAnd:  a64_and_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OOr:   a64_orr_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OXor:  a64_eor_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OShl:  a64_lsl_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OUShr: a64_lsr_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OSShr: a64_asr_reg(ctx, A64_X9, A64_X9, A64_X10, is64); break;
	case OSMod:
	case OUMod: {
		// AArch64 has no integer modulo — synthesise as a - (a/b)*b.
		// Note: this matches HL's "div by 0 => 0" semantics only because
		// SDIV/UDIV return 0 on zero divisor (AArch64-specified behavior).
		void (*divemit)(jit_ctx*, a64_greg, a64_greg, a64_greg, int) =
			(bop == OSMod) ? a64_sdiv : a64_udiv;
		divemit(ctx, A64_X11, A64_X9, A64_X10, is64);
		a64_mul(ctx, A64_X11, A64_X11, A64_X10, is64);
		a64_sub_reg(ctx, A64_X9, A64_X9, A64_X11, is64);
		break;
	}
	default:
		a64_brk(ctx, 0xBB01); // unreachable
		break;
	}
	store_vreg(ctx, A64_X9, dst);
}

// FP binary op (HF64 only for now — HF32 would use the .s forms).
static void op_binop_fp( jit_ctx *ctx, int dst, int a, int b, hl_op bop ) {
	if( bop == OSMod || bop == OUMod ) {
		// AArch64 has no FP modulo — call libc fmod (double) / fmodf (single).
		// FP args are already where we want them: d0 = a, d1 = b.
		load_vreg_fp(ctx, A64_V0, a);
		load_vreg_fp(ctx, A64_V1, b);
		emit_call_native_ptr(ctx, (void*)(intptr_t)fmod);
		store_vreg_fp(ctx, A64_V0, dst);
		return;
	}
	load_vreg_fp(ctx, A64_V16, a);
	load_vreg_fp(ctx, A64_V17, b);
	switch( bop ) {
	case OAdd:  a64_fadd_d(ctx, A64_V16, A64_V16, A64_V17); break;
	case OSub:  a64_fsub_d(ctx, A64_V16, A64_V16, A64_V17); break;
	case OMul:  a64_fmul_d(ctx, A64_V16, A64_V16, A64_V17); break;
	case OSDiv: a64_fdiv_d(ctx, A64_V16, A64_V16, A64_V17); break;
	default:    a64_brk(ctx, 0xBB02); break;
	}
	store_vreg_fp(ctx, A64_V16, dst);
}

// Conditional jump for integer/pointer compare. cond is the AArch64 condition
// corresponding to the HL OJxxx semantics ; targetOpIdx is the HL opcode
// position to branch to (we'll resolve to a byte position later).
static void op_jump_compare( jit_ctx *ctx, int a, int b, hl_op op, int targetOpIdx ) {
	hl_type_kind ka = ctx->f->regs[a]->kind;
	hl_type_kind kb = ctx->f->regs[b]->kind;
	// Mixed-type / Dynamic / function comparisons: route through
	// hl_dyn_compare so boxed values get unboxed and compared deeply.
	// This is what makes `Null<Bool> == Bool` (after ToDyn) work — both
	// sides arrive as vdynamic pointers, and a raw pointer-compare would
	// always say "different object". See jit.c::op_jump:2088.
	if( ka == HDYN || kb == HDYN || ka == HFUN || kb == HFUN ) {
		load_vreg(ctx, A64_X0, a);
		load_vreg(ctx, A64_X1, b);
		emit_call_native_ptr(ctx, (void*)hl_dyn_compare);
		// Result in x0. For JEq/JNotEq: check x0 == 0.
		// For JSLt/JSGt/etc.: invalid sentinel (0xAABBCCDD) must short-
		// circuit to "no jump"; otherwise compare result to 0 signed.
		if( op == OJEq || op == OJNotEq ) {
			a64_cmp_imm(ctx, A64_X0, 0, 0);
			int pos = a64_bcond(ctx, op == OJEq ? A64_EQ : A64_NE, 0);
			register_jump(ctx, pos, targetOpIdx);
		} else {
			// Treat invalid as "do not jump" by setting x0 = 0 in that case.
			a64_mov_imm32(ctx, A64_X9, (int32_t)0xAABBCCDD);
			a64_cmp_reg(ctx, A64_X0, A64_X9, 0);
			int skip = a64_bcond(ctx, A64_EQ, 0);
			a64_cmp_imm(ctx, A64_X0, 0, 0);
			a64_cond cond;
			switch( op ) {
			case OJSLt: cond = A64_LT; break;
			case OJSGte: cond = A64_GE; break;
			case OJSGt: cond = A64_GT; break;
			case OJSLte: cond = A64_LE; break;
			default: cond = A64_AL; break;
			}
			int pos = a64_bcond(ctx, cond, 0);
			register_jump(ctx, pos, targetOpIdx);
			a64_patch_branch(ctx, skip, BUF_POS());
		}
		return;
	}
	// HOBJ/HSTRUCT with a runtime compareFun (e.g. String): a == b iff
	// pointer-equal OR both non-null AND compareFun(a,b) == 0. Without
	// this, `"pos"+"ition" == "position"` returns false because the two
	// String objects have different identities.
	if( (ka == HOBJ || ka == HSTRUCT) && (op == OJEq || op == OJNotEq) ) {
		hl_runtime_obj *rt = hl_get_obj_rt(ctx->f->regs[a]);
		// Route through hl_dyn_compare instead of the rt->compareFun
		// shortcut — same end result for the value-equal case, and avoids
		// edge cases with un-initialised rt->compareFun pointers.
		void *compareFn = (rt && rt->compareFun) ? (void*)hl_dyn_compare : NULL;
		if( compareFn ) {
			//   cmp a, b
			//   b.eq  L_eq           ; same pointer → equal
			//   cbz a, L_neq         ; a==null    → not equal
			//   cbz b, L_neq         ; b==null    → not equal
			//   call compareFun(a, b)
			//   cbz w0, L_eq         ; result==0  → equal
			// L_neq:                  ; "not equal" lands here
			//   {OJEq: nothing | OJNotEq: b target}
			//   b L_done
			// L_eq:                   ; "equal" lands here
			//   {OJEq: b target | OJNotEq: nothing}
			// L_done:
			load_vreg(ctx, A64_X9, a);
			load_vreg(ctx, A64_X10, b);
			a64_cmp_reg(ctx, A64_X9, A64_X10, 1);
			int j_ptreq  = a64_bcond(ctx, A64_EQ, 0);    // → L_eq
			a64_cmp_imm(ctx, A64_X9, 0, 1);
			int j_a_null = a64_bcond(ctx, A64_EQ, 0);    // → L_neq
			a64_cmp_imm(ctx, A64_X10, 0, 1);
			int j_b_null = a64_bcond(ctx, A64_EQ, 0);    // → L_neq
			a64_mov_reg(ctx, A64_X0, A64_X9, 1);
			a64_mov_reg(ctx, A64_X1, A64_X10, 1);
			emit_call_native_ptr(ctx, compareFn);
			a64_cmp_imm(ctx, A64_X0, 0, 0);
			int j_cmpeq  = a64_bcond(ctx, A64_EQ, 0);    // → L_eq
			// fall-through here is "not equal" — patch the null branches:
			a64_patch_branch(ctx, j_a_null, BUF_POS());
			a64_patch_branch(ctx, j_b_null, BUF_POS());
			if( op == OJNotEq ) {
				int p = a64_b(ctx, 0);
				register_jump(ctx, p, targetOpIdx);
			}
			int j_to_done = a64_b(ctx, 0);
			// L_eq lands here:
			a64_patch_branch(ctx, j_ptreq, BUF_POS());
			a64_patch_branch(ctx, j_cmpeq, BUF_POS());
			if( op == OJEq ) {
				int p = a64_b(ctx, 0);
				register_jump(ctx, p, targetOpIdx);
			}
			// L_done:
			a64_patch_branch(ctx, j_to_done, BUF_POS());
			return;
		}
	}
	// HNULL — Haxe `Null<T>` boxing. Pointer-equal OR (both non-null and
	// deep value compare). For now we approximate with hl_dyn_compare too
	// (the dyn-compare path handles HNULL correctly via type-tagged unbox).
	if( ka == HNULL || kb == HNULL ) {
		load_vreg(ctx, A64_X0, a);
		load_vreg(ctx, A64_X1, b);
		emit_call_native_ptr(ctx, (void*)hl_dyn_compare);
		a64_cmp_imm(ctx, A64_X0, 0, 0);
		int pos = a64_bcond(ctx, op == OJEq ? A64_EQ : op == OJNotEq ? A64_NE : A64_AL, 0);
		register_jump(ctx, pos, targetOpIdx);
		return;
	}
	if( ka == HF32 || ka == HF64 ) {
		// Float compare — FCMP sets FPSR flags; the AArch64 b.cond
		// mapping for IEEE 754 ordered/unordered matches our needs.
		load_vreg_fp(ctx, A64_V16, a);
		load_vreg_fp(ctx, A64_V17, b);
		a64_fcmp_d(ctx, A64_V16, A64_V17); // HF32 still uses double form in the bring-up; HF32 path is approximate
		a64_cond cond;
		switch( op ) {
		case OJEq:     cond = A64_EQ; break;
		case OJNotEq:  cond = A64_NE; break;
		case OJSLt:    cond = A64_MI; break;   // signed less than for FP: N=1 (and not unordered)
		case OJSGte:   cond = A64_GE; break;
		case OJSGt:    cond = A64_GT; break;
		case OJSLte:   cond = A64_LS; break;
		case OJNotLt:  cond = A64_GE; break;
		case OJNotGte: cond = A64_MI; break;
		default:       cond = A64_AL; break;
		}
		int pos = a64_bcond(ctx, cond, 0);
		register_jump(ctx, pos, targetOpIdx);
		return;
	}
	// Integer compare — pick 32 or 64-bit based on operand kind. Doing
	// 64-bit on zero-extended HI32s silently flips signed comparisons.
	int is64 = (ka == HI64 || ka == HGUID || hl_is_ptr(ctx->f->regs[a])) ? 1 : 0;
	load_vreg(ctx, A64_X9, a);
	load_vreg(ctx, A64_X10, b);
	a64_cmp_reg(ctx, A64_X9, A64_X10, is64);
	a64_cond cond;
	switch( op ) {
	case OJEq:     cond = A64_EQ; break;
	case OJNotEq:  cond = A64_NE; break;
	case OJSLt:    cond = A64_LT; break;
	case OJSGte:   cond = A64_GE; break;
	case OJSGt:    cond = A64_GT; break;
	case OJSLte:   cond = A64_LE; break;
	case OJULt:    cond = A64_CC; break;
	case OJUGte:   cond = A64_CS; break;
	case OJNotLt:  cond = A64_GE; break;   // "not less than" = signed >=
	case OJNotGte: cond = A64_LT; break;
	default:       cond = A64_AL; break;
	}
	int pos = a64_bcond(ctx, cond, 0);
	register_jump(ctx, pos, targetOpIdx);
}

// -----------------------------------------------------------------------
//  Public API
// -----------------------------------------------------------------------

jit_ctx *hl_jit_alloc( void ) {
	jit_ctx *ctx = (jit_ctx*)malloc(sizeof(jit_ctx));
	memset(ctx, 0, sizeof(jit_ctx));
	hl_alloc_init(&ctx->falloc);
	hl_alloc_init(&ctx->galloc);
	ctx->closure_list = NULL;
	return ctx;
}

void hl_jit_free( jit_ctx *ctx, h_bool can_reset ) {
	(void)can_reset;
	if( !ctx ) return;
	if( ctx->startBuf ) free(ctx->startBuf);
	hl_free(&ctx->falloc);
	hl_free(&ctx->galloc);
	free(ctx);
}

static void hl_jit_init_module( jit_ctx *ctx, hl_module *m ) {
	ctx->m = m;
	hl_free(&ctx->galloc);
	hl_alloc_init(&ctx->galloc);
}

void hl_jit_init( jit_ctx *ctx, hl_module *m ) {
	hl_jit_init_module(ctx, m);
	// Trampolines must sit at known offsets the JIT later patches into
	// hl_setup. Emit them upfront so functions land at offset >= c2hl_size.
	ctx->c2hl = emit_c2hl_trampoline(ctx);
	ctx->hl2c = -1; // not yet implemented; OCallClosure will BRK
}

void hl_jit_reset( jit_ctx *ctx, hl_module *m ) {
	ctx->buf.b = ctx->startBuf;
	hl_jit_init_module(ctx, m);
	ctx->c2hl = emit_c2hl_trampoline(ctx);
	ctx->hl2c = -1;
}

// Per-opcode codegen.
//
// Bring-up codegen is still spill-everything: every HL vreg lives in its
// stack slot, ops load to x9/x10, compute, store back. Hot paths and
// register allocation can land later without rewriting this dispatcher.
static void jit_opcode( jit_ctx *ctx, hl_opcode *op, int opIdx ) {
	ctx->opsPos[opIdx] = BUF_POS();
	hl_module *m = ctx->m;
	hl_function *f = ctx->f;
	switch( op->op ) {
	case ONop:
	case OLabel:
		break;

	// ---------------- Moves / constants ----------------
	case OMov:
	case OUnsafeCast:
		if( vreg_is_fp(f, op->p1) ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			load_vreg(ctx, A64_X9, op->p2);
			store_vreg(ctx, A64_X9, op->p1);
		}
		break;
	case OInt:
		a64_mov_imm32(ctx, A64_X9, m->code->ints[op->p2]);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OBool:
		a64_mov_imm32(ctx, A64_X9, op->p2 ? 1 : 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OFloat: {
		// HL stores doubles in m->code->floats; load address, load double.
		double dv = m->code->floats[op->p2];
		int64_t bits;
		memcpy(&bits, &dv, sizeof(double));
		a64_mov_imm64(ctx, A64_X9, bits);
		// mov to FP via FMOV Dn, Xn — encoded as FMOV (general) variant.
		// Quick path: store the bit-pattern, then reload as FP.
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case ONull:
		a64_mov_imm64(ctx, A64_X9, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OString: {
		// Use hl_get_ustring — m->code->ustrings is lazily populated and
		// reading the raw array can yield NULL strings at JIT time. The
		// helper widens the UTF-8 entry to UTF-16 on demand and caches it.
		const uchar *s = hl_get_ustring(m->code, op->p2);
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)s);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OBytes: {
		// version >= 5 stores bytes in a single blob indexed by bytes_pos;
		// earlier versions inlined them in strings[].
		char *bp = m->code->version >= 5
			? m->code->bytes + m->code->bytes_pos[op->p2]
			: m->code->strings[op->p2];
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)bp);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}

	// ---------------- Globals ----------------
	case OGetGlobal: {
		void *addr = m->globals_data + m->globals_indexes[op->p2];
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)addr);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 8, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OSetGlobal: {
		void *addr = m->globals_data + m->globals_indexes[op->p1];
		load_vreg(ctx, A64_X9, op->p2);
		a64_mov_imm64(ctx, A64_X10, (int64_t)(intptr_t)addr);
		a64_str_imm(ctx, A64_X9, A64_X10, 0, 8);
		break;
	}

	// ---------------- Field access (HOBJ/HSTRUCT only — fast path) ----------------
	case OField: {
		hl_type *st = f->regs[op->p2];
		if( st->kind == HOBJ || st->kind == HSTRUCT ) {
			hl_runtime_obj *rt = hl_get_obj_rt(st);
			int field_off = rt->fields_indexes[op->p3];
			int sz = hl_type_size(f->regs[op->p1]);
			int is_fp = vreg_is_fp(f, op->p1);
			load_vreg(ctx, A64_X9, op->p2);
			// LDR unsigned-offset imm12 is scaled by access size, so the
			// max representable byte offset is 4095 * sz, aligned.
			int fits_imm = (field_off >= 0)
				&& ((field_off & (sz - 1)) == 0)
				&& (field_off / sz <= 4095);
			if( is_fp ) {
				int is_d = f->regs[op->p1]->kind == HF64;
				if( fits_imm ) {
					a64_ldr_fp(ctx, A64_V16, A64_X9, field_off, is_d);
				} else {
					a64_add_imm(ctx, A64_X9, A64_X9, field_off, 1);
					a64_ldr_fp(ctx, A64_V16, A64_X9, 0, is_d);
				}
				store_vreg_fp(ctx, A64_V16, op->p1);
			} else if( fits_imm ) {
				a64_ldr_imm(ctx, A64_X10, A64_X9, field_off, sz, 0);
				store_vreg(ctx, A64_X10, op->p1);
			} else {
				a64_add_imm(ctx, A64_X9, A64_X9, field_off, 1);
				a64_ldr_imm(ctx, A64_X10, A64_X9, 0, sz, 0);
				store_vreg(ctx, A64_X10, op->p1);
			}
		} else if( st->kind == HVIRTUAL ) {
			// vptr = hl_vfields(o)[op->p3] — i.e. *(void**)(o + sizeof(vvirtual) + op->p3*HL_WSIZE)
			// if non-null:  dst = *vptr
			// if null:      dst = hl_dyn_get*(o, hash(field), dst_type)
			hl_type *dt = f->regs[op->p1];
			int vfield_off = (int)sizeof(vvirtual) + op->p3 * (int)sizeof(void*);
			load_vreg(ctx, A64_X9, op->p2);            // x9 = o
			a64_ldr_imm(ctx, A64_X10, A64_X9, vfield_off, 8, 0); // x10 = vfield[p3]
			a64_cmp_imm(ctx, A64_X10, 0, 1);
			int j_has = a64_bcond(ctx, A64_NE, 0);
			// null path: hl_dyn_get
			int hash = st->virt->fields[op->p3].hashed_name;
			load_vreg(ctx, A64_X0, op->p2);
			a64_mov_imm32(ctx, A64_X1, hash);
			void *fn; int needs_type = 1;
			switch( dt->kind ) {
			case HF32: fn = (void*)hl_dyn_getf;  needs_type = 0; break;
			case HF64: fn = (void*)hl_dyn_getd;  needs_type = 0; break;
			case HI64: case HGUID: fn = (void*)hl_dyn_geti64; needs_type = 0; break;
			case HI32: case HUI16: case HUI8: case HBOOL: fn = (void*)hl_dyn_geti; break;
			default: fn = (void*)hl_dyn_getp; break;
			}
			if( needs_type ) a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)dt);
			emit_call_native_ptr(ctx, fn);
			if( dt->kind == HF32 || dt->kind == HF64 )
				store_vreg_fp(ctx, A64_V0, op->p1);
			else
				store_vreg(ctx, A64_X0, op->p1);
			int j_end = a64_b(ctx, 0);
			// non-null path: dst = *vptr
			a64_patch_branch(ctx, j_has, BUF_POS());
			if( vreg_is_fp(f, op->p1) ) {
				a64_ldr_fp(ctx, A64_V16, A64_X10, 0, dt->kind == HF64);
				store_vreg_fp(ctx, A64_V16, op->p1);
			} else {
				a64_ldr_imm(ctx, A64_X11, A64_X10, 0, vreg_size(f, op->p1), 0);
				store_vreg(ctx, A64_X11, op->p1);
			}
			a64_patch_branch(ctx, j_end, BUF_POS());
		} else {
			a64_brk(ctx, 0xF1E1);
		}
		break;
	}
	case OSetField: {
		hl_type *dt = f->regs[op->p1];
		if( dt->kind == HOBJ || dt->kind == HSTRUCT ) {
			hl_runtime_obj *rt = hl_get_obj_rt(dt);
			int field_off = rt->fields_indexes[op->p2];
			int sz = hl_type_size(f->regs[op->p3]);
			int is_fp = vreg_is_fp(f, op->p3);
			int fits_imm = (field_off >= 0)
				&& ((field_off & (sz - 1)) == 0)
				&& (field_off / sz <= 4095);
			load_vreg(ctx, A64_X9, op->p1);
			if( !fits_imm ) {
				a64_add_imm(ctx, A64_X9, A64_X9, field_off, 1);
				field_off = 0;
			}
			if( is_fp ) {
				load_vreg_fp(ctx, A64_V16, op->p3);
				a64_str_fp(ctx, A64_V16, A64_X9, field_off, f->regs[op->p3]->kind == HF64);
			} else {
				load_vreg(ctx, A64_X10, op->p3);
				a64_str_imm(ctx, A64_X10, A64_X9, field_off, sz);
			}
		} else if( dt->kind == HVIRTUAL ) {
			// vfield = hl_vfields(o)[op->p2]
			//   non-null: *vfield = src
			//   null:     hl_dyn_set{p,i,...}(o, hash, vt, src)
			int vfield_off = (int)sizeof(vvirtual) + op->p2 * (int)sizeof(void*);
			hl_type *vt = dt->virt->fields[op->p2].t;
			int hash = dt->virt->fields[op->p2].hashed_name;
			load_vreg(ctx, A64_X9, op->p1);
			a64_ldr_imm(ctx, A64_X10, A64_X9, vfield_off, 8, 0);
			a64_cmp_imm(ctx, A64_X10, 0, 1);
			int j_has = a64_bcond(ctx, A64_NE, 0);
			// null: dyn_set
			load_vreg(ctx, A64_X0, op->p1);
			a64_mov_imm32(ctx, A64_X1, hash);
			void *fn;
			hl_type *src_t = f->regs[op->p3];
			switch( src_t->kind ) {
			case HF32:
				fn = (void*)hl_dyn_setf;
				load_vreg_fp(ctx, A64_V0, op->p3);
				break;
			case HF64:
				fn = (void*)hl_dyn_setd;
				load_vreg_fp(ctx, A64_V0, op->p3);
				break;
			case HI64: case HGUID:
				fn = (void*)hl_dyn_seti64;
				load_vreg(ctx, A64_X2, op->p3);
				break;
			case HI32: case HUI16: case HUI8: case HBOOL:
				fn = (void*)hl_dyn_seti;
				a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)src_t);
				load_vreg(ctx, A64_X3, op->p3);
				break;
			default:
				fn = (void*)hl_dyn_setp;
				a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)src_t);
				load_vreg(ctx, A64_X3, op->p3);
				break;
			}
			emit_call_native_ptr(ctx, fn);
			int j_end = a64_b(ctx, 0);
			// non-null: *vfield = src
			a64_patch_branch(ctx, j_has, BUF_POS());
			if( vreg_is_fp(f, op->p3) ) {
				load_vreg_fp(ctx, A64_V16, op->p3);
				a64_str_fp(ctx, A64_V16, A64_X10, 0, src_t->kind == HF64);
			} else {
				load_vreg(ctx, A64_X11, op->p3);
				a64_str_imm(ctx, A64_X11, A64_X10, 0, hl_type_size(src_t));
			}
			(void)vt;
			a64_patch_branch(ctx, j_end, BUF_POS());
		} else {
			a64_brk(ctx, 0xF1E2);
		}
		break;
	}
	case OGetThis: {
		// vreg 0 holds "this"
		hl_runtime_obj *rt = hl_get_obj_rt(f->regs[0]);
		int field_off = rt->fields_indexes[op->p2];
		int sz = hl_type_size(f->regs[op->p1]);
		int is_fp = vreg_is_fp(f, op->p1);
		int fits_imm = (field_off >= 0)
			&& ((field_off & (sz - 1)) == 0)
			&& (field_off / sz <= 4095);
		load_vreg(ctx, A64_X9, 0);
		if( !fits_imm ) { a64_add_imm(ctx, A64_X9, A64_X9, field_off, 1); field_off = 0; }
		if( is_fp ) {
			a64_ldr_fp(ctx, A64_V16, A64_X9, field_off, f->regs[op->p1]->kind == HF64);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_ldr_imm(ctx, A64_X10, A64_X9, field_off, sz, 0);
			store_vreg(ctx, A64_X10, op->p1);
		}
		break;
	}
	case OSetThis: {
		hl_runtime_obj *rt = hl_get_obj_rt(f->regs[0]);
		int field_off = rt->fields_indexes[op->p1];
		int sz = hl_type_size(f->regs[op->p2]);
		int is_fp = vreg_is_fp(f, op->p2);
		int fits_imm = (field_off >= 0)
			&& ((field_off & (sz - 1)) == 0)
			&& (field_off / sz <= 4095);
		load_vreg(ctx, A64_X9, 0);
		if( !fits_imm ) { a64_add_imm(ctx, A64_X9, A64_X9, field_off, 1); field_off = 0; }
		if( is_fp ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_str_fp(ctx, A64_V16, A64_X9, field_off, f->regs[op->p2]->kind == HF64);
		} else {
			load_vreg(ctx, A64_X10, op->p2);
			a64_str_imm(ctx, A64_X10, A64_X9, field_off, sz);
		}
		break;
	}

	// ---------------- Arith ----------------
	case OAdd: case OSub: case OMul:
	case OSDiv: case OUDiv: case OSMod: case OUMod:
	case OAnd: case OOr: case OXor:
	case OShl: case OSShr: case OUShr:
		if( vreg_is_fp(f, op->p1) ) op_binop_fp(ctx, op->p1, op->p2, op->p3, op->op);
		else                        op_binop_int(ctx, op->p1, op->p2, op->p3, op->op);
		break;
	case ONeg:
		if( vreg_is_fp(f, op->p1) ) {
			// FNEG (scalar) double: 0x1E614000 | (vn << 5) | vd
			//                single: 0x1E214000 | (vn << 5) | vd
			load_vreg_fp(ctx, A64_V16, op->p2);
			int is_d = f->regs[op->p1]->kind == HF64;
			a64_emit(ctx, (is_d ? 0x1E614000 : 0x1E214000)
				| ((A64_V16 & 0x1f) << 5) | (A64_V16 & 0x1f));
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			load_vreg(ctx, A64_X9, op->p2);
			a64_sub_reg(ctx, A64_X9, A64_SP_OR_ZR, A64_X9, f->regs[op->p1]->kind == HI64);
			store_vreg(ctx, A64_X9, op->p1);
		}
		break;
	case ONot:
		load_vreg(ctx, A64_X9, op->p2);
		a64_eor_reg(ctx, A64_X9, A64_X9, A64_X9, 0); // wrong — placeholder
		a64_mov_imm32(ctx, A64_X10, 1);
		// Actual logical-not for bool: XOR with 1.
		load_vreg(ctx, A64_X9, op->p2);
		a64_mov_imm32(ctx, A64_X10, 1);
		a64_eor_reg(ctx, A64_X9, A64_X9, A64_X10, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OIncr:
		load_vreg(ctx, A64_X9, op->p1);
		// add_imm 64-bit is fine on the zero-extended low 32 bits: the
		// upper 32 bits stay zero, and the subsequent store_vreg writes
		// only `vreg_size` bytes (4 for HI32) so we never expose the
		// high half.
		a64_add_imm(ctx, A64_X9, A64_X9, 1, f->regs[op->p1]->kind == HI64);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case ODecr:
		load_vreg(ctx, A64_X9, op->p1);
		a64_sub_imm(ctx, A64_X9, A64_X9, 1, f->regs[op->p1]->kind == HI64);
		store_vreg(ctx, A64_X9, op->p1);
		break;

	// ---------------- Calls ----------------
	case OCall0: op_call_fun(ctx, op->p1, op->p2, 0, NULL); break;
	case OCall1: op_call_fun(ctx, op->p1, op->p2, 1, &op->p3); break;
	case OCall2: {
		int args[2] = { op->p3, (int)(intptr_t)op->extra };
		op_call_fun(ctx, op->p1, op->p2, 2, args);
		break;
	}
	case OCall3: {
		int args[3] = { op->p3, op->extra[0], op->extra[1] };
		op_call_fun(ctx, op->p1, op->p2, 3, args);
		break;
	}
	case OCall4: {
		int args[4] = { op->p3, op->extra[0], op->extra[1], op->extra[2] };
		op_call_fun(ctx, op->p1, op->p2, 4, args);
		break;
	}
	case OCallN:
		op_call_fun(ctx, op->p1, op->p2, op->p3, op->extra);
		break;

	// ---------------- Branches ----------------
	case OJAlways: {
		int pos = a64_b(ctx, 0);
		register_jump(ctx, pos, opIdx + 1 + op->p1);
		break;
	}
	case OJTrue: case OJFalse:
	case OJNull: case OJNotNull: {
		load_vreg(ctx, A64_X9, op->p1);
		a64_cmp_imm(ctx, A64_X9, 0, 1);
		a64_cond cond = (op->op == OJFalse || op->op == OJNull) ? A64_EQ : A64_NE;
		int pos = a64_bcond(ctx, cond, 0);
		register_jump(ctx, pos, opIdx + 1 + op->p2);
		break;
	}
	case OJEq: case OJNotEq:
	case OJSLt: case OJSGte: case OJSLte: case OJSGt:
	case OJULt: case OJUGte:
	case OJNotLt: case OJNotGte:
		op_jump_compare(ctx, op->p1, op->p2, op->op, opIdx + 1 + op->p3);
		break;

	// ---------------- Exception handling (setjmp-based) ----------------
	// HL exception model: an OTrap allocates an hl_trap_ctx on the stack,
	// chains it into hl_get_thread()->trap_current, then setjmp's. If
	// setjmp returns 0 we fall through (normal try-body); if non-zero we
	// branch to the catch handler with the exception loaded into `dst`.
	case OTrap: {
		int trap_size = ((int)sizeof(hl_trap_ctx) + 15) & ~15;
		int prev_off  = (int)offsetof(hl_trap_ctx, prev);
		int tcheck_off = (int)offsetof(hl_trap_ctx, tcheck);
		int tc_off    = (int)offsetof(hl_thread_info, trap_current);
		int exc_off   = (int)offsetof(hl_thread_info, exc_value);
		// Allocate the trap_ctx.
		a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, trap_size, 1);
		// Get thread, link the trap_ctx in.
		emit_call_native_ptr(ctx, (void*)hl_get_thread);
		// CRITICAL: x19 is callee-saved (AAPCS64) and our prologue does NOT
		// save it. Writing to it here clobbered the caller's value across
		// the JIT'd function — observed as a libhl C function whose `b`
		// (a hl_buffer*) became the `hl_get_thread()` return value because
		// the compiler had stashed `b` in x19. Use x9 (caller-saved) only.
		a64_mov_reg(ctx, A64_X9, A64_X0, 1);      // x9 = thread
		// t->prev = thread->trap_current
		a64_ldr_imm(ctx, A64_X10, A64_X9, tc_off, 8, 0);
		a64_str_imm(ctx, A64_X10, A64_SP_OR_ZR, prev_off, 8);
		// thread->trap_current = sp
		a64_mov_reg(ctx, A64_X10, A64_SP_OR_ZR, 1); // mov x10, sp via add x10, sp, #0
		a64_add_imm(ctx, A64_X10, A64_SP_OR_ZR, 0, 1);
		a64_str_imm(ctx, A64_X10, A64_X9, tc_off, 8);
		// t->tcheck = NULL (we don't yet honour typed catch)
		a64_str_imm(ctx, A64_SP_OR_ZR /* xzr */, A64_SP_OR_ZR, tcheck_off, 8);
		// x0 = &t->buf  (offset 0 within trap_ctx == current sp)
		a64_add_imm(ctx, A64_X0, A64_SP_OR_ZR, 0, 1);
		emit_call_native_ptr(ctx, (void*)setjmp);
		a64_cmp_imm(ctx, A64_X0, 0, 0);
		int j_no_exc = a64_bcond(ctx, A64_EQ, 0);
		// Exception path: pop the trap_ctx, fetch exc_value, branch to catch.
		a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, trap_size, 1);
		emit_call_native_ptr(ctx, (void*)hl_get_thread);
		a64_ldr_imm(ctx, A64_X0, A64_X0, exc_off, 8, 0);
		store_vreg(ctx, A64_X0, op->p1);
		int j_to_catch = a64_b(ctx, 0);
		register_jump(ctx, j_to_catch, opIdx + 1 + op->p2);
		// Normal path lands here; the try-body executes with the trap live.
		a64_patch_branch(ctx, j_no_exc, BUF_POS());
		break;
	}
	case OEndTrap: {
		int trap_size = ((int)sizeof(hl_trap_ctx) + 15) & ~15;
		int prev_off  = (int)offsetof(hl_trap_ctx, prev);
		int tc_off    = (int)offsetof(hl_thread_info, trap_current);
		emit_call_native_ptr(ctx, (void*)hl_get_thread);
		a64_mov_reg(ctx, A64_X9, A64_X0, 1);
		a64_ldr_imm(ctx, A64_X10, A64_X9, tc_off, 8, 0);
		a64_ldr_imm(ctx, A64_X10, A64_X10, prev_off, 8, 0);
		a64_str_imm(ctx, A64_X10, A64_X9, tc_off, 8);
		a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, trap_size, 1);
		break;
	}
	case OCatch:
		// OCatch is just a label / jump landing pad; no codegen needed.
		break;
	case OAssert:
		// HL inserts OAssert at compiler-rejected casts and similar; call
		// hl_assert which throws an HL exception with message "assert".
		emit_call_native_ptr(ctx, (void*)hl_assert);
		break;
	case OPrefetch:
		// Prefetch is a hint; safely ignored on a non-optimised backend.
		break;

	// ---------------- Enum minimal support ----------------
	case OEnumIndex:
		// venum layout: { hl_type *t; int index; ... } — index at offset 8.
		load_vreg(ctx, A64_X9, op->p2);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 8, 4, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OEnumAlloc: {
		// dst = hl_alloc_enum(dst_type, constructor_idx)
		hl_type *t = f->regs[op->p1];
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)t);
		a64_mov_imm32(ctx, A64_X1, op->p2);
		emit_call_native_ptr(ctx, (void*)hl_alloc_enum);
		store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case OMakeEnum: {
		// Like OEnumAlloc + copy each param to result + construct->offsets[i].
		// IMPORTANT: iterate up to op->p3 (the bytecode-encoded param count
		// and the length of op->extra), NOT cons->nparams. They match for
		// well-formed bytecode, but reading past op->extra reads stale memory
		// and writes whatever junk vreg slots happen to be there. Iterating
		// by op->p3 also bounds the writes at exactly the params the source
		// program intends to set.
		hl_type *t = f->regs[op->p1];
		hl_enum_construct *cons = &t->tenum->constructs[op->p2];
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)t);
		a64_mov_imm32(ctx, A64_X1, op->p2);
		emit_call_native_ptr(ctx, (void*)hl_alloc_enum);
		for( int i = 0; i < op->p3; i++ ) {
			int off = cons->offsets[i];
			hl_type *pt = cons->params[i];
			int is_fp = (pt->kind == HF32 || pt->kind == HF64);
			int sz = hl_type_size(pt);
			if( is_fp ) {
				load_vreg_fp(ctx, A64_V16, op->extra[i]);
				a64_str_fp(ctx, A64_V16, A64_X0, off, pt->kind == HF64);
			} else {
				load_vreg(ctx, A64_X9, op->extra[i]);
				a64_str_imm(ctx, A64_X9, A64_X0, off, sz);
			}
		}
		store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case OEnumField: {
		// p2 = src enum vreg, p3 = constructor index,
		// (int)(intptr_t)op->extra = field index inside that constructor.
		// (Note: the AR descriptor in opcodes.h is misleading — extra is
		// encoded as a single int cast, not as an array pointer.)
		hl_type *st = f->regs[op->p2];
		int cidx = op->p3;
		int fidx = (int)(intptr_t)op->extra;
		hl_enum_construct *cons = &st->tenum->constructs[cidx];
		int off = cons->offsets[fidx];
		hl_type *pt = cons->params[fidx];
		load_vreg(ctx, A64_X9, op->p2);
		if( vreg_is_fp(f, op->p1) ) {
			a64_ldr_fp(ctx, A64_V16, A64_X9, off, pt->kind == HF64);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_ldr_imm(ctx, A64_X10, A64_X9, off, hl_type_size(pt), 0);
			store_vreg(ctx, A64_X10, op->p1);
		}
		break;
	}
	case OSetEnumField: {
		// (venum)dst->fields[op->p2] = src
		hl_type *dt = f->regs[op->p1];
		hl_enum_construct *cons = &dt->tenum->constructs[0]; // SetEnumField targets construct 0 in HL
		int off = cons->offsets[op->p2];
		hl_type *pt = cons->params[op->p2];
		load_vreg(ctx, A64_X9, op->p1);
		if( vreg_is_fp(f, op->p3) ) {
			load_vreg_fp(ctx, A64_V16, op->p3);
			a64_str_fp(ctx, A64_V16, A64_X9, off, pt->kind == HF64);
		} else {
			load_vreg(ctx, A64_X10, op->p3);
			a64_str_imm(ctx, A64_X10, A64_X9, off, hl_type_size(pt));
		}
		break;
	}

	// ---------------- Switch ----------------
	// Bring-up uses a linear cmp-and-branch chain. A proper jump table
	// is faster for large switches but needs an extra patch direction
	// (immediate→target after layout).
	case OSwitch: {
		int ncases = op->p2;
		load_vreg(ctx, A64_X9, op->p1);
		for( int i = 0; i < ncases; i++ ) {
			if( i < 4096 ) a64_cmp_imm(ctx, A64_X9, i, 0);
			else {
				a64_mov_imm32(ctx, A64_X10, i);
				a64_cmp_reg(ctx, A64_X9, A64_X10, 0);
			}
			int pos = a64_bcond(ctx, A64_EQ, 0);
			register_jump(ctx, pos, opIdx + 1 + op->extra[i]);
		}
		// Out-of-range: fall through to the next opcode (HL default).
		break;
	}

	// ---------------- Misc ----------------
	case ONullCheck: {
		load_vreg(ctx, A64_X9, op->p1);
		a64_cmp_imm(ctx, A64_X9, 0, 1);
		int pos_nz = a64_bcond(ctx, A64_NE, 0);
		// Null: call hl_null_access — throws a proper HL exception that the
		// try/catch infrastructure can intercept.
		emit_call_native_ptr(ctx, (void*)hl_null_access);
		// hl_null_access does not return, but emit_call_native_ptr followed by
		// the patch lets us continue codegen cleanly. The patch makes non-null
		// paths skip the call.
		int after = BUF_POS();
		a64_patch_branch(ctx, pos_nz, after);
		break;
	}
	case ORet:
		if( f->regs[op->p1]->kind == HVOID ) {
			// no value to return
		} else if( vreg_is_fp(f, op->p1) ) {
			load_vreg_fp(ctx, A64_V0, op->p1);
		} else {
			load_vreg(ctx, A64_X0, op->p1);
		}
		emit_epilogue(ctx);
		break;

	// ---------------- Dynamic field access ----------------
	// Field name is m->code->strings[op->p3]; the hash is computed at JIT
	// time and embedded as a constant.
	case ODynGet: {
		hl_type *dt = f->regs[op->p1];
		int hash = hl_hash_utf8(m->code->strings[op->p3]);
		load_vreg(ctx, A64_X0, op->p2);
		a64_mov_imm32(ctx, A64_X1, hash);
		void *fn;
		int needs_type = 1;
		switch( dt->kind ) {
		case HF32:  fn = (void*)hl_dyn_getf;  needs_type = 0; break;
		case HF64:  fn = (void*)hl_dyn_getd;  needs_type = 0; break;
		case HI64: case HGUID: fn = (void*)hl_dyn_geti64; needs_type = 0; break;
		case HI32: case HUI16: case HUI8: case HBOOL:
			fn = (void*)hl_dyn_geti; break;
		default:    fn = (void*)hl_dyn_getp; break;
		}
		if( needs_type ) a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)dt);
		emit_call_native_ptr(ctx, fn);
		if( dt->kind == HF32 || dt->kind == HF64 )
			store_vreg_fp(ctx, A64_V0, op->p1);
		else
			store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case ODynSet: {
		// Encoding: p1 = obj vreg, p2 = field-name *string index*,
		// p3 = value vreg. The "R" in the opcode descriptor for p2 is
		// misleading — it's actually a constant index into code->strings.
		hl_type *vt = f->regs[op->p3];
		int hash = (int)hl_hash_gen(hl_get_ustring(m->code, op->p2), true);
		load_vreg(ctx, A64_X0, op->p1);              // obj
		a64_mov_imm32(ctx, A64_X1, hash);             // hfield
		void *fn;
		switch( vt->kind ) {
		case HF32:
			fn = (void*)hl_dyn_setf;
			load_vreg_fp(ctx, A64_V0, op->p3);
			break;
		case HF64:
			fn = (void*)hl_dyn_setd;
			load_vreg_fp(ctx, A64_V0, op->p3);
			break;
		case HI64: case HGUID:
			fn = (void*)hl_dyn_seti64;
			load_vreg(ctx, A64_X2, op->p3);
			break;
		case HI32: case HUI16: case HUI8: case HBOOL:
			fn = (void*)hl_dyn_seti;
			a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)vt);
			load_vreg(ctx, A64_X3, op->p3);
			break;
		default:
			fn = (void*)hl_dyn_setp;
			a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)vt);
			load_vreg(ctx, A64_X3, op->p3);
			break;
		}
		emit_call_native_ptr(ctx, fn);
		break;
	}

	// ---------------- Closures ----------------
	case OStaticClosure: {
		// Allocate a vclosure in module-lifetime storage; chain it on
		// closure_list so hl_jit_code can patch its fun pointer from
		// "findex" to the absolute target address once functions are placed.
		int fid = op->p2;
		vclosure *c = (vclosure*)hl_malloc(&m->ctx.alloc, sizeof(vclosure));
		c->hasValue = 0;
		int fidx = m->functions_indexes[fid];
		if( fidx >= m->code->nfunctions ) {
			c->t = m->code->natives[fidx - m->code->nfunctions].t;
			c->fun = m->functions_ptrs[fid];
			c->value = NULL;
		} else {
			c->t = m->code->functions[fidx].type;
			c->fun = (void*)(intptr_t)fid;
			c->value = ctx->closure_list;
			ctx->closure_list = c;
		}
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)c);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OInstanceClosure: {
		// dst = hl_alloc_closure_ptr(ftype, fun_ptr, captured_value)
		// fun_ptr is staged via a 4-instruction MOVZ+3xMOVK chain patched in
		// hl_jit_code once functions_ptrs is final.
		hl_type *fnt = m->code->functions[m->functions_indexes[op->p2]].type;
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)fnt);
		int patch_pos = BUF_POS();
		a64_movz(ctx, A64_X1, 0, 0, 1);
		a64_movk(ctx, A64_X1, 0, 1, 1);
		a64_movk(ctx, A64_X1, 0, 2, 1);
		a64_movk(ctx, A64_X1, 0, 3, 1);
		jlist *j = (jlist*)hl_malloc(&ctx->galloc, sizeof(jlist));
		j->pos = patch_pos;
		j->target = IMM64_TAG(op->p2);
		j->next = ctx->calls;
		ctx->calls = j;
		load_vreg(ctx, A64_X2, op->p3);
		emit_call_native_ptr(ctx, (void*)hl_alloc_closure_ptr);
		store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case OVirtualClosure: {
		// dst = hl_alloc_closure_ptr(method_type, obj->type->vobj_proto[p3], obj)
		// method_type is resolved at JIT time by walking ra->t->obj proto/super
		// chain to find the proto with pindex == p3. The function pointer comes
		// from runtime: obj->type (off 0) -> ->vobj_proto (off 16) -> [p3 * 8].
		hl_type *ot = f->regs[op->p2];
		hl_type *mt = NULL;
		while( mt == NULL && ot != NULL && ot->kind == HOBJ ) {
			for( int i = 0; i < ot->obj->nproto; i++ ) {
				hl_obj_proto *pp = ot->obj->proto + i;
				if( pp->pindex == op->p3 ) {
					mt = m->code->functions[m->functions_indexes[pp->findex]].type;
					break;
				}
			}
			ot = ot->obj->super;
		}
		if( mt == NULL ) { a64_brk(ctx, 0xCDDC); break; }
		// x9 = obj
		load_vreg(ctx, A64_X9, op->p2);
		// x9 = obj->type  (offset 0)
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 8, 0);
		// x9 = obj->type->vobj_proto  (offset HL_WSIZE*2 = 16)
		a64_ldr_imm(ctx, A64_X9, A64_X9, 16, 8, 0);
		// x9 = vobj_proto[p3]
		a64_ldr_imm(ctx, A64_X9, A64_X9, op->p3 * 8, 8, 0);
		// hl_alloc_closure_ptr(mt, x9, obj)
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)mt);
		a64_mov_reg(ctx, A64_X1, A64_X9, 1);
		load_vreg(ctx, A64_X2, op->p2);
		emit_call_native_ptr(ctx, (void*)hl_alloc_closure_ptr);
		store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case OCallClosure: {
		// HDYN closure: args are HDYN, we use hl_dyn_call(c, args[], nargs)
		// then convert result via the matching hl_dyn_castX helper.
		if( f->regs[op->p2]->kind == HDYN ) {
			int nargs = op->p3;
			int args_size = nargs * 8;
			if( args_size & 15 ) args_size += 16 - (args_size & 15);
			// Total stack: args array + 16B scratch for result cast.
			int total = args_size + 16;
			a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, total, 1);
			// Build args[i] = the vdynamic* sitting in op->extra[i]'s slot.
			for( int i = 0; i < nargs; i++ ) {
				load_vreg(ctx, A64_X9, op->extra[i]);
				a64_str_imm(ctx, A64_X9, A64_SP_OR_ZR, i * 8, 8);
			}
			// hl_dyn_call(closure, args_ptr, nargs)
			load_vreg(ctx, A64_X0, op->p2);
			a64_mov_reg(ctx, A64_X1, A64_SP_OR_ZR, 1);
			a64_add_imm(ctx, A64_X1, A64_SP_OR_ZR, 0, 1);
			a64_mov_imm32(ctx, A64_X2, nargs);
			emit_call_native_ptr(ctx, (void*)hl_dyn_call);
			// Result vdynamic* in X0. Cast to dst type if not void.
			if( op->p1 >= 0 && f->regs[op->p1]->kind != HVOID ) {
				hl_type *dt = f->regs[op->p1];
				if( dt->kind == HDYN ) {
					// Direct store, no cast needed.
					store_vreg(ctx, A64_X0, op->p1);
				} else {
					// Stash vdynamic* at SP+args_size, pass &stash to cast helper.
					a64_str_imm(ctx, A64_X0, A64_SP_OR_ZR, args_size, 8);
					a64_add_imm(ctx, A64_X0, A64_SP_OR_ZR, args_size, 1);
					// Argument 2 is always source-type = HDYN; arg 3 (if any) is dst type.
					static hl_type t_dynamic = { HDYN };
					a64_mov_imm64(ctx, A64_X1, (int64_t)(intptr_t)&t_dynamic);
					void *cast_fn;
					int has_arg3 = 0;
					switch( dt->kind ) {
					case HF32: cast_fn = (void*)hl_dyn_castf; break;
					case HF64: cast_fn = (void*)hl_dyn_castd; break;
					case HI64: case HGUID: cast_fn = (void*)hl_dyn_casti64; break;
					case HI32: case HUI16: case HUI8: case HBOOL:
						cast_fn = (void*)hl_dyn_casti; has_arg3 = 1; break;
					default:
						cast_fn = (void*)hl_dyn_castp; has_arg3 = 1; break;
					}
					if( has_arg3 ) a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)dt);
					emit_call_native_ptr(ctx, cast_fn);
					if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
					else                        store_vreg(ctx, A64_X0, op->p1);
				}
			}
			a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, total, 1);
			break;
		}
		// Load hasValue field (offset HL_WSIZE*2 = 16).
		load_vreg(ctx, A64_X16, op->p2);
		a64_ldr_imm(ctx, A64_X9, A64_X16, 16, 4, 0);
		a64_cmp_imm(ctx, A64_X9, 0, 0);
		int jnz = a64_bcond(ctx, A64_NE, 0);
		// No captured value: prepare args, load c->fun (offset 8), BLR.
		prepare_call_args(ctx, op->p3, op->extra);
		load_vreg(ctx, A64_X16, op->p2);
		a64_ldr_imm(ctx, A64_X16, A64_X16, 8, 8, 0);
		a64_blr(ctx, A64_X16);
		int jend = a64_b(ctx, 0);
		a64_patch_branch(ctx, jnz, BUF_POS());
		// With captured value: x0 = captured, then user args fill x1..x7 +
		// d0..d7 + stack following AAPCS64 with one fewer GPR slot.
		// Pass 1: count stack-passed args (x0 taken by captured → ngpr starts at 1).
		{
			int ngpr_c = 1, nfpr_c = 0;
			int stack_bytes_c = 0;
			for( int i = 0; i < op->p3; i++ ) {
				hl_type *t = f->regs[op->extra[i]];
				int is_fp = (t->kind == HF32 || t->kind == HF64);
				if( is_fp ) { if( nfpr_c < 8 ) nfpr_c++; else stack_bytes_c += 8; }
				else        { if( ngpr_c < 8 ) ngpr_c++; else stack_bytes_c += 8; }
			}
			if( stack_bytes_c & 0xf ) stack_bytes_c += 16 - (stack_bytes_c & 0xf);
			if( stack_bytes_c ) a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, stack_bytes_c, 1);
			// Pass 2: place args.
			ngpr_c = 1; nfpr_c = 0;
			int stack_off_c = 0;
			for( int i = 0; i < op->p3; i++ ) {
				hl_type *t = f->regs[op->extra[i]];
				int is_fp = (t->kind == HF32 || t->kind == HF64);
				if( is_fp ) {
					if( nfpr_c < 8 ) {
						load_vreg_fp(ctx, (a64_vreg)(A64_V0 + nfpr_c), op->extra[i]);
						nfpr_c++;
					} else {
						load_vreg_fp(ctx, A64_V16, op->extra[i]);
						a64_str_fp(ctx, A64_V16, A64_SP_OR_ZR, stack_off_c, t->kind == HF64);
						stack_off_c += 8;
					}
				} else {
					if( ngpr_c < 8 ) {
						load_vreg(ctx, (a64_greg)(A64_X0 + ngpr_c), op->extra[i]);
						ngpr_c++;
					} else {
						load_vreg(ctx, A64_X9, op->extra[i]);
						a64_str_imm(ctx, A64_X9, A64_SP_OR_ZR, stack_off_c, vreg_size(f, op->extra[i]));
						stack_off_c += 8;
					}
				}
			}
			// Load c->value into x0 last so the loop's load_vreg into x1+ doesn't
			// clobber it via cache MOV.
			load_vreg(ctx, A64_X16, op->p2);
			a64_ldr_imm(ctx, A64_X0, A64_X16, 24, 8, 0);
			a64_ldr_imm(ctx, A64_X16, A64_X16, 8, 8, 0);
			a64_blr(ctx, A64_X16);
			if( stack_bytes_c ) a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, stack_bytes_c, 1);
		}
		a64_patch_branch(ctx, jend, BUF_POS());
		if( op->p1 >= 0 && f->regs[op->p1]->kind != HVOID ) {
			if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
			else store_vreg(ctx, A64_X0, op->p1);
		}
		break;
	}

	// ---------------- Cast helpers ----------------
	case OToVirtual: {
		// dst = hl_to_virtual(dst_type, src)
		hl_type *dt = f->regs[op->p1];
		// Ensure the runtime obj of src is initialised (x86 backend does
		// hl_get_obj_rt() unconditionally for HOBJ inputs at JIT time).
		hl_type *st = f->regs[op->p2];
		if( st->kind == HOBJ ) hl_get_obj_rt(st);
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)dt);
		load_vreg(ctx, A64_X1, op->p2);
		emit_call_native_ptr(ctx, (void*)hl_to_virtual);
		store_vreg(ctx, A64_X0, op->p1);
		break;
	}
	case OToSFloat: {
		// dst (HF32 or HF64) ← src (int* or other float)
		if( op->p1 == op->p2 ) break;
		hl_type *st = f->regs[op->p2];
		hl_type *dt = f->regs[op->p1];
		if( st->kind == HI32 || st->kind == HUI16 || st->kind == HUI8 ) {
			load_vreg(ctx, A64_X9, op->p2);
			if( dt->kind == HF64 ) a64_scvtf_d(ctx, A64_V16, A64_X9, 0);
			else                    a64_scvtf_s(ctx, A64_V16, A64_X9, 0);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else if( st->kind == HI64 ) {
			load_vreg(ctx, A64_X9, op->p2);
			if( dt->kind == HF64 ) a64_scvtf_d(ctx, A64_V16, A64_X9, 1);
			else                    a64_scvtf_s(ctx, A64_V16, A64_X9, 1);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else if( st->kind == HF64 && dt->kind == HF32 ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_fcvt_d_to_s(ctx, A64_V16, A64_V16);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else if( st->kind == HF32 && dt->kind == HF64 ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_fcvt_s_to_d(ctx, A64_V16, A64_V16);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_brk(ctx, 0xF20C);
		}
		break;
	}
	case OToUFloat: {
		if( op->p1 == op->p2 ) break;
		hl_type *st = f->regs[op->p2];
		hl_type *dt = f->regs[op->p1];
		if( st->kind == HI32 || st->kind == HUI16 || st->kind == HUI8 || st->kind == HBOOL ) {
			load_vreg(ctx, A64_X9, op->p2);
			if( dt->kind == HF64 ) a64_ucvtf_d(ctx, A64_V16, A64_X9, 0);
			else                    a64_ucvtf_s(ctx, A64_V16, A64_X9, 0);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else if( st->kind == HI64 ) {
			load_vreg(ctx, A64_X9, op->p2);
			if( dt->kind == HF64 ) a64_ucvtf_d(ctx, A64_V16, A64_X9, 1);
			else                    a64_ucvtf_s(ctx, A64_V16, A64_X9, 1);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_brk(ctx, 0xF20D);
		}
		break;
	}
	case OToInt: {
		hl_type *st = f->regs[op->p2];
		hl_type *dt = f->regs[op->p1];
		if( op->p1 == op->p2 ) break;
		if( st->kind == HF64 ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_fcvtzs_d(ctx, A64_X9, A64_V16, dt->kind == HI64);
			store_vreg(ctx, A64_X9, op->p1);
		} else if( st->kind == HF32 ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_fcvtzs_s(ctx, A64_X9, A64_V16, dt->kind == HI64);
			store_vreg(ctx, A64_X9, op->p1);
		} else if( dt->kind == HI64 && st->kind == HI32 ) {
			// Sign-extend 32→64. Use SXTW Xd, Wn — encoded as SBFM #0,#31.
			// Quick path: load 4-byte signed, store as 8-byte (ldur with
			// signed extend then stur 8-byte). Our load_vreg with size 4
			// is zero-extend, so we must sign-extend explicitly: SBFM.
			load_vreg(ctx, A64_X9, op->p2);
			// SBFM Xd, Xn, #0, #31  (SXTW)
			a64_emit(ctx, 0x93407C00 | ((A64_X9 & 0x1f) << 5) | (A64_X9 & 0x1f));
			store_vreg(ctx, A64_X9, op->p1);
		} else {
			// Plain copy — load and store with appropriate widths.
			load_vreg(ctx, A64_X9, op->p2);
			store_vreg(ctx, A64_X9, op->p1);
		}
		break;
	}

	// ---------------- Box primitives into Dynamic ----------------
	case OToDyn: {
		// For pointer-kinded source: if NULL, dst = NULL; else hl_alloc_dynamic(ra->t),
		// store the pointer at offset 8.
		// For non-pointer: hl_alloc_dynamic(ra->t), store value at offset 8.
		hl_type *st = f->regs[op->p2];
		int is_ptr = hl_is_ptr(st);
		int sz = vreg_size(f, op->p2);
		int is_fp = vreg_is_fp(f, op->p2);
		int jskip_pos = -1;
		if( is_ptr ) {
			load_vreg(ctx, A64_X9, op->p2);
			a64_cmp_imm(ctx, A64_X9, 0, 1);
			int jnz = a64_bcond(ctx, A64_NE, 0);
			// null branch: result = NULL
			a64_mov_imm64(ctx, A64_X9, 0);
			store_vreg(ctx, A64_X9, op->p1);
			jskip_pos = a64_b(ctx, 0);
			a64_patch_branch(ctx, jnz, BUF_POS());
		}
		// hl_alloc_dynamic(ra->t)
		a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)st);
		emit_call_native_ptr(ctx, (void*)hl_alloc_dynamic);
		// Copy value into result at offset 8 (HDYN_VALUE).
		if( is_fp ) {
			load_vreg_fp(ctx, A64_V16, op->p2);
			a64_str_fp(ctx, A64_V16, A64_X0, 8, st->kind == HF64);
		} else {
			load_vreg(ctx, A64_X9, op->p2);
			a64_str_imm(ctx, A64_X9, A64_X0, 8, sz <= 4 ? 4 : 8);
		}
		store_vreg(ctx, A64_X0, op->p1);
		if( jskip_pos >= 0 ) a64_patch_branch(ctx, jskip_pos, BUF_POS());
		break;
	}

	// ---------------- Casts ----------------
	case OSafeCast: {
		// dst = hl_dyn_cast<X>(&src, src_type, dst_type)
		// (FP/I64 variants drop the dst_type arg.)
		hl_type *dt = f->regs[op->p1];
		hl_type *st = f->regs[op->p2];
		// x0 = &src_slot
		int off = vreg_offset(op->p2);
		if( off >= -4095 && off <= 4095 ) {
			if( off < 0 ) a64_sub_imm(ctx, A64_X0, A64_FP, -off, 1);
			else          a64_add_imm(ctx, A64_X0, A64_FP, off, 1);
		} else {
			a64_mov_imm64(ctx, A64_X0, off);
			a64_add_reg(ctx, A64_X0, A64_FP, A64_X0, 1);
		}
		a64_mov_imm64(ctx, A64_X1, (int64_t)(intptr_t)st);
		void *fn; int two_arg = 0;
		switch( dt->kind ) {
		case HF32:  fn = (void*)hl_dyn_castf;  two_arg = 1; break;
		case HF64:  fn = (void*)hl_dyn_castd;  two_arg = 1; break;
		case HI64: case HGUID: fn = (void*)hl_dyn_casti64; two_arg = 1; break;
		case HI32: case HUI16: case HUI8: case HBOOL:
			fn = (void*)hl_dyn_casti; break;
		default:    fn = (void*)hl_dyn_castp; break;
		}
		if( !two_arg ) a64_mov_imm64(ctx, A64_X2, (int64_t)(intptr_t)dt);
		emit_call_native_ptr(ctx, fn);
		if( dt->kind == HF32 || dt->kind == HF64 )
			store_vreg_fp(ctx, A64_V0, op->p1);
		else
			store_vreg(ctx, A64_X0, op->p1);
		break;
	}

	// ---------------- Type metadata ----------------
	case OType:
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)(m->code->types + op->p2));
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OGetType: {
		// dst = (src != NULL) ? src->t : &hlt_void
		load_vreg(ctx, A64_X9, op->p2);
		a64_cmp_imm(ctx, A64_X9, 0, 1);
		int pos_nz = a64_bcond(ctx, A64_NE, 0);
		// null branch
		a64_mov_imm64(ctx, A64_X9, (int64_t)(intptr_t)&hlt_void);
		int pos_end = a64_b(ctx, 0);
		// non-null branch
		int nz_target = BUF_POS();
		a64_patch_branch(ctx, pos_nz, nz_target);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 8, 0); // load v->t
		int end_target = BUF_POS();
		a64_patch_branch(ctx, pos_end, end_target);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OGetTID:
		// dst = src->kind (int32 at offset 0 of hl_type)
		load_vreg(ctx, A64_X9, op->p2);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 4, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;

	// ---------------- Allocation ----------------
	case ONew: {
		// dst = hl_alloc_obj/dynobj/virtual(dst_type)
		hl_type *t = f->regs[op->p1];
		void *allocFn = NULL;
		int nargs = 1;
		switch( t->kind ) {
		case HOBJ: case HSTRUCT: allocFn = (void*)hl_alloc_obj; break;
		case HDYNOBJ: allocFn = (void*)hl_alloc_dynobj; nargs = 0; break;
		case HVIRTUAL: allocFn = (void*)hl_alloc_virtual; break;
		default: a64_brk(ctx, 0x0E01); goto onew_done; // unhandled type
		}
		if( nargs ) a64_mov_imm64(ctx, A64_X0, (int64_t)(intptr_t)t);
		emit_call_native_ptr(ctx, allocFn);
		store_vreg(ctx, A64_X0, op->p1);
		onew_done:;
		break;
	}

	// ---------------- Calls (extended) ----------------
	case OCallThis: {
		// Equivalent to OCallN where arg 0 = vreg 0 (this), method index = p2.
		// vtable: this->t->vobj_proto[p2]
		int nargs = op->p3 + 1;
		int *callargs = (int*)hl_malloc(&ctx->falloc, sizeof(int) * nargs);
		callargs[0] = 0;
		for( int i = 1; i < nargs; i++ ) callargs[i] = op->extra[i-1];
		// First, fetch the function pointer from the vtable.
		// this is vreg 0, t is at offset 0, vobj_proto at offset 16 (HL_WSIZE*2).
		load_vreg(ctx, A64_X9, 0);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 8, 0);   // x9 = this->t
		a64_ldr_imm(ctx, A64_X9, A64_X9, 16, 8, 0);  // x9 = t->vobj_proto
		// Now we'd want to load proto[p2] and BLR it. But we also need to
		// set up args first. Pre-compute the function pointer into x19? No,
		// we don't preserve x19 across our trivial codegen. Simplest: stash
		// the proto base in [fp - frame - 8] temp slot? For bring-up just
		// reload after arg setup.
		// Strategy: do prepare_call_args first (uses x9/x10 internally,
		// but only the load_vreg helpers — they end with the loaded value).
		// Then re-fetch the proto pointer.
		(void)callargs; // suppress unused — we use it below.
		prepare_call_args(ctx, nargs, callargs);
		load_vreg(ctx, A64_X16, 0);
		a64_ldr_imm(ctx, A64_X16, A64_X16, 0, 8, 0);  // this->t
		a64_ldr_imm(ctx, A64_X16, A64_X16, 16, 8, 0); // t->vobj_proto
		a64_ldr_imm(ctx, A64_X16, A64_X16, op->p2 * 8, 8, 0);
		a64_blr(ctx, A64_X16);
		if( op->p1 >= 0 && f->regs[op->p1]->kind != HVOID ) {
			if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
			else store_vreg(ctx, A64_X0, op->p1);
		}
		break;
	}
	case OCallMethod: {
		hl_type *ot = f->regs[op->extra[0]];
		if( ot->kind == HOBJ ) {
			// Vtable dispatch: this->t->vobj_proto[p2](this, args...)
			prepare_call_args(ctx, op->p3, op->extra);
			load_vreg(ctx, A64_X16, op->extra[0]);
			a64_ldr_imm(ctx, A64_X16, A64_X16, 0, 8, 0);
			a64_ldr_imm(ctx, A64_X16, A64_X16, 16, 8, 0);
			a64_ldr_imm(ctx, A64_X16, A64_X16, op->p2 * 8, 8, 0);
			a64_blr(ctx, A64_X16);
			if( op->p1 >= 0 && f->regs[op->p1]->kind != HVOID ) {
				if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
				else store_vreg(ctx, A64_X0, op->p1);
			}
		} else if( ot->kind == HVIRTUAL ) {
			// vfield_ptr = hl_vfields(o)[op->p2]  (the resolved method)
			//   if non-null: call vfield_ptr(o->value, args[1..])
			//   else: hl_dyn_call_obj(o->value, ftype, hashed_name, packed_args, ret)
			int vfield_off = (int)sizeof(vvirtual) + op->p2 * (int)sizeof(void*);
			load_vreg(ctx, A64_X9, op->extra[0]);              // x9 = o
			a64_ldr_imm(ctx, A64_X10, A64_X9, vfield_off, 8, 0); // x10 = vfield
			a64_cmp_imm(ctx, A64_X10, 0, 1);
			int j_has = a64_bcond(ctx, A64_NE, 0);
			// ---- NULL path: hl_dyn_call_obj ----
			// Build packed args[op->p3-1] on the stack, plus an optional
			// vdynamic for non-pointer non-void return values.
			hl_type *dt = (op->p1 >= 0) ? f->regs[op->p1] : NULL;
			int nargs = op->p3 - 1;
			int need_dyn = (dt && !hl_is_ptr(dt) && dt->kind != HVOID) ? 1 : 0;
			int params_size = nargs * 8 + (need_dyn ? (int)sizeof(vdynamic) : 0);
			if( params_size & 15 ) params_size += 16 - (params_size & 15);
			if( params_size ) a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, params_size, 1);
			// Fill arg slots at [sp+0..sp+(nargs-1)*8] with pointer-to-value
			// (for non-ptr types, we need the address of the source vreg slot;
			// for ptr types, the value itself).
			for( int i = 0; i < nargs; i++ ) {
				int vi = op->extra[i + 1];
				hl_type *at = f->regs[vi];
				if( hl_is_ptr(at) ) {
					load_vreg(ctx, A64_X11, vi);
				} else {
					int off = vreg_offset(vi);
					if( off >= -4095 && off <= 0 )
						a64_sub_imm(ctx, A64_X11, A64_FP, -off, 1);
					else if( off >= 0 && off <= 4095 )
						a64_add_imm(ctx, A64_X11, A64_FP, off, 1);
					else {
						a64_mov_imm64(ctx, A64_X11, off);
						a64_add_reg(ctx, A64_X11, A64_FP, A64_X11, 1);
					}
				}
				a64_str_imm(ctx, A64_X11, A64_SP_OR_ZR, i * 8, 8);
			}
			// args: x0 = o->value, x1 = ftype, x2 = hashed_name,
			//       x3 = packed args ptr (sp), x4 = ret ptr (or NULL)
			load_vreg(ctx, A64_X0, op->extra[0]);
			a64_ldr_imm(ctx, A64_X0, A64_X0, 8, 8, 0); // o->value (offset HL_WSIZE)
			a64_mov_imm64(ctx, A64_X1, (int64_t)(intptr_t)ot->virt->fields[op->p2].t);
			a64_mov_imm32(ctx, A64_X2, ot->virt->fields[op->p2].hashed_name);
			a64_mov_reg(ctx, A64_X3, A64_SP_OR_ZR, 1);
			a64_add_imm(ctx, A64_X3, A64_SP_OR_ZR, 0, 1);
			if( need_dyn ) {
				int dyn_off = params_size - (int)sizeof(vdynamic);
				a64_add_imm(ctx, A64_X4, A64_SP_OR_ZR, dyn_off, 1);
			} else {
				a64_mov_imm64(ctx, A64_X4, 0);
			}
			emit_call_native_ptr(ctx, (void*)hl_dyn_call_obj);
			// Result handling.
			if( op->p1 >= 0 && dt->kind != HVOID ) {
				if( need_dyn ) {
					int dyn_off = params_size - (int)sizeof(vdynamic);
					if( vreg_is_fp(f, op->p1) )
						a64_ldr_fp(ctx, A64_V0, A64_SP_OR_ZR, dyn_off + 8, dt->kind == HF64);
					else
						a64_ldr_imm(ctx, A64_X0, A64_SP_OR_ZR, dyn_off + 8, vreg_size(f, op->p1), 0);
				}
				if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
				else                        store_vreg(ctx, A64_X0, op->p1);
			}
			if( params_size ) a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, params_size, 1);
			int j_end = a64_b(ctx, 0);
			// ---- Non-NULL path: call vfield_ptr(o->value, args[1..]) ----
			a64_patch_branch(ctx, j_has, BUF_POS());
			// AAPCS64: x0 reserved for o->value. Args extra[1..nargs] fill
			// x1..x7 + d0..d7 + stack (in declaration order across both banks
			// for the overflow). Compute the overflow stack first.
			{
				int ngpr_m = 1, nfpr_m = 0;
				int stack_overflow = 0;
				for( int i = 0; i < nargs; i++ ) {
					hl_type *at = f->regs[op->extra[i + 1]];
					int is_fp = (at->kind == HF32 || at->kind == HF64);
					if( is_fp ) { if( nfpr_m < 8 ) nfpr_m++; else stack_overflow += 8; }
					else        { if( ngpr_m < 8 ) ngpr_m++; else stack_overflow += 8; }
				}
				if( stack_overflow & 0xf ) stack_overflow += 16 - (stack_overflow & 0xf);
				int total = 16 + stack_overflow; // 16B stash + overflow
				int stash_off = stack_overflow;  // stash above overflow args
				a64_sub_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, total, 1);
				// Stash vfield (currently in x10) at [SP + stash_off + 0].
				a64_str_imm(ctx, A64_X10, A64_SP_OR_ZR, stash_off, 8);
				// Compute o->value into x11 and stash at [SP + stash_off + 8].
				load_vreg(ctx, A64_X11, op->extra[0]);
				a64_ldr_imm(ctx, A64_X11, A64_X11, 8, 8, 0);
				a64_str_imm(ctx, A64_X11, A64_SP_OR_ZR, stash_off + 8, 8);
				// Place args.
				ngpr_m = 1; nfpr_m = 0;
				int stack_off_m = 0;
				for( int i = 0; i < nargs; i++ ) {
					int vi = op->extra[i + 1];
					hl_type *at = f->regs[vi];
					int is_fp = (at->kind == HF32 || at->kind == HF64);
					if( is_fp ) {
						if( nfpr_m < 8 ) {
							load_vreg_fp(ctx, (a64_vreg)(A64_V0 + nfpr_m), vi);
							nfpr_m++;
						} else {
							load_vreg_fp(ctx, A64_V16, vi);
							a64_str_fp(ctx, A64_V16, A64_SP_OR_ZR, stack_off_m, at->kind == HF64);
							stack_off_m += 8;
						}
					} else {
						if( ngpr_m < 8 ) {
							load_vreg(ctx, (a64_greg)(A64_X0 + ngpr_m), vi);
							ngpr_m++;
						} else {
							load_vreg(ctx, A64_X9, vi);
							a64_str_imm(ctx, A64_X9, A64_SP_OR_ZR, stack_off_m, vreg_size(f, vi));
							stack_off_m += 8;
						}
					}
				}
				// Recover vfield and o->value.
				a64_ldr_imm(ctx, A64_X16, A64_SP_OR_ZR, stash_off, 8, 0);
				a64_ldr_imm(ctx, A64_X0,  A64_SP_OR_ZR, stash_off + 8, 8, 0);
				a64_blr(ctx, A64_X16);
				if( op->p1 >= 0 && f->regs[op->p1]->kind != HVOID ) {
					if( vreg_is_fp(f, op->p1) ) store_vreg_fp(ctx, A64_V0, op->p1);
					else                         store_vreg(ctx, A64_X0, op->p1);
				}
				a64_add_imm(ctx, A64_SP_OR_ZR, A64_SP_OR_ZR, total, 1);
			}
			a64_patch_branch(ctx, j_end, BUF_POS());
		} else {
			a64_brk(ctx, 0xCAA0);
		}
		break;
	}

	// ---------------- Exceptions (no return) ----------------
	case OThrow:
		load_vreg(ctx, A64_X0, op->p1);
		emit_call_native_ptr(ctx, (void*)hl_throw);
		// unreachable, but be safe
		break;
	case ORethrow:
		load_vreg(ctx, A64_X0, op->p1);
		emit_call_native_ptr(ctx, (void*)hl_rethrow);
		break;

	// ---------------- Ref operations ----------------
	case ORef: {
		// dst = &vreg[op->p2] — i.e. compute FP + vreg_offset(p2)
		int off = vreg_offset(op->p2);
		if( off >= -4095 && off <= 4095 ) {
			if( off < 0 ) a64_sub_imm(ctx, A64_X9, A64_FP, -off, 1);
			else          a64_add_imm(ctx, A64_X9, A64_FP, off, 1);
		} else {
			a64_mov_imm64(ctx, A64_X10, off);
			a64_add_reg(ctx, A64_X9, A64_FP, A64_X10, 1);
		}
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OUnref:
		load_vreg(ctx, A64_X9, op->p2);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 8, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	case OSetref:
		load_vreg(ctx, A64_X9, op->p1);   // ref
		load_vreg(ctx, A64_X10, op->p2);  // value
		a64_str_imm(ctx, A64_X10, A64_X9, 0, 8);
		break;
	case ORefData: {
		// dst = ra + sizeof(varray) — pointer to the array's payload data.
		// x86 backend only handles HARRAY here; mirror that.
		hl_type *st = f->regs[op->p2];
		if( st->kind != HARRAY ) { a64_brk(ctx, 0xCDDB); break; }
		load_vreg(ctx, A64_X9, op->p2);
		a64_add_imm(ctx, A64_X9, A64_X9, (int)sizeof(varray), 1);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case ORefOffset: {
		// dst = ra + rb * sizeof(elem), elem size from dst->t->tparam.
		int elem_sz = hl_type_size(f->regs[op->p1]->tparam);
		load_vreg(ctx, A64_X9, op->p2);   // base
		load_vreg(ctx, A64_X10, op->p3);  // offset count
		switch( elem_sz ) {
		case 1:
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			break;
		case 2: case 4: case 8: {
			// add x9, x9, x10, lsl #log2(elem_sz)
			int sh = (elem_sz == 2) ? 1 : (elem_sz == 4) ? 2 : 3;
			uint32_t ins =
				(1u << 31) | (0x0B << 24) | (0u << 22) /*LSL*/ |
				((A64_X10 & 0x1f) << 16) | ((sh & 0x3f) << 10) |
				((A64_X9 & 0x1f) << 5) | (A64_X9 & 0x1f);
			a64_emit(ctx, ins);
			break;
		}
		default:
			// Generic: x10 *= elem_sz then add. Use MOVZ + MUL.
			a64_mov_imm64(ctx, A64_X11, elem_sz);
			a64_mul(ctx, A64_X10, A64_X10, A64_X11, 1);
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			break;
		}
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}

	// ---------------- Array ----------------
	case OArraySize: {
		// dst = src->size — for non-abstract arrays at offset 16 (HL_WSIZE*2)
		hl_type *st = f->regs[op->p2];
		int offset = (st->kind == HABSTRACT) ? 12 : 16; // wsize+4 vs wsize*2
		load_vreg(ctx, A64_X9, op->p2);
		a64_ldr_imm(ctx, A64_X9, A64_X9, offset, 4, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}

	// ---------------- Typed array element access ----------------
	// dst = arr[idx]. Non-abstract (regular) varray only — abstract arrays
	// (raw memory) need a separate path the bring-up doesn't cover yet.
	case OGetArray: {
		hl_type *src_t = f->regs[op->p2];
		if( src_t->kind == HABSTRACT ) {
			// HABSTRACT array: raw memory, no varray header.
			// If dst is HOBJ/HSTRUCT: return the *address* (LEA, osize = rt->size).
			// Else: read the value at addr (osize = sizeof(void*)).
			hl_type *dt = f->regs[op->p1];
			int is_addr = (dt->kind == HOBJ || dt->kind == HSTRUCT);
			int osize;
			if( is_addr ) {
				hl_runtime_obj *rt = hl_get_obj_rt(dt);
				osize = rt->size;
			} else {
				osize = 8;
			}
			load_vreg(ctx, A64_X9, op->p2);   // base
			load_vreg(ctx, A64_X10, op->p3);  // index
			// addr = base + idx*osize
			if( osize == 1 ) {
				a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			} else if( osize == 2 || osize == 4 || osize == 8 ) {
				int sh = (osize == 2) ? 1 : (osize == 4) ? 2 : 3;
				uint32_t ins =
					(1u << 31) | (0x0B << 24) | (0u << 22) |
					((A64_X10 & 0x1f) << 16) | ((sh & 0x3f) << 10) |
					((A64_X9 & 0x1f) << 5) | (A64_X9 & 0x1f);
				a64_emit(ctx, ins);
			} else {
				a64_mov_imm64(ctx, A64_X11, osize);
				a64_mul(ctx, A64_X10, A64_X10, A64_X11, 1);
				a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			}
			if( is_addr ) {
				store_vreg(ctx, A64_X9, op->p1);  // return the address
			} else {
				// Load value at addr. Use dst's vreg_size.
				int sz = vreg_size(f, op->p1);
				if( vreg_is_fp(f, op->p1) ) {
					a64_ldr_fp(ctx, A64_V16, A64_X9, 0, dt->kind == HF64);
					store_vreg_fp(ctx, A64_V16, op->p1);
				} else {
					a64_ldr_imm(ctx, A64_X9, A64_X9, 0, sz, 0);
					store_vreg(ctx, A64_X9, op->p1);
				}
			}
			break;
		}
		int elem_sz = hl_type_size(f->regs[op->p1]);
		int header = (int)sizeof(varray);
		load_vreg(ctx, A64_X9, op->p2);    // array ptr
		load_vreg(ctx, A64_X10, op->p3);   // index
		// addr = ptr + idx * elem_sz + header
		if( elem_sz == 1 ) {
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		} else {
			// idx * elem_sz: synthesise via shift if power of two.
			if( elem_sz == 2 )      a64_mov_imm32(ctx, A64_X11, 1);
			else if( elem_sz == 4 ) a64_mov_imm32(ctx, A64_X11, 2);
			else if( elem_sz == 8 ) a64_mov_imm32(ctx, A64_X11, 3);
			else                    { a64_mov_imm32(ctx, A64_X11, elem_sz); }
			if( elem_sz == 2 || elem_sz == 4 || elem_sz == 8 ) {
				a64_lsl_reg(ctx, A64_X10, A64_X10, A64_X11, 1);
			} else {
				a64_mul(ctx, A64_X10, A64_X10, A64_X11, 1);
			}
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		}
		a64_add_imm(ctx, A64_X9, A64_X9, header, 1);
		if( vreg_is_fp(f, op->p1) ) {
			a64_ldr_fp(ctx, A64_V16, A64_X9, 0, f->regs[op->p1]->kind == HF64);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_ldr_imm(ctx, A64_X9, A64_X9, 0, elem_sz, 0);
			store_vreg(ctx, A64_X9, op->p1);
		}
		break;
	}
	case OSetArray: {
		hl_type *dst_t = f->regs[op->p1];
		if( dst_t->kind == HABSTRACT ) {
			// HABSTRACT array: arr[idx] = value, no varray header.
			// If value is HOBJ/HSTRUCT: memcpy struct contents (rt->size bytes).
			// Else: write value (sizeof(void*) bytes).
			hl_type *rb_t = f->regs[op->p3];
			int is_struct_copy = (rb_t->kind == HOBJ || rb_t->kind == HSTRUCT);
			int osize;
			if( is_struct_copy ) {
				hl_runtime_obj *rt = hl_get_obj_rt(rb_t);
				osize = rt->size;
			} else {
				osize = 8;
			}
			load_vreg(ctx, A64_X9, op->p1);   // base
			load_vreg(ctx, A64_X10, op->p2);  // index
			// addr = base + idx*osize → X9
			if( osize == 1 ) {
				a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			} else if( osize == 2 || osize == 4 || osize == 8 ) {
				int sh = (osize == 2) ? 1 : (osize == 4) ? 2 : 3;
				uint32_t ins =
					(1u << 31) | (0x0B << 24) | (0u << 22) |
					((A64_X10 & 0x1f) << 16) | ((sh & 0x3f) << 10) |
					((A64_X9 & 0x1f) << 5) | (A64_X9 & 0x1f);
				a64_emit(ctx, ins);
			} else {
				a64_mov_imm64(ctx, A64_X11, osize);
				a64_mul(ctx, A64_X10, A64_X10, A64_X11, 1);
				a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
			}
			if( is_struct_copy ) {
				// memcpy(addr, src_ptr, osize) — src is HOBJ/HSTRUCT pointer.
				a64_mov_reg(ctx, A64_X0, A64_X9, 1);
				load_vreg(ctx, A64_X1, op->p3);
				a64_mov_imm32(ctx, A64_X2, osize);
				emit_call_native_ptr(ctx, (void*)memcpy);
			} else {
				// Simple value store. Use sizeof(void*) bytes.
				load_vreg(ctx, A64_X11, op->p3);
				a64_str_imm(ctx, A64_X11, A64_X9, 0, 8);
			}
			break;
		}
		int elem_sz = hl_type_size(f->regs[op->p3]);
		int header = (int)sizeof(varray);
		load_vreg(ctx, A64_X9, op->p1);    // array ptr
		load_vreg(ctx, A64_X10, op->p2);   // index
		if( elem_sz == 1 ) {
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		} else {
			if( elem_sz == 2 )      a64_mov_imm32(ctx, A64_X11, 1);
			else if( elem_sz == 4 ) a64_mov_imm32(ctx, A64_X11, 2);
			else if( elem_sz == 8 ) a64_mov_imm32(ctx, A64_X11, 3);
			else                    { a64_mov_imm32(ctx, A64_X11, elem_sz); }
			if( elem_sz == 2 || elem_sz == 4 || elem_sz == 8 ) {
				a64_lsl_reg(ctx, A64_X10, A64_X10, A64_X11, 1);
			} else {
				a64_mul(ctx, A64_X10, A64_X10, A64_X11, 1);
			}
			a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		}
		a64_add_imm(ctx, A64_X9, A64_X9, header, 1);
		if( vreg_is_fp(f, op->p3) ) {
			load_vreg_fp(ctx, A64_V16, op->p3);
			a64_str_fp(ctx, A64_V16, A64_X9, 0, f->regs[op->p3]->kind == HF64);
		} else {
			load_vreg(ctx, A64_X11, op->p3);
			a64_str_imm(ctx, A64_X11, A64_X9, 0, elem_sz);
		}
		break;
	}

	// ---------------- Untyped memory access ----------------
	// OGetI8/I16/OGetMem: dst = *(T*)(base + index)
	case OGetI8: {
		load_vreg(ctx, A64_X9, op->p2);   // base
		load_vreg(ctx, A64_X10, op->p3);  // index
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 1, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OGetI16: {
		load_vreg(ctx, A64_X9, op->p2);
		load_vreg(ctx, A64_X10, op->p3);
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		a64_ldr_imm(ctx, A64_X9, A64_X9, 0, 2, 0);
		store_vreg(ctx, A64_X9, op->p1);
		break;
	}
	case OGetMem: {
		// Size of dst type determines the access width.
		int sz = vreg_size(f, op->p1);
		load_vreg(ctx, A64_X9, op->p2);
		load_vreg(ctx, A64_X10, op->p3);
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		if( vreg_is_fp(f, op->p1) ) {
			a64_ldr_fp(ctx, A64_V16, A64_X9, 0, f->regs[op->p1]->kind == HF64);
			store_vreg_fp(ctx, A64_V16, op->p1);
		} else {
			a64_ldr_imm(ctx, A64_X9, A64_X9, 0, sz, 0);
			store_vreg(ctx, A64_X9, op->p1);
		}
		break;
	}
	case OSetI8: {
		load_vreg(ctx, A64_X9, op->p1);   // base
		load_vreg(ctx, A64_X10, op->p2);  // index
		load_vreg(ctx, A64_X11, op->p3);  // value
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		a64_str_imm(ctx, A64_X11, A64_X9, 0, 1);
		break;
	}
	case OSetI16: {
		load_vreg(ctx, A64_X9, op->p1);
		load_vreg(ctx, A64_X10, op->p2);
		load_vreg(ctx, A64_X11, op->p3);
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		a64_str_imm(ctx, A64_X11, A64_X9, 0, 2);
		break;
	}
	case OSetMem: {
		int sz = vreg_size(f, op->p3);
		load_vreg(ctx, A64_X9, op->p1);
		load_vreg(ctx, A64_X10, op->p2);
		a64_add_reg(ctx, A64_X9, A64_X9, A64_X10, 1);
		if( vreg_is_fp(f, op->p3) ) {
			load_vreg_fp(ctx, A64_V16, op->p3);
			a64_str_fp(ctx, A64_V16, A64_X9, 0, f->regs[op->p3]->kind == HF64);
		} else {
			load_vreg(ctx, A64_X11, op->p3);
			a64_str_imm(ctx, A64_X11, A64_X9, 0, sz);
		}
		break;
	}

	default:
		// Loud failure: BRK with the opcode in the immediate.
		a64_brk(ctx, (uint16_t)op->op);
		break;
	}
}

int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f ) {
	ctx->m = m;
	ctx->f = f;
	ctx->functionPos = BUF_POS();
	ctx->opsPos = (int*)hl_malloc(&ctx->falloc, sizeof(int) * f->nops);

	// Frame size: 8 bytes per vreg. Real backend will pack by type.
	int frame = 8 * f->nregs;
	cache_clear();
	emit_prologue(ctx, frame);

	// Prescan for jump targets — those opcodes start with cache cleared
	// since the incoming control flow might come from elsewhere with a
	// different cache state. Labels and Catch points are unconditional
	// targets; J* opcodes name an explicit target offset.
	unsigned char *is_target = (unsigned char*)hl_malloc(&ctx->falloc, f->nops);
	memset(is_target, 0, f->nops);
	for( int i = 0; i < f->nops; i++ ) {
		hl_opcode *o = f->ops + i;
		int t = -1;
		switch( o->op ) {
		case OJAlways: t = i + 1 + o->p1; break;
		case OJTrue: case OJFalse: case OJNull: case OJNotNull:
		case OTrap:
			t = i + 1 + o->p2; break;
		case OJSLt: case OJSGte: case OJSGt: case OJSLte:
		case OJULt: case OJUGte: case OJNotLt: case OJNotGte:
		case OJEq: case OJNotEq:
			t = i + 1 + o->p3; break;
		case OSwitch:
			for( int k = 0; k < o->p2; k++ ) {
				int tk = i + 1 + o->extra[k];
				if( tk >= 0 && tk < f->nops ) is_target[tk] = 1;
			}
			break;
		default: break;
		}
		if( t >= 0 && t < f->nops ) is_target[t] = 1;
		// Labels and Catch are always targets (or boundary).
		if( o->op == OLabel || o->op == OCatch ) is_target[i] = 1;
	}

	for( int i = 0; i < f->nops; i++ ) {
		if( is_target[i] ) cache_clear();
		jit_opcode(ctx, f->ops + i, i);
	}

	// Resolve intra-function jumps (none emitted yet in bring-up codegen).
	jlist *j = ctx->jumps;
	while( j ) {
		int target = ctx->opsPos[j->target];
		a64_patch_branch(ctx, j->pos, target);
		j = j->next;
	}
	ctx->jumps = NULL;

	hl_free(&ctx->falloc);
	hl_alloc_init(&ctx->falloc);
	return ctx->functionPos;
}

void *hl_jit_code( jit_ctx *ctx, hl_module *m, int *codesize, hl_debug_infos **debug, hl_module *previous ) {
	int size = BUF_POS();
	if( size & 4095 ) size += 4096 - (size & 4095);
	// Debug: dump the raw buffer to a file before placing it into the
	// executable region. Disassemble with:
	//   llvm-objdump --disassemble --triple=arm64 --raw -b binary <file>
	// (or `--mattr=+all` to enable everything). Enable with HL_JIT_DUMP=<path>.
	const char *dumpPath = getenv("HL_JIT_DUMP");
	if( dumpPath ) {
		FILE *fp = fopen(dumpPath, "wb");
		if( fp ) {
			fwrite(ctx->startBuf, 1, BUF_POS(), fp);
			fclose(fp);
			// Also dump function offsets so we can locate each function in the
			// disassembly. One line per function: "<findex> <byte-offset>".
			char idxPath[512];
			snprintf(idxPath, sizeof(idxPath), "%s.idx", dumpPath);
			FILE *fi = fopen(idxPath, "w");
			if( fi ) {
				for( int i = 0; i < m->code->nfunctions; i++ ) {
					hl_function *f = m->code->functions + i;
					void *fp2 = m->functions_ptrs[f->findex];
					fprintf(fi, "%d 0x%x %d\n",
						f->findex, (unsigned)(intptr_t)fp2, f->nops);
				}
				fclose(fi);
			}
		}
	}
	void *code = hl_alloc_executable_memory(size);
	if( code == NULL ) return NULL;
	hl_jit_write_begin();
	memcpy(code, ctx->startBuf, BUF_POS());
	// Patch staged HL→HL BL instructions. At this point
	// m->functions_ptrs[target] holds the *byte offset* of the target
	// function within our JIT buffer (the module loop in module.c set it
	// from the int returned by hl_jit_function). After this hl_jit_code
	// returns, module.c rewrites those entries to absolute pointers.
	uint32_t *base = (uint32_t*)code;
	jlist *c = ctx->calls;
	while( c ) {
		if( CALL_TARGET_IS_BL(c->target) ) {
			void *fp = m->functions_ptrs[c->target];
			if( fp != NULL ) {
				int target_off = (int)(intptr_t)fp;
				int delta_words = (target_off - c->pos) >> 2;
				if( delta_words < -(1<<25) || delta_words >= (1<<25) ) {
					// Out of range — bring-up backend has no veneers yet.
					return NULL;
				}
				uint32_t *slot = base + (c->pos >> 2);
				*slot = (*slot & 0xfc000000) | ((uint32_t)delta_words & 0x03ffffff);
			}
		} else if( CALL_TARGET_IS_IMM64(c->target) ) {
			// Patch a 4-instruction MOVZ+3xMOVK chain at c->pos to
			// materialise the absolute address of function `fid`.
			int fid = IMM64_FINDEX(c->target);
			void *fp = m->functions_ptrs[fid];
			uint64_t abs_addr = (uint64_t)(uintptr_t)((unsigned char*)code + (intptr_t)fp);
			uint32_t *slot = base + (c->pos >> 2);
			for( int k = 0; k < 4; k++ ) {
				uint16_t chunk = (uint16_t)((abs_addr >> (k*16)) & 0xffff);
				// Clear bits 20:5 (imm16) and set them to chunk.
				slot[k] = (slot[k] & 0xFFE0001F) | ((uint32_t)chunk << 5);
			}
		}
		c = c->next;
	}
	ctx->calls = NULL;
	// Patch OStaticClosure structs: their c->fun fields hold the target
	// findex; rewrite each one to the absolute target address now that
	// functions_ptrs has been populated. Mirrors jit.c:4745-4768.
	{
		vclosure *cls = ctx->closure_list;
		while( cls ) {
			vclosure *next = (vclosure*)cls->value;
			int fidx = (int)(intptr_t)cls->fun;
			void *fabs = m->functions_ptrs[fidx];
			cls->fun = (fabs == NULL) ? NULL : ((unsigned char*)code + (intptr_t)fabs);
			cls->value = NULL;
			cls = next;
		}
		ctx->closure_list = NULL;
	}
	(void)previous;
	hl_jit_write_end(code, size);
	// Patch hl_setup so libhl's dynamic call path (hl_call_method) can
	// reach into our JIT'd code. This is the AArch64 equivalent of the
	// block at jit.c:4686-4694.
	if( call_jit_c2hl_native == NULL ) {
		call_jit_c2hl_native = (unsigned char*)code + ctx->c2hl;
		hl_setup.static_call = callback_c2hl_arm64;
		hl_setup.static_call_ref = true;
		hl_setup.get_wrapper = get_wrapper_arm64;
	}
	*codesize = size;
	*debug = ctx->debug;
	return code;
}

void hl_jit_patch_method( void *old_fun, void **new_fun_table ) {
	// Stub: live module reload not supported by the AArch64 bring-up.
	(void)old_fun; (void)new_fun_table;
}
