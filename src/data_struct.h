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
#ifndef HL_DATA_STRUCT_H
#define HL_DATA_STRUCT_H

#include <hl.h>

#if defined(__GNUC__) || defined(__clang__)
#define INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE inline
#endif

#define STRUCT_DEF_SIZE 2
#define for_iter(name,var,set) name##__value var; for(int __idx=0;name##_iter_next(set,&var,__idx);__idx++)
#define for_iter_back(name,var,set) name##__value var; for(int __idx=(set).cur-1;name##_iter_prev(set,&var,__idx);__idx--)

#define S_TYPE			ptr_set
#define S_NAME(name)	ptr_set_##name
#define S_VALUE			void*
#include "data_struct.c"
#define ptr_set_add(set,v)		ptr_set_add_impl(DEF_ALLOC,&(set),v)

#define S_TYPE			int_arr
#define S_NAME(name)	int_arr_##name
#define S_VALUE			int
#include "data_struct.c"
#define int_arr_add(set,v)		int_arr_add_impl(DEF_ALLOC,&(set),v)

#define S_SORTED

#define S_TYPE			int_set
#define S_NAME(name)	int_set_##name
#define S_VALUE			int
#include "data_struct.c"
#define int_set_add(set,v)		int_set_add_impl(DEF_ALLOC,&(set),v)

#define S_MAP

#define S_TYPE			int_map
#define S_NAME(name)	int_map_##name
#define S_KEY			int
#define S_VALUE			int
#include "data_struct.c"
#define int_map_add(map,k,v)		int_map_add_impl(DEF_ALLOC,&(map),k,v)
#define int_map_replace(map,k,v)	int_map_replace_impl(DEF_ALLOC,&(map),k,v)

#define S_TYPE			ptr_map
#define S_NAME(name)	ptr_map_##name
#define S_KEY			int
#define S_VALUE			void*
#include "data_struct.c"
#define ptr_map_add(map,k,v)		ptr_map_add_impl(DEF_ALLOC,&(map),k,v)
#define ptr_map_replace(map,k,v)	ptr_map_replace_impl(DEF_ALLOC,&(map),k,v)

#undef S_MAP
#undef S_SORTED

#endif
