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
#ifndef _NEKO_VM_H
#define _NEKO_VM_H
#include "neko.h"

typedef void (*neko_printer)( const char *data, int size, void *param );
typedef void (*thread_main_func)( void *param );

typedef struct _neko_vm neko_vm;

typedef void (*neko_stat_func)( neko_vm *vm, const char *kind, int start );

C_FUNCTION_BEGIN

EXTERN void neko_global_init();
EXTERN void neko_global_free();
EXTERN void neko_gc_major();
EXTERN void neko_gc_loop();
EXTERN void neko_gc_stats( int *heap, int *free );
EXTERN int neko_thread_create( thread_main_func init, thread_main_func main, void *param, void **handle );
EXTERN void neko_thread_blocking( thread_main_func f, void *p );
EXTERN bool neko_thread_register( bool t );

EXTERN neko_vm *neko_vm_alloc( void *unused );
EXTERN neko_vm *neko_vm_current();
EXTERN value neko_exc_stack( neko_vm *vm );
EXTERN value neko_call_stack( neko_vm *vm );
EXTERN void *neko_vm_custom( neko_vm *vm, vkind k );
EXTERN void neko_vm_set_custom( neko_vm *vm, vkind k, void *v );
EXTERN value neko_vm_execute( neko_vm *vm, void *module );
EXTERN void neko_vm_select( neko_vm *vm );
EXTERN int neko_vm_jit( neko_vm *vm, int enable_jit );
EXTERN int neko_vm_trusted( neko_vm *vm, int trusted );
EXTERN value neko_default_loader( char **argv, int argc );
EXTERN void neko_vm_redirect( neko_vm *vm, neko_printer print, void *param );
EXTERN void neko_vm_set_stats( neko_vm *vm, neko_stat_func fstats, neko_stat_func pstats );
EXTERN void neko_vm_dump_stack( neko_vm *vm );

EXTERN int neko_is_big_endian();

C_FUNCTION_END

#endif
/* ************************************************************************ */
