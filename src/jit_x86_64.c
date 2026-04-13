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
#include <hlmodule.h>
#include <jit.h>
#include "data_struct.h"

typedef enum {
	RAX = 0,
	RCX = 1,
	RDX = 2,
	RBX = 3,
	RSP = 4,
	RBP = 5,
	RSI = 6,
	RDI = 7,
#ifdef HL_64
	R8 = 8,
	R9 = 9,
	R10	= 10,
	R11	= 11,
	R12	= 12,
	R13	= 13,
	R14	= 14,
	R15	= 15,
#endif
	_UNUSED = 0xFF
} Reg;

#define R(id,mode)		((mode) | ((id)<<8) | FL_NATREG)
#define X(id)		R(id,M_NONE)
#define MMX(id)		R(id+64,M_F64)

const char *hl_natreg_str( int reg ) {
	static char out[16];
	static const char *regs_str[] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
	emit_mode k = (reg & 0xFF);
	Reg r = ((reg >> 8) & 0xFF);
	switch( k ) {
	case M_I32:
		if( r < 8 )
			sprintf(out,"E%s",regs_str[r]);
		else
			sprintf(out,"R%dD%s",r,r<16?"":"???");
		break;
	case M_UI16:
		if( r < 8 )
			sprintf(out,"%s",regs_str[r]);
		else
			sprintf(out,"R%dW%s",r,r<16?"":"???");
		break;
	case M_UI8:
		if( r < 8 )
			sprintf(out,"%s",regs_str[r]);
		else
			sprintf(out,"R%dB%s",r,r<16?"":"???");
		break;
	case M_F32:
		r -= 64;
		sprintf(out,"XMM%df%s",r,r >= 0 && r < 16 ? "" : "???");
		break;
	case M_F64:
		r -= 64;
		sprintf(out,"XMM%d%s",r,r >= 0 && r < 16 ? "" : "???");
		break;
	default:
		if( r < 8 )
			sprintf(out,"R%s",regs_str[r]);
		else
			sprintf(out,"R%d%s",r,r<16?"":"???");
		break;
	}
	return out;
}

void hl_jit_init_regs( regs_config cfg ) {
#	ifdef HL_WIN_CALL
	static int scratch_regs[] = { X(RAX), X(RCX), X(RDX), X(R8), X(R9), X(R10), X(R11) };
	static int free_regs[] = { X(RSI), X(RDI), X(RBX), X(R12), X(R13), X(R14), X(R15) };
	static int call_regs[] = { X(RCX), X(RDX), X(R8), X(R9) };
	cfg[1].nargs = 4;
	cfg[1].nscratchs = 6;
#	else
	static int scratch_regs[] = { X(RAX), X(RCX), X(RDX), X(RSI), X(RDI), X(R8), X(R9), X(R10), X(R11) };
	static int free_regs[] = { X(RBX), X(R12), X(R13), X(R14), X(R15) };
	static int call_regs[] = { X(RDI), X(RSI), X(RDX), X(RCX), X(R8), X(R9) };
	cfg[1].nargs = 8;
	cfg[1].nscratchs = 16;
#	endif
	static int floats[] = {
		MMX(0), MMX(1), MMX(2), MMX(3), 
		MMX(4), MMX(5), MMX(6), MMX(7), 
		MMX(8), MMX(9), MMX(10), MMX(11), 
		MMX(12), MMX(13), MMX(14), MMX(15)
	};
	cfg[0].ret = scratch_regs[0];
	cfg[0].nscratchs = sizeof(scratch_regs) / sizeof(int);
	cfg[0].npersists = sizeof(free_regs) / sizeof(int);
	cfg[0].nargs = sizeof(call_regs) / sizeof(int);
	cfg[0].scratch = (ereg*)scratch_regs;
	cfg[0].persist = (ereg*)free_regs;
	cfg[0].arg = (ereg*)call_regs;
	cfg[1].ret = floats[0];
	cfg[1].scratch = (ereg*)floats;
	cfg[1].arg = (ereg*)floats;
	cfg[1].persist = (ereg*)floats+cfg[1].nscratchs;
	cfg[1].npersists = 16 - cfg[1].nscratchs;
}
