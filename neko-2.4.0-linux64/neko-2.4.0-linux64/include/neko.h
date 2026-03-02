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
#ifndef _NEKO_H
#define _NEKO_H

// OS FLAGS
#if defined(_WIN32)
#	define NEKO_WINDOWS
#endif

#if defined(__APPLE__) || defined(macintosh)
#	define NEKO_MAC
#endif

#if defined(linux) || defined(__linux__)
#	define NEKO_LINUX
#endif

#if defined(__FreeBSD_kernel__)
#	define NEKO_GNUKBSD
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#	define NEKO_BSD
#endif

#if defined(__GNU__)
#	define NEKO_HURD
#endif

// COMPILER/PROCESSOR FLAGS
#if defined(__GNUC__)
#	define NEKO_GCC
#endif

#if defined(_MSC_VER)
#	define NEKO_VCC
// remove deprecated C API usage warnings
#	pragma warning( disable : 4996 )
#endif

#if defined(__MINGW32__)
#	define NEKO_MINGW
#endif

#if defined(__CYGWIN__)
#	define NEKO_CYGWIN
#endif

#if defined(__i386__) || defined(_WIN32)
#	define NEKO_X86
#endif

#if defined(__ppc__)
#	define NEKO_PPC
#endif

#if defined(_64BITS) || defined(__x86_64__) || defined(_M_AMD64)
#	define NEKO_64BITS
#endif

#if defined(NEKO_LINUX) || defined(NEKO_MAC) || defined(NEKO_BSD) || defined(NEKO_GNUKBSD) || defined(NEKO_HURD) || defined(NEKO_CYGWIN)
#	define NEKO_POSIX
#endif

#if defined(NEKO_GCC)
#	define NEKO_THREADED
#	define NEKO_DIRECT_THREADED
#endif

#ifndef NEKO_NO_THREADS
#	define NEKO_THREADS
#endif

#include <stddef.h>
#ifndef NEKO_VCC
#	include <stdint.h>
#endif

/* #undef NEKO_XLOCALE_H */

/* #undef NEKO_BIG_ENDIAN */

#ifndef NEKO_BIG_ENDIAN
#	define NEKO_LITTLE_ENDIAN
#endif

/* #undef NEKO_JIT_DISABLE */
/* #undef NEKO_JIT_DEBUG */

#if !defined(NEKO_JIT_DISABLE) && defined(NEKO_X86) && !defined(NEKO_MAC) && !defined(_WIN64)
#define NEKO_JIT_ENABLE
#endif

#define NEKO_VERSION_MAJOR	2
#define NEKO_VERSION_MINOR	4
#define NEKO_VERSION_PATCH	0
#define NEKO_VERSION		240

#define NEKO_BUILD_YEAR		2024

#define NEKO_MODULE_PATH	"/usr/local/lib/neko"

typedef intptr_t int_val;

typedef enum {
	VAL_INT			= 0xFF,
	VAL_NULL		= 0,
	VAL_FLOAT		= 1,
	VAL_BOOL		= 2,
	VAL_STRING		= 3,
	VAL_OBJECT		= 4,
	VAL_ARRAY		= 5,
	VAL_FUNCTION	= 6,
	VAL_ABSTRACT	= 7,
	VAL_INT32		= 8,
	VAL_PRIMITIVE	= 6 | 16,
	VAL_JITFUN		= 6 | 32,
	VAL_32_BITS		= 0xFFFFFFFF
} val_type;

struct _value {
	val_type t;
};

struct _buffer;
typedef int field;
typedef struct { int __zero; } *vkind;
typedef struct _value *value;

typedef struct {
	field id;
	value v;
} objcell;

typedef struct _objtable
{
	int count;
	objcell *cells;
} objtable;

typedef struct _buffer *buffer;
typedef double tfloat;

typedef void (*finalizer)(value v);

#pragma pack(4)
typedef struct {
	val_type t;
	tfloat f;
} vfloat;

typedef struct {
	val_type t;
	int i;
} vint32;
#pragma pack()

typedef struct _vobject {
	val_type t;
	objtable table;
	struct _vobject *proto;
} vobject;

typedef struct {
	val_type t;
	int nargs;
	void *addr;
	value env;
	void *module;
} vfunction;

typedef struct {
	val_type t;
	char c;
} vstring;

typedef struct {
	val_type t;
	value ptr;
} varray;

typedef struct {
	val_type t;
	vkind kind;
	void *data;
} vabstract;

typedef struct hcell {
	int hkey;
	value key;
	value val;
	struct hcell *next;
} hcell;

typedef struct {
	hcell **cells;
	int ncells;
	int nitems;
} vhash;

struct _mt_local;
struct _mt_lock;
typedef struct _mt_local mt_local;
typedef struct _mt_lock mt_lock;

#define NEKO_TAG_BITS		4

#define val_tag(v)			(*(val_type*)(v))
#define val_short_tag(v)	(val_tag(v)&((1<<NEKO_TAG_BITS) - 1))
#define val_is_null(v)		((v) == val_null)
#define val_is_int(v)		((((int)(int_val)(v)) & 1) != 0)
#define val_is_any_int(v)	(val_is_int(v) || val_tag(v) == VAL_INT32)
#define val_is_bool(v)		((v) == val_true || (v) == val_false)
#define val_is_number(v)	(val_is_int(v) || val_tag(v) == VAL_FLOAT || val_tag(v) == VAL_INT32)
#define val_is_float(v)		(!val_is_int(v) && val_tag(v) == VAL_FLOAT)
#define val_is_int32(v)		(!val_is_int(v) && val_tag(v) == VAL_INT32)
#define val_is_string(v)	(!val_is_int(v) && val_short_tag(v) == VAL_STRING)
#define val_is_function(v)	(!val_is_int(v) && val_short_tag(v) == VAL_FUNCTION)
#define val_is_object(v)	(!val_is_int(v) && val_tag(v) == VAL_OBJECT)
#define val_is_array(v)		(!val_is_int(v) && val_short_tag(v) == VAL_ARRAY)
#define val_is_abstract(v)  (!val_is_int(v) && val_tag(v) == VAL_ABSTRACT)
#define val_is_kind(v,t)	(val_is_abstract(v) && val_kind(v) == (t))
#define val_check_kind(v,t)	if( !val_is_kind(v,t) ) neko_error();
#define val_check_function(f,n) if( !val_is_function(f) || (val_fun_nargs(f) != (n) && val_fun_nargs(f) != VAR_ARGS) ) neko_error();
#define val_check(v,t)		if( !val_is_##t(v) ) neko_error();
#define val_data(v)			((vabstract*)(v))->data
#define val_kind(v)			((vabstract*)(v))->kind

#define val_type(v)			(val_is_int(v) ? VAL_INT : val_short_tag(v))
#define val_int(v)			(((int)(int_val)(v)) >> 1)
#define val_float(v)		(CONV_FLOAT ((vfloat*)(v))->f)
#define val_int32(v)		(((vint32*)(v))->i)
#define val_any_int(v)		(val_is_int(v)?val_int(v):val_int32(v))
#define val_bool(v)			((v) == val_true)
#define val_number(v)		(val_is_int(v)?val_int(v):((val_tag(v)==VAL_FLOAT)?val_float(v):val_int32(v)))
#define val_hdata(v)		((vhash*)val_data(v))
#define val_string(v)		(&((vstring*)(v))->c)
#define val_strlen(v)		((signed)(((unsigned)val_tag(v)) >> NEKO_TAG_BITS))
#define val_set_length(v,l) val_tag(v) = val_short_tag(v) | ((l) << NEKO_TAG_BITS)
#define val_set_size		val_set_length

#define val_array_size(v)	((signed)(((unsigned)val_tag(v)) >> NEKO_TAG_BITS))
#define val_array_ptr(v)	(&((varray*)(v))->ptr)
#define val_fun_nargs(v)	((vfunction*)(v))->nargs
#define alloc_int(v)		((value)(int_val)((((int)(v)) << 1) | 1))
#define alloc_bool(b)		((b)?val_true:val_false)

#define max_array_size		((1 << (32 - NEKO_TAG_BITS)) - 1)
#define max_string_size		((1 << (32 - NEKO_TAG_BITS)) - 1)
#define invalid_comparison	0xFE

#undef EXTERN
#undef EXPORT
#undef IMPORT
#if defined(NEKO_VCC) || defined(NEKO_MINGW)
#	define INLINE __inline
#	define EXPORT __declspec( dllexport )
#	define IMPORT __declspec( dllimport )
#else
#	define INLINE inline
#	define EXPORT
#	define IMPORT
#endif

#if defined(NEKO_SOURCES) || defined(NEKO_STANDALONE)
#	define EXTERN EXPORT
#else
#	define EXTERN IMPORT
#endif

#define VEXTERN extern EXTERN

#ifdef __cplusplus
#	define C_FUNCTION_BEGIN extern "C" {
#	define C_FUNCTION_END	};
#else
#	define C_FUNCTION_BEGIN
#	define C_FUNCTION_END
#	ifndef true
#		define true 1
#		define false 0
		typedef int bool;
#	endif
#endif

// the two upper bits must be either 00 or 11
#define need_32_bits(i) ( (((unsigned int)(i)) + 0x40000000) & 0x80000000 )
#define alloc_best_int(i) (need_32_bits(i) ? alloc_int32(i) : alloc_int(i))

#define neko_error()		return NULL
#define failure(msg)		_neko_failure(alloc_string(msg),__FILE__,__LINE__)
#define bfailure(buf)		_neko_failure(buffer_to_string(buf),__FILE__,__LINE__)

#ifndef CONV_FLOAT
#	define CONV_FLOAT
#endif

#ifdef NEKO_POSIX
#	include <errno.h>
#	define POSIX_LABEL(name)	name:
#	define HANDLE_EINTR(label)	if( errno == EINTR ) goto label
#	define HANDLE_FINTR(f,label) if( ferror(f) && errno == EINTR ) goto label
#else
#	define POSIX_LABEL(name)
#	define HANDLE_EINTR(label)
#	define HANDLE_FINTR(f,label)
#endif

#define VAR_ARGS (-1)
#define DEFINE_PRIM_MULT(func) C_FUNCTION_BEGIN EXPORT void *func##__MULT() { return (void*)(&func); } C_FUNCTION_END
#define DEFINE_PRIM(func,nargs) C_FUNCTION_BEGIN EXPORT void *func##__##nargs() { return (void*)(&func); } C_FUNCTION_END
#define DEFINE_PRIM_WITH_NAME(func,name,nargs) C_FUNCTION_BEGIN EXPORT void *name##__##nargs() { return (void*)(&func); } C_FUNCTION_END
#define DEFINE_KIND(name) int_val __kind_##name = 0; vkind name = (vkind)&__kind_##name;

#ifdef NEKO_STANDALONE
#	define DEFINE_ENTRY_POINT(name)
#else
#	define DEFINE_ENTRY_POINT(name) C_FUNCTION_BEGIN void name(); EXPORT void *__neko_entry_point() { return &name; } C_FUNCTION_END
#endif

#ifdef HEADER_IMPORTS
#	define H_EXTERN IMPORT
#else
#	define H_EXTERN EXPORT
#endif

#define DECLARE_PRIM(func,nargs) C_FUNCTION_BEGIN H_EXTERN void *func##__##nargs(); C_FUNCTION_END
#define DECLARE_KIND(name) C_FUNCTION_BEGIN H_EXTERN extern vkind name; C_FUNCTION_END

#define alloc_int32			neko_alloc_int32
#define alloc_float			neko_alloc_float
#define alloc_string		neko_alloc_string
#define alloc_empty_string	neko_alloc_empty_string
#define copy_string			neko_copy_string
#define val_this			neko_val_this
#define val_id				neko_val_id
#define val_field			neko_val_field
#define alloc_object		neko_alloc_object
#define alloc_field			neko_alloc_field
#define alloc_array			neko_alloc_array
#define val_call0			neko_val_call0
#define val_call1			neko_val_call1
#define val_call2			neko_val_call2
#define val_call3			neko_val_call3
#define val_callN			neko_val_callN
#define val_ocall0			neko_val_ocall0
#define val_ocall1			neko_val_ocall1
#define val_ocall2			neko_val_ocall2
#define val_ocallN			neko_val_ocallN
#define val_callEx			neko_val_callEx
#define	alloc_root			neko_alloc_root
#define free_root			neko_free_root
#define alloc				neko_alloc
#define alloc_private		neko_alloc_private
#define alloc_abstract		neko_alloc_abstract
#define alloc_function		neko_alloc_function
#define alloc_buffer		neko_alloc_buffer
#define buffer_append		neko_buffer_append
#define buffer_append_sub	neko_buffer_append_sub
#define buffer_append_char	neko_buffer_append_char
#define buffer_length		neko_buffer_length
#define buffer_to_string	neko_buffer_to_string
#define val_buffer			neko_val_buffer
#define val_compare			neko_val_compare
#define val_print			neko_val_print
#define val_gc				neko_val_gc
#define val_throw			neko_val_throw
#define val_rethrow			neko_val_rethrow
#define val_iter_fields		neko_val_iter_fields
#define val_field_name		neko_val_field_name
#define val_hash			neko_val_hash
#define k_hash				neko_k_hash
#define kind_share			neko_kind_share
#define kind_lookup			neko_kind_lookup

#define alloc_local			neko_alloc_local
#define local_get			neko_local_get
#define local_set			neko_local_set
#define free_local			neko_free_local
#define alloc_lock			neko_alloc_lock
#define lock_acquire		neko_lock_acquire
#define lock_try			neko_lock_try
#define lock_release		neko_lock_release
#define free_lock			neko_free_lock

C_FUNCTION_BEGIN

	VEXTERN vkind k_hash;

	VEXTERN value val_null;
	VEXTERN value val_true;
	VEXTERN value val_false;

	EXTERN value alloc_float( tfloat t );
	EXTERN value alloc_int32( int i );

	EXTERN value alloc_string( const char *str );
	EXTERN value alloc_empty_string( unsigned int size );
	EXTERN value copy_string( const char *str, int_val size );

	EXTERN value val_this();
	EXTERN field val_id( const char *str );
	EXTERN value val_field( value o, field f );
	EXTERN value alloc_object( value o );
	EXTERN void alloc_field( value obj, field f, value v );
	EXTERN void val_iter_fields( value obj, void f( value v, field f, void * ), void *p );
	EXTERN value val_field_name( field f );

	EXTERN value alloc_array( unsigned int n );
	EXTERN value alloc_abstract( vkind k, void *data );

	EXTERN value val_call0( value f );
	EXTERN value val_call1( value f, value arg );
	EXTERN value val_call2( value f, value arg1, value arg2 );
	EXTERN value val_call3( value f, value arg1, value arg2, value arg3 );
	EXTERN value val_callN( value f, value *args, int nargs );
	EXTERN value val_ocall0( value o, field f );
	EXTERN value val_ocall1( value o, field f, value arg );
	EXTERN value val_ocall2( value o, field f, value arg1, value arg2 );
	EXTERN value val_ocallN( value o, field f, value *args, int nargs );
	EXTERN value val_callEx( value vthis, value f, value *args, int nargs, value *exc );

	EXTERN value *alloc_root( unsigned int nvals );
	EXTERN void free_root( value *r );
	EXTERN char *alloc( unsigned int nbytes );
	EXTERN char *alloc_private( unsigned int nbytes );
	EXTERN value alloc_function( void *c_prim, unsigned int nargs, const char *name );

	EXTERN buffer alloc_buffer( const char *init );
	EXTERN void buffer_append( buffer b, const char *s );
	EXTERN void buffer_append_sub( buffer b, const char *s, int_val len );
	EXTERN void buffer_append_char( buffer b, char c );
	EXTERN value buffer_to_string( buffer b );
	EXTERN int buffer_length( buffer b );
	EXTERN void val_buffer( buffer b, value v );

	EXTERN int val_compare( value a, value b );
	EXTERN void val_print( value s );
	EXTERN void val_gc( value v, finalizer f );
	EXTERN void val_throw( value v );
	EXTERN void val_rethrow( value v );
	EXTERN int val_hash( value v );

	EXTERN void kind_share( vkind *k, const char *name );
	EXTERN vkind kind_lookup( const char *name );
	EXTERN void _neko_failure( value msg, const char *file, int line );

	// MULTITHREADING API
	EXTERN mt_local *alloc_local();
	EXTERN void *local_get( mt_local *l );
	EXTERN void local_set( mt_local *l, void *v );
	EXTERN void free_local( mt_local *l );

	EXTERN mt_lock *alloc_lock();
	EXTERN void lock_acquire( mt_lock *l );
	EXTERN int lock_try( mt_lock *l );
	EXTERN void lock_release( mt_lock *l );
	EXTERN void free_lock( mt_lock *l );

C_FUNCTION_END

#endif
/* ************************************************************************ */
