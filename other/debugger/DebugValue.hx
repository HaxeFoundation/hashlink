import format.hl.Data;
using format.hl.Tools;
import DebugApi.Pointer;

enum DebugValueRepr {
	VUndef;
	VNull;
	VInt( i : Int );
	VFloat( v : Float );
	VBool( b : Bool );
	VPointer( v : Pointer );
	VString( v : String );
	VClosure( p : DebugFunRepr, d : DebugValue );
	VFunction( p : DebugFunRepr );
}

enum DebugFunRepr {
	FUnknown( p : Pointer );
	FIndex( i : Int );
}

typedef DebugValue = { v : DebugValueRepr, t : HLType };


typedef DebugObj = {
	var name : String;
	var size :  Int;
	var fieldNames : Array<String>;
	var parent : DebugObj;
	var fields : Map<String,{
		var name : String;
		var t : HLType;
		var offset : Int;
	}>;
}


enum DebugFlag {
	Is64; // runs in 64 bit mode
	Bool4; // bool = 4 bytes (instead of 1)
}

class DebugValueReader {

	var code : format.hl.Data;
	var protoCache : Map<String,DebugObj>;
	var flags : haxe.EnumFlags<DebugFlag>;
	var ptrSize : Int;

	function typeSize( t : HLType ) {
		return switch( t ) {
		case HVoid: 0;
		case HUi8: 1;
		case HUi16: 2;
		case HI32, HF32: 4;
		case HF64: 8;
		case HBool:
			return flags.has(Bool4) ? 4 : 1;
		default:
			return flags.has(Is64) ? 8 : 4;
		}
	}

	inline function align( v : Int, size : Int ) {
		var d = v & (size - 1);
		if( d != 0 ) v += size - d;
		return v;
	}

	function getObjectProto( o : ObjPrototype ) : DebugObj {

		var p = protoCache.get(o.name);
		if( p != null )
			return p;

		var parent = o.tsuper == null ? null : switch( o.tsuper ) { case HObj(o): getObjectProto(o); default: throw "assert"; };
		var size = parent == null ? ptrSize : parent.size;
		var fields = parent == null ? new Map() : [for( k in parent.fields.keys() ) k => parent.fields.get(k)];

		for( f in o.fields ) {
			var sz = typeSize(f.t);
			size = align(size, sz);
			fields.set(f.name, { name : f.name, t : f.t, offset : size });
			size += sz;
		}

		p = {
			name : o.name,
			size : size,
			parent : parent,
			fields : fields,
			fieldNames : [for( o in o.fields ) o.name],
		};
		protoCache.set(p.name, p);

		return p;
	}


	public function valueStr( v : DebugValue ) {
		return switch( v.v ) {
		case VUndef: "undef"; // null read / outside bounds
		case VNull: "null";
		case VInt(i): "" + i;
		case VFloat(v): "" + v;
		case VBool(b): b?"true":"false";
		case VPointer(v): v.toString();
		case VString(s): s;
		case VClosure(f, d): funStr(f) + "[" + valueStr(d) + "]";
		case VFunction(f): funStr(f);
		}
	}

	function funStr( f : DebugFunRepr ) {
		return switch( f ) {
		case FUnknown(p): "fun(" + p.toString() + ")";
		case FIndex(i): "fun(" + getFunctionName(i) + ")";
		}
	}

	function getFunctionName( idx : Int ) {
		var s = resolveSymbol(idx, 0);
		return s.file+":" + s.line;
	}

	function resolveSymbol( fidx : Int, fpos : Int ) {
		var f = code.functions[fidx];
		var fid = f.debug[fpos << 1];
		var fline = f.debug[(fpos << 1) + 1];
		return { file : code.debugFiles[fid], line : fline };
	}

	function typeStr( t : HLType ) {
		inline function fstr(t) {
			return switch(t) {
			case HFun(_): "(" + typeStr(t) + ")";
			default: typeStr(t);
			}
		};
		return switch( t ) {
		case HVoid: "Void";
		case HUi8: "hl.UI8";
		case HUi16: "hl.UI16";
		case HI32: "Int";
		case HF32: "Single";
		case HF64: "Float";
		case HBool: "Bool";
		case HBytes: "hl.Bytes";
		case HDyn: "Dynamic";
		case HFun(f):
			if( f.args.length == 0 ) "Void -> " + fstr(f.ret) else [for( a in f.args ) fstr(a)].join(" -> ") + " -> " + fstr(f.ret);
		case HObj(o):
			o.name;
		case HArray:
			"hl.NativeArray";
		case HType:
			"hl.Type";
		case HRef(t):
			"hl.Ref<" + typeStr(t) + ">";
		case HVirtual(fl):
			"{ " + [for( f in fl ) f.name+" : " + typeStr(f.t)].join(", ") + " }";
		case HDynObj:
			"hl.DynObj";
		case HAbstract(name):
			"hl.NativeAbstract<" + name+">";
		case HEnum(e):
			e.name;
		case HNull(t):
			"Null<" + typeStr(t) + ">";
		case HAt(_):
			throw "assert";
		}
	}

	function readField( v : DebugValue, name : String ) : DebugValue {
		var ptr = switch( v.v ) {
		case VUndef, VNull: null;
		case VPointer(p): p;
		default:
			return null;
		}
		switch( v.t ) {
		case HObj(o):
			var f = getObjectProto(o).fields.get(name);
			if( f == null )
				return null;
			return ptr == null ? { v : VUndef, t : f.t } : readVal(ptr.offset(f.offset), f.t);
		case HVirtual(fl):
			for( i in 0...fl.length )
				if( fl[i].name == name ) {
					var t = fl[i].t;
					if( ptr == null )
						return { v : VUndef, t : t };
					var addr = readMemPointer(ptr.offset(ptrSize * (3 + i)));
					if( addr != null )
						return readVal(addr, t);
					var realValue = readMemPointer(ptr.offset(ptrSize));
					if( realValue == null )
						return null;
					return readField({ v : VPointer(realValue), t : HDyn }, name);
				}
			return null;
		case HDynObj:
			if( ptr == null )
				return { v : VUndef, t : HDyn };
			var lookup = readMemPointer(ptr).offset(typeStructSize());
			var data = readMemPointer(ptr.offset(ptrSize));
			var nfields = readI32(ptr.offset(ptrSize * 2));
			// lookup, similar to hl_lookup_find
			var hash = name.hash();
			var min = 0;
			var max = nfields;
			while( min < max ) {
				var mid = (min + max) >> 1;
				var lid = lookup.offset(mid * (ptrSize + 8));
				var h = readI32(lid.offset(ptrSize)/*hashed_name*/);
				if( h < hash )
					min = mid + 1;
				else if( h > hash )
					max = mid;
				else {
					var t = readMemType(lid);
					var offset = readI32(lid.offset(ptrSize + 4));
					return readVal(data.offset(offset), t);
				}
			}
			return null;
		case HDyn:
			if( ptr == null )
				return { v : VUndef, t  : HDyn };
			return readField({ v : v.v, t : readMemType(ptr) }, name);
		default:
			return null;
		}
	}

	function readMemType( p : Pointer ) {
		var p = readMemPointer(p);
		switch( readI32(p) ) {
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
			// HFun
			throw "TODO";
		case 11:
			return HArray;
		case 12:
			return HType;
		case 13:
			return HRef(readMemType(p.offset(ptrSize)));
		case 15:
			return HDynObj;
		case 10, 14, 16, 17: // HObj, HVirtual, HAbstract, HEnum
			return typeFromAddr(p);
		case 18:
			return HNull(readMemType(p.offset(ptrSize)));
		case x:
			throw "Unknown type #" + x;
		}
	}

	public function readVal( p : Pointer, t : HLType ) : DebugValue {
		var v = switch( t ) {
		case HVoid:
			VNull;
		case HUi8:
			var m = readMem(p, 1);
			VInt(m.getUI8(0));
		case HUi16:
			var m = readMem(p, 2);
			VInt(m.getUI16(0));
		case HI32:
			VInt(readI32(p));
		case HF32:
			var m = readMem(p, 4);
			VFloat(m.getF32(0));
		case HF64:
			var m = readMem(p, 8);
			VFloat(m.getF64(0));
		case HBool:
			var m = readMem(p, 1);
			VBool(m.getUI8(0) != 0);
		default:
			p = readMemPointer(p);
			return valueCast(p, t);
		};
		return { v : v, t : t };
	}

	function valueCast( p : Pointer, t : HLType ) {
		if( p == null )
			return { v : VNull, t : t };
		var v = VPointer(p);
		switch( t ) {
		case HObj(o):
			t = readMemType(p);
			switch( o.name ) {
			case "String":
				var bytes = readMemPointer(p.offset(ptrSize));
				var length = readI32(p.offset(ptrSize * 2));
				var mem = readMem(bytes, (length + 1) * 2);
				v = VString(@:privateAccess String.fromUCS2(mem));
			default:
			}
		case HVirtual(_):
			var v = readMemPointer(p.offset(ptrSize));
			if( v != null )
				return valueCast(v, HDyn);
		case HDyn:
			var t = readMemType(p);
			if( t.isDynamic() )
				return valueCast(p, t);
			v = readVal(p.offset(8), t).v;
		case HNull(t):
			v = readVal(p.offset(8), t).v;
		case HRef(t):
			v = readVal(p, t).v;
		case HFun(_):
			var funPtr = readMemPointer(p.offset(ptrSize));
			var hasValue = readI32(p.offset(ptrSize * 2));
			var fidx = functionFromAddr(funPtr);
			var fval = fidx == null ? FUnknown(funPtr) : FIndex(fidx);
			if( hasValue == 1 ) {
				var value = readVal(p.offset(ptrSize * 3), HDyn);
				v = VClosure(fval, value);
			} else
				v = VFunction(fval);
		default:
		}
		return { v : v, t : t };
	}

	function readMemPointer( p : Pointer ) {
		var m = readMem(p, ptrSize);
		if( flags.has(Is64) )
			return Pointer.make(m.getI32(0), m.getI32(4));
		return Pointer.make(m.getI32(0), 0);
	}

	inline function typeStructSize() {
		return ptrSize * 3;
	}

	function readI32( p : Pointer ) {
		return readMem(p, 4).getI32(0);
	}

	function readMem( p : Pointer, size : Int ) : hl.Bytes {
		throw "TODO";
		return null;
	}

	function typeFromAddr( p : Pointer ) : HLType {
		throw "assert";
		return null;
	}

	function functionFromAddr( p : Pointer ) : Null<Int> {
		throw "assert";
		return null;
	}

}

