/*
 * Copyright (C)2005-2022 Haxe Foundation
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
#ifndef _NEKO_MOD_H
#define _NEKO_MOD_H
#include "neko.h"

typedef struct _neko_debug {
	int base;
	unsigned int bits;
} neko_debug;

typedef struct _neko_module {
	void *jit;
	unsigned int nglobals;
	unsigned int nfields;
	unsigned int codesize;
	value name;
	value *globals;
	value *fields;
	value loader;
	value exports;
	value dbgtbl;
	neko_debug *dbgidxs;
	int_val *code;
	value jit_gc;
} neko_module;

typedef void *readp;
typedef int (*reader)( readp p, void *buf, int size );

typedef struct {
	char *p;
	int len;
} string_pos;

C_FUNCTION_BEGIN

VEXTERN field neko_id_module;
VEXTERN vkind neko_kind_module;
EXTERN neko_module *neko_read_module( reader r, readp p, value loader );
EXTERN int neko_file_reader( readp p, void *buf, int size ); // FILE *
EXTERN int neko_string_reader( readp p, void *buf, int size ); // string_pos *
EXTERN value neko_select_file( value path, const char *file, const char *ext );

C_FUNCTION_END

#endif
/* ************************************************************************ */
