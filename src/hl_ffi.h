#ifndef HLFFI_H
#define HLFFI_H

#include <hl.h>

// match GNU C++ mangling
#define HL_TYPE_STR "vcsilfdbBDPOATR??X?N?S?g"

#define HL_NO_ARG
#define HL_VOID						"v"
#define HL_I8						"c"
#define HL_I16						"s"
#define HL_I32						"i"
#define HL_I64						"l"
#define HL_F32						"f"
#define HL_F64						"d"
#define HL_BOOL						"b"
#define HL_BYTES					"B"
#define HL_DYN						"D"
#define HL_FUN(t, args)				"P" args "_" t
#define HL_OBJ(fields)				"O" fields "_"
#define HL_ARR						"A"
#define HL_TYPE						"T"
#define HL_REF(t)					"R" t
#define HL_ABSTRACT(name)			"X" #name "_"
#define HL_NULL(t)					"N" t
#define HL_STRUCT					"S"
#define HL_GUID						"g"

#define HL_STRING					_OBJ(_BYTES HL_I32)

typedef struct {
	hl_type *t;
	uchar *bytes;
	int length;
} vstring;

#define HL__DEFINE_PRIM_WITH_NAME(t,name,args,realName)	C_FUNCTION_BEGIN EXPORT void *hlp_##realName( const char **sign ) { *sign = HL_FUN(t,args); return (void*)(&HL_NAME(name)); } C_FUNCTION_END

#if !defined(HL_NAME)
#	define HL_NAME(p)					p
#	ifdef LIBHL_EXPORTS
#		define HL_PRIM				EXPORT
#		define HL_DEFINE_PRIM(t,name,args)				HL__DEFINE_PRIM_WITH_NAME(t,hl_##name,args,name)
#		define HL_DEFINE_PRIM_WITH_NAME					HL__DEFINE_PRIM_WITH_NAME
#	else
#		define HL_PRIM
#		define HL_DEFINE_PRIM(t,name,args)
#		define HL_DEFINE_PRIM_WITH_NAME(t,name,args,realName)
#	endif
#elif defined(LIBHL_STATIC)
#	ifdef __cplusplus
#		define	HL_PRIM				extern "C"
#	else
#		define	HL_PRIM
#	endif
#	define HL_DEFINE_PRIM(t,name,args)
#	define HL_DEFINE_PRIM_WITH_NAME(t,name,args,realName)
#else
#	ifdef __cplusplus
#		define	HL_PRIM				extern "C" EXPORT
#	else
#		define	HL_PRIM				EXPORT
#	endif
#	define HL_DEFINE_PRIM(t,name,args)					HL__DEFINE_PRIM_WITH_NAME(t,name,args,name)
#	define HL_DEFINE_PRIM_WITH_NAME						HL__DEFINE_PRIM_WITH_NAME
#endif


#if defined(HL_GCC) && !defined(HL_CONSOLE)
#	ifdef HL_CLANG
#		define HL_NO_OPT	__attribute__ ((optnone))
#	else
#		define HL_NO_OPT	__attribute__((optimize("-O0")))
#	endif
#else
#	define HL_NO_OPT
#endif

// legacy ffi without HL_ prefix
#ifndef HL_DISABLE_LEGACY_FFI

#define TYPE_STR HL_TYPE_STR

#undef  _VOID
#define _NO_ARG
#define _VOID						HL_VOID
#define	_I8							HL_I8
#define _I16						HL_I16
#define _I32						HL_I32
#define _I64						HL_I64
#define _F32						HL_F32
#define _F64						HL_F64
#define _BOOL						HL_BOOL
#define _BYTES						HL_BYTES
#define _DYN						HL_DYN
#define _FUN(t, args)				HL_FUN(t, args)
#define _OBJ(fields)				HL_OBJ(fields)
#define _ARR						HL_ARR
#define _TYPE						HL_TYPE
#define _REF(t)						HL_REF(t)
#define _ABSTRACT(name)				HL_ABSTRACT(name)
#undef _NULL
#define _NULL(t)					HL_NULL(t)
#define _STRUCT						HL_STRUCT
#define _GUID						HL_GUID

#undef _STRING
#define _STRING						HL_STRING

#define DEFINE_PRIM					HL_DEFINE_PRIM
#define DEFINE_PRIM_WITH_NAME		HL_DEFINE_PRIM_WITH_NAME

#endif // ifndef HL_DISABLE_LEGACY_FFI

#endif // ifndef HLFFI_H
