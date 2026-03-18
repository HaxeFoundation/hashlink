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

#ifndef OP_BEGIN
#	define		OP_BEGIN	typedef enum {
#	define		OP_END		} hl_op;
#endif

#define X 0			// unused
#define R 1			// register (written if first arg)
#define R_NW 2		// register but not written
#define C 3			// constant
#define G 4			// global table index
#define J 6			// jump index
#define AR 5		// constant number of arguments
#define VAR_ARGS -1 // represents the variable constant

#ifndef OP
#	define OP(o,_a,_b,_c) o,
#endif

OP_BEGIN
	OP(OMov,R,R,X)
	OP(OInt,R,G,X)
	OP(OFloat,R,G,X)
	OP(OBool,R,C,X)
	OP(OBytes,R,G,X)
	OP(OString,R,G,X)
	OP(ONull,R,X,X)

	OP(OAdd,R,R,R)
	OP(OSub,R,R,R)
	OP(OMul,R,R,R)
	OP(OSDiv,R,R,R)
	OP(OUDiv,R,R,R)
	OP(OSMod,R,R,R)
	OP(OUMod,R,R,R)
	OP(OShl,R,R,R)
	OP(OSShr,R,R,R)
	OP(OUShr,R,R,R)
	OP(OAnd,R,R,R)
	OP(OOr,R,R,R)
	OP(OXor,R,R,R)

	OP(ONeg,R,R,X)
	OP(ONot,R,R,X)

	OP(OIncr,R,X,X)
	OP(ODecr,R,X,X)

	OP(OCall0,R,R,X)
	OP(OCall1,R,R,R)
	OP(OCall2,R,AR,4)
	OP(OCall3,R,AR,5)
	OP(OCall4,R,AR,6)
	OP(OCallN,R,AR,VAR_ARGS)
	OP(OCallMethod,R,AR,VAR_ARGS)
	OP(OCallThis,R,AR,VAR_ARGS)
	OP(OCallClosure,R,AR,VAR_ARGS)

	OP(OStaticClosure,R,G,X)
	OP(OInstanceClosure,R,R,G)
	OP(OVirtualClosure,R,R,G)

	OP(OGetGlobal,R,G,X)
	OP(OSetGlobal,R_NW,G,X)
	OP(OField,R,R,C)
	OP(OSetField,R_NW,R,C)
	OP(OGetThis,R,C,X)
	OP(OSetThis,R_NW,R,X)
	OP(ODynGet,R,R,C)
	OP(ODynSet,R_NW,R,C)

	OP(OJTrue,R_NW,J,X)
	OP(OJFalse,R_NW,J,X)
	OP(OJNull,R_NW,J,X)
	OP(OJNotNull,R_NW,J,X)
	OP(OJSLt,R_NW,R,J)
	OP(OJSGte,R_NW,R,J)
	OP(OJSGt,R_NW,R,J)
	OP(OJSLte,R_NW,R,J)
	OP(OJULt,R_NW,R,J)
	OP(OJUGte,R_NW,R,J)
	OP(OJNotLt,R_NW,R,J)
	OP(OJNotGte,R_NW,R,J)
	OP(OJEq,R_NW,R,J)
	OP(OJNotEq,R_NW,R,J)
	OP(OJAlways,J,X,X)

	OP(OToDyn,R,R,X)
	OP(OToSFloat,R,R,X)
	OP(OToUFloat,R,R,X)
	OP(OToInt,R,R,X)
	OP(OSafeCast,R,R,X)
	OP(OUnsafeCast,R,R,X)
	OP(OToVirtual,R,R,X)

	OP(OLabel,X,X,X)
	OP(ORet,R_NW,X,X)
	OP(OThrow,R_NW,X,X)
	OP(ORethrow,R_NW,X,X)
	OP(OSwitch,R_NW,AR,VAR_ARGS)
	OP(ONullCheck,R_NW,X,X)
	OP(OTrap,R_NW,J,X)
	OP(OEndTrap,R,X,X)

	OP(OGetI8,R,R,R)
	OP(OGetI16,R,R,R)
	OP(OGetMem,R,R,R)
	OP(OGetArray,R,R,R)
	OP(OSetI8,R,R,R)
	OP(OSetI16,R,R,R)
	OP(OSetMem,R,R,R)
	OP(OSetArray,R,R,R)

	OP(ONew,R,X,X)
	OP(OArraySize,R,R,X)
	OP(OType,R,R,X)
	OP(OGetType,R,R,X)
	OP(OGetTID,R,R,X)

	OP(ORef,R,R,X)
	OP(OUnref,R,R,X)
	OP(OSetref,R_NW,R,X)

	OP(OMakeEnum,R,AR,VAR_ARGS)
	OP(OEnumAlloc,R,R,X)
	OP(OEnumIndex,R,R,X)
	OP(OEnumField,R,AR,4)
	OP(OSetEnumField,R_NW,R,C)

	OP(OAssert,X,X,X)
	OP(ORefData,R,R,X)
	OP(ORefOffset,R,R,C)
	OP(ONop,X,X,X)
	OP(OPrefetch,R_NW,C,C)
	OP(OAsm,C,C,C)
	OP(OCatch,J,X,X)
	// --
	OP(OLast,X,X,X)
OP_END

#undef OP_BEGIN
#undef OP_END
#undef OP

#undef X
#undef R
#undef R_NW
#undef C
#undef G
#undef J
#undef AR
#undef VAR_ARGS
