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

/* AArch64 backend for the IR-based JIT. */

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

// STACK_REG uses ereg encoding 5, so X5 uses logical encoding 32.
#define X5_LOGICAL	32

// Debug callback trampolines support up to 64 bytes of overflow arguments.
#define TRAMPOLINE_STACK_ARGS	64

// largest code sequence emitted for a single IR op
#define MAX_OP_SIZE	128

#define R(id)		MK_REG(id, R_REG)
#define V(id)		MK_REG((id) + 64, R_REG)

static void patch_imm12( unsigned char *out, int pos, int target_lo12, int scale );
static void patch_adrp_imm21( void *code, int pc_abs, int target_abs );

void hl_jit_init_regs( regs_config *cfg ) {
	// X15-X17 are backend temporaries. X18 is reserved by some AAPCS64 platforms.
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

	// V8-V15 are callee-saved. V29-V31 are backend temporaries.
	static int float_scratch[] = {
		V(0), V(1), V(2), V(3), V(4), V(5), V(6), V(7),
		V(16), V(17), V(18), V(19), V(20), V(21), V(22), V(23),
		V(24), V(25), V(26), V(27), V(28)
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

	// shifts and divisions can use any register
	cfg->req_bit_shifts = 0;
	cfg->req_div_a = 0;
	cfg->req_div_b = 0;

	cfg->stack_reg = R(SP_REG);
	cfg->stack_pos = R(FP);
	cfg->stack_align = 16;
	// SP must stay 16-byte aligned for any memory access through it
	cfg->min_stack_args_size = 16;
#if defined(HL_MAC) || defined(HL_IOS) || defined(HL_TVOS)
	cfg->min_native_stack_args_size = 1; // Apple packs stack args at their natural size
#else
	cfg->min_native_stack_args_size = 8;
#endif

#ifdef GEN_DEBUG
	cfg->debug_prefix_size = 0;
#endif
}

const char *hl_natreg_str( int reg, emit_mode m ) {
	static char out[16];
	int r = REG_REG(reg);
	// undo the gpr_id remapping
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

#define ARM_TMP1	X16    // backend-private scratch (IP0)
#define ARM_TMP2	X17    // backend-private scratch (IP1)
#define ARM_TMP3	X15    // backend-private scratch (excluded from regalloc)

// STACK_REG maps to FP and X5_LOGICAL maps to X5
static Arm64Reg gpr_id( ereg r ) {
	int v = REG_REG(r);
	if( v == STACK_REG ) return FP;
	if( v == X5_LOGICAL ) return X5;
	return (Arm64Reg)v;
}

static Arm64FpReg fpr_id( ereg r ) {
	return (Arm64FpReg)(REG_REG(r) - 64);
}

static void emit_bitfield( code_ctx *ctx, int sf, int opc, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd );

// UXTB/UXTH : C callers leave the upper bits of small arguments undefined
static void emit_zero_extend( code_ctx *ctx, Arm64Reg dst, Arm64Reg src, emit_mode mode ) {
	emit_bitfield(ctx, 0, /*UBFM*/0x02, 0, mode == M_UI8 ? 7 : 15, src, dst);
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
	// 1 = 64-bit, 0 = 32-bit
	return (m == M_PTR || m == M_F64) ? 1 : 0;
}

static bool is_fp_mode( emit_mode m ) { return m == M_F32 || m == M_F64; }

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
	uint32_t mag_lo = mag & 0xFFF;
	uint32_t mag_hi = mag >> 12;
	if( mag_hi <= 0xFFF ) {
		encode_add_sub_imm(ctx, 1, op, 0, 1, (int)mag_hi, SP_REG, SP_REG);
		if( mag_lo )
			encode_add_sub_imm(ctx, 1, op, 0, 0, (int)mag_lo, SP_REG, SP_REG);
		return;
	}
	// must be the extended-register form: the shifted-register form reads 31 as XZR, not SP
	load_immediate(ctx, (int64_t)mag, ARM_TMP1, true);
	encode_add_sub_ext(ctx, 1, op, 0, ARM_TMP1, /*option=UXTX*/3, /*imm3=*/0, SP_REG, SP_REG);
}

// op : 0 = ADD, 1 = SUB. returns false if mag needs more than two imm12 instructions
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

// avoid : a register holding a live value that the offset temp must not clobber
static void emit_ld_st_ex( code_ctx *ctx, bool is_load, emit_mode mode, int reg_t, Arm64Reg base, int offs, Arm64Reg avoid ) {
	int size = ls_size_for(mode);
	int V = is_fp_mode(mode) ? 1 : 0;
	int opc = is_load ? 1 : 0; // 0 = STR, 1 = LDR
	int scale = 1 << size;
	if( offs >= 0 && (offs & (scale - 1)) == 0 && (offs / scale) < 0x1000 ) {
		encode_ldr_str_imm(ctx, size, V, opc, offs / scale, base, (Arm64Reg)reg_t);
		return;
	}
	if( offs >= -256 && offs < 256 ) {
		encode_ldur_stur(ctx, size, V, opc, offs, base, (Arm64Reg)reg_t);
		return;
	}
	// offset temp must differ from base and avoid, and from reg_t for stores
	// (a load reads the offset before writing reg_t)
	Arm64Reg off_tmp = ARM_TMP1;
	bool gpr_store = V == 0 && !is_load;
	bool bad_t1 = (gpr_store && reg_t == ARM_TMP1) || base == ARM_TMP1 || avoid == ARM_TMP1;
	if( bad_t1 ) off_tmp = ARM_TMP2;
	bool bad_t2 = (gpr_store && reg_t == off_tmp) || base == off_tmp || avoid == off_tmp;
	if( bad_t2 ) off_tmp = ARM_TMP3;
	bool bad_t3 = (gpr_store && reg_t == off_tmp) || base == off_tmp || avoid == off_tmp;
	if( bad_t3 ) jit_error("aarch64 emit_ld_st: no free offset temp");
	load_immediate(ctx, offs, off_tmp, true);
	encode_ldr_str_reg(ctx, size, V, opc, off_tmp, /*option=*/3 /*LSL*/, /*S=*/0, base, (Arm64Reg)reg_t);
}

static void emit_ld_st( code_ctx *ctx, bool is_load, emit_mode mode, int reg_t, Arm64Reg base, int offs ) {
	emit_ld_st_ex(ctx, is_load, mode, reg_t, base, offs, (Arm64Reg)-1 /*no avoid*/);
}

// ORR can't encode SP, ADD #0 can
static void emit_mov_gpr( code_ctx *ctx, Arm64Reg dst, Arm64Reg src, int sf ) {
	if( dst == src ) return;
	if( dst == SP_REG || src == SP_REG ) {
		// ADD dst, src, #0
		encode_add_sub_imm(ctx, sf, 0, 0, 0, 0, src, dst);
	} else {
		// ORR dst, XZR, src
		encode_logical_reg(ctx, sf, 0x01, 0, 0, src, 0, XZR, dst);
	}
}

static void emit_mov_fpr( code_ctx *ctx, Arm64FpReg dst, Arm64FpReg src, emit_mode mode ) {
	if( dst == src ) return;
	int type = (mode == M_F64) ? 1 : 0; // 1=double, 0=single
	// FMOV
	encode_fp_1src(ctx, /*M=*/0, /*S=*/0, type, /*opcode=*/0, src, dst);
}

static void emit_load_const( code_ctx *ctx, ereg out, uint64_t value, emit_mode mode );

static int  reserve_const_segment( code_ctx *ctx, int size, int align );
static int  alloc_const( code_ctx *ctx, uint64_t value, int adrp_pos );
static void emit_const_load( code_ctx *ctx, Arm64Reg dst, uint64_t value );
static void emit_pool_offset_addr( code_ctx *ctx, Arm64Reg dst, int const_offset );
static Arm64FpReg materialize_fpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64FpReg tmp );
static Arm64Reg   materialize_gpr( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp );
static Arm64Reg   materialize_gpr_ex( code_ctx *ctx, ereg src, emit_mode mode, Arm64Reg tmp, Arm64Reg avoid );

// never uses a temp register
static void emit_lea_imm( code_ctx *ctx, Arm64Reg out, Arm64Reg base, int offs ) {
	if( offs == 0 ) {
		emit_mov_gpr(ctx, out, base, 1);
		return;
	}
	if( emit_addsub_imm_2step(ctx, offs < 0, out, base, offs < 0 ? -(uint32_t)offs : (uint32_t)offs) )
		return;
	if( out == base ) jit_error("aarch64 LEA offset out of range");
	load_immediate(ctx, offs, out, true);
	encode_add_sub_reg(ctx, 1, 0, 0, 0, out, 0, base, out);
}

static void emit_mov( code_ctx *ctx, ereg dst, ereg src, emit_mode mode ) {
	int dst_kind = REG_KIND(dst);
	int src_kind = REG_KIND(src);

	if( dst_kind == R_REG && src_kind == R_REG ) {
		// MK_STACK_OFFS or LEA-rewritten ADDRESS : dst = reg + offs
		if( !is_fp_mode(mode) && REG_VALUE(src) != 0 ) {
			emit_lea_imm(ctx, gpr_id(dst), gpr_id(src), REG_VALUE(src));
			return;
		}
		if( is_fp_mode(mode) )
			emit_mov_fpr(ctx, fpr_id(dst), fpr_id(src), mode);
		else if( mode == M_UI8 || mode == M_UI16 )
			emit_zero_extend(ctx, gpr_id(dst), gpr_id(src), mode);
		else
			emit_mov_gpr(ctx, gpr_id(dst), gpr_id(src), sf_for(mode));
		return;
	}
	if( dst_kind == R_REG && src_kind == R_REG_PTR ) {
		Arm64Reg base = gpr_id(src);
		int offs = REG_VALUE(src);
		int reg_t = is_fp_mode(mode) ? fpr_id(dst) : gpr_id(dst);
		emit_ld_st(ctx, /*is_load=*/true, mode, reg_t, base, offs);
		return;
	}
	if( dst_kind == R_REG_PTR && src_kind == R_REG ) {
		Arm64Reg base = gpr_id(dst);
		int offs = REG_VALUE(dst);
		int reg_t;
		if( is_fp_mode(mode) )
			reg_t = fpr_id(src);
		else if( REG_VALUE(src) != 0 ) {
			reg_t = base == ARM_TMP1 ? ARM_TMP2 : ARM_TMP1;
			emit_lea_imm(ctx, reg_t, gpr_id(src), REG_VALUE(src));
		} else
			reg_t = gpr_id(src);
		emit_ld_st(ctx, /*is_load=*/false, mode, reg_t, base, offs);
		return;
	}
	if( src_kind == R_CONST ) {
		emit_load_const(ctx, dst, (uint64_t)REG_VALUE(src), mode);
		return;
	}
	if( dst_kind == R_REG_PTR && src_kind == R_REG_PTR ) {
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

static void emit_load_const( code_ctx *ctx, ereg out, uint64_t value, emit_mode mode ) {
	if( REG_KIND(out) != R_REG ) {
		// store the bit pattern through ARM_TMP1, floats included
		emit_mode store_mode = is_fp_mode(mode) ? (mode == M_F32 ? M_I32 : M_PTR) : mode;
		load_immediate(ctx, (int64_t)value, ARM_TMP1, sf_for(store_mode) == 1);
		Arm64Reg base = gpr_id(out);
		int offs = REG_VALUE(out);
		emit_ld_st(ctx, /*is_load=*/false, store_mode, ARM_TMP1, base, offs);
		return;
	}
	if( is_fp_mode(mode) ) {
		// F32 constants are stored in the low 32 bits: load them with a 32-bit LDR
		Arm64FpReg fp_dst = fpr_id(out);
		int adrp_pos = byte_count(ctx->code);
		int size = (mode == M_F32) ? 2 : 3;
		encode_adrp(ctx, 0, 0, ARM_TMP1);                              // ADRP X16, page
		// LDR Sd|Dd, [X16, #lo12], imm12 patched later
		encode_ldr_str_imm(ctx, size, 1, 1, 0, ARM_TMP1, (Arm64Reg)fp_dst);
		alloc_const(ctx, value, adrp_pos);
		return;
	}
	load_immediate(ctx, (int64_t)value, gpr_id(out), sf_for(mode) == 1);
}

// PUSH FP saves FP+LR as a pair, taking the place of x86 RIP+RBP.
// other PUSH/POP move SP by 16 to keep it aligned
static void emit_push( code_ctx *ctx, ereg r, emit_mode mode ) {
	if( is_fp_mode(mode) ) {
		Arm64FpReg src = (REG_KIND(r) == R_REG) ? fpr_id(r) : materialize_fpr(ctx, r, mode, (Arm64FpReg)31);
		emit_sp_offs(ctx, -16);
		encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/1, /*opc=*/0 /*STR*/, 0, SP_REG, (Arm64Reg)src);
		return;
	}
	// materialize_gpr keeps the MK_STACK_OFFS offset, gpr_id would drop it
	Arm64Reg src = materialize_gpr(ctx, r, mode, ARM_TMP1);
	if( src == FP && REG_KIND(r) == R_REG && REG_VALUE(r) == 0 ) {
		// STP X29, X30, [SP, #-16]!
		encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x03, /*imm7=*/-2 & 0x7F, LR, SP_REG, FP);
		return;
	}
	emit_sp_offs(ctx, -16);
	encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/0, /*opc=*/0, 0, SP_REG, src);
}

static void emit_pop( code_ctx *ctx, ereg r, emit_mode mode ) {
	if( REG_KIND(r) != R_REG ) {
		ereg tmp = is_fp_mode(mode) ? V(V29) : R(ARM_TMP2);
		emit_pop(ctx, tmp, mode);
		emit_mov(ctx, r, tmp, mode);
		return;
	}
	if( is_fp_mode(mode) ) {
		encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/1, /*opc=*/1 /*LDR*/, 0, SP_REG, (Arm64Reg)fpr_id(r));
		emit_sp_offs(ctx, 16);
		return;
	}
	Arm64Reg dst = gpr_id(r);
	if( dst == FP ) {
		// LDP X29, X30, [SP], #16
		encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x01, /*imm7=*/2, LR, SP_REG, FP);
		return;
	}
	encode_ldr_str_imm(ctx, /*size=*/3, /*V=*/0, /*opc=*/1, 0, SP_REG, dst);
	emit_sp_offs(ctx, 16);
}

// e->size_offs holds the OJxxx opcode, read back by the following JCOND/CMOV
static void emit_cmp( code_ctx *ctx, einstr *e ) {
	if( is_fp_mode(e->mode) ) {
		Arm64FpReg ra = materialize_fpr(ctx, e->a, e->mode, (Arm64FpReg)29);
		Arm64FpReg rb = materialize_fpr(ctx, e->b, e->mode, (Arm64FpReg)30);
		int type = (e->mode == M_F64) ? 1 : 0;
		encode_fp_compare(ctx, /*M=*/0, /*S=*/0, type, rb, /*op=*/0, ra);
		return;
	}
	int sf = sf_for(e->mode);
	Arm64Reg a = materialize_gpr(ctx, e->a, e->mode, ARM_TMP1);
	if( REG_KIND(e->b) == R_CONST ) {
		int64_t val = (int64_t)REG_VALUE(e->b);
		if( val >= 0 && val <= 0xFFF ) {
			// CMP Xa, #imm
			encode_add_sub_imm(ctx, sf, 1, 1, 0, (int)val, a, XZR);
			return;
		}
		if( val < 0 && -val <= 0xFFF ) {
			// CMN Xa, #imm
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
	Arm64Reg a = materialize_gpr(ctx, e->a, e->mode, ARM_TMP1);
	// TST Xa, Xa
	encode_logical_reg(ctx, sf, 0x03, 0, 0, a, 0, a, XZR);
}

static void add_branch_fixup( code_ctx *ctx, int code_pos, int target_op, int is_cond ) {
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, code_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, target_op);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->branch_fixups, is_cond);
}

static void add_addr_fixup( code_ctx *ctx, int adrp_pos, int target_op ) {
	int_arr_add_impl(&ctx->jit->galloc, &ctx->addr_fixups, adrp_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->addr_fixups, target_op);
}

static void emit_jump( code_ctx *ctx, int target_op_offset ) {
	int target = ctx->cur_op + 1 + target_op_offset;
	int pos = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0);
	add_branch_fixup(ctx, pos, target, 0);
}

static void emit_jump_cond( code_ctx *ctx, ArmCondition cond, int target_op_offset ) {
	int target = ctx->cur_op + 1 + target_op_offset;
	if( ctx->long_cond ) {
		encode_branch_cond(ctx, 2, (ArmCondition)(cond ^ 1)); // skip the B
		int pos = byte_count(ctx->code);
		encode_branch_uncond(ctx, 0);
		add_branch_fixup(ctx, pos, target, 0);
		return;
	}
	int pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, cond);
	add_branch_fixup(ctx, pos, target, 1);
}

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
	// FCMP with a NaN sets NZCV=0011, so GE, GT, LO and LS are false
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
		// true for NaN
		return COND_HS;
	case OJNotGte:
		// true for NaN
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
		// imm19 at [23:5]
		if( imm < -(1 << 18) || imm >= (1 << 18) ) {
			ctx->cond_overflow = true;
			return;
		}
		*insn = (*insn & ~(0x7FFFF << 5)) | ((imm & 0x7FFFF) << 5);
	} else {
		// imm26 at [25:0]
		if( imm < -(1 << 25) || imm >= (1 << 25) )
			jit_error("aarch64 B out of range");
		*insn = (*insn & ~0x03FFFFFF) | (imm & 0x03FFFFFF);
	}
}

// returns the register holding src, loading it into tmp if needed
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
		int adrp_pos = byte_count(ctx->code);
		encode_adrp(ctx, 0, 0, ARM_TMP1);
		encode_ldr_str_imm(ctx, 3, 1, 1, 0, ARM_TMP1, (Arm64Reg)tmp);
		alloc_const(ctx, (uint64_t)REG_VALUE(src), adrp_pos);
		return tmp;
	}
	jit_error("aarch64 materialize_fpr: unsupported operand kind");
	return (Arm64FpReg)0;
}

static void emit_bitfield( code_ctx *ctx, int sf, int opc, int immr, int imms, Arm64Reg Rn, Arm64Reg Rd ) {
	// [31]=sf, [30:29]=opc (00=SBFM, 01=BFM, 10=UBFM), [28:23]=100110, [22]=N(=sf),
	// [21:16]=immr, [15:10]=imms, [9:5]=Rn, [4:0]=Rd
	unsigned int insn = ((unsigned)sf << 31) | ((unsigned)opc << 29) | (0x26u << 23) |
	                    ((unsigned)sf << 22) | ((immr & 0x3F) << 16) | ((imms & 0x3F) << 10) |
	                    ((Rn & 0x1F) << 5) | (Rd & 0x1F);
	EMIT32(ctx, insn);
}

static void emit_sxt_to_ptr( code_ctx *ctx, emit_mode in_mode, Arm64Reg Rn, Arm64Reg Rd ) {
	// SXTB / SXTH / SXTW
	switch( in_mode ) {
	case M_UI8:  emit_bitfield(ctx, 1, 0x00, 0, 7, Rn, Rd); break;
	case M_UI16: emit_bitfield(ctx, 1, 0x00, 0, 15, Rn, Rd); break;
	case M_I32:  emit_bitfield(ctx, 1, 0x00, 0, 31, Rn, Rd); break;
	default: jit_error("emit_sxt_to_ptr unsupported in_mode");
	}
}

static void emit_uxt_to_w( code_ctx *ctx, emit_mode in_mode, Arm64Reg Rn, Arm64Reg Rd ) {
	switch( in_mode ) {
	case M_UI8:  encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, Rn, Rd); break;   // AND Wd, Wn, #0xFF
	case M_UI16: encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, Rn, Rd); break;  // AND Wd, Wn, #0xFFFF
	default: jit_error("emit_uxt_to_w unsupported in_mode");
	}
}

static void emit_div_mod( code_ctx *ctx, hl_op op, Arm64Reg out, Arm64Reg a, Arm64Reg b, int sf );
static void patch_helper_branch( code_ctx *ctx, int pos, int target );

static void emit_binop_int( code_ctx *ctx, hl_op op, ereg out_e, ereg a_e, ereg b_e, emit_mode mode ) {
	int sf = sf_for(mode);
	Arm64Reg out = (REG_KIND(out_e) == R_REG) ? gpr_id(out_e) : ARM_TMP1;
	Arm64Reg a = materialize_gpr(ctx, a_e, mode, ARM_TMP1);

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

// Integer divide / modulo with Haxe semantics:
//   OUDiv:  b == 0       => 0
//   OUMod:  b == 0       => 0
//   OSDiv:  b == 0 || -1 => a*b   (avoids INT_MIN / -1)
//   OSMod:  b == 0 || -1 => 0
static void emit_div_mod( code_ctx *ctx, hl_op op, Arm64Reg out, Arm64Reg a, Arm64Reg b, int sf ) {
	bool unsign = (op == OUDiv || op == OUMod);
	bool is_div = (op == OSDiv || op == OUDiv);

	encode_logical_reg(ctx, sf, 0x03, 0, 0, b, 0, b, XZR);  // TST b, b
	int jz_pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);  // patched later

	int jneg_pos = -1;
	if( !unsign ) {
		// CMN b, #1
		encode_add_sub_imm(ctx, sf, 0, 1, 0, 1, b, XZR);
		jneg_pos = byte_count(ctx->code);
		encode_branch_cond(ctx, 0, COND_EQ);
	}

	// encode_div : 0 = UDIV, 1 = SDIV
	if( is_div ) {
		encode_div(ctx, sf, unsign ? 0 : 1, b, a, out);
	} else {
		// MSUB needs a and b after the divide has written out
		Arm64Reg a_safe = a, b_safe = b;
		if( out == a ) {
			emit_mov_gpr(ctx, ARM_TMP3, a, sf);
			a_safe = ARM_TMP3;
			if( b == a ) b_safe = ARM_TMP3;
		}
		if( out == b && b_safe == b ) {
			Arm64Reg t = (a_safe == ARM_TMP1) ? ARM_TMP2 : ARM_TMP1;
			emit_mov_gpr(ctx, t, b, sf);
			b_safe = t;
		}
		encode_div(ctx, sf, unsign ? 0 : 1, b_safe, a_safe, out);
		// out = a - out * b
		encode_madd_msub(ctx, sf, 1, b_safe, a_safe, out, out);
	}
	int jdone_pos = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0);

	int special_pos = byte_count(ctx->code);
	if( op == OSDiv ) {
		// out = a * b
		encode_madd_msub(ctx, sf, 0, b, XZR, a, out);
	} else {
		encode_logical_reg(ctx, sf, 0x01, 0, 0, XZR, 0, XZR, out);  // ORR out, XZR, XZR
	}

	int after = byte_count(ctx->code);

	patch_helper_branch(ctx, jz_pos, special_pos);
	if( jneg_pos >= 0 ) patch_helper_branch(ctx, jneg_pos, special_pos);
	patch_helper_branch(ctx, jdone_pos, after);
}

static void emit_binop_fp( code_ctx *ctx, hl_op op, ereg out_e, ereg a_e, ereg b_e, emit_mode mode ) {
	bool out_to_mem = (REG_KIND(out_e) != R_REG);
	Arm64FpReg out = out_to_mem ? (Arm64FpReg)31 : fpr_id(out_e);
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
		// NEG
		encode_add_sub_reg(ctx, sf, 1, 0, 0, a, 0, XZR, out);
		break;
	case ONot:
		// EOR out, a, #1 (N must equal sf)
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

// e->size_offs is the input mode
static void emit_conv( code_ctx *ctx, einstr *e, ereg out_e, bool unsign ) {
	emit_mode out_mode = e->mode;
	emit_mode in_mode = (emit_mode)e->size_offs;
	bool out_fp = is_fp_mode(out_mode);
	bool in_fp = is_fp_mode(in_mode);

	Arm64Reg a_gpr = 0;
	Arm64FpReg a_fpr = (Arm64FpReg)0;
	if( in_fp ) {
		a_fpr = materialize_fpr(ctx, e->a, in_mode, (Arm64FpReg)29);
	} else {
		a_gpr = materialize_gpr(ctx, e->a, in_mode, ARM_TMP1);
	}

	bool out_to_mem = REG_KIND(out_e) != R_REG;
	Arm64Reg dst_gpr = (!out_fp && !out_to_mem) ? gpr_id(out_e)
	                  : (!out_fp ? ARM_TMP2 : 0);
	Arm64FpReg dst_fpr = (out_fp && !out_to_mem) ? fpr_id(out_e)
	                    : (out_fp ? (Arm64FpReg)31 : (Arm64FpReg)0);

	if( in_fp && out_fp ) {
		// FCVT
		int type = (in_mode == M_F64) ? 1 : 0;
		int opcode = (in_mode == M_F32) ? 0x05 : 0x04; // F32->F64 = 0x05, F64->F32 = 0x04
		encode_fp_1src(ctx, 0, 0, type, opcode, a_fpr, dst_fpr);
	} else if( in_fp && !out_fp ) {
		// FCVTZS / FCVTZU
		int sf = sf_for(out_mode);
		int type = (in_mode == M_F64) ? 1 : 0;
		int rmode = 3;       // round toward zero
		int opc = unsign ? 1 : 0;  // 0=FCVTZS, 1=FCVTZU
		encode_fcvt_int(ctx, sf, 0, type, rmode, opc, a_fpr, dst_gpr);
	} else if( !in_fp && out_fp ) {
		// SCVTF / UCVTF
		int sf = sf_for(in_mode);
		int type = (out_mode == M_F64) ? 1 : 0;
		int rmode = 0;
		int opc = unsign ? 3 : 2;  // 2=SCVTF, 3=UCVTF
		// UI8/UI16 are always unsigned, whatever the unsign flag
		Arm64Reg src = a_gpr;
		if( in_mode == M_UI8 || in_mode == M_UI16 ) {
			emit_uxt_to_w(ctx, in_mode, src, ARM_TMP1);
			src = ARM_TMP1;
		}
		encode_int_fcvt(ctx, sf, 0, type, rmode, opc, src, dst_fpr);
	} else {
		switch( in_mode ) {
		case M_UI8:
		case M_UI16:
			// UI8/UI16 are always unsigned
			if( out_mode == M_PTR || out_mode == M_I32 ) {
				emit_uxt_to_w(ctx, in_mode, a_gpr, dst_gpr);
			} else if( out_mode == M_UI16 || out_mode == M_UI8 ) {
				emit_uxt_to_w(ctx, out_mode, a_gpr, dst_gpr);
			}
			break;
		case M_I32:
			if( out_mode == M_PTR ) {
				if( unsign ) emit_mov_gpr(ctx, dst_gpr, a_gpr, 0); // MOV Wd, Wn zero-extends
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
			Arm64Reg base = gpr_id(out_e);
			int offs = REG_VALUE(out_e);
			emit_ld_st(ctx, false, out_mode, dst_fpr, base, offs);
		} else {
			emit_mov(ctx, out_e, R(ARM_TMP2), out_mode);
		}
	}
}

static void emit_store( code_ctx *ctx, einstr *e ) {
	int offs = e->size_offs;
	Arm64Reg base;
	if( REG_KIND(e->a) == R_REG ) {
		base = gpr_id(e->a);
		// MK_STACK_OFFS carries its offset in REG_VALUE
		offs += REG_VALUE(e->a);
	} else {
		emit_mov(ctx, R(ARM_TMP1), e->a, M_PTR);
		base = ARM_TMP1;
	}
	if( is_fp_mode(e->mode) ) {
		if( REG_KIND(e->b) == R_REG ) {
			emit_ld_st(ctx, false, e->mode, fpr_id(e->b), base, offs);
		} else {
			// store the bit pattern through a GPR
			Arm64Reg tmp = (base == ARM_TMP1) ? ARM_TMP2 : ARM_TMP1;
			emit_mode int_mode = (e->mode == M_F32) ? M_I32 : M_PTR;
			if( REG_KIND(e->b) == R_CONST ) {
				load_immediate(ctx, (int64_t)REG_VALUE(e->b), tmp, sf_for(int_mode) == 1);
			} else if( REG_KIND(e->b) == R_REG_PTR ) {
				// base may be in ARM_TMP1
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
			// MK_STACK_OFFS or LEA-rewritten ADDRESS
			emit_lea_imm(ctx, tmp, gpr_id(e->b), REG_VALUE(e->b));
		} else if( REG_KIND(e->b) == R_CONST ) {
			load_immediate(ctx, (int64_t)REG_VALUE(e->b), tmp, sf_for(e->mode) == 1);
		} else if( REG_KIND(e->b) == R_REG_PTR ) {
			// base may be in ARM_TMP1
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
		emit_lea_imm(ctx, out, a, offs);
	} else {
		if( mult != 1 && mult != 2 && mult != 4 && mult != 8 )
			jit_error("aarch64 LEA: unsupported scale");
		int shift = (mult == 1) ? 0 : (mult == 2) ? 1 : (mult == 4) ? 2 : 3;
		// the index is 32-bit: only its low 32 bits may feed the address
		Arm64Reg b = materialize_gpr_ex(ctx, e->b, M_I32, ARM_TMP2, a);
		// out = a + UXTW(b) << shift
		encode_add_sub_ext(ctx, /*sf=*/1, /*op=*/0, /*S=*/0, b, /*option=UXTW*/2, shift, a, out);
		if( offs != 0 ) emit_lea_imm(ctx, out, out, offs);
	}

	if( REG_KIND(out_e) != R_REG ) emit_mov(ctx, out_e, R(ARM_TMP1), M_PTR);
}

static void emit_cmov_arm( code_ctx *ctx, ereg out_e, ereg a_e, emit_mode mode, ArmCondition cond ) {
	if( REG_KIND(out_e) != R_REG ) jit_error("aarch64 CMOV non-reg out");
	if( is_fp_mode(mode) ) {
		Arm64FpReg out = fpr_id(out_e);
		Arm64FpReg a = materialize_fpr(ctx, a_e, mode, V29);
		encode_fp_cond_select(ctx, mode == M_F64, out, cond, a, out);
		return;
	}
	// load a sub-word source (e.g. a bool) at its own width, not adjacent stack bytes
	int sf = (hl_emit_mode_sizes[mode] == 8) ? 1 : 0;
	Arm64Reg out = gpr_id(out_e);
	Arm64Reg a = materialize_gpr(ctx, a_e, mode, ARM_TMP1);
	// CSEL out, a, out, cond
	encode_cond_select(ctx, sf, 0, out, cond, 0, a, out);
}

// X17 rather than X16 : a stack slot access with a large offset uses X16
static void emit_xchg( code_ctx *ctx, einstr *e ) {
	if( REG_KIND(e->a) != R_REG && REG_KIND(e->b) != R_REG )
		jit_error("aarch64 XCHG with two memory operands");
	ereg tmp = is_fp_mode(e->mode) ? V(V29) : R(ARM_TMP2);
	emit_mov(ctx, tmp, e->a, e->mode);
	emit_mov(ctx, e->a, e->b, e->mode);
	emit_mov(ctx, e->b, tmp, e->mode);
}

static void emit_cxchg( code_ctx *ctx, einstr *e ) {
	if( REG_KIND(e->a) != R_REG || REG_KIND(e->b) != R_REG )
		jit_error("aarch64 CXCHG with non-reg operand");
	ArmCondition cond = get_cond_jump(ctx);
	if( is_fp_mode(e->mode) ) {
		Arm64FpReg ra = fpr_id(e->a);
		Arm64FpReg rb = fpr_id(e->b);
		emit_mov_fpr(ctx, V29, ra, e->mode);
		encode_fp_cond_select(ctx, e->mode == M_F64, ra, cond, rb, ra);
		encode_fp_cond_select(ctx, e->mode == M_F64, rb, cond, V29, rb);
		return;
	}
	Arm64Reg ra = gpr_id(e->a);
	Arm64Reg rb = gpr_id(e->b);
	emit_mov_gpr(ctx, ARM_TMP1, ra, 1);
	encode_cond_select(ctx, 1, 0, ra, cond, 0, rb, ra);
	encode_cond_select(ctx, 1, 0, rb, cond, 0, ARM_TMP1, rb);
}

static void emit_push_const( code_ctx *ctx, einstr *e ) {
	if( e->mode != M_PTR ) jit_error("aarch64 PUSH_CONST non-ptr mode");
	load_immediate(ctx, (int64_t)e->value, ARM_TMP1, true);
	emit_sp_offs(ctx, -16);
	encode_ldr_str_imm(ctx, 3, 0, 0, 0, SP_REG, ARM_TMP1);  // STR X16, [SP]
}

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

// returns the offset of value in const_table; the ADRP at adrp_pos is patched later
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

static void emit_const_load( code_ctx *ctx, Arm64Reg dst, uint64_t value ) {
	int adrp_pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, dst);
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, dst, dst); // LDR Xd, [Xd, #0]
	alloc_const(ctx, value, adrp_pos);
}

// address of an offset inside the const table (jump tables)
static void emit_pool_offset_addr( code_ctx *ctx, Arm64Reg dst, int const_offset ) {
	int adrp_pos = byte_count(ctx->code);
	encode_adrp(ctx, 0, 0, dst);
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 0, dst, dst);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, ctx->jit->out_pos + adrp_pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->const_refs, const_offset);
}

static void emit_call_fun( code_ctx *ctx, einstr *e ) {
	int pos = byte_count(ctx->code);
	encode_branch_link(ctx, 0); // imm26 patched in flush_consts
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, ctx->jit->out_pos + pos);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, (int)e->a);
	int_arr_add_impl(&ctx->jit->galloc, &ctx->funs, /*kind=BL*/0);
}

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

// null access functions are called through their stubs, which are in BL range
static void emit_call_ptr( code_ctx *ctx, einstr *e ) {
	uint64_t target = (uint64_t)e->value;
	int near_pos = -1;
	if( target == (uint64_t)(uintptr_t)hl_null_access )
		near_pos = ctx->null_access_pos;
	else if( target == (uint64_t)(uintptr_t)hl_jit_null_field_access )
		near_pos = ctx->null_field_pos;

	if( near_pos >= 0 ) {
		int pos = ctx->jit->out_pos + byte_count(ctx->code);
		intptr_t delta = (intptr_t)near_pos - (intptr_t)pos;
		int imm26 = (int)(delta >> 2);
		encode_branch_link(ctx, imm26);
	} else if( ctx->jit->mod->debug && ctx->jit->code_funs.trampoline
		&& e->size_offs <= TRAMPOLINE_STACK_ARGS && hl_jit_is_callback((void*)target) ) {
		// lets the debugger step into native callbacks
		emit_const_load(ctx, ARM_TMP1, target);
		int pos = ctx->jit->out_pos + byte_count(ctx->code);
		intptr_t delta = (intptr_t)ctx->jit->code_funs.trampoline - (intptr_t)pos;
		if( delta & 3 ) jit_error("aarch64 debug trampoline misaligned");
		encode_branch_link(ctx, (int)(delta >> 2));
	} else {
		emit_const_load(ctx, ARM_TMP1, target);
		encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	}
	// zero-extend sub-word return values
	if( e->mode == M_UI8 )
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 7, X0, X0);
	else if( e->mode == M_UI16 )
		encode_logical_imm(ctx, 0, 0x00, 0, 0, 15, X0, X0);
}

static void emit_call_reg( code_ctx *ctx, einstr *e ) {
	Arm64Reg target = materialize_gpr(ctx, e->a, M_PTR, ARM_TMP1);
	encode_branch_reg(ctx, /*BLR*/1, target);
}

// table entries are absolute addresses, filled in hl_codegen_final
static void emit_jump_table( code_ctx *ctx, einstr *e ) {
	int n = e->nargs;
	int start = reserve_const_segment(ctx, 8 * n, 16);

	Arm64Reg idx;
	if( REG_KIND(e->a) == R_REG ) {
		Arm64Reg src = gpr_id(e->a);
		// MOV W17, Wsrc zero-extends
		encode_logical_reg(ctx, 0, 0x01, 0, 0, src, 0, XZR, ARM_TMP2);
		idx = ARM_TMP2;
	} else {
		emit_mov(ctx, R(ARM_TMP2), e->a, M_I32);
		encode_logical_reg(ctx, 0, 0x01, 0, 0, ARM_TMP2, 0, XZR, ARM_TMP2);
		idx = ARM_TMP2;
	}

	emit_pool_offset_addr(ctx, ARM_TMP1, start);
	// LDR X16, [X16, idx, LSL #3]
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
	// PRFM
	encode_ldr_str_imm(ctx, 3, 0, 2, 0, base, (Arm64Reg)prfop);
}

void hl_codegen_flush( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	if( ctx->flushed ) return;
	ctx->flushed = true;
	jit->code_size = ctx->code.cur;
	jit->code_instrs = ctx->code.values;
	jit->code_pos_map = ctx->pos_map;
	if( ctx->pos_map ) ctx->pos_map[ctx->cur_op + 1] = ctx->code.cur;
}

void hl_codegen_function( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	int funs_prev = int_arr_count(ctx->funs);
	int const_refs_prev = int_arr_count(ctx->const_refs);
	int const_addr_prev = int_arr_count(ctx->const_addr);
	ctx->long_cond = false;
retry:
	ctx->const_addr.cur = const_addr_prev;
	ctx->cond_overflow = false;
	ctx->funs.cur = funs_prev;
	ctx->const_refs.cur = const_refs_prev;
	ctx->flushed = false;
	byte_free(&ctx->code);
	int_arr_free(&ctx->branch_fixups);
	int_arr_free(&ctx->addr_fixups);
	free(ctx->pos_map);
	ctx->pos_map = (int*)malloc((jit->reg_instr_count + 1) * sizeof(int));
	ctx->pos_map[0] = 0;
	byte_reserve(ctx->code, MAX_OP_SIZE);
	ctx->code.cur -= MAX_OP_SIZE;

	for( int cur_pos = 0; cur_pos < jit->reg_instr_count; cur_pos++ ) {
		einstr *e = jit->reg_instrs + cur_pos;
		ereg out = jit->reg_writes[cur_pos];
		byte_reserve(ctx->code, MAX_OP_SIZE);
		ctx->code.cur -= MAX_OP_SIZE;
		ctx->cur_op = cur_pos;
		if( cur_pos > 0 ) ctx->pos_map[cur_pos] = ctx->code.cur;

		switch( e->op ) {
		case LOAD_ARG:
			if( (e->mode == M_UI8 || e->mode == M_UI16) && REG_KIND(out) == R_REG )
				emit_zero_extend(ctx, gpr_id(out), gpr_id(out), e->mode);
			else
				continue;
			break;
		case NOP:
			// NOP
			EMIT32(ctx, 0xD503201F);
			break;
		case MOV:
			emit_mov(ctx, out, e->a, e->mode);
			break;
		case LOAD_CONST:
			emit_load_const(ctx, out, e->value, e->mode);
			break;
		case RET:
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
			// BRK #0
			EMIT32(ctx, 0xD4200000);
			break;
		case BINOP:
			if( is_fp_mode(e->mode) )
				emit_binop_fp(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
			else
				emit_binop_int(ctx, (hl_op)e->size_offs, out, e->a, e->b, e->mode);
			break;
		case UNOP:
			// boolean not is emitted as a two-operand UNOP (OXor with a constant)
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
			emit_cmov_arm(ctx, out, e->a, e->mode, get_cond_jump(ctx));
			break;
		case XCHG:
			emit_xchg(ctx, e);
			break;
		case CXCHG:
			emit_cxchg(ctx, e);
			break;
		case PUSH_CONST:
			emit_push_const(ctx, e);
			break;
		case PUSH_ADDR:
			{
				int target_op = ctx->cur_op + 1 + e->size_offs;
				int adrp_pos = byte_count(ctx->code);
				encode_adrp(ctx, 0, 0, ARM_TMP1);
				encode_add_sub_imm(ctx, 1, 0, 0, 0, 0, ARM_TMP1, ARM_TMP1);
				add_addr_fixup(ctx, adrp_pos, target_op);
				emit_push(ctx, R(ARM_TMP1), M_PTR);
			}
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
			jit_error("aarch64: ADDRESS reached backend (regs phase should rewrite)");
			break;
		case CATCH:
			break;
		default:
			jit_assert();
			break;
		}

		if( ctx->code.cur > ctx->code.max ) jit_error("aarch64 code buffer overrun");
	}

	hl_codegen_flush(jit);

	for( int i = 0; i < int_arr_count(ctx->branch_fixups); i += 3 ) {
		int pos = int_arr_get(ctx->branch_fixups, i);
		int target_op = int_arr_get(ctx->branch_fixups, i + 1);
		int is_cond = int_arr_get(ctx->branch_fixups, i + 2);
		int target_byte_pos = ctx->pos_map[target_op];
		patch_branch(ctx, pos, target_byte_pos, is_cond);
	}
	if( ctx->cond_overflow ) {
		if( ctx->long_cond ) jit_error("aarch64 branch out of range");
		ctx->long_cond = true;
		goto retry;
	}

	// PUSH_ADDR labels. ADRP is page-relative so this must use output offsets, not function offsets
	for( int i = 0; i < int_arr_count(ctx->addr_fixups); i += 2 ) {
		int adrp_pos = int_arr_get(ctx->addr_fixups, i);
		int target_op = int_arr_get(ctx->addr_fixups, i + 1);
		int target_off = ctx->pos_map[target_op];
		int tgt_out = jit->out_pos + target_off;
		patch_adrp_imm21(ctx->code.values + adrp_pos, jit->out_pos + adrp_pos, tgt_out);
		patch_imm12(ctx->code.values, adrp_pos + 4, tgt_out & 0xFFF, /*scale=*/1);
	}

	// jump table entries : op index to output offset
	for( int i = const_addr_prev; i < int_arr_count(ctx->const_addr); i += 2 ) {
		int target_op = int_arr_get(ctx->const_addr, i + 1);
		int offs = jit->out_pos + ctx->pos_map[target_op];
		ctx->const_addr.values[i + 1] = offs;
	}
}

// same as flush_function in jit_x86_64.c
static void flush_helper( code_ctx *ctx, int start ) {
	hl_jit_define_function(ctx->jit, start, ctx->jit->out_pos + byte_count(ctx->code) - start);
	while( byte_count(ctx->code) & 15 )
		EMIT32(ctx, 0xD503201F); // NOP
	if( byte_count(ctx->code) > ctx->code.max ) jit_error("aarch64 trampoline overrun");
}

static void patch_helper_branch( code_ctx *ctx, int pos, int target ) {
	int delta = (target - pos) >> 2;
	unsigned int *insn = (unsigned int*)&ctx->code.values[pos];
	unsigned int op = (*insn >> 26) & 0x3F;
	if( op == 0x05 || op == 0x25 ) {
		// B / BL
		*insn = (*insn & ~0x03FFFFFFu) | ((unsigned)delta & 0x03FFFFFF);
	} else {
		// B.cond
		*insn = (*insn & ~(0x7FFFFu << 5)) | ((unsigned)(delta & 0x7FFFF) << 5);
	}
}

// STP X29, X30, [SP, #-16]! ; MOV X29, SP
static void emit_helper_prologue( code_ctx *ctx ) {
	encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x03, /*imm7=*/-2 & 0x7F, LR, SP_REG, FP);
	emit_mov_gpr(ctx, FP, SP_REG, 1);
}

// MOV SP, X29 ; LDP X29, X30, [SP], #16 ; RET
static void emit_helper_epilogue( code_ctx *ctx ) {
	emit_mov_gpr(ctx, SP_REG, FP, 1);
	encode_ldp_stp(ctx, /*opc=*/2, /*V=*/0, /*mode=*/0x01, /*imm7=*/2, LR, SP_REG, FP);
	encode_branch_reg(ctx, /*RET*/2, LR);
}

// field : the field hash is pushed before the call site (PUSH_CONST, PUSH_ADDR)
static void emit_null_stub( code_ctx *ctx, void *target, bool field ) {
	emit_helper_prologue(ctx);
	if( field ) encode_ldr_str_imm(ctx, 2, 0, 1, 32 / 4, FP, X0); // LDR W0, [FP, #32]
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)target);
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	EMIT32(ctx, 0xD4200000); // BRK #0
}

// X0 = function, X1 = vargs (see callback_c2hl), X2 = stack args count
static void emit_c2hl_trampoline( code_ctx *ctx ) {
	emit_helper_prologue(ctx);
	emit_mov_gpr(ctx, ARM_TMP1, X0, 1);  // X16 = fn
	emit_mov_gpr(ctx, ARM_TMP2, X1, 1);  // X17 = vargs
	emit_mov_gpr(ctx, X9, X2, 1);        // X9  = stack count

	encode_ldp_stp(ctx, 0x02, 0, 0x02, 0, X1, ARM_TMP2, X0); // LDP X0,X1, [X17, #0]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 2, X3, ARM_TMP2, X2); // LDP X2,X3, [X17, #16]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 4, X5, ARM_TMP2, X4); // LDP X4,X5, [X17, #32]
	encode_ldp_stp(ctx, 0x02, 0, 0x02, 6, X7, ARM_TMP2, X6); // LDP X6,X7, [X17, #48]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 8,  (Arm64Reg)1, ARM_TMP2, (Arm64Reg)0);  // LDP D0,D1, [X17, #64]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 10, (Arm64Reg)3, ARM_TMP2, (Arm64Reg)2);  // LDP D2,D3, [X17, #80]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 12, (Arm64Reg)5, ARM_TMP2, (Arm64Reg)4);  // LDP D4,D5, [X17, #96]
	encode_ldp_stp(ctx, 0x01, 1, 0x02, 14, (Arm64Reg)7, ARM_TMP2, (Arm64Reg)6);  // LDP D6,D7, [X17, #112]

	// copy vargs.stack[0..X9] to 16-byte stack slots, matching min_stack_args_size
	int cbz_skip_pos = byte_count(ctx->code);
	encode_cbz_cbnz(ctx, /*sf=*/1, /*op=*/0, 0, X9);

	// X10 = X9 << 4
	emit_bitfield(ctx, /*sf=*/1, /*opc=UBFM*/0x02, /*immr=*/(64 - 4) & 0x3F, /*imms=*/63 - 4, X9, X10);

	// SUB SP, SP, X10 (extended-register form, see emit_sp_offs)
	encode_add_sub_ext(ctx, 1, 1, 0, X10, /*UXTX*/3, 0, SP_REG, SP_REG);

	encode_add_sub_imm(ctx, 1, 0, 0, 0, MAX_ARGS * HL_WSIZE, ARM_TMP2, X12);  // X12 = &vargs.stack[0]
	emit_mov_gpr(ctx, X13, SP_REG, 1);
	emit_mov_gpr(ctx, X14, X9, 1);

	int loop_top = byte_count(ctx->code);
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, X12, X15);                      // LDR X15, [X12, #0]
	encode_ldr_str_imm(ctx, 3, 0, 0, 0, X13, X15);                      // STR X15, [X13, #0]
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 8, X12, X12);                   // ADD X12, X12, #8
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, X13, X13);                  // ADD X13, X13, #16
	encode_add_sub_imm(ctx, 1, 1, 1, 0, 1, X14, X14);                   // SUBS X14, X14, #1
	int loop_branch_pos = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_NE);                                 // B.NE loop_top
	patch_helper_branch(ctx, loop_branch_pos, loop_top);

	int after_stack = byte_count(ctx->code);
	patch_helper_branch(ctx, cbz_skip_pos, after_stack);

	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);

	emit_helper_epilogue(ctx);
}

// X0 = closure. calls hl_jit_wrapper_ptr, or hl_jit_wrapper_d for a float return,
// with (closure, caller stack args, spilled arg registers)
static void emit_hl2c_trampoline( code_ctx *ctx ) {
	hl_type_fun *ft = NULL;

	emit_helper_prologue(ctx);
	emit_sp_offs(ctx, -128);

	encode_ldp_stp(ctx, 0x02, 0, 0x12, 0, X1, SP_REG, X0); // STP X0,X1, [SP, #0]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 2, X3, SP_REG, X2); // STP X2,X3, [SP, #16]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 4, X5, SP_REG, X4); // STP X4,X5, [SP, #32]
	encode_ldp_stp(ctx, 0x02, 0, 0x12, 6, X7, SP_REG, X6); // STP X6,X7, [SP, #48]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 8,  (Arm64Reg)1, SP_REG, (Arm64Reg)0); // STP D0,D1, [SP, #64]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 10, (Arm64Reg)3, SP_REG, (Arm64Reg)2); // STP D2,D3, [SP, #80]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 12, (Arm64Reg)5, SP_REG, (Arm64Reg)4); // STP D4,D5, [SP, #96]
	encode_ldp_stp(ctx, 0x01, 1, 0x12, 14, (Arm64Reg)7, SP_REG, (Arm64Reg)6); // STP D6,D7, [SP, #112]

	// W9 = cl->t->fun->ret->kind
	emit_mov_gpr(ctx, X9, X0, 1);
	encode_ldr_str_imm(ctx, 3, 0, 1, 0, X9, X9);
	encode_ldr_str_imm(ctx, 3, 0, 1, 1, X9, X9);
	int ret_offset = (int)(int_val)&ft->ret;
	if( (ret_offset & 7) == 0 && (unsigned)ret_offset < 0x8000 )
		encode_ldr_str_imm(ctx, 3, 0, 1, ret_offset / 8, X9, X9);
	else {
		load_immediate(ctx, ret_offset, X10, true);
		encode_ldr_str_reg(ctx, 3, 0, 1, X10, /*option=*/3, /*S=*/0, X9, X9);
	}
	encode_ldr_str_imm(ctx, 2, 0, 1, 0, X9, X9);

	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF64, X9, XZR);   // CMP W9, #HF64
	int jeq_f64 = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);
	encode_add_sub_imm(ctx, 0, 1, 1, 0, HF32, X9, XZR);   // CMP W9, #HF32
	int jeq_f32 = byte_count(ctx->code);
	encode_branch_cond(ctx, 0, COND_EQ);

	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)hl_jit_wrapper_ptr);
	int jdone_default = byte_count(ctx->code);
	encode_branch_uncond(ctx, 0);

	int float_path = byte_count(ctx->code);
	patch_helper_branch(ctx, jeq_f64, float_path);
	patch_helper_branch(ctx, jeq_f32, float_path);
	emit_const_load(ctx, ARM_TMP1, (uint64_t)(uintptr_t)hl_jit_wrapper_d);

	int after_select = byte_count(ctx->code);
	patch_helper_branch(ctx, jdone_default, after_select);

	// X1 = caller stack args, above the saved FP/LR
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, FP, X1);
	emit_mov_gpr(ctx, X2, SP_REG, 1);
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);

	emit_helper_epilogue(ctx);
}

// debug mode : native callbacks go through here so the debugger can step into them
// X16 = native, stack args are copied below our frame, hl_jit_trampoline marks the return point
static void emit_debug_trampoline( code_ctx *ctx ) {
	emit_helper_prologue(ctx);
	emit_sp_offs(ctx, -TRAMPOLINE_STACK_ARGS);
	// X10 = caller stack args, X11 = SP
	encode_add_sub_imm(ctx, 1, 0, 0, 0, 16, FP, X10);
	emit_mov_gpr(ctx, X11, SP_REG, 1);
	for( int off = 0; off < TRAMPOLINE_STACK_ARGS; off += 16 ) {
		encode_ldp_stp(ctx, 0x02, 0, 0x02, off / 8, X12, X10, X9); // LDP X9,X12,[X10,#off]
		encode_ldp_stp(ctx, 0x02, 0, 0x12, off / 8, X12, X11, X9); // STP X9,X12,[X11,#off]
	}
	encode_branch_reg(ctx, /*BLR*/1, ARM_TMP1);
	hl_jit_trampoline = ctx->jit->out_pos + byte_count(ctx->code);
	emit_helper_epilogue(ctx);
}

void hl_codegen_init( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;
	byte_reserve(ctx->code, 4096);
	ctx->code.cur -= 4096;

	ctx->null_access_pos = jit->out_pos + byte_count(ctx->code);
	emit_null_stub(ctx, (void*)hl_null_access, false);
	flush_helper(ctx, ctx->null_access_pos);

	ctx->null_field_pos = jit->out_pos + byte_count(ctx->code);
	emit_null_stub(ctx, (void*)hl_jit_null_field_access, true);
	flush_helper(ctx, ctx->null_field_pos);

	jit->code_funs.c2hl = jit->out_pos + byte_count(ctx->code);
	emit_c2hl_trampoline(ctx);
	flush_helper(ctx, jit->code_funs.c2hl);

	jit->code_funs.hl2c = jit->out_pos + byte_count(ctx->code);
	emit_hl2c_trampoline(ctx);
	flush_helper(ctx, jit->code_funs.hl2c);

	if( jit->mod->debug ) {
		jit->code_funs.trampoline = jit->out_pos + byte_count(ctx->code);
		emit_debug_trampoline(ctx);
		flush_helper(ctx, jit->code_funs.trampoline);
	}

	hl_codegen_flush(jit);
}

// immlo at [30:29], immhi at [23:5]. offsets are in jit->output, which is mapped page aligned
static void patch_adrp_imm21( void *code, int pc_abs, int target_abs ) {
	int imm21 = (target_abs >> 12) - (pc_abs >> 12);
	unsigned int *insn = (unsigned int*)code;
	unsigned int immlo = (unsigned)(imm21 & 0x3);
	unsigned int immhi = (unsigned)((imm21 >> 2) & 0x7FFFF);
	*insn = (*insn & ~((0x3u << 29) | (0x7FFFFu << 5)))
	      | (immlo << 29) | (immhi << 5);
}

// imm12 at [21:10], scale is the access size (1 for ADD)
static void patch_imm12( unsigned char *out, int pos, int target_lo12, int scale ) {
	unsigned int *insn = (unsigned int*)(out + pos);
	unsigned int imm12 = (unsigned)((target_lo12 / scale) & 0xFFF);
	*insn = (*insn & ~(0xFFFu << 10)) | (imm12 << 10);
}

void hl_codegen_flush_consts( jit_ctx *jit ) {
	code_ctx *ctx = jit->code;

	for( int i = 0; i < int_arr_count(ctx->funs); i += 3 ) {
		int pos = int_arr_get(ctx->funs, i);
		int fid = int_arr_get(ctx->funs, i + 1);
		int kind = int_arr_get(ctx->funs, i + 2);
		intptr_t target_offs = (intptr_t)jit->mod->functions_ptrs[fid];
		if( kind == 0 ) {
			intptr_t delta = target_offs - (intptr_t)pos;
			if( delta < -(intptr_t)(1<<27) || delta >= (intptr_t)(1<<27) )
				jit_error("aarch64 BL target out of imm26 range");
			int imm26 = (int)(delta >> 2);
			unsigned int *insn = (unsigned int*)(jit->output + pos);
			*insn = (*insn & ~0x03FFFFFFu) | ((unsigned)imm26 & 0x03FFFFFF);
		} else {
			// ADRP + ADD
			patch_adrp_imm21(jit->output + pos, pos, (int)target_offs);
			int lo12 = (int)target_offs & 0xFFF;
			patch_imm12(jit->output, pos + 4, lo12, /*scale=*/1);
		}
	}
	int_arr_reset(&ctx->funs);

	// LDR imm12 is scaled by 8, the table must be 8-byte aligned
	while( jit->out_pos & 7 ) {
		if( jit->out_pos < jit->out_max ) jit->output[jit->out_pos] = 0;
		jit->out_pos++;
	}

	jit->code_size = byte_count(ctx->const_table);
	jit->code_instrs = ctx->const_table.values;
	ctx->const_table_pos = jit->out_pos;

	for( int i = 0; i < int_arr_count(ctx->const_refs); i += 2 ) {
		int adrp_pos = int_arr_get(ctx->const_refs, i);
		int coffs = int_arr_get(ctx->const_refs, i + 1);
		int target = ctx->const_table_pos + coffs;
		patch_adrp_imm21(jit->output + adrp_pos, adrp_pos, target);
		// the second instruction is a LDR (scaled imm12) or an ADD
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
	// jump table entries
	for( int i = 0; i < int_arr_count(ctx->const_addr); i += 2 ) {
		int table_offs = int_arr_get(ctx->const_addr, i);
		int target_offs = int_arr_get(ctx->const_addr, i + 1);
		*(void**)(jit->final_code + ctx->const_table_pos + table_offs) =
			jit->final_code + target_offs;
	}
	int_arr_free(&ctx->const_addr);
}
