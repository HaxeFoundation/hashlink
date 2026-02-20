#ifndef HLFFI_H
#define HLFFI_H

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

#define HL_STRING					HL_OBJ(HL_BYTES HL_I32)

#define HL__DEFINE_PRIM_WITH_NAME(t,name,args,realName)	C_FUNCTION_BEGIN EXPORT void *hlp_##realName( const char **sign ) { *sign = HL_FUN(t,args); return (void*)(&HL_NAME(name)); } C_FUNCTION_END

#define HL_DEFINE_PRIM(t,name,args) HL__DEFINE_PRIM_WITH_NAME(t,name,args,name)
#define HL_DEFINE_PRIM_WITH_NAME    HL__DEFINE_PRIM_WITH_NAME

#endif // ifndef HLFFI_H
