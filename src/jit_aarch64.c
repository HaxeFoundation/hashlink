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

/*
 * AArch64 JIT backend for the HL2 IR JIT.
 *
 * Phase 2 + 3: function shell + simple ops + arithmetic + memory + conversions.
 * Calls/trampolines and the constant pool are still phase 4.
 */

#if !defined(__aarch64__) && !defined(_M_ARM64)
#  error "This file is for AArch64 architecture only."
#endif

#include <hlmodule.h>
#include <jit.h>
#include "jit_aarch64_emit.h"
#include <string.h>
#include <stdio.h>

#ifdef HL_DEBUG
#	define GEN_DEBUG
#endif

// IR ereg encoding 5 is reserved (`STACK_REG` in jit.h) — the regs phase uses
// it to label stack-bound vregs.  ARM hardware register X5 happens to use the
// same hardware encoding, which would create a fatal aliasing if we exposed
// X5 through the regs configuration as encoding 5.  Re-encode X5 as the
// otherwise-unused IR slot 32; gpr_id maps it back to hardware X5 at emit
// time.  (FP regs encode in the 64..127 range and have no such conflict.)
#define X5_LOGICAL	32

#define R(id)		MK_REG(id, R_REG)
#define V(id)		MK_REG((id) + 64, R_REG)

// ============================================================================
// Register class declaration (AAPCS64, Linux + Apple)
// ============================================================================

void hl_jit_init_regs( regs_config *cfg ) {
	// Integer registers.
	// X15/X16/X17 reserved as backend-private temporaries. X16/X17 are the
	// linker IP0/IP1 (the Apple dynamic linker may clobber them at indirect
	// branches). X15 (ARM_TMP3) is reserved as a third scratch for op
	// handlers that need three independent temps at once — notably emit_store
	// when both base and data are spilled (TMP1 holds base, TMP2 holds data,
	// and emit_ld_st still needs a temp for the offset-register encoding).
	// X18 reserved on Apple/Windows as platform register; conservatively skipped on Linux too.
	// X29 = FP, X30 = LR, X31 = SP/XZR — special-purpose.
	static int scratch_regs[] = {
		R(X0), R(X1), R(X2), R(X3), R(X4), R(X5_LOGICAL), R(X6), R(X7),
		R(X8), R(X9), R(X10), R(X11), R(X12), R(X13), R(X14)
	};
	static int persist_regs[] = {
		R(X19), R(X20), R(X21), R(X22), R(X23),
		R(X24), R(X25), R(X26), R(X27), R(X28)
	};
	static int arg_regs[] = {
		R(X0), R(X1), R(X2), R(X3), R(X4), R(X5_LOGICAL), R(X6), R(X7)
	};
	cfg->regs.ret = scratch_regs[0];
	cfg->regs.nscratchs = sizeof(scratch_regs) / sizeof(int);
	cfg->regs.npersists = sizeof(persist_regs) / sizeof(int);
	cfg->regs.nargs = sizeof(arg_regs) / sizeof(int);
	cfg->regs.scratch = (ereg*)scratch_regs;
	cfg->regs.persist = (ereg*)persist_regs;
	cfg->regs.arg = (ereg*)arg_regs;

	// Float registers (V0-V31; lower 64 bits of V8-V15 are callee-saved per AAPCS64).
	static int float_scratch[] = {
		V(0), V(1), V(2), V(3), V(4), V(5), V(6), V(7),
		V(16), V(17), V(18), V(19), V(20), V(21), V(22), V(23),
		V(24), V(25), V(26), V(27), V(28), V(29), V(30), V(31)
	};
	static int float_persist[] = {
		V(8), V(9), V(10), V(11), V(12), V(13), V(14), V(15)
	};
	static int float_args[] = {
		V(0), V(1), V(2), V(3), V(4), V(5), V(6), V(7)
	};
	cfg->floats.ret = float_scratch[0];
	cfg->floats.nscratchs = sizeof(float_scratch) / sizeof(int);
	cfg->floats.npersists = sizeof(float_persist) / sizeof(int);
	cfg->floats.nargs = sizeof(float_args) / sizeof(int);
	cfg->floats.scratch = (ereg*)float_scratch;
	cfg->floats.persist = (ereg*)float_persist;
	cfg->floats.arg = (ereg*)float_args;

	// ARM has no register pinning constraints for shifts (LSLV/LSRV/ASRV accept
	// any source) or division (SDIV/UDIV write any destination).
	cfg->req_bit_shifts = 0;
	cfg->req_div_a = 0;
	cfg->req_div_b = 0;

	cfg->stack_reg = R(SP_REG); // X31 (SP)
	cfg->stack_pos = R(FP);     // X29
	cfg->stack_align = 16;      // AAPCS64 mandates
	// Each stack-passed arg consumes 16 bytes to keep SP 16-byte aligned —
	// any [SP, ...] memory access with misaligned SP traps under EL0
	// alignment enforcement on Linux/macOS. emit_push correspondingly moves
	// SP by 16 per arg, so the IR's call-arg accounting matches.
	cfg->stack_arg_size = 16;
	// HL→native calls follow the platform C ABI, which differs from the HL
	// internal convention. Linux/Windows AAPCS64 uses 8-byte slots; Apple
	// ARM64 packs args at their natural size with natural alignment. The
	// regs phase emits these via STACK_OFFS + STORE-to-[SP+offs] instead of
	// the PUSH-based path used for HL→HL.
#if defined(HL_MAC) || defined(HL_IOS) || defined(HL_TVOS)
	cfg->native_stack_layout = NATIVE_STACK_LAYOUT_APPLE_ARM64;
#else
	cfg->native_stack_layout = NATIVE_STACK_LAYOUT_AAPCS64;
#endif

#ifdef GEN_DEBUG
	cfg->debug_prefix_size = 4; // ARM instructions are fixed 4 bytes
#endif
}

// ============================================================================
// Disassembly helper
// ============================================================================

const char *hl_natreg_str( int reg, emit_mode m ) {
	static char out[16];
	int r = REG_REG(reg);
	// Reverse the remappings used in gpr_id so debug output reflects the
	// hardware register actually emitted.
	int hw = (r == X5_LOGICAL) ? 5 : (r == STACK_REG) ? 29 : r;
	switch( m ) {
	case M_I32:
	case M_UI16:
	case M_UI8:
		if( hw == 31 )
			sprintf(out, "WZR");
		else if( hw < 31 )
			sprintf(out, "W%d", hw);
		else
			sprintf(out, "W%d???", hw);
		break;
	case M_F32:
		hw = r - 64;
		sprintf(out, "S%d%s", hw, hw >= 0 && hw < 32 ? "" : "???");
		break;
	case M_F64:
		hw = r - 64;
		sprintf(out, "D%d%s", hw, hw >= 0 && hw < 32 ? "" : "???");
		break;
	default:
		if( hw == 31 )
			sprintf(out, "SP");
		else if( hw == 29 )
			sprintf(out, "FP");
		else if( hw == 30 )
			sprintf(out, "LR");
		else if( hw < 31 )
			sprintf(out, "X%d", hw);
		else
			sprintf(out, "X%d???", hw);
		break;
	}
	return out;
}

// ============================================================================
// Backend lifecycle
// ============================================================================

void hl_codegen_alloc( jit_ctx *jit ) {
	code_ctx *ctx = (code_ctx*)malloc(sizeof(code_ctx));
	memset(ctx, 0, sizeof(code_ctx));
	jit->code = ctx;
	ctx->jit = jit;
}

void hl_codegen_free( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	if( ctx == NULL ) return;
	free(ctx);
}

// ============================================================================
// Helpers
// ============================================================================

#define ARM_TMP1	X16    // backend-private scratch (IP0)
#define ARM_TMP2	X17    // backend-private scratch (IP1)
#define ARM_TMP3	X15    // backend-private scratch (excluded from regalloc)

// Map an IR ereg to a physical AArch64 GPR encoding (0..31).
//   IR encoding 5 (STACK_REG) → ARM FP (X29); the regs phase uses 5 as a
//     stack-bound-vreg marker, and after ENTER lowers `MOV stack_pos,
//     stack_reg` we keep that in X29.
//   IR encoding X5_LOGICAL (32) → ARM X5; the remap shifts X5 out of slot 5
//     so the IR's STACK_REG sentinel does not alias it.
static Arm64Reg gpr_id( ereg r ) {
	int v = REG_REG(r);
	if( v == STACK_REG ) return FP;
	if( v == X5_LOGICAL ) return X5;
	return (Arm64Reg)v;
}

static Arm64FpReg fpr_id( ereg r ) {
	return (Arm64FpReg)(REG_REG(r) - 64);
}

// LDR/STR `size` field: 0=8b, 1=16b, 2=32b, 3=64b.
static int ls_size_for( emit_mode m ) {
	switch( m ) {
	case M_UI8:  return 0;
	case M_UI16: return 1;
	case M_I32:
	case M_F32:  return 2;
	case M_PTR:
	case M_F64:  return 3;
	default:     return 3;
	}
}

static int sf_for( emit_mode m ) {
	// 1 = 64-bit, 0 = 32-bit (sub-word loads/stores still use 64-bit reg encoding).
	return (m == M_PTR || m == M_F64) ? 1 : 0;
}

static bool is_fp_mode( emit_mode m ) { return m == M_F32 || m == M_F64; }

// ----------------------------------------------------------------------------
// Stack pointer arithmetic with arbitrary signed delta.
// `delta > 0` => SP += delta, `delta < 0` => SP -= |delta|.
// Uses imm12 + optional LSL #12 when possible; falls back through ARM_TMP1.
// ----------------------------------------------------------------------------
static void emit_sp_offs( code_ctx *ctx, int delta ) {
	if( delta == 0 ) return;
	int op = (delta < 0) ? 1 : 0; // 0 = ADD, 1 = SUB
	uint32_t mag = (uint32_t)(delta < 0 ? -delta : delta);
	if( mag <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, op, 0, 0, (int)mag, SP_REG, SP_REG);
		return;
	}
	if( (mag & 0xFFF) == 0 && (mag >> 12) <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, op, 0, 1, (int)(mag >> 12), SP_REG, SP_REG);
		return;
	}
	// Try two-step imm: hi part (LSL #12) + lo part, both ≤ 0xFFF.
	uint32_t mag_lo = mag & 0xFFF;
	uint32_t mag_hi = mag >> 12;
	if( mag_hi <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, op, 0, 1, (int)mag_hi, SP_REG, SP_REG);
		if( mag_lo )
			encode_add_sub_imm(ctx, 1, op, 0, 0, (int)mag_lo, SP_REG, SP_REG);
		return;
	}
	// Fall back to register form.  Must use ADD/SUB (extended register) — the
	// shifted-register form interprets register 31 as XZR, not SP, so
	// `SUB SP, SP, X16` would silently become `SUB XZR, XZR, X16` (a NOP).
	// Extended-register form with option=UXTX(011), imm3=0 treats Rd/Rn=31
	// as SP, which is what we want.
	load_immediate(ctx, (int64_t)mag, ARM_TMP1, true);
	encode_add_sub_ext(ctx, 1, op, 0, ARM_TMP1, /*option=UXTX*/3, /*imm3=*/0, SP_REG, SP_REG);
}

// ----------------------------------------------------------------------------
// ADD/SUB-imm with optional 12-bit shift, returns true if `mag` fits.
// Emits `op (ADD/SUB) Rd, Rn, #mag` using up to two instructions.
// Caller picks 0=ADD or 1=SUB.
// ----------------------------------------------------------------------------
static bool emit_addsub_imm_2step( code_ctx *ctx, int op, Arm64Reg Rd, Arm64Reg Rn, uint32_t mag ) {
	if( mag <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, op, 0, 0, (int)mag, Rn, Rd);
		return true;
	}
	if( (mag >> 12) <= 0xFFF ) {
		uint32_t hi = mag >> 12, lo = mag & 0xFFF;
		encode_add_sub_imm(ctx, 1, op, 0, 1, (int)hi, Rn, Rd);
		if( lo )
			encode_add_sub_imm(ctx, 1, op, 0, 0, (int)lo, Rd, Rd);
		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------
// Load/store with FP-relative or arbitrary base+offs.
// Picks LDR/STR(unsigned imm scaled) when offset fits, else LDUR/STUR (signed,
// unscaled, ±256), else falls back to a register-offset form.
//
// Register-form offset requires a scratch register that must NOT collide with
// reg_t (for STR Xt,[base,Xt] would store the offset value at the offset
// location). When base also lives in a backend temp (ARM_TMP1/TMP2), we may
// run out of disjoint temps. In that case, fold the offset into base in place
// using ADD/SUB-imm (preserving base across the load/store), which is valid
// for magnitudes up to 0xFFFFFF.
// ----------------------------------------------------------------------------
static void emit_ld_st_ex( code_ctx *ctx, bool is_load, emit_mode mode, int reg_t, Arm64Reg base, int offs, Arm64Reg avoid ) {
	int size = ls_size_for(mode);
	int V = is_fp_mode(mode) ? 1 : 0;
	int opc = is_load ? 1 : 0; // 0=STR, 1=LDR (for V=0 GPR; same for V=1 FP)
	int scale = 1 << size;
	if( offs >= 0 && (offs & (scale - 1)) == 0 && (offs / scale) < 0x1000 ) {
		encode_ldr_str_imm(ctx, size, V, opc, offs / scale, base, (Arm64Reg)reg_t);
		return;
	}
	if( offs >= -256 && offs < 256 ) {
		encode_ldur_stur(ctx, size, V, opc, offs, base, (Arm64Reg)reg_t);
		return;
	}
	// Pick an offset temp.  Constraints:
	//   - For stores, off_tmp must not equal reg_t (else STR Xt,[base,Xt]
	//     writes the offset value instead of the data).  Loads are immune
	//     since LDR reads the offset register before writing reg_t.
	//   - off_tmp must not equal base (the load/store needs base intact).
	//   - off_tmp must not equal `avoid` (a caller-supplied register the
	//     caller has parked a live value in — typically the OUTER base in
	//     emit_store/emit_load_addr while loading the data argument).
	// For FP loads/stores, reg_t is a V-register, so V-vs-X never collides.
	Arm64Reg off_tmp = ARM_TMP1;
	if( V == 0 ) {
		bool bad_t1 = (!is_load && reg_t == ARM_TMP1) || base == ARM_TMP1 || avoid == ARM_TMP1;
		if( bad_t1 ) off_tmp = ARM_TMP2;
		bool bad_t2 = (!is_load && reg_t == off_tmp) || base == off_tmp || avoid == off_tmp;
		if( bad_t2 ) off_tmp = ARM_TMP3;
		bool bad_t3 = (!is_load && reg_t == off_tmp) || base == off_tmp || avoid == off_tmp;
		if( bad_t3 ) jit_error("aarch64 emit_ld_st: no free offset temp");
	}
	load_immediate(ctx, offs, off_tmp, true);
	encode_ldr_str_reg(ctx, size, V, opc, off_tmp, /*option=*/3 /*LSL*/, /*S=*/0, base, (Arm64Reg)reg_t);
}

static void emit_ld_st( code_ctx *ctx, bool is_load, emit_mode mode, int reg_t, Arm64Reg base, int offs ) {
	emit_ld_st_ex(ctx, is_load, mode, reg_t, base, offs, (Arm64Reg)-1 /*no avoid*/);
}

// MOV between two GPRs. Handles SP as source/dest (ARM disallows ORR with SP).
static void emit_mov_gpr( code_ctx *ctx, Arm64Reg dst, Arm64Reg src, int sf ) {
	if( dst == src ) return;
	if( dst == SP_REG || src == SP_REG ) {
		// ADD <dst>, <src>, #0  (only form that accepts SP).
		encode_add_sub_imm(ctx, sf, 0, 0, 0, 0, src, dst);
	} else {
		// ORR <dst>, XZR, <src>
		encode_logical_reg(ctx, sf, 0x01, 0, 0, src, 0, XZR, dst);
	}
}

// MOV between two FP regs (preserves the lane size used by the mode).
// Uses ORR.16B (same encoding regardless of S/D since it's a bitwise move).
// FMOV is also an option; we use FMOV (scalar) for clarity.
static void emit_mov_fpr( code_ctx *ctx, Arm64FpReg dst, Arm64FpReg src, emit_mode mode ) {
	if( dst == src ) return;
	int type = (mode == M_F64) ? 1 : 0; // 1=double, 0=single
	// FMOV (register) opcode = 0
	encode_fp_1src(ctx, /*M=*/0, /*S=*/0, type, /*opcode=*/0, src, dst);
}

// Generic MOV that mirrors x86's emit_mov: handles reg/reg, reg/mem, mem/reg.
// imm-to-reg goes through emit_load_const.
static void emit_load_const( code_ctx *ctx, ereg out, uint64_t value, emit_mode mode );

// Phase 4 forward declarations (defined later in this file).
static int  reserve_const_segment( code_ctx *ctx, int size, int align );
static int  alloc_const( code_ctx *ctx, uint64_t value, int adrp_pos );
static void emit_const_load( code_ctx *ctx, Arm64Reg dst, uint64_t value );
static void emit_const_addr( code_ctx *ctx, Arm64Reg dst, uint64_t value );
static void emit_pool_offset_addr( code_ctx *ctx, Arm64Reg dst, int const_offset );
static Arm64FpReg materialize_fpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64FpReg tmp );
static Arm64Reg   materialize_gpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp );
static Arm64Reg   materialize_gpr_ex( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp, Arm64Reg avoid );

// LEA-like: out = base + offs.  Used when an operand encodes an address as
// (R_REG, value=offs) — e.g. MK_STACK_OFFS, or the LEA-rewritten ADDRESS op.
static void emit_lea_imm( code_ctx *ctx, Arm64Reg out, Arm64Reg base, int offs ) {
	if( offs == 0 ) {
		emit_mov_gpr(ctx, out, base, 1);
	} else if( offs > 0 && offs <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, 0, 0, 0, offs, base, out);
	} else if( offs < 0 && -offs <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, 1, 0, 0, -offs, base, out);
	} else {
		load_immediate(ctx, offs, ARM_TMP1, true);
		encode_add_sub_reg(ctx, 1, 0, 0, 0, ARM_TMP1, 0, base, out);
	}
}

static void emit_mov( code_ctx *ctx, ereg dst, ereg src, emit_mode mode ) {
	int dst_kind = REG_KIND(dst);
	int src_kind = REG_KIND(src);

	if( dst_kind == R_REG && src_kind == R_REG ) {
		// MK_STACK_OFFS / LEA-rewritten ADDRESS: src encodes (reg, offs).
		// Treat as an address computation: dst = src_reg + offs.
		if( !is_fp_mode(mode) && REG_VALUE(src) != 0 ) {
			emit_lea_imm(ctx, gpr_id(dst), gpr_id(src), REG_VALUE(src));
			return;
		}
		if( is_fp_mode(mode) )
			emit_mov_fpr(ctx, fpr_id(dst), fpr_id(src), mode);
		else
			emit_mov_gpr(ctx, gpr_id(dst), gpr_id(src), sf_for(mode));
		return;
	}
	if( dst_kind == R_REG && src_kind == R_REG_PTR ) {
		// LOAD: dst <- [base + offs]
		Arm64Reg base = gpr_id(src);
		int offs = REG_VALUE(src);
		int reg_t = is_fp_mode(mode) ? fpr_id(dst) : gpr_id(dst);
		emit_ld_st(ctx, /*is_load=*/true, mode, reg_t, base, offs);
		return;
	}
	if( dst_kind == R_REG_PTR && src_kind == R_REG ) {
		// STORE: [base + offs] <- src
		Arm64Reg base = gpr_id(dst);
		int offs = REG_VALUE(dst);
		int reg_t = is_fp_mode(mode) ? fpr_id(src) : gpr_id(src);
		emit_ld_st(ctx, /*is_load=*/false, mode, reg_t, base, offs);
		return;
	}
	if( dst_kind == R_REG && src_kind == R_CONST ) {
		emit_load_const(ctx, dst, (uint64_t)REG_VALUE(src), mode);
		return;
	}
	if( dst_kind == R_REG_PTR && src_kind == R_REG_PTR ) {
		// memory-to-memory: load through a scratch register, then store.
		// Use V31 for FP modes and ARM_TMP1 for integer/pointer modes — both
		// are reserved as backend-private scratch.
		Arm64Reg sb = gpr_id(src);
		int so = REG_VALUE(src);
		Arm64Reg db = gpr_id(dst);
		int doff = REG_VALUE(dst);
		if( is_fp_mode(mode) ) {
			emit_ld_st(ctx, /*is_load=*/true,  mode, (Arm64FpReg)31, sb, so);
			emit_ld_st(ctx, /*is_load=*/false, mode, (Arm64FpReg)31, db, doff);
		} else {
			emit_ld_st(ctx, /*is_load=*/true,  mode, ARM_TMP1, sb, so);
			emit_ld_st(ctx, /*is_load=*/false, mode, ARM_TMP1, db, doff);
		}
		return;
	}
	jit_error("aarch64 emit_mov: unhandled operand kinds");
}

// ----------------------------------------------------------------------------
// LOAD_CONST: integer immediate or floating constant.
// Float constants need the literal pool (Phase 4). For Phase 2 only ints.
// ----------------------------------------------------------------------------
static void emit_load_const( code_ctx *ctx, ereg out, uint64_t value, emit_mode mode ) {
	if( REG_KIND(out) != R_REG ) {
		// emit-into-memory: load the bit pattern into ARM_TMP1 and store as the
		// requested width.  For floats we treat the FP constant's bit pattern as
		// an integer — the resulting STR writes the same bytes a FP STR would.
		emit_mode store_mode = is_fp_mode(mode) ? (mode == M_F32 ? M_I32 : M_PTR) : mode;
		load_immediate(ctx, (int64_t)value, ARM_TMP1, sf_for(store_mode) == 1);
		Arm64Reg base = gpr_id(out);
		int offs = REG_VALUE(out);
		emit_ld_st(ctx, /*is_load=*/false, store_mode, ARM_TMP1, base, offs);
		return;
	}
	if( is_fp_mode(mode) ) {
		// Float constants live in the literal pool: ADRP+LDR into the FP reg.
		// jit_emit.c packs F32 constants into the low 32 bits of `value` with
		// the upper 32 bits zeroed, so we must use the matching width-encoding
		// (size=2 → LDR Sd, ...). Loading 8 bytes would pull the zero high
		// half into D and yield a subnormal double when read as F64.
		Arm64FpReg fp_dst = fpr_id(out);
		int adrp_pos = byte_count(ctx->code);
		int size = (mode == M_F32) ? 2 : 3;
		encode_adrp(ctx, 0, 0, ARM_TMP1);                              // ADRP X16, page
		// LDR Sd|Dd, [X16, #lo12]   V=1, opc=01, imm12=placeholder
		encode_ldr_str_imm(ctx, size, 1, 1, 0, ARM_TMP1, (Arm64Reg)fp_dst);
		alloc_const(ctx, value, adrp_pos);
		return;
	}
	load_immediate(ctx, (int64_t)value, gpr_id(out), sf_for(mode) == 1);
}

// ----------------------------------------------------------------------------
// PUSH / POP. ARM has no explicit push/pop; we use STR/LDR with pre/post-index
// on SP. To match the x86 stack-offset accounting (which assumes 16 bytes are
// already consumed by RIP+RBP), PUSH X29 emits STP X29, X30, [SP, #-16]! so
// LR is implicitly saved as part of FP-save. POP X29 mirrors with LDP.
// All other PUSH/POPs use 16-byte SP movement (8 bytes wasted) to keep SP
// 16-byte aligned per AAPCS64.
// ----------------------------------------------------------------------------
static void emit_push( code_ctx *ctx, ereg r, emit_mode mode ) {
	if( is_fp_mode(mode) ) {
		// SUB SP, SP, #16; STR Dn, [SP].  Materialize through V31 if r is not a register.
		Arm64FpReg src = (REG_KIND(r) == R_REG) ? fpr_id(r) : materialize_fpr(ctx, r, mode, (Arm64FpReg)31);
		emit_sp_offs(ctx, -16);
		encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/1, /*opc=*/0 /*STR*/, 0, SP_REG, (Arm64Reg)src);
		return;
	}
	// materialize_gpr handles MK_STACK_OFFS by adding the offset; gpr_id alone
	// would discard it (mapping STACK_REG->FP and ignoring REG_VALUE).
	Arm64Reg src = materialize_gpr(ctx, r, mode, ARM_TMP1);
	if( src == FP && REG_KIND(r) == R_REG && REG_VALUE(r) == 0 ) {
		// True PUSH FP (prologue) — emit STP x29,x30,[sp,#-16]! to also save LR.
		encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x03, /*imm7=*/-2 & 0x7F, LR, SP_REG, FP);
		return;
	}
	// SUB SP, SP, #16; STR Xn, [SP]
	emit_sp_offs(ctx, -16);
	encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/0, /*opc=*/0, 0, SP_REG, src);
}

static void emit_pop( code_ctx *ctx, ereg r, emit_mode mode ) {
	if( REG_KIND(r) != R_REG ) jit_error("aarch64 POP non-reg not implemented");
	if( is_fp_mode(mode) ) {
		encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/1, /*opc=*/1 /*LDR*/, 0, SP_REG, (Arm64Reg)fpr_id(r));
		emit_sp_offs(ctx, 16);
		return;
	}
	Arm64Reg dst = gpr_id(r);
	if( dst == FP ) {
		// LDP X29, X30, [SP], #16   opc=10, V=0, mode=01 (post-index load), imm7=2
		encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x01, /*imm7=*/2, LR, SP_REG, FP);
		return;
	}
	// LDR Xn, [SP]; ADD SP, SP, #16
	encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/0, /*opc=*/1, 0, SP_REG, dst);
	emit_sp_offs(ctx, 16);
}

// ----------------------------------------------------------------------------
// CMP / TEST.  e->mode tells us int width / float; e->size_offs holds the
// upstream OJxxx opcode (consumed later by the JCOND/CMOV that follows).
// ----------------------------------------------------------------------------
static void emit_cmp( code_ctx *ctx, einstr *e ) {
	if( is_fp_mode(e->mode) ) {
		// FCMP. NaN handling deferred; bare FCMP is correct for ordered compares
		// and gives QNaN-as-unordered which matches ARM defaults.
		Arm64FpReg ra = materialize_fpr(ctx, e->a, e->mode, (Arm64FpReg)29);
		Arm64FpReg rb = materialize_fpr(ctx, e->b, e->mode, (Arm64FpReg)30);
		int type = (e->mode == M_F64) ? 1 : 0;
		encode_fp_compare(ctx, /*M=*/0, /*S=*/0, type, rb, /*op=*/0, ra);
		return;
	}
	// Integer compare: SUBS XZR, Xa, Xb  (or imm form).
	// materialize_gpr handles R_REG (incl. MK_STACK_OFFS via emit_lea_imm),
	// R_CONST, and R_REG_PTR — picking gpr_id alone would drop the FP+N
	// offset for stack-allocated addresses.
	int sf = sf_for(e->mode);
	Arm64Reg a = materialize_gpr(ctx, e->a, e->mode, ARM_TMP1);
	if( REG_KIND(e->b) == R_CONST ) {
		int64_t val = (int64_t)REG_VALUE(e->b);
		if( val >= 0 && val <= 0xFFF ) {
			// CMP Xa, #imm  (SUBS XZR, Xa, #imm; sf, op=1, S=1)
			encode_add_sub_imm(ctx, sf, 1, 1, 0, (int)val, a, XZR);
			return;
		}
		if( val < 0 && -val <= 0xFFF ) {
			// CMN Xa, #imm  (ADDS XZR, Xa, #imm)
			encode_add_sub_imm(ctx, sf, 0, 1, 0, (int)-val, a, XZR);
			return;
		}
		load_immediate(ctx, val, ARM_TMP2, sf == 1);
		encode_add_sub_reg(ctx, sf, 1, 1, 0, ARM_TMP2, 0, a, XZR);
		return;
	}
	Arm64Reg b = materialize_gpr_ex(ctx, e->b, e->mode, ARM_TMP2, a);
	encode_add_sub_reg(ctx, sf, 1, 1, 0, b, 0, a, XZR);
}

static void emit_test( code_ctx *ctx, einstr *e ) {
	if( is_fp_mode(e->mode) ) jit_error("aarch64 TEST float not supported");
	int sf = sf_for(e->mode);
	// materialize_gpr folds MK_STACK_OFFS (R_REG kind + non-zero REG_VALUE)
	// into FP+N so we never TST raw FP for stack-allocated address operands.
	Arm64Reg a = materialize_gpr(ctx, e->a, e->mode, ARM_TMP1);
	// TST Xa, Xa  (ANDS XZR, Xa, Xa); opc=11 (ANDS), shift=0, N=0
	encode_logical_reg(ctx, sf, 0x03, 0, 0, a, 0, a, XZR);
}

// ----------------------------------------------------------------------------
// JCOND / JUMP — branch fixups patched after function emit.
// ----------------------------------------------------------------------------
static void add_branch_fixup( code_ctx *ctx, int code_pos, int target_op, int is_cond ) {
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, code_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, target_op);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, is_cond);
}

static void emit_jump( code_ctx *ctx, int target_op_offset ) {
	// target_op_offset is the IR-relative displacement, target = cur_op + 1 + offset
	int target = ctx->cur_op + 1 + target_op_offset;
	int pos = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0); // placeholder
	add_branch_fixup(ctx, pos, target, 0);
}

static void emit_jump_cond( code_ctx *ctx, ArmCondition cond, int target_op_offset ) {
	int target = ctx->cur_op + 1 + target_op_offset;
	int pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, cond);
	add_branch_fixup(ctx, pos, target, 1);
}

// Mirror x86 get_cond_jump: walk back through MOV/JCOND/CMOV/XCHG/CXCHG to find
// the comparison whose flags this JCOND/CMOV consumes.  Translate the upstream
// OJxxx opcode into an ARM condition code.
static ArmCondition get_cond_jump( code_ctx *ctx ) {
	int prev = 0;
	einstr *p;
	do {
		p = ctx->jit->reg_instrs + ctx->cur_op - (++prev);
	} while( p->op == MOV || p->op == JCOND || p->op == CMOV || p->op == XCHG || p->op == CXCHG );
	switch( p->size_offs ) {
	case OJFalse:
	case OJNull:
		return COND_EQ;
	case OJTrue:
	case OJNotNull:
		return COND_NE;
	// For ARM64 FCMP, NaN sets N=0, Z=0, C=1, V=1.  IEEE 754 ordered
	// predicates need to evaluate FALSE for NaN. HS (C==1) and HI (C==1
	// && Z==0) both fire on NaN — wrong. GE (N==V) and GT (Z==0 && N==V)
	// reject NaN since V differs from N. The x86 backend can use JUGte/JUGt
	// for FP only because x86 UCOMISS sets CF=1 on NaN, making JAE/JA
	// reject it; ARM's carry conventions are inverted from x86's.
	// LO (C==0) and LS (C==0 || Z==1) already reject NaN on ARM (C=1).
	case OJSGte:
		return COND_GE;
	case OJSGt:
		return COND_GT;
	case OJUGte:
		return COND_HS;
	case OJSLt:
		return is_fp_mode(p->mode) ? COND_LO : COND_LT;
	case OJSLte:
		return is_fp_mode(p->mode) ? COND_LS : COND_LE;
	case OJULt:
		return COND_LO;
	case OJEq:
		return COND_EQ;
	case OJNotEq:
		return COND_NE;
	case OJNotLt:
		// HS (C==1) fires on NaN (C=1) and on ordered >= (C=1) ✓.
		// GE (N==V) would reject NaN (V=1, N=0) — wrong, NaN means
		// "not less than" should fire.
		return COND_HS;
	case OJNotGte:
		// LT (N!=V) is signed less-than for INT, and for FP it fires on
		// NaN (V=1) — the right semantics for "not >=".
		// LO (C==0) would not fire on NaN (C=1) — wrong for FP.
		return COND_LT;
	case 0:
		if( p->op == DEBUG_BREAK ) return COND_EQ;
		// fallthrough
	default:
		jit_error("aarch64 get_cond_jump: unknown OJ opcode");
		return COND_AL;
	}
}

static void patch_branch( code_ctx *ctx, int pos, int target_byte_pos, int is_cond ) {
	int delta = target_byte_pos - pos;
	if( delta & 3 ) jit_error("aarch64 branch target not 4-byte aligned");
	int imm = delta >> 2;
	unsigned int *insn = (unsigned int*)&ctx->code.values[pos];
	if( is_cond ) {
		// imm19 lives in bits [23:5]; cond + 0x54000000 prefix retained.
		if( imm < -(1 << 18) || imm >= (1 << 18) )
			jit_error("aarch64 B.cond out of range (Phase 2 limit)");
		*insn = (*insn & ~(0x7FFFF << 5)) | ((imm & 0x7FFFF) << 5);
	} else {
		// imm26 lives in bits [25:0]; opcode 000101.
		if( imm < -(1 << 25) || imm >= (1 << 25) )
			jit_error("aarch64 B out of range");
		*insn = (*insn & ~0x03FFFFFF) | (imm & 0x03FFFFFF);
	}
}

// ----------------------------------------------------------------------------
// Operand materialization: ensure src is a live register; load through a temp
// if it's a constant or memory.  Returns the GPR encoding to use.
// ----------------------------------------------------------------------------
static Arm64Reg materialize_gpr_ex( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp, Arm64Reg avoid ) {
	if( REG_KIND(src) == R_REG ) {
		Arm64Reg base = gpr_id(src);
		int v = REG_VALUE(src);
		if( v == 0 ) return base;
		emit_lea_imm(ctx, tmp, base, v);
		return tmp;
	}
	if( REG_KIND(src) == R_CONST ) {
		load_immediate(ctx, (int64_t)REG_VALUE(src), tmp, sf_for(mode) == 1);
		return tmp;
	}
	if( REG_KIND(src) == R_REG_PTR ) {
		// Load directly via emit_ld_st_ex so the offset-temp picker can avoid
		// `avoid` (typically the caller's outer base register).
		emit_ld_st_ex(ctx, true, mode, tmp, gpr_id(src), REG_VALUE(src), avoid);
		return tmp;
	}
	emit_mov(ctx, R(tmp), src, mode);
	return tmp;
}

static Arm64Reg materialize_gpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp ) {
	return materialize_gpr_ex(ctx, src, mode, tmp, (Arm64Reg)-1);
}

static Arm64FpReg materialize_fpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64FpReg tmp ) {
	if( REG_KIND(src) == R_REG ) return fpr_id(src);
	if( REG_KIND(src) == R_REG_PTR ) {
		Arm64Reg base = gpr_id(src);
		int offs = REG_VALUE(src);
		emit_ld_st(ctx, /*is_load=*/true, mode, tmp, base, offs);
		return tmp;
	}
	if( REG_KIND(src) == R_CONST ) {
		// FP constants always live in the literal pool.
		int adrp_pos = byte_count(ctx->code);
		encode_adrp(ctx, 0, 0, ARM_TMP1);
		encode_ldr_str_imm(ctx, 3, 1, 1, 0, ARM_TMP1, (Arm64Reg)tmp);
		alloc_const(ctx, (uint64_t)REG_VALUE(src), adrp_pos);
		return tmp;
	}
	jit_error("aarch64 materialize_fpr: unsupported operand kind");
	return (Arm64FpReg)0;
}

// ----------------------------------------------------------------------------
// Bitfield helpers (SBFM / UBFM raw encoding) for sign/zero-extension.
// ----------------------------------------------------------------------------
static void emit_bitfield( code_ctx *ctx, int sf, int opc, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd ) {
	// [31]=sf, [30:29]=opc (00=SBFM, 01=BFM, 10=UBFM), [28:23]=100110, [22]=N(=sf),
	// [21:16]=immr, [15:10]=imms, [9:5]=Rn, [4:0]=Rd
	unsigned int insn = ((unsigned)sf << 31) | ((unsigned)opc << 29) | (0x26u << 23) |
	                    ((unsigned)sf << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10) |
	                    ((Rn & 0x1F) << 5) | (Rd & 0x1F);
	EMIT32(ctx, insn);
}

static void emit_sxt_to_int( code_ctx *ctx, emit_mode in_mode, Arm64Reg Rn, Arm64Reg Rd ) {
	// SXTB Wd, Wn / SXTH Wd, Wn — produce sign-extended 32-bit result.
	switch( in_mode ) {
	case M_UI8:  emit_bitfield(ctx, 0, 0x00, 0, 7, Rn, Rd); break;
	case M_UI16: emit_bitfield(ctx, 0, 0x00, 0, 15, Rn, Rd); break;
	default: jit_error("emit_sxt_to_int unsupported in_mode");
	}
}

static void emit_sxt_to_ptr( code_ctx *ctx, emit_mode in_mode, Arm64Reg Rn, Arm64Reg Rd ) {
	// SBFM Xd, Xn, #0, #N — sign-extend to 64-bit.
	switch( in_mode ) {
	case M_UI8:  emit_bitfield(ctx, 1, 0x00, 0, 7, Rn, Rd); break;
	case M_UI16: emit_bitfield(ctx, 1, 0x00, 0, 15, Rn, Rd); break;
	case M_I32:  emit_bitfield(ctx, 1, 0x00, 0, 31, Rn, Rd); break;
	default: jit_error("emit_sxt_to_ptr unsupported in_mode");
	}
}

static void emit_uxt_to_w( code_ctx *ctx, emit_mode in_mode, Arm64Reg Rn, Arm64Reg Rd ) {
	// UXTB Wd, Wn / UXTH Wd, Wn — implemented as AND Wd, Wn, #mask.
	switch( in_mode ) {
	case M_UI8:  encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, Rn, Rd); break;   // AND Wd, Wn, #0xFF
	case M_UI16: encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, Rn, Rd); break;  // AND Wd, Wn, #0xFFFF
	default: jit_error("emit_uxt_to_w unsupported in_mode");
	}
}

// ----------------------------------------------------------------------------
// BINOP / UNOP integer.  e->size_offs encodes the upstream Haxe op (OAdd, ...).
// ARM has 3-operand ALU so we can write directly to `out` from `a, b`.
// ----------------------------------------------------------------------------
static void emit_div_mod( code_ctx *ctx, hl_op op, Arm64Reg out, Arm64Reg a, Arm64Reg b, int sf );

static void emit_binop_int( code_ctx *ctx, hl_op op, ereg out_e, ereg a_e, ereg b_e, emit_mode mode ) {
	int sf = sf_for(mode);
	Arm64Reg out = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP1;
	Arm64Reg a = materialize_gpr(ctx, a_e, mode, ARM_TMP1);

	// Constant-imm fast paths (ADD/SUB/AND/OR/XOR with small immediates).
	if( REG_KIND(b_e) == R_CONST ) {
		int64_t v = (int64_t)REG_VALUE(b_e);
		if( (op == OAdd || op == OSub) && v >= 0 && v <= 0xFFF ) {
			encode_add_sub_imm(ctx, sf, op == OSub ? 1 : 0, 0, 0, (int)v, a, out);
			goto store_out;
		}
		if( (op == OAdd || op == OSub) && v < 0 && -v <= 0xFFF ) {
			encode_add_sub_imm(ctx, sf, op == OSub ? 0 : 1, 0, 0, (int)-v, a, out);
			goto store_out;
		}
	}

	Arm64Reg b = materialize_gpr_ex(ctx, b_e, mode, ARM_TMP2, a);

	switch( op ) {
	case OAdd: encode_add_sub_reg(ctx, sf, 0, 0, 0, b, 0, a, out); break;
	case OSub: encode_add_sub_reg(ctx, sf, 1, 0, 0, b, 0, a, out); break;
	case OMul: encode_madd_msub(ctx, sf, 0, b, XZR, a, out); break;
	case OAnd: encode_logical_reg(ctx, sf, 0x00, 0, 0, b, 0, a, out); break;
	case OOr:  encode_logical_reg(ctx, sf, 0x01, 0, 0, b, 0, a, out); break;
	case OXor: encode_logical_reg(ctx, sf, 0x02, 0, 0, b, 0, a, out); break;
	case OShl: encode_shift_reg(ctx, sf, 0x00, b, a, out); break;  // LSLV
	case OUShr: encode_shift_reg(ctx, sf, 0x01, b, a, out); break; // LSRV
	case OSShr: encode_shift_reg(ctx, sf, 0x02, b, a, out); break; // ASRV
	case OSDiv:
	case OUDiv:
	case OSMod:
	case OUMod:
		emit_div_mod(ctx, op, out, a, b, sf);
		break;
	default:
		jit_error("aarch64 emit_binop_int: unsupported op");
	}

	// Sub-word result truncation.  Loads/stores already truncate, but ALU on
	// 32-bit reg leaves upper W zero already; we only need a mask for 8/16-bit.
	if( mode == M_UI8 ) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, out, out);   // AND Wd, Wd, #0xFF
	} else if( mode == M_UI16 ) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, out, out);  // AND Wd, Wd, #0xFFFF
	}

store_out:
	if( REG_KIND(out_e) != R_REG ) {
		emit_mov(ctx, out_e, R(ARM_TMP1), mode);
	}
}

// ----------------------------------------------------------------------------
// Integer divide / modulo with Haxe semantics:
//   OUDiv:  b == 0       => 0
//   OUMod:  b == 0       => 0
//   OSDiv:  b == 0 || -1 => a*b   (matches x86; avoids INT_MIN/-1 overflow trap)
//   OSMod:  b == 0 || -1 => 0
// ARM SDIV/UDIV give 0 for div/0, but mod via MSUB needs explicit guarding.
// ----------------------------------------------------------------------------
static void emit_div_mod( code_ctx *ctx, hl_op op, Arm64Reg out, Arm64Reg a, Arm64Reg b, int sf ) {
	bool unsign = (op == OUDiv || op == OUMod);
	bool is_div = (op == OSDiv || op == OUDiv);

	// Test b for 0; signed ops also test for -1.
	encode_logical_reg(ctx, sf, 0x03, 0, 0, b, 0, b, XZR);  // TST b, b
	int jz_pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);  // patched later

	int jneg_pos = -1;
	if( !unsign ) {
		// CMN b, #1  (= b + 1; sets Z if b == -1)
		encode_add_sub_imm(ctx, sf, 0, 1, 0, 1, b, XZR);
		jneg_pos = byte_count(ctx->code);
		encode_branch_cond(ctx, 0, COND_EQ);
	}

	// Mainline.  encode_div's U bit is 0=UDIV, 1=SDIV (per the ARM ARM
	// bit-10 encoding) — pass `unsign ? 0 : 1`, NOT the inverse.
	if( is_div ) {
		// SDIV/UDIV out, a, b
		encode_div(ctx, sf, unsign ? 0 : 1, b, a, out);
	} else {
		// MSUB needs the ORIGINAL `a` and `b` after the divide; SDIV writes
		// `out`, so any of {out==a, out==b} would clobber a source.  Spill
		// the aliased operand(s) to backend temps first.  ARM_TMP3 is
		// reserved precisely for cases like this where we need a third
		// independent register.
		Arm64Reg a_safe = a, b_safe = b;
		if( out == a ) {
			emit_mov_gpr(ctx, ARM_TMP3, a, sf);
			a_safe = ARM_TMP3;
			if( b == a ) b_safe = ARM_TMP3; // a==b too: same value in TMP3
		}
		if( out == b && b_safe == b ) {
			// Need a different temp from a_safe (which may be ARM_TMP3 already).
			Arm64Reg t = (a_safe == ARM_TMP1) ? ARM_TMP2 : ARM_TMP1;
			emit_mov_gpr(ctx, t, b, sf);
			b_safe = t;
		}
		encode_div(ctx, sf, unsign ? 0 : 1, b_safe, a_safe, out);
		// MSUB out, out, b_safe, a_safe  =>  out = a_safe - out * b_safe
		encode_madd_msub(ctx, sf, 1, b_safe, a_safe, out, out);
	}
	int jdone_pos = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0);

	// Special case path: result = 0 (mod or unsigned div) or a*b (signed div).
	int special_pos = byte_count(ctx->code);
	if( op == OSDiv ) {
		// out = a * b
		encode_madd_msub(ctx, sf, 0, b, XZR, a, out);
	} else {
		// out = 0
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, out);  // ORR out, XZR, XZR
	}

	int after = byte_count(ctx->code);

	// Patch branches.
	int delta_jz = (special_pos - jz_pos) >> 2;
	*(unsigned int*)&ctx->code.values[jz_pos] =
		(*(unsigned int*)&ctx->code.values[jz_pos] & ~(0x7FFFF << 5)) | ((delta_jz & 0x7FFFF) << 5);
	if( jneg_pos >= 0 ) {
		int delta_jn = (special_pos - jneg_pos) >> 2;
		*(unsigned int*)&ctx->code.values[jneg_pos] =
			(*(unsigned int*)&ctx->code.values[jneg_pos] & ~(0x7FFFF << 5)) | ((delta_jn & 0x7FFFF) << 5);
	}
	int delta_done = (after - jdone_pos) >> 2;
	*(unsigned int*)&ctx->code.values[jdone_pos] =
		(*(unsigned int*)&ctx->code.values[jdone_pos] & ~0x03FFFFFF) | (delta_done & 0x03FFFFFF);
}

// ----------------------------------------------------------------------------
// BINOP / UNOP float.
// ----------------------------------------------------------------------------
static void emit_binop_fp( code_ctx *ctx, hl_op op, ereg out_e, ereg a_e, ereg b_e, emit_mode mode ) {
	bool out_to_mem = (REG_KIND(out_e) != R_REG);
	Arm64FpReg out = out_to_mem ? (Arm64FpReg)31 : fpr_id(out_e);
	// Use V29/V30 as scratch FP regs (in our scratch list, won't collide with `out`=V31).
	Arm64FpReg a = materialize_fpr(ctx, a_e, mode, (Arm64FpReg)29);
	Arm64FpReg b = materialize_fpr(ctx, b_e, mode, (Arm64FpReg)30);
	int type = (mode == M_F64) ? 1 : 0;
	int opcode;
	switch( op ) {
	case OAdd:  opcode = 0x02; break; // FADD
	case OSub:  opcode = 0x03; break; // FSUB
	case OMul:  opcode = 0x00; break; // FMUL
	case OSDiv: opcode = 0x01; break; // FDIV
	default: jit_error("aarch64 emit_binop_fp: unsupported op");
	}
	encode_fp_arith(ctx, /*M=*/0, /*S=*/0, type, b, opcode, a, out);
	if( out_to_mem ) {
		Arm64Reg base = gpr_id(out_e);
		int offs = REG_VALUE(out_e);
		emit_ld_st(ctx, false, mode, out, base, offs);
	}
}

static void emit_unop( code_ctx *ctx, hl_op op, ereg out_e, ereg a_e, emit_mode mode ) {
	if( is_fp_mode(mode) ) {
		bool out_to_mem = (REG_KIND(out_e) != R_REG);
		Arm64FpReg out = out_to_mem ? (Arm64FpReg)31 : fpr_id(out_e);
		Arm64FpReg a = materialize_fpr(ctx, a_e, mode, (Arm64FpReg)29);
		int type = (mode == M_F64) ? 1 : 0;
		switch( op ) {
		case ONeg: encode_fp_1src(ctx, 0, 0, type, /*FNEG*/2, a, out); break;
		default: jit_error("aarch64 emit_unop float: unsupported op");
		}
		if( out_to_mem ) {
			Arm64Reg base = gpr_id(out_e);
			int offs = REG_VALUE(out_e);
			emit_ld_st(ctx, false, mode, out, base, offs);
		}
		return;
	}
	int sf = sf_for(mode);
	Arm64Reg out = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP1;
	Arm64Reg a = materialize_gpr(ctx, a_e, mode, ARM_TMP1);
	switch( op ) {
	case ONeg:
		// SUB out, XZR, a  (NEG alias)
		encode_add_sub_reg(ctx, sf, 1, 0, 0, a, 0, XZR, out);
		break;
	case ONot:
		// EOR out, a, #1  (boolean toggle).  N must equal sf for value 1.
		encode_logical_imm(ctx, sf, 0x02, sf, 0, 0, a, out);
		break;
	case OIncr:
		encode_add_sub_imm(ctx, sf, 0, 0, 0, 1, a, out);
		break;
	case ODecr:
		encode_add_sub_imm(ctx, sf, 1, 0, 0, 1, a, out);
		break;
	default:
		jit_error("aarch64 emit_unop: unsupported op");
	}
	if( mode == M_UI8 ) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, out, out);
	} else if( mode == M_UI16 ) {
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, out, out);
	}
	if( REG_KIND(out_e) != R_REG ) emit_mov(ctx, out_e, R(ARM_TMP1), mode);
}

// ----------------------------------------------------------------------------
// CONV / CONV_UNSIGNED.  e->mode = output mode, e->size_offs = input mode.
// ----------------------------------------------------------------------------
static void emit_conv( code_ctx *ctx, einstr *e, ereg out_e, bool unsign ) {
	emit_mode out_mode = e->mode;
	emit_mode in_mode = (emit_mode)e->size_offs;
	bool out_fp = is_fp_mode(out_mode);
	bool in_fp = is_fp_mode(in_mode);

	// Materialize source.
	Arm64Reg a_gpr = 0;
	Arm64FpReg a_fpr = (Arm64FpReg)0;
	if( in_fp ) {
		a_fpr = materialize_fpr(ctx, e->a, in_mode, (Arm64FpReg)29);
	} else {
		a_gpr = materialize_gpr(ctx, e->a, in_mode, ARM_TMP1);
	}

	// Pick output register encoding.  When the result lives in memory we route
	// the value through a backend-private temporary in the appropriate class.
	bool out_to_mem = REG_KIND(out_e) != R_REG;
	Arm64Reg dst_gpr = (!out_fp && !out_to_mem) ? gpr_id(out_e)
	                  : (!out_fp ? ARM_TMP2 : 0);
	// V31 is in our scratch list and serves as an FP temp; we still need to
	// emit a follow-up STR if the output is memory.
	Arm64FpReg dst_fpr = (out_fp && !out_to_mem) ? fpr_id(out_e)
	                    : (out_fp ? (Arm64FpReg)31 : (Arm64FpReg)0);

	if( in_fp && out_fp ) {
		// FCVT between F32/F64
		int type = (in_mode == M_F64) ? 1 : 0;       // input type
		int opcode = (in_mode == M_F32) ? 0x05 : 0x04; // F32->F64 = 0x05, F64->F32 = 0x04
		encode_fp_1src(ctx, 0, 0, type, opcode, a_fpr, dst_fpr);
	} else if( in_fp && !out_fp ) {
		// FP -> int.  FCVTZS / FCVTZU (round toward zero).
		int sf = sf_for(out_mode);
		int type = (in_mode == M_F64) ? 1 : 0;
		int rmode = 3;       // round toward zero
		int opc = unsign ? 1 : 0;  // 0=FCVTZS, 1=FCVTZU
		encode_fcvt_int(ctx, sf, 0, type, rmode, opc, a_fpr, dst_gpr);
	} else if( !in_fp && out_fp ) {
		// int -> FP. SCVTF / UCVTF.
		int sf = sf_for(in_mode);
		int type = (out_mode == M_F64) ? 1 : 0;
		int rmode = 0;
		int opc = unsign ? 3 : 2;  // 2=SCVTF, 3=UCVTF
		// First, widen sub-word inputs to full width.  UI8/UI16 are
		// unsigned regardless of the `unsign` flag (which here selects
		// SCVTF vs UCVTF), so always zero-extend the byte/half before
		// the FP conversion.
		Arm64Reg src = a_gpr;
		if( in_mode == M_UI8 || in_mode == M_UI16 ) {
			emit_uxt_to_w(ctx, in_mode, src, ARM_TMP1);
			src = ARM_TMP1;
		}
		encode_int_fcvt(ctx, sf, 0, type, rmode, opc, src, dst_fpr);
	} else {
		// int -> int.
		switch( in_mode ) {
		case M_UI8:
		case M_UI16:
			// UI8/UI16 are inherently unsigned in HL — widening to a larger
			// integer must always zero-extend, matching x86's MOVZX.  The
			// `unsign` flag is only meaningful for FP conversions.
			if( out_mode == M_PTR || out_mode == M_I32 ) {
				emit_uxt_to_w(ctx, in_mode, a_gpr, dst_gpr);
			} else if( out_mode == M_UI16 || out_mode == M_UI8 ) {
				emit_uxt_to_w(ctx, out_mode, a_gpr, dst_gpr);
			}
			break;
		case M_I32:
			if( out_mode == M_PTR ) {
				if( unsign ) emit_mov_gpr(ctx, dst_gpr, a_gpr, 0); // MOV Wd, Wn — zero-extends to X
				else emit_sxt_to_ptr(ctx, M_I32, a_gpr, dst_gpr);
			} else {
				emit_mov_gpr(ctx, dst_gpr, a_gpr, sf_for(out_mode));
				if( out_mode == M_UI8 || out_mode == M_UI16 )
					emit_uxt_to_w(ctx, out_mode, dst_gpr, dst_gpr);
			}
			break;
		case M_PTR:
			if( out_mode == M_I32 ) {
				emit_mov_gpr(ctx, dst_gpr, a_gpr, 0);  // truncate
			} else if( out_mode == M_UI8 || out_mode == M_UI16 ) {
				emit_uxt_to_w(ctx, out_mode, a_gpr, dst_gpr);
			} else {
				emit_mov_gpr(ctx, dst_gpr, a_gpr, 1);
			}
			break;
		default:
			jit_error("aarch64 emit_conv: unsupported int conversion");
		}
	}

	if( out_to_mem ) {
		if( out_fp ) {
			// STR D31/S31, [base+offs] — base might be inside a register operand
			// of `out_e`; use emit_ld_st with the FP class.
			Arm64Reg base = gpr_id(out_e);
			int offs = REG_VALUE(out_e);
			emit_ld_st(ctx, false, out_mode, dst_fpr, base, offs);
		} else {
			emit_mov(ctx, out_e, R(ARM_TMP2), out_mode);
		}
	}
}

// ----------------------------------------------------------------------------
// STORE / LOAD_ADDR / LEA.
// ----------------------------------------------------------------------------
static void emit_store( code_ctx *ctx, einstr *e ) {
	int offs = e->size_offs;
	Arm64Reg base;
	if( REG_KIND(e->a) == R_REG ) {
		base = gpr_id(e->a);
		// MK_STACK_OFFS(v) and MK_ADDR-like values encode the offset in the
		// register's value field; combine it with size_offs.  For regular
		// register operands REG_VALUE is 0, so this is a no-op.
		offs += REG_VALUE(e->a);
	} else {
		emit_mov(ctx, R(ARM_TMP1), e->a, M_PTR);
		base = ARM_TMP1;
	}
	if( is_fp_mode(e->mode) ) {
		if( REG_KIND(e->b) == R_REG ) {
			emit_ld_st(ctx, false, e->mode, fpr_id(e->b), base, offs);
		} else {
			// Route the bit pattern through a GPR.  STR writes the same bytes
			// regardless of FP vs. INT class.
			Arm64Reg tmp = (base == ARM_TMP1) ? ARM_TMP2 : ARM_TMP1;
			emit_mode int_mode = (e->mode == M_F32) ? M_I32 : M_PTR;
			if( REG_KIND(e->b) == R_CONST ) {
				load_immediate(ctx, (int64_t)REG_VALUE(e->b), tmp, sf_for(int_mode) == 1);
			} else if( REG_KIND(e->b) == R_REG_PTR ) {
				// Spilled FP vreg: load via emit_ld_st_ex so the offset-temp picker
				// can avoid clobbering `base` (parked in ARM_TMP1 when e->a was spilled).
				emit_ld_st_ex(ctx, true, int_mode, tmp, gpr_id(e->b), REG_VALUE(e->b), base);
			} else {
				emit_mov(ctx, R(tmp), e->b, int_mode);
			}
			emit_ld_st_ex(ctx, false, int_mode, tmp, base, offs, (Arm64Reg)-1);
		}
		return;
	}
	int reg_t;
	if( REG_KIND(e->b) == R_REG && REG_VALUE(e->b) == 0 ) {
		reg_t = gpr_id(e->b);
	} else {
		Arm64Reg tmp = (base == ARM_TMP1) ? ARM_TMP2 : ARM_TMP1;
		if( REG_KIND(e->b) == R_REG ) {
			// MK_STACK_OFFS / LEA-rewritten ADDRESS: source encodes (reg, offs).
			// Materialize the effective address into tmp.
			emit_lea_imm(ctx, tmp, gpr_id(e->b), REG_VALUE(e->b));
		} else if( REG_KIND(e->b) == R_CONST ) {
			load_immediate(ctx, (int64_t)REG_VALUE(e->b), tmp, sf_for(e->mode) == 1);
		} else if( REG_KIND(e->b) == R_REG_PTR ) {
			// Load directly via emit_ld_st_ex so we can tell it to avoid
			// clobbering `base` (which lives in ARM_TMP1 when e->a was spilled).
			emit_ld_st_ex(ctx, true, e->mode, tmp, gpr_id(e->b), REG_VALUE(e->b), base);
		} else {
			emit_mov(ctx, R(tmp), e->b, e->mode);
		}
		reg_t = tmp;
	}
	emit_ld_st_ex(ctx, false, e->mode, reg_t, base, offs, (Arm64Reg)-1);
}

static void emit_load_addr( code_ctx *ctx, einstr *e, ereg out_e ) {
	emit_mode lmode = (emit_mode)e->nargs;
	Arm64Reg base;
	int offs = e->size_offs;
	if( REG_KIND(e->a) == R_REG ) {
		base = gpr_id(e->a);
		offs += REG_VALUE(e->a);
	} else {
		emit_mov(ctx, R(ARM_TMP1), e->a, M_PTR);
		base = ARM_TMP1;
	}
	if( is_fp_mode(lmode) ) {
		if( REG_KIND(out_e) == R_REG ) {
			emit_ld_st(ctx, true, lmode, fpr_id(out_e), base, offs);
		} else {
			// FP load into V31 then STR to memory dst.
			emit_ld_st(ctx, true, lmode, (Arm64FpReg)31, base, offs);
			Arm64Reg out_base = gpr_id(out_e);
			int out_offs = REG_VALUE(out_e);
			emit_ld_st(ctx, false, lmode, (Arm64FpReg)31, out_base, out_offs);
		}
		return;
	}
	Arm64Reg dst = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP2;
	emit_ld_st(ctx, true, lmode, dst, base, offs);
	if( REG_KIND(out_e) != R_REG ) {
		emit_mov(ctx, out_e, R(ARM_TMP2), e->mode);
	}
}

static void emit_lea( code_ctx *ctx, einstr *e, ereg out_e ) {
	int mult = e->size_offs & 0xFF;
	int offs = e->size_offs >> 8;
	if( REG_KIND(e->a) == R_REG ) offs += REG_VALUE(e->a);

	Arm64Reg out = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP1;
	Arm64Reg a;
	if( REG_KIND(e->a) == R_REG ) {
		a = gpr_id(e->a);
	} else {
		emit_mov(ctx, R(ARM_TMP1), e->a, M_PTR);
		a = ARM_TMP1;
	}

	if( mult == 0 || IS_NULL(e->b) ) {
		// out = a + offs
		if( offs == 0 ) {
			emit_mov_gpr(ctx, out, a, 1);
		} else if( offs > 0 && offs <= 0xFFF ) {
			encode_add_sub_imm(ctx, 1, 0, 0, 0, offs, a, out);
		} else if( offs < 0 && -offs <= 0xFFF ) {
			encode_add_sub_imm(ctx, 1, 1, 0, 0, -offs, a, out);
		} else {
			load_immediate(ctx, offs, ARM_TMP2, true);
			encode_add_sub_reg(ctx, 1, 0, 0, 0, ARM_TMP2, 0, a, out);
		}
	} else {
		if( mult != 1 && mult != 2 && mult != 4 && mult != 8 )
			jit_error("aarch64 LEA: unsupported scale");
		int shift = (mult == 1) ? 0 : (mult == 2) ? 1 : (mult == 4) ? 2 : 3;
		// Index width matches HL semantics — array indexes are M_I32.  Materialize
		// from a 32-bit slot so we don't read garbage from the adjacent vreg, and
		// use the extended-register ADD with UXTW so only the lower 32 bits feed
		// the address calculation.
		Arm64Reg b = materialize_gpr_ex(ctx, e->b, M_I32, ARM_TMP2, a);
		// out = a + UXTW(b) << shift
		encode_add_sub_ext(ctx, /*sf=*/1, /*op=*/0, /*S=*/0, b, /*option=UXTW*/2, shift, a, out);
		if( offs != 0 ) {
			if( offs > 0 && offs <= 0xFFF ) {
				encode_add_sub_imm(ctx, 1, 0, 0, 0, offs, out, out);
			} else if( offs < 0 && -offs <= 0xFFF ) {
				encode_add_sub_imm(ctx, 1, 1, 0, 0, -offs, out, out);
			} else {
				load_immediate(ctx, offs, ARM_TMP2, true);
				encode_add_sub_reg(ctx, 1, 0, 0, 0, ARM_TMP2, 0, out, out);
			}
		}
	}

	if( REG_KIND(out_e) != R_REG ) emit_mov(ctx, out_e, R(ARM_TMP1), M_PTR);
}

// ----------------------------------------------------------------------------
// CMOV / XCHG / PUSH_CONST / PREFETCH.
// ----------------------------------------------------------------------------
static void emit_cmov_arm( code_ctx *ctx, ereg out_e, ereg a_e, ArmCondition cond ) {
	if( REG_KIND(out_e) != R_REG ) jit_error("aarch64 CMOV non-reg out");
	Arm64Reg out = gpr_id(out_e);
	Arm64Reg a = materialize_gpr(ctx, a_e, M_PTR, ARM_TMP1);
	// CSEL out, a, out, cond  (if cond: out=a; else out=out)
	encode_cond_select(ctx, 1, 0, out, cond, 0, a, out);
}

static void emit_xchg( code_ctx *ctx, einstr *e ) {
	if( REG_KIND(e->a) != R_REG || REG_KIND(e->b) != R_REG )
		jit_error("aarch64 XCHG with non-reg operand");
	Arm64Reg ra = gpr_id(e->a);
	Arm64Reg rb = gpr_id(e->b);
	emit_mov_gpr(ctx, ARM_TMP1, ra, 1);
	emit_mov_gpr(ctx, ra, rb, 1);
	emit_mov_gpr(ctx, rb, ARM_TMP1, 1);
}

static void emit_push_const( code_ctx *ctx, einstr *e ) {
	if( e->mode != M_PTR ) jit_error("aarch64 PUSH_CONST non-ptr mode");
	load_immediate(ctx, (int64_t)e->value, ARM_TMP1, true);
	emit_sp_offs(ctx, -16);
	encode_ldr_str_imm(ctx, 3, 0, 0, 0, SP_REG, ARM_TMP1);  // STR X16, [SP]
}

// ----------------------------------------------------------------------------
// Phase 4: constant-pool helpers.
// ----------------------------------------------------------------------------

static int reserve_const_segment( code_ctx *ctx, int size, int align ) {
	int pos = byte_count(ctx->const_table);
	if( align ) {
		int k = pos & (align - 1);
		if( k ) {
			byte_reserve_impl(&ctx->jit->galloc, &ctx->const_table, align - k);
			pos = byte_count(ctx->const_table);
		}
	}
	byte_reserve_impl(&ctx->jit->galloc, &ctx->const_table, size);
	return pos;
}

// Insert (or find) a 64-bit value in the constant table; record the current
// emission point as an ADRP+LDR (or ADRP+ADD) pair to be patched later.
// Returns the byte offset of the value inside ctx->const_table.
static int alloc_const( code_ctx *ctx, uint64_t value, int adrp_pos ) {
	int pos = value_map_find(ctx->const_table_lookup, value);
	if( pos < 0 ) {
		pos = reserve_const_segment(ctx, 8, 8);
		*(uint64_t*)byte_addr(ctx->const_table, pos) = value;
		value_map_add_impl(&ctx->jit->galloc, &ctx->const_table_lookup, value, pos);
	}
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, ctx->jit->out_pos + adrp_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, pos);
	return pos;
}

// Emit ADRP dst, page ; LDR dst, [dst, #lo12]  — load constant `value` from pool.
static void emit_const_load( code_ctx *ctx, Arm64Reg dst, uint64_t value ) {
	int adrp_pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, dst);                   // imm21 placeholder
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, dst, dst); // LDR Xd, [Xd, #0]
	alloc_const(ctx, value, adrp_pos);
}

// Emit ADRP dst, page ; ADD dst, dst, #lo12  — load address of pool entry `value`.
static void emit_const_addr( code_ctx *ctx, Arm64Reg dst, uint64_t value ) {
	int adrp_pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, dst);                                // imm21 placeholder
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 0, dst, dst);           // ADD Xd, Xd, #0
	alloc_const(ctx, value, adrp_pos);
}

// Emit ADRP+ADD pair targeting an offset INSIDE the const table (used for
// jump-table base addressing). The offset is recorded directly, not the value.
static void emit_pool_offset_addr( code_ctx *ctx, Arm64Reg dst, int const_offset ) {
	int adrp_pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, dst);
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 0, dst, dst);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, ctx->jit->out_pos + adrp_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, const_offset);
}

// ----------------------------------------------------------------------------
// Phase 4: call ops, LOAD_FUN, JUMP_TABLE.
// ----------------------------------------------------------------------------

// CALL_FUN: emit BL with a deferred imm26 patch (resolved in flush_consts once
// jit->mod->functions_ptrs[fid] holds the in-output offset).
static void emit_call_fun( code_ctx *ctx, einstr *e ) {
	int pos = byte_count(ctx->code);
	encode_branch_link(ctx, 0); // imm26 placeholder
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, ctx->jit->out_pos + pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, (int)e->a);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, /*kind=BL*/0);
}

// LOAD_FUN: emit ADRP + ADD with a deferred imm21+imm12 patch — produces the
// absolute address of the JIT-compiled function in `out`.
static void emit_load_fun( code_ctx *ctx, ereg out_e, int fid ) {
	Arm64Reg out = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP1;
	int pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, out);
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 0, out, out);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, ctx->jit->out_pos + pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, fid);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, /*kind=ADRP+ADD*/1);
	if( REG_KIND(out_e) != R_REG ) emit_mov(ctx, out_e, R(ARM_TMP1), M_PTR);
}

// CALL_PTR: indirect call via constant pool, with shortcuts for the two known
// near-call targets (hl_null_access, hl_jit_null_field_access).
static void emit_call_ptr( code_ctx *ctx, einstr *e ) {
	uint64_t target = (uint64_t)e->value;
	int near_pos = -1;
	if( target == (uint64_t)(uintptr_t)hl_null_access )
		near_pos = ctx->null_access_pos;
	else if( target == (uint64_t)(uintptr_t)hl_jit_null_field_access )
		near_pos = ctx->null_field_pos;

	if( near_pos >= 0 ) {
		// BL <stub> — direct PC-relative call to the trampoline emitted in
		// hl_codegen_init.  Both source and target are within the same output
		// buffer, so resolve the imm26 immediately.
		int pos = ctx->jit->out_pos + byte_count(ctx->code);
		intptr_t delta = (intptr_t)near_pos - (intptr_t)pos;
		int imm26 = (int)(delta >> 2);
		encode_branch_link(ctx, imm26);
	} else {
		emit_const_load(ctx, ARM_TMP1, target);
		encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	}
	// Sub-word return masking to match x86's MOVZX behavior.
	if( e->mode == M_UI8 )
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, X0, X0);
	else if( e->mode == M_UI16 )
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, X0, X0);
}

// CALL_REG: BLR <Xn>.
static void emit_call_reg( code_ctx *ctx, einstr *e ) {
	Arm64Reg target = materialize_gpr(ctx, e->a, M_PTR, ARM_TMP1);
	encode_branch_reg(ctx, /*BLR*/1, target);
}

// JUMP_TABLE: dispatch through a const_table-resident jump table whose entries
// are absolute target addresses (filled in hl_codegen_final).  Index value lives
// in e->a (32-bit int).  Falls through after BR — caller assumes no return.
static void emit_jump_table( code_ctx *ctx, einstr *e ) {
	int n = e->nargs;
	int start = reserve_const_segment(ctx, 8 * n, 16);

	// Materialize index as a zero-extended 64-bit value.  IR convention: e->a
	// holds an int (M_I32); MOV Wn, Wn zero-extends to X.
	Arm64Reg idx;
	if( REG_KIND(e->a) == R_REG ) {
		Arm64Reg src = gpr_id(e->a);
		// MOV W17, Wsrc — clears upper 32 bits.
		encode_logical_reg(ctx, 0, 0x01, 0, 0, src, 0, XZR, ARM_TMP2);
		idx = ARM_TMP2;
	} else {
		emit_mov(ctx, R(ARM_TMP2), e->a, M_I32);
		// Re-zero-extend to be safe.
		encode_logical_reg(ctx, 0, 0x01, 0, 0, ARM_TMP2, 0, XZR, ARM_TMP2);
		idx = ARM_TMP2;
	}

	emit_pool_offset_addr(ctx, ARM_TMP1, start);
	// LDR X16, [X16, idx, LSL #3]  size=3, V=0, opc=1, option=3 (LSL/UXTX), S=1
	encode_ldr_str_reg(ctx, 3, 0, 1, idx, /*option=*/3, /*S=*/1, ARM_TMP1, ARM_TMP1);
	encode_branch_reg(ctx, /*BR*/0, ARM_TMP1);

	ereg *args = hl_emit_get_args(ctx->jit->emit, e);
	for( int k = 0; k < n; k++ ) {
		int_arr_add_impl(&ctx->jit->galloc, &ctx->const_addr, start + k * 8);
		int_arr_add_impl(&ctx->jit->galloc, &ctx->const_addr, ctx->cur_op + (int)args[k] + 1);
	}
}

static void emit_prefetch( code_ctx *ctx, einstr *e ) {
	int prfop;
	switch( e->size_offs ) {
	case 0: prfop = 0; break;   // PLDL1KEEP
	case 1: prfop = 2; break;   // PLDL2KEEP
	case 2: prfop = 4; break;   // PLDL3KEEP
	case 3: prfop = 1; break;   // PLDL1STRM
	case 4: prfop = 16; break;  // PSTL1KEEP
	default: jit_error("aarch64 PREFETCH: bad size_offs");
	}
	Arm64Reg base;
	if( REG_KIND(e->a) == R_REG ) {
		base = gpr_id(e->a);
	} else {
		emit_mov(ctx, R(ARM_TMP1), e->a, M_PTR);
		base = ARM_TMP1;
	}
	// PRFM: size=11, V=0, opc=10, imm12=0, Rn=base, Rt=prfop
	encode_ldr_str_imm(ctx, 3, 0, 2, 0, base, (Arm64Reg)prfop);
}

// ============================================================================
// hl_codegen_flush
// ============================================================================

void hl_codegen_flush( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	jit->code_size = ctx->code.cur;
	jit->code_instrs = ctx->code.values;
	jit->code_pos_map = ctx->pos_map;
	if( ctx->pos_map ) ctx->pos_map[ctx->cur_op + 1] = ctx->code.cur;
}

// ============================================================================
// hl_codegen_function — the main per-IR-op switch
// ============================================================================

void hl_codegen_function( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	ctx->flushed = false;
	byte_free(&ctx->code);
	int_arr_free(&ctx->branch_fixups);
	free(ctx->pos_map);
	ctx->pos_map = (int*)malloc((jit->reg_instr_count + 1) * sizeof(int));
	ctx->pos_map[0] = 0;
	byte_reserve(ctx->code, 64);
	ctx->code.cur -= 64;

	int const_addr_prev = int_arr_count(ctx->const_addr);

	for( int cur_pos = 0; cur_pos < jit->reg_instr_count; cur_pos++ ) {
		einstr *e = jit->reg_instrs + cur_pos;
		ereg out = jit->reg_writes[cur_pos];
		byte_reserve(ctx->code, 64);
		ctx->code.cur -= 64;
		ctx->cur_op = cur_pos;
		if( cur_pos > 0 ) ctx->pos_map[cur_pos] = ctx->code.cur;

		switch( e->op ) {
		case LOAD_ARG:
			// nop — argument lives in its allocated register already
			continue;
		case NOP:
			// HINT #0  (NOP)
			EMIT32(ctx, 0xD503201F);
			break;
		case MOV:
			emit_mov(ctx, out, e->a, e->mode);
			break;
		case LOAD_CONST:
			emit_load_const(ctx, out, e->value, e->mode);
			break;
		case RET:
			// Result placement was handled by upstream regs phase via a preceding
			// regs_emit_mov(out, e->a). Here we just emit the actual return.
			encode_branch_reg(ctx, /*opc=*/2 /*RET*/, LR);
			break;
		case PUSH:
			emit_push(ctx, e->a, e->mode);
			break;
		case POP:
			emit_pop(ctx, e->a, e->mode);
			break;
		case STACK_OFFS:
			emit_sp_offs(ctx, e->size_offs);
			break;
		case CMP:
			emit_cmp(ctx, e);
			break;
		case TEST:
			emit_test(ctx, e);
			break;
		case JCOND:
			emit_jump_cond(ctx, get_cond_jump(ctx), e->size_offs);
			break;
		case JUMP:
			emit_jump(ctx, e->size_offs);
			break;
		case DEBUG_BREAK:
			// BRK #0  — encoded as 0xD4200000
			EMIT32(ctx, 0xD4200000);
			break;
		case BINOP:
			if( is_fp_mode(e->mode) )
				emit_binop_fp(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
			else
				emit_binop_int(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
			break;
		case UNOP:
			// jit_emit.c lowers `not b` and similar boolean toggles as a UNOP
			// with two operands (a, b=immediate, op=OXor).  Dispatch the
			// two-operand form through the regular binop handler so OXor/OAnd/OOr
			// don't need a second copy of the encoding.
			if( !IS_NULL(e->b) ) {
				if( is_fp_mode(e->mode) )
					emit_binop_fp(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
				else
					emit_binop_int(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
			} else {
				emit_unop(ctx, (hl_op)e->size_offs, out, e->a, e->mode);
			}
			break;
		case CONV:
			emit_conv(ctx, e, out, /*unsign=*/false);
			break;
		case CONV_UNSIGNED:
			emit_conv(ctx, e, out, /*unsign=*/true);
			break;
		case STORE:
			emit_store(ctx, e);
			break;
		case LOAD_ADDR:
			emit_load_addr(ctx, e, out);
			break;
		case LEA:
			emit_lea(ctx, e, out);
			break;
		case CMOV:
			emit_cmov_arm(ctx, out, e->a, get_cond_jump(ctx));
			break;
		case XCHG:
			emit_xchg(ctx, e);
			break;
		case CXCHG:
			// x86 emits BREAK() here too — atomic compare-exchange unimplemented.
			EMIT32(ctx, 0xD4200000);
			break;
		case PUSH_CONST:
			emit_push_const(ctx, e);
			break;
		case PREFETCH:
			emit_prefetch(ctx, e);
			break;
		case CALL_FUN:
			emit_call_fun(ctx, e);
			break;
		case CALL_PTR:
			emit_call_ptr(ctx, e);
			break;
		case CALL_REG:
			emit_call_reg(ctx, e);
			break;
		case LOAD_FUN:
			emit_load_fun(ctx, out, e->size_offs);
			break;
		case JUMP_TABLE:
			emit_jump_table(ctx, e);
			break;
		case ADDRESS:
			// Rewritten to LEA in the regs phase; should never reach here.
			jit_error("aarch64: ADDRESS reached backend (regs phase should rewrite)");
			break;
		case CATCH:
			// IR marker only (mirrors x86) — no code emitted.
			break;
		default:
			{
				static const char *op_names[] = {
					"LOAD_ADDR", "LOAD_CONST", "LOAD_ARG", "LOAD_FUN", "STORE",
					"LEA", "TEST", "CMP", "JCOND", "JUMP", "JUMP_TABLE",
					"BINOP", "UNOP", "CONV", "CONV_UNSIGNED", "RET",
					"CALL_PTR", "CALL_REG", "CALL_FUN", "MOV", "CMOV",
					"XCHG", "CXCHG", "PUSH_CONST", "PUSH", "POP",
					"ALLOC_STACK", "PREFETCH", "DEBUG_BREAK", "BLOCK",
					"ENTER", "STACK_OFFS", "CATCH", "ADDRESS", "NOP"
				};
				static char errbuf[128];
				const char *name = (e->op < (int)(sizeof(op_names)/sizeof(*op_names)))
					? op_names[e->op] : "?";
				snprintf(errbuf, sizeof(errbuf), "aarch64: unhandled IR op %s (%d) at cur_op=%d",
					name, e->op, cur_pos);
				jit_error(errbuf);
			}
			break;
		}

		if( ctx->code.cur > ctx->code.max ) jit_error("aarch64 code buffer overrun");
	}

	// Functions are 4-byte aligned naturally on ARM; no padding needed for now.
	hl_codegen_flush(jit);

	// Patch all in-function branches.
	for( int i = 0; i < int_arr_count(ctx->branch_fixups); i += 3 ) {
		int pos = int_arr_get(ctx->branch_fixups, i);
		int target_op = int_arr_get(ctx->branch_fixups, i + 1);
		int is_cond = int_arr_get(ctx->branch_fixups, i + 2);
		int target_byte_pos = ctx->pos_map[target_op];
		patch_branch(ctx, pos, target_byte_pos, is_cond);
	}

	// Convert any jump-table target_op_index entries recorded by emit_jump_table
	// into absolute byte offsets in the output buffer.
	for( int i = const_addr_prev; i < int_arr_count(ctx->const_addr); i += 2 ) {
		int target_op = int_arr_get(ctx->const_addr, i + 1);
		int offs = jit->out_pos + ctx->pos_map[target_op];
		ctx->const_addr.values[i + 1] = offs;
	}
}

// ============================================================================
// Phase 4: module-level emission.
// ============================================================================

// Helper: finalize a freshly-emitted helper stub (null-access stubs, c2hl,
// hl2c).  Mirrors x86's flush_function: reports the start/size to the unwind
// machinery and rounds the function buffer to 16 bytes.
static void flush_helper( code_ctx *ctx, int start ) {
	hl_jit_define_function(ctx->jit, start, ctx->jit->out_pos + byte_count(ctx->code) - start);
	while( byte_count(ctx->code) & 15 )
		EMIT32(ctx, 0xD503201F); // NOP
	if( byte_count(ctx->code) > ctx->code.max ) jit_error("aarch64 trampoline overrun");
}

// Patch a placeholder branch (B, BL, or B.cond) emitted at byte position `pos`
// to target byte position `target` in the same buffer.  Selects imm26 for
// unconditional and imm19 for conditional based on the opcode bits.
static void patch_helper_branch( code_ctx *ctx, int pos, int target ) {
	int delta = (target - pos) >> 2;
	unsigned int *insn = (unsigned int*)&ctx->code.values[pos];
	unsigned int op = (*insn >> 26) & 0x3F;
	if( op == 0x05 || op == 0x25 ) {
		// B / BL: imm26
		*insn = (*insn & ~0x03FFFFFFu) | ((unsigned)delta & 0x03FFFFFF);
	} else {
		// B.cond: imm19 in bits [23:5]
		*insn = (*insn & ~(0x7FFFFu << 5)) | ((unsigned)(delta & 0x7FFFF) << 5);
	}
}

// Emit a function prologue compatible with the Apple ARM64 + AAPCS64 ABI:
// STP X29, X30, [SP, #-16]! ; MOV X29, SP.
static void emit_helper_prologue( code_ctx *ctx ) {
	encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x03, /*imm7=*/-2 & 0x7F, LR, SP_REG, FP);
	emit_mov_gpr(ctx, FP, SP_REG, 1);
}

// Emit the standard epilogue used by all helpers/trampolines:
// MOV SP, X29 ; LDP X29, X30, [SP], #16 ; RET.
static void emit_helper_epilogue( code_ctx *ctx ) {
	emit_mov_gpr(ctx, SP_REG, FP, 1);
	encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x01, /*imm7=*/2, LR, SP_REG, FP);
	encode_branch_reg(ctx, /*RET*/2, LR);
}

// Emit hl_null_access stub: ADRP/LDR the C function pointer and BLR (it never
// returns; we still emit a BRK afterward to mirror x86).
static void emit_null_access_stub( code_ctx *ctx, void *target ) {
	emit_helper_prologue(ctx);
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)target);
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	EMIT32(ctx, 0xD4200000); // BRK #0
}

// Emit hl_jit_null_field_access stub.  The caller passes the field hash in W0.
// The C function takes one int argument (the hash), so our trampoline doesn't
// need to marshal — just forward.
static void emit_null_field_stub( code_ctx *ctx, void *target ) {
	emit_helper_prologue(ctx);
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)target);
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	EMIT32(ctx, 0xD4200000); // BRK #0
}

// Emit the c2hl trampoline.
//
// Called from C with: X0 = JIT-compiled fn ptr, X1 = &vargs (struct{regs[16];
// stack[16]}), X2 = stack-arg count.
//
// The C side (jit.c:callback_c2hl) populates vargs.regs[0..7] with int reg
// args, vargs.regs[8..15] with FP reg args, and vargs.stack[16-N..15] with the
// N stack args (leftmost stack arg at vargs.stack[15]).  We:
//   1. Load X0..X7 from [vargs+0..56] and D0..D7 from [vargs+64..120].
//   2. Push the stack args in reverse order so the leftmost ends up at SP+0.
//   3. BLR fn ; restore frame ; RET.
//
// X16/X17 hold the fn pointer and vargs through the call (they survive any
// data-load up to BLR; the dynamic linker only clobbers them at the BLR itself,
// at which point we're done with them).
static void emit_c2hl_trampoline( code_ctx *ctx ) {
	emit_helper_prologue(ctx);
	emit_mov_gpr(ctx, ARM_TMP1, X0, 1);  // X16 = fn
	emit_mov_gpr(ctx, ARM_TMP2, X1, 1);  // X17 = vargs
	emit_mov_gpr(ctx, X9, X2, 1);        // X9  = stack count

	// Load int arg regs from vargs.regs[0..7].
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 0, X1, ARM_TMP2, X0); // LDP X0,X1, [X17, #0]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 2, X3, ARM_TMP2, X2); // LDP X2,X3, [X17, #16]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 4, X5, ARM_TMP2, X4); // LDP X4,X5, [X17, #32]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 6, X7, ARM_TMP2, X6); // LDP X6,X7, [X17, #48]
	// Load FP arg regs from vargs.regs[8..15] (= byte offsets 64..120).
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 8,  (Arm64Reg)1, ARM_TMP2, (Arm64Reg)0);  // LDP D0,D1, [X17, #64]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 10, (Arm64Reg)3, ARM_TMP2, (Arm64Reg)2);  // LDP D2,D3, [X17, #80]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 12, (Arm64Reg)5, ARM_TMP2, (Arm64Reg)4);  // LDP D4,D5, [X17, #96]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 14, (Arm64Reg)7, ARM_TMP2, (Arm64Reg)6);  // LDP D6,D7, [X17, #112]

	// Push stack args.  callback_c2hl in src/jit.c writes the i-th stack-passed
	// arg to vargs.stack[--sp] starting at sp = MAX_ARGS = 16.  So:
	//   stack[15]   = first/leftmost stack arg   (at vargs + 248)
	//   stack[14]   = second
	//   ...
	//   stack[16-N] = last/rightmost stack arg
	// AAPCS64 requires [SP+0] = leftmost stack arg.  We walk SOURCE DOWN from
	// &stack[15] while DEST walks UP from SP — N iterations total.  SP is
	// pre-allocated rounded up to a multiple of 16 (= N*8 + (N&1)*8 bytes).

	// CBZ X9, no_stack — skip everything if no stack args.
	int cbz_skip_pos = byte_count(ctx->code);
	encode_cbz_cbnz(ctx, /*sf=*/1, /*op=*/0, 0, X9);

	// X10 = X9 * 8 (size in bytes; LSL #3 via UBFM).
	emit_bitfield(ctx, /*sf=*/1, /*opc=UBFM*/0x02, /*immr=*/(64 - 3) & 0x3F, /*imms=*/63 - 3, X9, X10);

	// Pad: if X9 is odd, allocate +8 so SP stays 16-aligned.
	// AND X11, X9, #1 ; LSL X11, X11, #3 ; ADD X10, X10, X11.
	encode_logical_imm(ctx, 1, 0x00, 1, 0, 0, X9, X11);  // AND X11, X9, #1 (immr=0,imms=0,N=1 → 1)
	emit_bitfield(ctx, 1, 0x02, (64 - 3) & 0x3F, 63 - 3, X11, X11);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, X11, 0, X10, X10);

	// SUB SP, SP, X10  — must use ADD/SUB (extended register); the shifted-reg
	// form treats register 31 as XZR, not SP, so this would silently NOP out.
	encode_add_sub_ext(ctx, 1, 1, 0, X10, /*UXTX*/3, 0, SP_REG, SP_REG);

	// X12 = &vargs.stack[15] = vargs + 128 + 15*8 = vargs + 248
	// (the slot holding the LEFTMOST stack arg; source walks DOWN from here).
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 248, ARM_TMP2, X12);            // ADD X12, X17, #248

	// Destination pointer X13 = SP ([SP+0] = leftmost per AAPCS64).
	emit_mov_gpr(ctx, X13, SP_REG, 1);

	// Counter X14 = X9.
	emit_mov_gpr(ctx, X14, X9, 1);

	// Copy loop: walk SOURCE down, DEST up, X14 times.
	//   *X13 = *X12 ; X12 -= 8 ; X13 += 8 ; X14--.
	int loop_top = byte_count(ctx->code);
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, X12, X15);                      // LDR X15, [X12, #0]
	encode_ldr_str_imm(ctx, 3, 0, 0, 0, X13, X15);                      // STR X15, [X13, #0]
	encode_add_sub_imm(ctx, 1, 1, 0, 0, 8, X12, X12);                   // SUB X12, X12, #8
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 8, X13, X13);                   // ADD X13, X13, #8
	encode_add_sub_imm(ctx, 1, 1, 1, 0, 1, X14, X14);                   // SUBS X14, X14, #1
	int loop_branch_pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_NE);                                 // B.NE loop_top
	patch_helper_branch(ctx, loop_branch_pos, loop_top);

	// Patch the CBZ skip target = end of stack-push block.
	int after_stack = byte_count(ctx->code);
	patch_helper_branch(ctx, cbz_skip_pos, after_stack);
	// --- END STACK PUSH ---

	// BLR fn (X16).
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);

	emit_helper_epilogue(ctx);
}

// Emit the hl2c trampoline.  Called from JIT-compiled HL code; X0 holds the
// closure (vclosure_wrapper*), X1..X7,V0..V7 hold call args.  We:
//   1. Spill X0..X7 and V0..V7 into a 128-byte buffer beneath the saved frame.
//   2. Inspect cl->t->fun->ret->kind to decide between hl_jit_wrapper_ptr
//      (default) and hl_jit_wrapper_d (HF32/HF64 return).
//   3. Call wrapper(closure, &caller_stack_args, &spilled_regs).
static void emit_hl2c_trampoline( code_ctx *ctx ) {
	hl_type_fun *ft = NULL;

	emit_helper_prologue(ctx);
	emit_sp_offs(ctx, -128); // SUB SP, SP, #128

	// Spill X0..X7 → [SP+0..56].  mode 0x12 = signed-offset STORE.
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 0, X1, SP_REG, X0); // STP X0,X1, [SP, #0]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 2, X3, SP_REG, X2); // STP X2,X3, [SP, #16]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 4, X5, SP_REG, X4); // STP X4,X5, [SP, #32]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 6, X7, SP_REG, X6); // STP X6,X7, [SP, #48]
	// Spill V0..V7 → [SP+64..120] (V0 at lowest, matching wrapper expectations).
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 8,  (Arm64Reg)1, SP_REG, (Arm64Reg)0); // STP D0,D1, [SP, #64]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 10, (Arm64Reg)3, SP_REG, (Arm64Reg)2); // STP D2,D3, [SP, #80]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 12, (Arm64Reg)5, SP_REG, (Arm64Reg)4); // STP D4,D5, [SP, #96]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 14, (Arm64Reg)7, SP_REG, (Arm64Reg)6); // STP D6,D7, [SP, #112]

	// X9 = closure (still in X0 — copy to keep X0 alive across loads).
	emit_mov_gpr(ctx, X9, X0, 1);
	// X9 = X9->t            ; LDR X9, [X9, #0]
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, X9, X9);
	// X9 = X9->fun          ; LDR X9, [X9, #8]
	encode_ldr_str_imm(ctx, 3, 0, 1, 1, X9, X9);
	// X9 = X9->ret          ; LDR X9, [X9, #offsetof(hl_type_fun, ret)]
	int ret_offset = (int)(int_val)&ft->ret;
	if( (ret_offset & 7) == 0 && (unsigned)ret_offset < 0x8000 )
		encode_ldr_str_imm(ctx, 3, 0, 1, ret_offset / 8, X9, X9);
	else {
		load_immediate(ctx, ret_offset, X10, true);
		encode_ldr_str_reg(ctx, 3, 0, 1, X10, /*option=*/3, /*S=*/0, X9, X9);
	}
	// W9 = W9->kind         ; LDR W9, [X9, #0]
	encode_ldr_str_imm(ctx, 2, 0, 1, 0, X9, X9);

	// Branch on return-type kind.  HF64 / HF32 → wrapper_d; default → wrapper_ptr.
	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF64, X9, XZR);   // CMP W9, #HF64
	int jeq_f64 = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);
	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF32, X9, XZR);   // CMP W9, #HF32
	int jeq_f32 = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);

	// Default path: load wrapper_ptr.
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)hl_jit_wrapper_ptr);
	int jdone_default = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0);

	// Float path.
	int float_path = byte_count(ctx->code);
	patch_helper_branch(ctx, jeq_f64, float_path);
	patch_helper_branch(ctx, jeq_f32, float_path);
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)hl_jit_wrapper_d);

	int after_select = byte_count(ctx->code);
	patch_helper_branch(ctx, jdone_default, after_select);

	// Set up wrapper args:
	// X0 (closure)  — already in X0 across the type-walk because the LDR chain
	//                 above used X9 only.  ✓
	// X1 = caller stack args = X29 + 16 (skip saved fp+lr).
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, FP, X1);
	// X2 = &spilled regs = SP.
	emit_mov_gpr(ctx, X2, SP_REG, 1);

	// Call wrapper.
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);

	emit_helper_epilogue(ctx);
}

void hl_codegen_init( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	byte_reserve(ctx->code, 4096);
	ctx->code.cur -= 4096;

	// hl_null_access stub.
	ctx->null_access_pos = jit->out_pos + byte_count(ctx->code);
	emit_null_access_stub(ctx, (void*)hl_null_access);
	flush_helper(ctx, ctx->null_access_pos);

	// hl_jit_null_field_access stub.
	ctx->null_field_pos = jit->out_pos + byte_count(ctx->code);
	emit_null_field_stub(ctx, (void*)hl_jit_null_field_access);
	flush_helper(ctx, ctx->null_field_pos);

	// c2hl + hl2c trampolines.
	jit->code_funs.c2hl = jit->out_pos + byte_count(ctx->code);
	emit_c2hl_trampoline(ctx);
	flush_helper(ctx, jit->code_funs.c2hl);

	jit->code_funs.hl2c = jit->out_pos + byte_count(ctx->code);
	emit_hl2c_trampoline(ctx);
	flush_helper(ctx, jit->code_funs.hl2c);

	hl_codegen_flush(jit);
}

// ---------------------------------------------------------------------------
// hl_codegen_flush_consts: patch BL/ADRP/LDR/ADD references against absolute
// positions, then append the constant table to the output stream.
// ---------------------------------------------------------------------------

// Patch ADRP imm21 split (immlo at bits 30:29, immhi at bits 23:5) given a
// target byte address `target_abs` and the address `pc_abs` of the ADRP insn.
// Both are absolute byte offsets within `jit->output` (page-aligned arithmetic
// is preserved when the buffer is later mmap'd to a page-aligned VA).
static void patch_adrp_imm21( unsigned char *out, int pc_abs, int target_abs ) {
	int imm21 = (target_abs >> 12) - (pc_abs >> 12);
	unsigned int *insn = (unsigned int*)(out + pc_abs);
	unsigned int immlo = (unsigned)(imm21 & 0x3);
	unsigned int immhi = (unsigned)((imm21 >> 2) & 0x7FFFF);
	*insn = (*insn & ~((0x3u << 29) | (0x7FFFFu << 5)))
	      | (immlo << 29) | (immhi << 5);
}

// Patch ADD/LDR imm12 (bits 21:10).  `scale` is the instruction's natural
// immediate scale (1 for ADD, 8 for 64-bit LDR, etc.).  Caller guarantees the
// low bits of the target are aligned to `scale`.
static void patch_imm12( unsigned char *out, int pos, int target_lo12, int scale ) {
	unsigned int *insn = (unsigned int*)(out + pos);
	unsigned int imm12 = (unsigned)((target_lo12 / scale) & 0xFFF);
	*insn = (*insn & ~(0xFFFu << 10)) | (imm12 << 10);
}

void hl_codegen_flush_consts( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;

	// Patch cross-function call sites recorded in `funs`.
	for( int i = 0; i < int_arr_count(ctx->funs); i += 3 ) {
		int pos = int_arr_get(ctx->funs, i);
		int fid = int_arr_get(ctx->funs, i + 1);
		int kind = int_arr_get(ctx->funs, i + 2);
		intptr_t target_offs = (intptr_t)jit->mod->functions_ptrs[fid];
		if( kind == 0 ) {
			// BL imm26. AArch64 BL has a ±128 MB direct-call range; if the JIT
			// buffer ever grows beyond that, we'd silently truncate and call
			// the wrong address. Fail fast instead.
			intptr_t delta = target_offs - (intptr_t)pos;
			if( delta < -(intptr_t)(1<<27) || delta >= (intptr_t)(1<<27) )
				jit_error("aarch64 BL target out of imm26 range");
			int imm26 = (int)(delta >> 2);
			unsigned int *insn = (unsigned int*)(jit->output + pos);
			*insn = (*insn & ~0x03FFFFFFu) | ((unsigned)imm26 & 0x03FFFFFF);
		} else {
			// ADRP+ADD pair: pos = ADRP, pos+4 = ADD.
			patch_adrp_imm21(jit->output, pos, (int)target_offs);
			int lo12 = (int)target_offs & 0xFFF;
			patch_imm12(jit->output, pos + 4, lo12, /*scale=*/1);
		}
	}
	int_arr_reset(&ctx->funs);

	// Pad jit->out_pos to an 8-byte boundary so that constants at offset 0
	// (and every multiple of 8) within the table are reachable through LDR's
	// 8-byte-scaled imm12 field with no precision loss.
	while( jit->out_pos & 7 ) {
		if( jit->out_pos < jit->out_max ) jit->output[jit->out_pos] = 0;
		jit->out_pos++;
	}

	// Append the constant table to the output stream.
	jit->code_size = byte_count(ctx->const_table);
	jit->code_instrs = ctx->const_table.values;
	ctx->const_table_pos = jit->out_pos;

	// Patch ADRP+(LDR|ADD) const-pool refs.
	for( int i = 0; i < int_arr_count(ctx->const_refs); i += 2 ) {
		int adrp_pos = int_arr_get(ctx->const_refs, i);
		int coffs = int_arr_get(ctx->const_refs, i + 1);
		int target = ctx->const_table_pos + coffs;
		patch_adrp_imm21(jit->output, adrp_pos, target);
		// Detect whether the second insn is LDR (Xt|Dt|St) or ADD by inspecting
		// the top 10 bits (31:22). LDR (unsigned-imm) encoding is
		// `size 111 V 01 01 imm12 Rn Rt`; the 8-byte-scaled imm12 lives in
		// bits 21:10. ADD-imm leaves the imm12 unscaled.
		// Top10 bits (>>22) of canonical encodings:
		//   LDR Xt (size=11,V=0,opc=01): 0b1111100101 = 0x3E5  (scale=8)
		//   LDR Dt (size=11,V=1,opc=01): 0b1111110101 = 0x3F5  (scale=8)
		//   LDR St (size=10,V=1,opc=01): 0b1011110101 = 0x2F5  (scale=4)
		// ADD-imm always falls into the else.
		unsigned int second = *(unsigned int*)(jit->output + adrp_pos + 4);
		int lo12 = target & 0xFFF;
		switch( (second >> 22) & 0x3FF ) {
		case 0x3E5: // LDR Xt
		case 0x3F5: // LDR Dt
			patch_imm12(jit->output, adrp_pos + 4, lo12, /*scale=*/8);
			break;
		case 0x2F5: // LDR St
			patch_imm12(jit->output, adrp_pos + 4, lo12, /*scale=*/4);
			break;
		default:
			// ADD (imm), unscaled.
			patch_imm12(jit->output, adrp_pos + 4, lo12, /*scale=*/1);
			break;
		}
	}
	int_arr_reset(&ctx->const_refs);

	byte_free(&ctx->const_table);
	value_map_free(&ctx->const_table_lookup);
}

void hl_codegen_final( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	// Fill jump-table entries with absolute addresses inside final_code.
	for( int i = 0; i < int_arr_count(ctx->const_addr); i += 2 ) {
		int table_offs = int_arr_get(ctx->const_addr, i);
		int target_offs = int_arr_get(ctx->const_addr, i + 1);
		*(void**)(jit->final_code + ctx->const_table_pos + table_offs) =
			jit->final_code + target_offs;
	}
	int_arr_free(&ctx->const_addr);
}
