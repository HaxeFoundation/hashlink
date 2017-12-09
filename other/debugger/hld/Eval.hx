package hld;
import hld.Value;
import format.hl.Data.HLType;

class Eval {

	var align : Align;
	var api : Api;
	var jit : JitInfo;
	var module : Module;
	var sizeofVArray : Int;
	var parser : hscript.Parser;

	var t_string : HLType;

	var funIndex : Int;
	var codePos : Int;
	var ebp : Pointer;

	public var maxStringRec : Int = 3;
	public var maxArrLength : Int = 10;

	static var HASH_PREFIX = "$_h$";

	public function new(module,api,jit) {
		this.module = module;
		this.api = api;
		this.jit = jit;
		this.align = jit.align;
		parser = new hscript.Parser();
		sizeofVArray = align.ptr * 2 + align.typeSize(HI32) * 2;
		for( t in module.code.types )
			switch( t ) {
			case HObj(o):
				switch( o.name ) {
				case "String":
					t_string = t;
				default:
				}
			default:
			}
	}

	public function eval( expr : String, funIndex : Int, codePos : Int, ebp : Pointer ) {
		if( expr == null || expr == "" )
			return null;
		this.funIndex = funIndex;
		this.codePos = codePos;
		this.ebp = ebp;

		var expr = try parser.parseString(expr) catch( e : hscript.Expr.Error ) throw hscript.Printer.errorToString(e);
		return evalExpr(expr);
	}

	function evalExpr( e : hscript.Expr ) : Value {
		switch( e ) {
		case EConst(c):
			switch( c ) {
			case CInt(v):
				return { v : VInt(v), t : HI32 };
			case CFloat(f):
				return { v : VFloat(f), t : HF64 };
			case CString(s):
				return { v : VString(s, null), t : t_string };
			}
		case EIdent(i):
			var v = getVar(i);
			if( v == null )
				throw "Unknown identifier " + i;
			return v;
		case EArray(v, i):
			var v = evalExpr(v);
			var i = evalExpr(i);
			switch( v.v ) {
			case VArray(t, len, read, _):
				var i = toInt(i);
				return i < 0 || i >= len ? defVal(t) : read(i);
			default:
			}
			throw "Can't access " + valueStr(v) + "[" + valueStr(i) + "]";
		case EArrayDecl(vl):
			var vl = [for( v in vl ) evalExpr(v)];
			return { v : VArray(HDyn, vl.length, function(i) return vl[i], null), t : HDyn };
		case EBinop(op, e1, e2):
			switch( op ) {
			case "&&":
				return { v : VBool(toBool(evalExpr(e1)) && toBool(evalExpr(e2))), t : HBool };
			case "||":
				return { v : VBool(toBool(evalExpr(e1)) || toBool(evalExpr(e2))), t : HBool };
			default:
				return evalBinop(op, evalExpr(e1), evalExpr(e2));
			}
		case EBlock(el):
			var v = { v : VNull, t : HDyn };
			for( e in el )
				v = evalExpr(e);
			return v;
		case EField(e, f):
			var e = e;
			var path = [f];
			var v = null;
			while( true ) {
				switch( e ) {
				case EIdent(i):
					path.unshift(i);
					v = evalPath(path);
					break;
				case EField(e2, f):
					path.unshift(f);
					e = e2;
				default:
					v = evalExpr(e);
					break;
				}
			}
			for( f in path )
				v = readField(v, f);
			return v;
		case EIf(econd, e1, e2), ETernary(econd, e1, e2):
			if( toBool(evalExpr(econd)) )
				return evalExpr(e1);
			return e2 == null ? { v : VNull, t : HDyn } : evalExpr(e2);
		case EParent(e):
			return evalExpr(e);
		case EThrow(e):
			throw valueStr(evalExpr(e));
		case EUnop(op, prefix, e):
			return evalUnop(op, prefix, evalExpr(e));
		case EMeta(_, _, e):
			return evalExpr(e);
		case EObject(_), ENew(_), ECall(_), EFor(_), ETry(_), EReturn(_), EBreak, EContinue, EDoWhile(_), EFunction(_), EVar(_), EWhile(_), ESwitch(_):
			throw "Unsupported expression `" + hscript.Printer.toString(e) + "`";
		}
	}

	function evalBinop(op, v1:Value, v2:Value) : Value {
		switch( op ) {
		default:
			throw "Can't eval " + valueStr(v1) + " " + op + " " + valueStr(v2);
		}
	}

	function evalUnop(op, prefix:Bool, v:Value) : Value {
		switch( op ) {
		default:
			throw "Can't eval " + (prefix ? op + valueStr(v) : valueStr(v) + op);
		}
	}

	function defVal( t : HLType ) {
		return switch( t ) {
		case HUi8, HUi16, HI32, HI64: { v : VInt(0), t : t };
		case HF32, HF64: { v : VFloat(0.), t : t };
		case HBool: { v : VBool(false), t : t };
		default: { v : VNull, t : t };
		}
	}

	function toInt( v : Value ) {
		switch( v.v ) {
		case VNull, VUndef:
			return 0;
		case VInt(i):
			return i;
		case VFloat(f):
			return Std.int(f);
		default:
			throw "Can't case " + valueStr(v) + " to int";
		}
	}

	function toBool( v : Value ) {
		switch( v.v ) {
		case VNull, VUndef:
			return false;
		case VBool(b):
			return b;
		default:
			throw "Can't case " + valueStr(v) + " to int";
		}
	}

	function getVar( name : String ) {
		// locals
		var loc = module.getGraph(funIndex).getLocal(name, codePos);
		if( loc != null ) {
			var v = readReg(loc.rid);
			if( loc.index != null )
				switch( v.v ) {
				case VEnum(_, values): v = values[loc.index];
				case VUndef: v = { v : VUndef, t : loc.t };
				default: throw "assert";
				}
			return v;
		}

		// register
		if( ~/^\$[0-9]+$/.match(name) )
			return readReg(Std.parseInt(name.substr(1)));

		// global
		return getGlobal([name]);
	}

	function evalPath( path : Array<String> ) {
		var v = getVar(path[0]);
		if( v != null ) {
			path.shift();
			return v;
		}
		var g = getGlobal(path);
		if( g == null )
			throw "Unknown identifier " + path[0];
		return g;
	}

	function getGlobal( path : Array<String> ) {
		var g = module.resolveGlobal(path);
		if( g == null )
			return null;
		var v = readVal(jit.globals.offset(g.offset), g.type);
		for( f in path )
			v = readField(v, f);
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
		case VString(s,_): "\"" + escape(s) + "\"";
		case VClosure(f, d): funStr(f) + "[" + valueStr(d) + "]";
		case VFunction(f): funStr(f);
		case VArray(_, length, read, _):
			if( length <= maxArrLength )
				"["+[for(i in 0...length) valueStr(read(i))].join(", ")+"]";
			else {
				var arr = [for(i in 0...maxArrLength) valueStr(read(i))];
				arr.push("...");
				"["+arr.join(",")+"]:"+length;
			}
		case VMap(_, nkeys, readKey, readValue, _):
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
		var r = module.getFunctionRegs(funIndex)[index];
		if( r == null )
			return null;
		if( !module.getGraph(funIndex).isRegisterWritten(index, codePos) )
			return { v : VUndef, t : r.t };
		return readVal(ebp.offset(r.offset), r.t);
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
		if( p.isNull() )
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
				v = VString(str, p);
			case "hl.types.ArrayObj":
				var length = readI32(p.offset(align.ptr));
				var nativeArray = readPointer(p.offset(align.ptr * 2));
				var type = readType(nativeArray.offset(align.ptr));
				v = VArray(type, length, function(i) return readVal(nativeArray.offset(sizeofVArray + i * align.ptr), type), p);
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
		return VArray(t, length, function(i) return readVal(bytes.offset(i * size), t), p);
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
		case VArray(_, _, _, p): p;
		case VString(_, p): p;
		case VMap(_, _, _, _, p): p;
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
			var t = typeFromAddr(p);
			if( t != null )
				return t;
			// vclosure !
			var tfun = readPointer(p.offset(align.ptr));
			var tparent = readType(tfun.offset(align.ptr * 3));
			return switch( tparent ) {
			case HFun(f): var args = f.args.copy(); args.shift(); HFun({ args : args, ret: f.ret });
			default: throw "assert";
			}
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

