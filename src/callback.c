/*
 * Copyright (C)2005-2016 Haxe Foundation
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
#include <hl.h>

#define MAX_ARGS	16

static void *hl_callback_entry = NULL;

void *hl_callback( void *f, hl_type *t, void **args, vdynamic *ret ) {
	union {
		unsigned char b[MAX_ARGS * 8];
		unsigned short s[MAX_ARGS * 4];
		int i[MAX_ARGS * 2];
		double d[MAX_ARGS];
		void *p[MAX_ARGS * 8 / HL_WSIZE];
	} stack;	
	/*
		Same as jit(prepare_call_args) but writes values to the stack var
	*/
	int i, size = 0, pad = 0, pos = 0;
	for(i=0;i<t->fun->nargs;i++) {
		hl_type *at = t->fun->args[i];
		size += hl_type_size(at);
	}
	if( size & 15 )
		pad = 16 - (size&15);
	size = pos = pad;
	for(i=0;i<t->fun->nargs;i++) {
		// RTL
		int j = t->fun->nargs - 1 - i;
		hl_type *at = t->fun->args[j];
		void *v = args[j];
		int tsize = hl_type_size(at);
		size += tsize;
		if( hl_is_ptr(at) )
			*(void**)&stack.b[pos] = v;
		else switch( tsize ) {
		case 0:
			continue;
		case 1:
			stack.b[pos] = *(char*)v;
			break;
		case 2:
			*(short*)&stack.b[pos] = *(short*)v;
			break;
		case 4:
			*(int*)&stack.b[pos] = *(int*)v;
			break;
		case 8:
			*(double*)&stack.b[pos] = *(double*)v;
			break;
		default:
			hl_error("Invalid callback arg");
		}
		pos += tsize;
	}
	switch( t->fun->ret->kind ) {
	case HI8:
	case HI16:
	case HI32:
		ret->v.i = ((int (*)(void *, void *, int))hl_callback_entry)(f, &stack, (IS_64?pos>>3:pos>>2));
		return &ret->v.i;
	case HF32:
		ret->v.f = ((float (*)(void *, void *, int))hl_callback_entry)(f, &stack, (IS_64?pos>>3:pos>>2));
		return &ret->v.f;
	case HF64:
		ret->v.d = ((double (*)(void *, void *, int))hl_callback_entry)(f, &stack, (IS_64?pos>>3:pos>>2));
		return &ret->v.d;
	default:
		return ((void *(*)(void *, void *, int))hl_callback_entry)(f, &stack, (IS_64?pos>>3:pos>>2));
	}
}

static void *hl_call_wrapper_ptr( vclosure_wrapper *c ) {
	hl_debug_break();
	return NULL;
}

static void *hl_call_wrapper_all_ptr( vclosure_wrapper *c ) {
	return hl_wrapper_call(c,&c + 1, NULL);
}

static void *hl_get_wrapper( hl_type *t ) {
#	ifndef HL_64
	int i;
	for(i=0;i<t->fun->nargs;i++)
		if( !hl_is_ptr(t->fun->args[i]) )
			break;
	if( i == t->fun->nargs ) {
		switch( t->fun->ret->kind ) {
		case HF32:
		case HF64:
			hl_error("TODO");
			break;
		case HI8:
		case HI16:
		case HI32:
			hl_error("TODO");
			break;			
		default:
			return hl_call_wrapper_all_ptr;
		}
	} else 
#	endif
	{
		switch( t->fun->ret->kind ) {
		case HF32:
		case HF64:
			hl_error("TODO");
			break;
		case HI8:
		case HI16:
		case HI32:
			hl_error("TODO");
			break;			
		default:
			return hl_call_wrapper_ptr;
		}
	}
	return NULL;
}

void hl_callback_init( void *e ) {
	hl_callback_entry = e;
	hl_setup_callbacks(hl_callback, hl_get_wrapper);
}
