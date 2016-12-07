enum HLCodeFlag {
	HasDebug;
}

typedef HLCode = {
	var version : Int;
	var flags : haxe.EnumFlags<HLCodeFlag>;
	var ints : Array<Int>;
	var floats : Array<Float>;
	var strings : Array<String>;
	var debugFiles : Array<String>;
	var types : Array<HLType>;
	var globals : Array<HLType>;
	var natives : Array<NativeFunction>;
	var functions : Array<HLFunction>;
	var entryPoint : Int;
}

enum HLType {
	HVoid;
	HUi8;
	HUi16;
	HI32;
	HF32;
	HF64;
	HBool;
	HBytes;
	HDyn;
	HFun( fun : FunPrototype );
	HObj( proto : ObjPrototype );
	HArray;
	HType;
	HRef( t : HLType );
	HVirtual( fields : Array<{ name : String, t : HLType }> );
	HDynObj;
	HAbstract( name : String );
	HEnum( proto : EnumPrototype );
	HNull( t : HLType );
	// only for reader
	HAt( i : Int );
}

typedef FunPrototype = {
	var args : Array<HLType>;
	var ret : HLType;
}

typedef ObjPrototype = {
	var name : String;
	var tsuper : HLType;
	var fields : Array<{ name : String, t : HLType }>;
	var proto : Array<{ name : String, findex : Int, pindex : Int }>;
	var globalValue : Null<Int>;
}

typedef EnumPrototype = {
	var name : String;
	var globalValue : Null<Int>;
	var constructs : Array<{ name : String, params : Array<HLType> }>;
}

typedef NativeFunction = {
	var lib : String;
	var name : String;
	var t : HLType;
	var findex : Int;
}

typedef HLFunction = {
	var t : HLType;
	var findex : Int;
	var regs : Array<HLType>;
	var ops : Array<HLOpcode>;
	var debug : Array<Int>;
}

typedef FunTable = Array<AnyFunction>;

enum AnyFunction {
	FHL( f : HLFunction );
	FNative( f : NativeFunction );
}

typedef Reg = Int;
typedef Index<T> = Int;
typedef ObjField = Void;
typedef Global = Void;
typedef EnumConstruct = Void;

enum HLOpcode {
	OMov( dst : Reg, a : Reg );
	OInt( dst : Reg, i : Index<Int> );
	OFloat( dst : Reg, i : Index<Float> );
	OBool( dst : Reg, b : Bool );
	OBytes( dst : Reg, i : Index<String> );
	OString( dst : Reg, i : Index<String> );
	ONull( dst : Reg );
	OAdd( dst : Reg, a : Reg, b : Reg );
	OSub( dst : Reg, a : Reg, b : Reg );
	OMul( dst : Reg, a : Reg, b : Reg );
	OSDiv( dst : Reg, a : Reg, b : Reg );
	OUDiv( dst : Reg, a : Reg, b : Reg );
	OSMod( dst : Reg, a : Reg, b : Reg );
	OUMod( dst : Reg, a : Reg, b : Reg );
	OShl( dst : Reg, a : Reg, b : Reg );
	OSShr( dst : Reg, a : Reg, b : Reg );
	OUShr( dst : Reg, a : Reg, b : Reg );
	OAnd( dst : Reg, a : Reg, b : Reg );
	OOr( dst : Reg, a : Reg, b : Reg );
	OXor( dst : Reg, a : Reg, b : Reg );
	ONeg( dst : Reg, a : Reg );
	ONot( dst : Reg, a : Reg );
	OIncr( dst : Reg );
	ODecr( dst : Reg );
	OCall0( dst : Reg, i : Index<FunTable> );
	OCall1( dst : Reg, i : Index<FunTable>, a : Reg );
	OCall2( dst : Reg, i : Index<FunTable>, a : Reg, b : Reg );
	OCall3( dst : Reg, i : Index<FunTable>, a : Reg, b : Reg, c : Reg );
	OCall4( dst : Reg, i : Index<FunTable>, a : Reg, b : Reg, c : Reg, d : Reg );
	OCallN( dst : Reg, i : Index<FunTable>, args : Array<Reg> );
	OCallMethod( dst : Reg, i : Index<ObjField>, args : Array<Reg> );
	OCallThis( dst : Reg, i : Index<ObjField>, args : Array<Reg> );
	OCallClosure( dst : Reg, obj : Reg, args : Array<Reg> );
	OStaticClosure( dst : Reg, i : Index<FunTable> );
	OInstanceClosure( dst : Reg, i : Index<FunTable>, a : Reg );
	OVirtualClosure( dst : Reg, a : Reg, i : Index<ObjField> );
	OGetGlobal( dst : Reg, i : Index<Global> );
	OSetGlobal( i : Index<Global>, a : Reg );
	OField( dst : Reg, a : Reg, i : Index<ObjField> );
	OSetField( dst : Reg, i : Index<ObjField>, a : Reg );
	OGetThis( dst : Reg, i : Index<ObjField> );
	OSetThis( i : Index<ObjField>, a : Reg );
	ODynGet( dst : Reg, a : Reg, i : Index<String> );
	ODynSet( dst : Reg, i : Index<String>, a : Reg );
	OSetMethod( dst : Reg, i : Index<String>, t : Index<FunTable> );
	OJTrue( dst : Reg, offset : Int );
	OJFalse( dst : Reg, offset : Int );
	OJNull( dst : Reg, offset : Int );
	OJNotNull( dst : Reg, offset : Int );
	OJSLt( dst : Reg, a : Reg, offset : Int );
	OJSGte( dst : Reg, a : Reg, offset : Int );
	OJSGt( dst : Reg, a : Reg, offset : Int );
	OJSLte( dst : Reg, a : Reg, offset : Int );
	OJULt( dst : Reg, a : Reg, offset : Int );
	OJUGte( dst : Reg, a : Reg, offset : Int );
	OJEq( dst : Reg, a : Reg, offset : Int );
	OJNotEq( dst : Reg, a : Reg, offset : Int );
	OJAlways( offset : Int );
	OToDyn( dst : Reg, a : Reg );
	OToSFloat( dst : Reg, a : Reg );
	OToUFloat( dst : Reg, a : Reg );
	OToInt( dst : Reg, a : Reg );
	OSafeCast( dst : Reg, a : Reg );
	OUnsafeCast( dst : Reg, a : Reg );
	OToVirtual( dst : Reg, a : Reg );
	OLabel;
	ORet( dst : Reg );
	OThrow( dst : Reg );
	ORethrow( dst : Reg );
	OSwitch( dst : Reg, cases : Array<Int>, end : Int );
	ONullCheck( dst : Reg );
	OTrap( dst : Reg, end : Int );
	OEndTrap( last : Bool );
	OGetUI8( dst : Reg, a : Reg, b : Reg );
	OGetUI16( dst : Reg, a : Reg, b : Reg );
	OGetI32( dst : Reg, a : Reg, b : Reg );
	OGetF32( dst : Reg, a : Reg, b : Reg );
	OGetF64( dst : Reg, a : Reg, b : Reg );
	OGetArray( dst : Reg, a : Reg, b : Reg );
	OSetUI8( dst : Reg, a : Reg, b : Reg );
	OSetUI16( dst : Reg, a : Reg, b : Reg );
	OSetI32( dst : Reg, a : Reg, b : Reg );
	OSetF32( dst : Reg, a : Reg, b : Reg );
	OSetF64( dst : Reg, a : Reg, b : Reg );
	OSetArray( dst : Reg, a : Reg, b : Reg );
	ONew( dst : Reg );
	OArraySize( dst : Reg, a : Reg );
	OType( dst : Reg, t : Index<HLType> );
	OGetType( dst : Reg, a : Reg );
	OGetTID( dst : Reg, a : Reg );
	ORef( dst : Reg, a : Reg );
	OUnref( dst : Reg, a : Reg );
	OSetref( dst : Reg, a : Reg );
	OMakeEnum( dst : Reg, i : Index<EnumConstruct>, a : Array<Reg> );
	OEnumAlloc( dst : Reg, i : Index<EnumConstruct> );
	OEnumIndex( dst : Reg, a : Reg );
	OEnumField( dst : Reg, a : Reg, i : Index<EnumConstruct>, param : Int );
	OSetEnumField( dst : Reg, param : Int, a : Reg );
}

class HLReader {

	var version : Int;
	var i : haxe.io.Input;
	var strings : Array<String>;
	var types : Array<HLType>;
	var debugFiles : Array<String>;
	var flags : haxe.EnumFlags<HLCodeFlag>;
	var args1 : Array<Dynamic>;
	var args2 : Array<Dynamic>;
	var args3 : Array<Dynamic>;
	var args4 : Array<Dynamic>;
	var readCode : Bool;

	public function new( readCode = true ) {
		this.readCode = readCode;
		args1 = [0];
		args2 = [0,0];
		args3 = [0, 0, 0];
		args4 = [0, 0, 0, 0];
	}

	inline function READ() {
		return i.readByte();
	}

	function index() {
		var b = READ();
		if( (b & 0x80) == 0 )
			return b & 0x7F;
		if( (b & 0x40) == 0 ) {
			var v = READ() | ((b & 31) << 8);
			return (b & 0x20) == 0 ? v : -v;
		}
		var c = READ();
		var d = READ();
		var e = READ();
		var v = ((b & 31) << 24) | (c << 16) | (d << 8) | e;
		return (b & 0x20) == 0 ? v : -v;
	}

	function uindex() {
		var v = index();
		if( v < 0 ) throw "Expected uindex but got " + v;
		return v;
	}

	function readStrings(n) {
		var size = i.readInt32();
		var data = i.read(size);
		var out = [];
		var pos = 0;
		for( _ in 0...n ) {
			var sz = uindex();
			var str = data.getString(pos, sz);
			pos += sz + 1;
			out.push(str);
		}
		return out;
	}

	function getString() {
		var i = index();
		var s = strings[i];
		if( s == null ) throw "No string @" + i;
		return s;
	}

	function getType() {
		var i = index();
		var t = types[i];
		if( t == null ) throw "No type @" + i;
		return t;
	}

	function readType() {
		switch( READ() ) {
		case 0:
			return HVoid;
		case 1:
			return HUi8;
		case 2:
			return HUi16;
		case 3:
			return HI32;
		case 4:
			return HF32;
		case 5:
			return HF64;
		case 6:
			return HBool;
		case 7:
			return HBytes;
		case 8:
			return HDyn;
		case 9:
			return HFun({ args : [for( i in 0...READ() ) HAt(uindex())], ret : HAt(uindex()) });
		case 10:
			var p : ObjPrototype = {
				name : getString(),
				tsuper : null,
				fields : [],
				proto : [],
				globalValue : null,
			};
			var sup = index();
			if( sup >= 0 ) {
				p.tsuper = types[sup];
				if( p.tsuper == null ) throw "assert";
			}
			p.globalValue = uindex() - 1;
			if( p.globalValue < 0 ) p.globalValue = null;
			var nfields = uindex();
			var nproto = uindex();
			p.fields = [for( i in 0...nfields ) { name : getString(), t : HAt(uindex()) }];
			p.proto = [for( i in 0...nproto ) { name : getString(), findex : uindex(), pindex : index() }];
			return HObj(p);
		case 11:
			return HArray;
		case 12:
			return HType;
		case 13:
			return HRef(getType());
		case 14:
			return HVirtual([for( i in 0...uindex() ) { name : getString(), t : HAt(uindex()) }]);
		case 15:
			return HDynObj;
		case 16:
			return HAbstract(getString());
		case 17:
			var name = getString();
			var global = uindex() - 1;
			return HEnum({
				name : name,
				globalValue : global < 0 ? null : global,
				constructs : [for( i in 0...uindex() ) { name : getString(), params : [for( i in 0...uindex() ) HAt(uindex())] }],
			});
		case 18:
			return HNull(getType());
		case x:
			throw "Unsupported type value " + x;
		}
	}

	function fixType( t : HLType ) {
		switch( t ) {
		case HAt(i):
			return types[i];
		default:
			return t;
		}
	}

	function readFunction() : HLFunction {
		var t = getType();
		var idx = uindex();
		var nregs = uindex();
		var nops = uindex();
		return {
			t : t,
			findex : idx,
			regs : [for( i in 0...nregs ) getType()],
			ops : readCode ? [for( i in 0...nops ) readOp()] : skipOps(nops),
			debug : readDebug(nops),
		};
	}

	function skipOps(nops) {
		for( i in 0...nops ) {
			var op = READ();
			var args = OP_ARGS[op];
			if( args < 0 ) {
				switch( op ) {
				case 29, 30, 31, 32, 93:
					index();
					index();
					for( i in 0...uindex() ) index();
				case 69:
					// OSwitch
					uindex();
					for( i in 0...uindex() ) uindex();
					uindex();
				default:
					throw "Don't know how to handle opcode " + op + "("+Type.getEnumConstructs(HLOpcode)[op]+")";
				}
			} else {
				for( i in 0...args )
					index();
			}
		}
		return null;
	}

	function readOp() {
		var op = READ();
		var args = OP_ARGS[op];
		switch( args ) {
		case -1:
			switch( op ) {
			case 29, 30, 31, 32, 93:
				args3[0] = index();
				args3[1] = index();
				args3[2] = [for( i in 0...uindex() ) index()];
				return Type.createEnumIndex(HLOpcode, op, args3);
			case 69:
				// OSwitch
				args3[0] = uindex();
				args3[1] = [for( i in 0...uindex() ) uindex()];
				args3[2] = uindex();
				return Type.createEnumIndex(HLOpcode, op, args3);
			default:
				throw "Don't know how to handle opcode " + op + "("+Type.getEnumConstructs(HLOpcode)[op]+")";
			}
		case 1:
			args1[0] = index();
			return Type.createEnumIndex(HLOpcode, op, args1);
		case 2:
			args2[0] = index();
			args2[1] = index();
			return Type.createEnumIndex(HLOpcode, op, args2);
		case 3:
			args3[0] = index();
			args3[1] = index();
			args3[2] = index();
			return Type.createEnumIndex(HLOpcode, op, args3);
		case 4:
			args4[0] = index();
			args4[1] = index();
			args4[2] = index();
			args4[3] = index();
			return Type.createEnumIndex(HLOpcode, op, args4);
		default:
			return Type.createEnumIndex(HLOpcode, op, [for( i in 0...args ) index()]);
		}
	}

	function readDebug( nops ) {
		if( !flags.has(HasDebug) )
			return null;
		var curfile = -1, curline = 0;
		var debug = [];
		var i = 0;
		while( i < nops ) {
			var c = READ();
			if( (c & 1) != 0 ) {
				c >>= 1;
				curfile = (c << 8) | READ();
				if( curfile >= debugFiles.length )
					throw "Invalid debug file";
			} else if( (c & 2) != 0 ) {
				var delta = c >> 6;
				var count = (c >> 2) & 15;
				if( i + count > nops )
					throw "Outside range";
				while( count-- > 0 ) {
					debug[i<<1] = curfile;
					debug[(i<<1)|1] = curline;
					i++;
				}
				curline += delta;
			} else if( (c & 4) != 0 ) {
				curline += c >> 3;
				debug[i<<1] = curfile;
				debug[(i<<1)|1] = curline;
				i++;
			} else {
				var b2 = READ();
				var b3 = READ();
				curline = (c >> 3) | (b2 << 5) | (b3 << 13);
				debug[i<<1] = curfile;
				debug[(i<<1)|1] = curline;
				i++;
			}
		}
		return debug;
	}

	public function read( i : haxe.io.Input ) : HLCode {
		this.i = i;
		if( i.readString(3) != "HLB" )
			throw "Invalid HL file";
		version = READ();
		if( version > 1 )
			throw "HL Version " + version + " is not supported";
		flags = haxe.EnumFlags.ofInt(uindex());
		var nints = uindex();
		var nfloats = uindex();
		var nstrings = uindex();
		var ntypes = uindex();
		var nglobals = uindex();
		var nnatives = uindex();
		var nfunctions = uindex();
		var entryPoint = uindex();
		var ints = [for( _ in 0...nints ) i.readInt32()];
		var floats = [for( _ in 0...nfloats ) i.readDouble()];
		strings = readStrings(nstrings);
		debugFiles = null;
		if( flags.has(HasDebug) )
			debugFiles = readStrings(uindex());
		types = [];
		for( i in 0...ntypes )
			types[i] = readType();
		for( i in 0...ntypes )
			switch( types[i] ) {
			case HFun(f):
				for( i in 0...f.args.length ) f.args[i] = fixType(f.args[i]);
				f.ret = fixType(f.ret);
			case HObj(p):
				for( f in p.fields )
					f.t = fixType(f.t);
			case HVirtual(fl):
				for( f in fl )
					f.t = fixType(f.t);
			case HEnum(e):
				for( c in e.constructs )
					for( i in 0...c.params.length )
						c.params[i] = fixType(c.params[i]);
			default:
			}
		return {
			version : version,
			flags : flags,
			ints : ints,
			floats : floats,
			strings : strings,
			debugFiles : debugFiles,
			types : types,
			entryPoint : entryPoint,
			globals : [for( i in 0...nglobals ) getType()],
			natives : [for( i in 0...nnatives ) { lib : getString(), name : getString(), t : getType(), findex : uindex() }],
			functions : [for( i in 0...nfunctions ) readFunction()],
		};
	}


	static var OP_ARGS = [
		2,
		2,
		2,
		2,
		2,
		2,
		1,

		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,

		2,
		2,
		1,
		1,

		2,
		3,
		4,
		5,
		6,
		-1,
		-1,
		-1,
		-1,

		2,
		3,
		3,

		 2,
		2,
		3,
		3,
		2,
		2,
		3,
		3,
		3,

		2,
		2,
		2,
		2,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		1,

		2,
		2,
		2,
		2,
		2,
		2,
		2,

		0,
		1,
		1,
		1,
		-1,
		1,
		2,
		1,

		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,
		3,

		1,
		2,
		2,
		2,
		2,

		2,
		2,
		2,

		-1,
		2,
		2,
		4,
		3,
	];

}

class HLTools {

	public static function isDynamic( t : HLType ) {
		return switch( t ) {
		case HVoid, HUi8, HUi16, HI32, HF32, HF64, HBool, HAt(_):
			false;
		case HBytes, HType, HRef(_), HAbstract(_), HEnum(_):
			false;
		case HDyn, HFun(_), HObj(_), HArray, HVirtual(_), HDynObj, HNull(_):
			true;
		}
	}

	public static function isPtr( t : HLType ) {
		return switch( t ) {
		case HVoid, HUi8, HUi16, HI32, HF32, HF64, HBool, HAt(_):
			false;
		case HBytes, HType, HRef(_), HAbstract(_), HEnum(_):
			true;
		case HDyn, HFun(_), HObj(_), HArray, HVirtual(_), HDynObj, HNull(_):
			true;
		}
	}

	public static function hash( name : String ) {
		var h = 0;
		for( i in 0...name.length )
			h = 223 * h + StringTools.fastCodeAt(name,i);
		h %= 0x1FFFFF7B;
		return h;
	}

}