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

#ifdef _WIN32
#	define HL_WIN
#endif

#if defined(__APPLE__) || defined(__MACH__) || defined(macintosh)
#	define HL_MAC
#endif

#if defined(linux) || defined(__linux__)
#	define HL_LINUX
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define HL_BSD
#endif

#if defined(_64BITS) || defined(__x86_64__)
#	define HL64
#endif

#if defined(__GNUC__)
#	define NEKO_GCC
#endif

#if defined(_MSC_VER)
#	define HL_VCC
// remove deprecated C API usage warnings
#	pragma warning( disable : 4996 )
#endif

#include <stddef.h>
#ifndef HL_VCC
#	include <stdint.h>
#endif

#ifdef HL_64
#	define	HL_WSIZE 8
#else
#	define	HL_WSIZE 4
#endif

typedef void (_cdecl *fptr)();
typedef intptr_t int_val;

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#define HL_VERSION	010
#include "opcodes.h"

typedef struct hl_alloc_block hl_alloc_block;
typedef struct { hl_alloc_block *cur; } hl_alloc;

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
	int *extra;
} hl_opcode;

typedef struct hl_ptr_list hl_ptr_list;

typedef struct {
	int global;
	int nregs;
	int nops;
	hl_type **regs;
	hl_opcode *ops;
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
	hl_alloc	alloc;
} hl_code;

typedef struct {
	hl_code *code;
	int *globals_indexes;
	unsigned char *globals_data;
	void **functions_ptrs;
	void *jit_code;
} hl_module;

typedef struct jit_ctx jit_ctx;

void hl_alloc_init( hl_alloc *a );
void *hl_malloc( hl_alloc *a, int size );
void *hl_zalloc( hl_alloc *a, int size );
void hl_free( hl_alloc *a );

void hl_global_init();
void hl_global_free();

int hl_type_size( hl_type *t );
int hl_word_size( hl_type *t ); // same as hl_type_size, but round to the next word size

hl_code *hl_code_read( const unsigned char *data, int size );
void hl_code_free( hl_code *c );
const char* hl_op_name( int op );

hl_module *hl_module_alloc( hl_code *code );
int hl_module_init( hl_module *m );
void hl_module_free( hl_module *m );

jit_ctx *hl_jit_alloc();
void hl_jit_free( jit_ctx *ctx );
int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f );
void *hl_jit_code( jit_ctx *ctx, hl_module *m );

#endif