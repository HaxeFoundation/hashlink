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
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "hl.h"

int main( int argc, char *argv[] ) {
	if( argc == 1 ) {
		printf("HLVM %d.%d.%d (c)2015 Haxe Foundation\n  Usage : hl <file>\n",HL_VERSION/100,(HL_VERSION/10)%10,HL_VERSION%10);
		return 1;
	}
	hl_global_init();
	{
		hl_code *code;
		const char *file = argv[1];
		FILE *f = fopen(file,"rb");
		int pos, size;
		char *fdata;
		if( f == NULL ) {
			printf("File not found '%s'\n",file);
			return -1;
		}
		fseek(f, 0, SEEK_END);
		size = (int)ftell(f);
		fseek(f, 0, SEEK_SET);
		fdata = (char*)malloc(size);
		pos = 0;
		while( pos < size ) {
			int r = (int)fread(fdata + pos, 1, size-pos, f);
			if( r <= 0 ) {
				printf("Failed to read '%s'\n",file);
				return -1;
			}
			pos += r;
		}
		fclose(f);
		code = hl_code_read((unsigned char*)fdata, size);
		if( code == NULL )
			return -1;
		printf("TODO\n");
	}
	hl_global_free();
	return 0;
}
