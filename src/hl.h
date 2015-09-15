/*
 * Copyright (C)2015 Haxe Foundation
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
#ifndef HL_H
#define HL_H

#define HL_VERSION	010
#include "opcodes.h"

typedef enum {
	HVOID	= 0,
	HUI8	= 1,
	HI32	= 2,
	HF32	= 3,
	HF64	= 4,
	HBOOL	= 5,
	HANY	= 6,
	HFUN	= 7,
	// ---------
	HLAST	= 8
} hl_type_kind;

typedef struct hl_type hl_type;

struct hl_type {
	hl_type_kind kind;
	int nargs;
	hl_type **args;
	hl_type *ret;
};

typedef struct {
	const char *name;
	int global;
} hl_native;

typedef struct {
	hl_op op;
	int p1;
	int p2;
	int p3;
	void *extra;
} hl_opcode;

typedef struct hl_ptr_list hl_ptr_list;

typedef struct {
	int index;
	int nregs;
	int nops;
	int nallocs;
	hl_type **regs;
	hl_opcode *ops;
	void **allocs;
} hl_function;

typedef struct {
	int version;
	int nints;
	int nfloats;
	int nstrings;
	int ntypes;
	int nglobals;
	int nnatives;
	int nfunctions;
	int entrypoint;
	int*		ints;
	double*		floats;
	char**		strings;
	char*		strings_data;
	int*		strings_lens;
	hl_type*	types;
	hl_type**	globals;
	hl_native*	natives;
	hl_function*functions;
} hl_code;

void hl_global_init();
void hl_global_free();

hl_code *hl_code_read( const unsigned char *data, int size );

#endif