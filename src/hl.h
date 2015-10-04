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

#if defined(_64BITS) || defined(__x86_64__) || defined(_M_X64)
#	define HL_64
#endif

#if defined(__GNUC__)
#	define HL_GCC
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
#	define HL_WSIZE 8
#	define IS_64	1
#	ifdef HL_VCC
#		define _PTR_FMT	"%llX"
#	else
#		define _PTR_FMT	"%lX"
#	endif
#else
#	define HL_WSIZE 4
#	define IS_64	0
#	define _PTR_FMT	"%X"
#endif

typedef	enum { false = 0, true = 1 } bool;
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
	HDYN	= 6,
	HFUN	= 7,
	HOBJ	= 8,
	HBYTES	= 9,
	HARRAY	= 10,
	// ---------
	HLAST	= 11,
	_H_FORCE_INT = 0x7FFFFFFF
} hl_type_kind;

typedef struct hl_type hl_type;
typedef struct hl_runtime_obj hl_runtime_obj;

typedef struct {
	int nargs;
	hl_type *ret;
	hl_type *args;
} hl_type_fun;

typedef struct {
	const char *name;
	hl_type *t;
} hl_obj_field;

typedef struct {
	const char *name;
	int findex;
	int pindex;
} hl_obj_proto;

typedef struct {
	int nfields;
	int nproto;
	const char *name;
	hl_type *super;
	hl_obj_field *fields;
	hl_obj_proto *proto;
	hl_runtime_obj *rt;
} hl_type_obj;

struct hl_type {
	hl_type_kind kind;
	union {
		hl_type_fun *fun;
		hl_type_obj *obj;
		hl_type	*t;
	};
};

typedef struct {
	const char *name;
	hl_type *t;
	int findex;
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
	int findex;
	int nregs;
	int nops;
	hl_type *type;
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
	int *functions_indexes;
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
int hl_pad_size( int size, hl_type *t );

hl_code *hl_code_read( const unsigned char *data, int size );
void hl_code_free( hl_code *c );
const char* hl_op_name( int op );

hl_module *hl_module_alloc( hl_code *code );
int hl_module_init( hl_module *m );
void hl_module_free( hl_module *m );
hl_runtime_obj *hl_get_obj_rt( hl_module *m, hl_type *ot );
hl_runtime_obj *hl_get_obj_proto( hl_module *m, hl_type *ot );

jit_ctx *hl_jit_alloc();
void hl_jit_free( jit_ctx *ctx );
void hl_jit_init( jit_ctx *ctx, hl_module *m );
int hl_jit_init_callback( jit_ctx *ctx );
int hl_jit_function( jit_ctx *ctx, hl_module *m, hl_function *f );
void *hl_jit_code( jit_ctx *ctx, hl_module *m );

/* -------------------- RUNTIME ------------------------------ */

typedef struct vobj vobj;
typedef struct vclosure vclosure;

typedef struct {
	hl_type **t;
#	ifndef HL_64
	int __pad; // force align
#	endif
	union {
		unsigned char b;
		int i;
		float f;
		double d;
		vobj *o;
		vclosure *c;
		void *ptr;
	} v;
} vdynamic;

typedef struct {
	hl_type *t;
} vobj_proto;

struct vobj {
	vobj_proto *proto;
};

#define CL_HAS_V32		1
#define CL_HAS_V64		2

struct vclosure {
	hl_type **t;
	void *fun;
	int bits;
	union {
		int v32;
		int_val v64;
	};
};

struct hl_runtime_obj {
	int nfields; // absolute
	int nproto;  // absolute
	int size;
	int *fields_indexes;
	vobj_proto *proto;
	void *toString;
};

void *hl_alloc_executable_memory( int size );
void hl_free_executable_memory( void *ptr );

vdynamic *hl_alloc_dynamic( hl_type **t );
vobj *hl_alloc_obj( hl_module *m, hl_type *t );

void hl_callback_init( void *e );
void *hl_callback( void *f, int nargs, vdynamic **args );

vclosure *hl_alloc_closure_void( hl_module *m, int_val f );
vclosure *hl_alloc_closure_i32( hl_module *m, int_val f, int v32 );
vclosure *hl_alloc_closure_i64( hl_module *m, int_val f, int_val v64 );

#endif