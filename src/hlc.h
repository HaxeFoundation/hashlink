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
#ifndef HLC_H
#define HLC_H

#include <math.h>
#include <hl.h>

#ifdef HL_64
#	define PAD_64_VAL	,0
#else
#	define PAD_64_VAL
#endif

#ifdef HLC_BOOT

// undefine some commonly used names that can clash with class/var name
#undef CONST
#undef IN
#undef OUT
#undef OPTIONAL
#undef stdin
#undef stdout
#undef stderr
#undef DELETE
#undef ERROR
#undef NO_ERROR
#undef EOF
#undef STRICT
#undef TRUE
#undef FALSE
#undef CW_USEDEFAULT
#undef HIDDEN
#undef RESIZABLE
#undef __SIGN
#undef far
#undef FAR
#undef near
#undef NEAR
#undef GENERIC_READ
#undef INT_MAX
#undef INT_MIN
#undef BIG_ENDIAN
#undef LITTLE_ENDIAN
#undef ALTERNATE
#undef DIFFERENCE
#undef DOUBLE_CLICK
#undef WAIT_FAILED
#undef OVERFLOW
#undef UNDERFLOW
#undef DOMAIN
#undef TRANSPARENT
#undef CopyFile
#undef COLOR_HIGHLIGHT
#undef __valid

// disable some warnings triggered by HLC code generator

#ifdef HL_VCC
#	pragma warning(disable:4100) // unreferenced param
#	pragma warning(disable:4101) // unreferenced local var
#	pragma warning(disable:4102) // unreferenced label
#	pragma warning(disable:4204) // nonstandard extension
#	pragma warning(disable:4221) // nonstandard extension
#	pragma warning(disable:4244) // possible loss of data
#	pragma warning(disable:4700) // uninitialized local variable used
#	pragma warning(disable:4701) // potentially uninitialized local variable
#	pragma warning(disable:4702) // unreachable code
#	pragma warning(disable:4703) // potentially uninitialized local
#	pragma warning(disable:4715) // control paths must return a value
#	pragma warning(disable:4716) // must return a value (ends with throw)
#	pragma warning(disable:4723) // potential divide by 0
#else
#	pragma GCC diagnostic ignored "-Wunused-variable"
#	pragma GCC diagnostic ignored "-Wunused-function"
#	pragma GCC diagnostic ignored "-Wcomment" // comment in comment
#	ifdef HL_CLANG
#	pragma GCC diagnostic ignored "-Wreturn-type"
#	pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#	else
#	pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#	pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#	endif
#endif

#endif

extern void *hlc_static_call(void *fun, hl_type *t, void **args, vdynamic *out);
extern void *hlc_get_wrapper(hl_type *t);
extern void hl_entry_point();

#define HL__ENUM_CONSTRUCT__	hl_type *t; int index;
#define HL__ENUM_INDEX__(v)		((venum*)(v))->index

#if defined(HL_VCC)
#define __hl_prefetch_m0(addr) _mm_prefetch((char*)addr, _MM_HINT_T0)
#define __hl_prefetch_m1(addr) _mm_prefetch((char*)addr, _MM_HINT_T1)
#define __hl_prefetch_m2(addr) _mm_prefetch((char*)addr, _MM_HINT_T2)
#define __hl_prefetch_m3(addr) _mm_prefetch((char*)addr, _MM_HINT_NTA)
#ifdef _MM_HINT_ET1
#define __hl_prefetch_m4(addr) _mm_prefetch((char*)addr, _MM_HINT_ET1)
#else
#define __hl_prefetch_m4(addr) _m_prefetchw((char*)addr)
#endif
#elif defined(HL_CLANG) || defined (HL_GCC)
#define __hl_prefetch_m0(addr) __builtin_prefetch((void*)addr, 0, 3)
#define __hl_prefetch_m1(addr) __builtin_prefetch((void*)addr, 0, 2)
#define __hl_prefetch_m2(addr) __builtin_prefetch((void*)addr, 0, 1)
#define __hl_prefetch_m3(addr) __builtin_prefetch((void*)addr, 0, 0)
#define __hl_prefetch_m4(addr) __builtin_prefetch((void*)addr, 1)
#elif
#define __hl_prefetch_m0(addr)
#define __hl_prefetch_m1(addr)
#define __hl_prefetch_m2(addr)
#define __hl_prefetch_m3(addr)
#define __hl_prefetch_m4(addr)
#endif

#endif
