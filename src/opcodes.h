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

#ifndef OP
#	define OP(o,_) o,
#endif

OP_BEGIN
	OP(OMov,2)
	OP(OInt,2)
	OP(OFloat,2)
	OP(OBool,2)
	OP(OBytes,2)
	OP(OString,2)
	OP(ONull,1)

	OP(OAdd,3)
	OP(OSub,3)
	OP(OMul,3)
	OP(OSDiv,3)
	OP(OUDiv,3)
	OP(OSMod,3)
	OP(OUMod,3)
	OP(OShl,3)
	OP(OSShr,3)
	OP(OUShr,3)
	OP(OAnd,3)
	OP(OOr,3)
	OP(OXor,3)

	OP(ONeg,2)
	OP(ONot,2)
	OP(OIncr,1)
	OP(ODecr,1)

	OP(OCall0,2)
	OP(OCall1,3)
	OP(OCall2,4)
	OP(OCall3,5)
	OP(OCall4,6)
	OP(OCallN,-1)
	OP(OCallMethod,-1)
	OP(OCallThis,-1)
	OP(OCallClosure,-1)

	OP(OStaticClosure,2)
	OP(OInstanceClosure,3)
	OP(OVirtualClosure,3)

	OP(OGetGlobal, 2)
	OP(OSetGlobal,2)
	OP(OField,3)
	OP(OSetField,3)
	OP(OGetThis,2)
	OP(OSetThis,2)
	OP(ODynGet,3)
	OP(ODynSet,3)

	OP(OJTrue,2)
	OP(OJFalse,2)
	OP(OJNull,2)
	OP(OJNotNull,2)
	OP(OJSLt,3)
	OP(OJSGte,3)
	OP(OJSGt,3)
	OP(OJSLte,3)
	OP(OJULt,3)
	OP(OJUGte,3)
	OP(OJNotLt,3)
	OP(OJNotGte,3)
	OP(OJEq,3)
	OP(OJNotEq,3)
	OP(OJAlways,1)

	OP(OToDyn,2)
	OP(OToSFloat,2)
	OP(OToUFloat,2)
	OP(OToInt,2)
	OP(OSafeCast,2)
	OP(OUnsafeCast,2)
	OP(OToVirtual,2)

	OP(OLabel,0)
	OP(ORet,1)
	OP(OThrow,1)
	OP(ORethrow,1)
	OP(OSwitch,-1)
	OP(ONullCheck,1)
	OP(OTrap,2)
	OP(OEndTrap,1)

	OP(OGetI8,3)
	OP(OGetI16,3)
	OP(OGetMem,3)
	OP(OGetArray,3)
	OP(OSetI8,3)
	OP(OSetI16,3)
	OP(OSetMem,3)
	OP(OSetArray,3)

	OP(ONew,1)
	OP(OArraySize,2)
	OP(OType,2)
	OP(OGetType,2)
	OP(OGetTID,2)

	OP(ORef,2)
	OP(OUnref,2)
	OP(OSetref,2)

	OP(OMakeEnum,-1)
	OP(OEnumAlloc,2)
	OP(OEnumIndex,2)
	OP(OEnumField,4)
	OP(OSetEnumField,3)

	OP(OAssert,0)
	OP(ORefData,2)
	OP(ORefOffset,3)
	OP(ONop,0)
	OP(OPrefetch, 3)
	OP(OAsm, 3)
	OP(OCatch, 1)
	// --
	OP(OLast,0)
OP_END

#undef OP_BEGIN
#undef OP_END
#undef OP
