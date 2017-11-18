package hld;
import hld.Value;

class Eval {

	var align : Align;
	var api : Api;
	var jit : JitInfo;
	var module : Module;
	var currentFunIndex : Int;
	var currentCodePos : Int;
	var currentEbp : Pointer;
	var sizeofVArray : Int;

	public var maxStringRec : Int = 3;
	public var maxArrLength : Int = 10;

	static var HASH_PREFIX = "$_h$";

	public function new(module,api,jit) {
		this.module = module;
		this.api = api;
		this.jit = jit;
		this.align = jit.align;
		sizeofVArray = align.ptr * 2 + align.typeSize(HI32) * 2;
	}

	public function eval( name : String, funIndex : Int, codePos : Int, ebp : Pointer ) {
		if( name == null || name == "" )
			return null;
		currentFunIndex = funIndex;
		currentCodePos = codePos;
		currentEbp = ebp;

		var path = name.split(".");
		var v;

		// TODO : look in locals

		if( ~/^\$[0-9]+$/.match(path[0]) ) {

			// register
			v = readReg(Std.parseInt(path.shift().substr(1)));
			if( v == null )
				return null;

		} else {

			// global
			var g = module.resolveGlobal(path);
			if( g == null )
				return null;

			v = readVal(jit.globals.offset(g.offset), g.type);
		}

		for( p in path )
			v = readField(v, p);
		return v;
	}

	function escape( s : String ) {
		s = s.split("\\").join("\\\\");
		s = s.split("\n").join("\\n");
		s = s.split("\r").join("\\r");
		s = s.split("\t").join("\\t");
		s = s.split('"').join('\\"');
		return s;
	}

	public function valueStr( v : Value ) {
		if( maxStringRec < 0 && v.t.isPtr() )
			return "<...>";
		maxStringRec--;
		var str = switch( v.v ) {
		case VUndef: "undef"; // null read / outside bounds
		case VNull: "null";
		case VInt(i): "" + i;
		case VFloat(v): "" + v;
		case VBool(b): b?"true":"false";
		case VPointer(v): v.toString();
		case VString(s): "\"" + escape(s) + "\"";
		case VClosure(f, d): funStr(f) + "[" + valueStr(d) + "]";
		case VFunction(f): funStr(f);
		case VArray(_, length, read):
			if( length <= maxArrLength )
				[for(i in 0...length) valueStr(read(i))].toString();
			else {
				var arr = [for(i in 0...maxArrLength) valueStr(read(i))];
				arr.push("...");
				arr.toString()+":"+length;
			}
		case VMap(_, nkeys, readKey, readValue):
			var max = nkeys < maxArrLength ? nkeys : maxArrLength;
			var content = [for( i in 0...max ) { var k = readKey(i); valueStr(k) + "=>" + valueStr(readValue(i)); }];
			if( max != nkeys ) {
				content.push("...");
				content.toString() + ":" + nkeys;
			} else
				content.toString();
		case VType(t):
			t.toString();
		case VEnum(c, values):
			if( values.length == 0 )
				c
			else
				c + "(" + [for( v in values ) valueStr(v)].join(", ") + ")";
		}
		maxStringRec++;
		return str;
	}

	function funStr( f : FunRepr ) {
		return switch( f ) {
		case FUnknown(p): "fun(" + p.toString() + ")";
		case FIndex(i): "fun(" + getFunctionName(i) + ")";
		}
	}

	function getFunctionName( idx : Int ) {
		var s = module.resolveSymbol(idx, 0);
		return s.file+":" + s.line;
	}

	function readReg(index) {
		var r = module.getFunctionRegs(currentFunIndex)[index];
		if( r == null )
			return null;
		return readVal(currentEbp.offset(r.offset), r.t);
	}

	public function readVal( p : Pointer, t : HLType ) : Value {
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
		case HI64:
			throw "TODO:readI64";
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
			p = readPointer(p);
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
			t = readType(p);
			switch( t ) {
			case HObj(o2): o = o2;
			default:
			}
			switch( o.name ) {
			case "String":
				var bytes = readPointer(p.offset(align.ptr));
				var length = readI32(p.offset(align.ptr * 2));
				var str = readUCS2(bytes, length);
				v = VString(str);
			case "hl.types.ArrayObj":
				var length = readI32(p.offset(align.ptr));
				var nativeArray = readPointer(p.offset(align.ptr * 2));
				var type = readType(nativeArray.offset(align.ptr));
				v = VArray(type, length, function(i) return readVal(nativeArray.offset(sizeofVArray + i * align.ptr), type));
			case "hl.types.ArrayBytes_Int":
				v = makeArrayBytes(p, HI32);
			case "hl.types.ArrayBytes_Float":
				v = makeArrayBytes(p, HF64);
			case "hl.types.ArrayBytes_Single":
				v = makeArrayBytes(p, HF32);
			case "hl.types.ArrayBytes_hl_UI16":
				v = makeArrayBytes(p, HUi16);
			case "hl.types.ArrayDyn":
				// hide implementation details, substitute underlying array
				v = readField({ v : v, t : t }, "array").v;
			//case "haxe.ds.IntMap":
			//	v = makeMap(readPointer(p.offset(align.ptr)), HI32);
			default:
			}
		case HVirtual(_):
			var v = readPointer(p.offset(align.ptr));
			if( v != null )
				return valueCast(v, HDyn);
		case HDyn:
			var t = readType(p);
			if( t.isDynamic() )
				return valueCast(p, t);
			v = readVal(p.offset(8), t).v;
		case HNull(t):
			v = readVal(p.offset(8), t).v;
		case HRef(t):
			v = readVal(p, t).v;
		case HFun(_):
			var funPtr = readPointer(p.offset(align.ptr));
			var hasValue = readI32(p.offset(align.ptr * 2));
			var fidx = jit.functionFromAddr(funPtr);
			var fval = fidx == null ? FUnknown(funPtr) : FIndex(fidx);
			if( hasValue == 1 ) {
				var value = readVal(p.offset(align.ptr * 3), HDyn);
				v = VClosure(fval, value);
			} else
				v = VFunction(fval);
		case HType:
			v = VType(readType(p,true));
		case HBytes:
			// maybe try reading string data ?
		case HEnum(e):
			var index = readI32(p.offset(align.ptr));
			var c = module.getEnumProto(e)[index];
			v = VEnum(c.name,[for( a in c.params ) readVal(p.offset(a.offset),a.t)]);
		default:
		}
		return { v : v, t : t };
	}

	function makeArrayBytes( p : Pointer, t : HLType ) {
		var length = readI32(p.offset(align.ptr));
		var bytes = readPointer(p.offset(align.ptr * 2));
		var size = align.typeSize(t);
		return VArray(t, length, function(i) return readVal(bytes.offset(i * size), t));
	}

	public function getFields( v : Value ) : Array<String> {
		var ptr = switch( v.v ) {
		case VPointer(p): p;
		default:
			return null;
		}
		switch( v.t ) {
		case HObj(o):
			function getRec(o:format.hl.Data.ObjPrototype) {
				var parent = o.tsuper == null ? [] : getRec(switch( o.tsuper ) { case HObj(o): o; default: throw "assert"; });
				return parent.concat(module.getObjectProto(o).fieldNames);
			}
			return getRec(o);
		case HVirtual(fields):
			return [for( f in fields ) f.name];
		case HDynObj:
			var lookup = readPointer(ptr.offset(align.ptr));
			var nfields = readI32(ptr.offset(align.ptr * 4));
			var fields = [];
			for( i in 0...nfields ) {
				var l = lookup.offset(i * (align.ptr + 8));
				var h = readI32(l.offset(align.ptr)); // hashed_name
				var name = module.reverseHash(h);
				if( name == null ) name = HASH_PREFIX + h;
				fields.push(name);
			}
			return fields;
		default:
			return null;
		}
	}

	public function readField( v : Value, name : String ) : Value {
		var ptr = switch( v.v ) {
		case VUndef, VNull: null;
		case VPointer(p): p;
		default:
			return null;
		}
		switch( v.t ) {
		case HObj(o):
			var f = module.getObjectProto(o).fields.get(name);
			if( f == null )
				return null;
			return ptr == null ? { v : VUndef, t : f.t } : readVal(ptr.offset(f.offset), f.t);
		case HVirtual(fl):
			for( i in 0...fl.length )
				if( fl[i].name == name ) {
					var t = fl[i].t;
					if( ptr == null )
						return { v : VUndef, t : t };
					var addr = readPointer(ptr.offset(align.ptr * (3 + i)));
					if( addr != null )
						return readVal(addr, t);
					var realValue = readPointer(ptr.offset(align.ptr));
					if( realValue == null )
						return null;
					var v = readField({ v : VPointer(realValue), t : HDyn }, name);
					if( v == null )
						return { v : VUndef, t : t };
					return v;
				}
			return null;
		case HDynObj:
			if( ptr == null )
				return { v : VUndef, t : HDyn };
			var lookup = readPointer(ptr.offset(align.ptr * 1));
			var raw_data = readPointer(ptr.offset(align.ptr * 2));
			var values = readPointer(ptr.offset(align.ptr * 3));
			var nfields = readI32(ptr.offset(align.ptr * 4));
			// lookup, similar to hl_lookup_find
			var hash = StringTools.startsWith(name,HASH_PREFIX) ? Std.parseInt(name.substr(HASH_PREFIX.length)) : name.hash();
			var min = 0;
			var max = nfields;
			while( min < max ) {
				var mid = (min + max) >> 1;
				var lid = lookup.offset(mid * (align.ptr + 8));
				var h = readI32(lid.offset(align.ptr)); // hashed_name
				if( h < hash )
					min = mid + 1;
				else if( h > hash )
					max = mid;
				else {
					var t = readType(lid);
					var offset = readI32(lid.offset(align.ptr + 4));
					return readVal(t.isPtr() ? values.offset(offset * align.ptr) : raw_data.offset(offset), t);
				}
			}
			return null;
		case HDyn:
			if( ptr == null )
				return { v : VUndef, t  : HDyn };
			return readField({ v : v.v, t : readType(ptr) }, name);
		default:
			return null;
		}
	}

	function readPointer( p : Pointer ) {
		if( align.is64 ) {
			var m = readMem(p, 8);
			return m.getPointer(0,align);
		}
		return Pointer.make(readI32(p), 0);
	}

	function readMem( p : Pointer, size : Int ) {
		var b = new Buffer(size);
		if( !api.read(p, b, size) )
			throw "Failed to read @" + p.toString() + ":" + size;
		return b;
	}

	function readUCS2( ptr : Pointer, length : Int ) {
		var mem = readMem(ptr, (length + 1) << 2);
		return mem.readStringUCS2(0,length);
	}

	function readI32( p : Pointer ) {
		return readMem(p, 4).getI32(0);
	}

	function readType( p : Pointer, direct = false ) {
		if( !direct )
			p = readPointer(p);
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
			return HI64;
		case 5:
			return HF32;
		case 6:
			return HF64;
		case 7:
			return HBool;
		case 8:
			return HBytes;
		case 9:
			return HDyn;
		case 10:
			// HFun
			throw "TODO";
		case 12:
			return HArray;
		case 13:
			return HType;
		case 14:
			return HRef(readType(p.offset(align.ptr)));
		case 16:
			return HDynObj;
		case 11, 15, 17, 18: // HObj, HVirtual, HAbstract, HEnum
			return typeFromAddr(p);
		case 19:
			return HNull(readType(p.offset(align.ptr)));
		case x:
			throw "Unknown type #" + x;
		}
	}

	inline function typeStructSize() {
		return align.ptr * 4;
	}

	function typeFromAddr( p : Pointer ) : HLType {
		var tid = Std.int( p.sub(@:privateAccess jit.allTypes) / typeStructSize() );
		return module.code.types[tid];
	}

}

