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
 *
 * Opcode quick reference (decimal / hex):
 *   0/0x00 OMov            1/0x01 OInt            2/0x02 OFloat          3/0x03 OBool
 *   4/0x04 OBytes          5/0x05 OString         6/0x06 ONull           7/0x07 OAdd
 *   8/0x08 OSub            9/0x09 OMul           10/0x0a OSDiv          11/0x0b OUDiv
 *  12/0x0c OSMod          13/0x0d OUMod          14/0x0e OShl           15/0x0f OSShr
 *  16/0x10 OUShr          17/0x11 OAnd           18/0x12 OOr            19/0x13 OXor
 *  20/0x14 ONeg           21/0x15 ONot           22/0x16 OIncr          23/0x17 ODecr
 *  24/0x18 OCall0         25/0x19 OCall1         26/0x1a OCall2         27/0x1b OCall3
 *  28/0x1c OCall4         29/0x1d OCallN         30/0x1e OCallMethod    31/0x1f OCallThis
 *  32/0x20 OCallClosure   33/0x21 OStaticClosure 34/0x22 OInstanceClosure 35/0x23 OVirtualClosure
 *  36/0x24 OGetGlobal     37/0x25 OSetGlobal     38/0x26 OField         39/0x27 OSetField
 *  40/0x28 OGetThis       41/0x29 OSetThis       42/0x2a ODynGet        43/0x2b ODynSet
 *  44/0x2c OJTrue         45/0x2d OJFalse        46/0x2e OJNull         47/0x2f OJNotNull
 *  48/0x30 OJSLt          49/0x31 OJSGte         50/0x32 OJSGt          51/0x33 OJSLte
 *  52/0x34 OJULt          53/0x35 OJUGte         54/0x36 OJNotLt        55/0x37 OJNotGte
 *  56/0x38 OJEq           57/0x39 OJNotEq        58/0x3a OJAlways       59/0x3b OToDyn
 *  60/0x3c OToSFloat      61/0x3d OToUFloat      62/0x3e OToInt         63/0x3f OSafeCast
 *  64/0x40 OUnsafeCast    65/0x41 OToVirtual     66/0x42 OLabel         67/0x43 ORet
 *  68/0x44 OThrow         69/0x45 ORethrow       70/0x46 OSwitch        71/0x47 ONullCheck
 *  72/0x48 OTrap          73/0x49 OEndTrap       74/0x4a OGetI8         75/0x4b OGetI16
 *  76/0x4c OGetMem        77/0x4d OGetArray      78/0x4e OSetI8         79/0x4f OSetI16
 *  80/0x50 OSetMem        81/0x51 OSetArray      82/0x52 ONew           83/0x53 OArraySize
 *  84/0x54 OType          85/0x55 OGetType       86/0x56 OGetTID        87/0x57 ORef
 *  88/0x58 OUnref         89/0x59 OSetref        90/0x5a OMakeEnum      91/0x5b OEnumAlloc
 *  92/0x5c OEnumIndex     93/0x5d OEnumField     94/0x5e OSetEnumField  95/0x5f OAssert
 *  96/0x60 ORefData       97/0x61 ORefOffset     98/0x62 ONop           99/0x63 OPrefetch
 * 100/0x64 OAsm          101/0x65 OCatch
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
