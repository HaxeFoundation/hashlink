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
#include <stdlib.h>
#include <string.h>
#include "jit_common.h"

/*
 * Shared JIT utilities used by both x86 and AArch64 backends
 */

// Global unused register
preg _unused = { RUNUSED, 0, 0, NULL };
preg *UNUSED = &_unused;

/**
 * Ensure the JIT code buffer has enough space for the next operation.
 * Automatically grows the buffer if needed.
 */
void jit_buf(jit_ctx *ctx) {
	if( BUF_POS() > ctx->bufSize - MAX_OP_SIZE ) {
		int nsize = ctx->bufSize * 4 / 3;
		unsigned char *nbuf;
		int curpos;
		if( nsize == 0 ) {
			int i;
			for(i=0;i<ctx->m->code->nfunctions;i++)
				nsize += ctx->m->code->functions[i].nops;
			nsize *= 4;
		}
		if( nsize < ctx->bufSize + MAX_OP_SIZE * 4 )
			nsize = ctx->bufSize + MAX_OP_SIZE * 4;
		curpos = BUF_POS();
		nbuf = (unsigned char*)malloc(nsize);
		if( nbuf == NULL ) {
			printf("JIT ERROR: Failed to allocate %d bytes for code buffer\n", nsize);
			jit_exit();
		}
		if( ctx->startBuf ) {
			memcpy(nbuf, ctx->startBuf, curpos);
			free(ctx->startBuf);
		}
		ctx->startBuf = nbuf;
		ctx->buf.b = nbuf + curpos;
		ctx->bufSize = nsize;
	}
}
