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
	public var maxBytesLength : Int = 128;

	static var HASH_PREFIX = "$_h$";

	public function new(module,api,jit) {
		this.module = module;
		this.api = api;
		this.jit = jit;
		this.align = jit.align;
		parser = new hscript.Parser();
		parser.identChars += "$";
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

	public function setContext( funIndex : Int, codePos : Int, ebp : Pointer ) {
		this.funIndex = funIndex;
		this.codePos = codePos;
		this.ebp = ebp;
	}

	public function eval( expr : String ) {
		if( expr == null || expr == "" )
			return null;
		var expr = try parser.parseString(expr) catch( e : hscript.Expr.Error ) throw hscript.Printer.errorToString(e);
		return evalExpr(expr);
	}

	public function setValue( expr : String, value : String ) {
		var ref = ref(expr);
		if( ref.ptr.isNull() )
			throw "Can't set null ptr";
		var value = eval(value);
		if( value == null )
			return null;
		value = castTo(value, ref.t);
		switch( [ref.t, value.v] ) {
		case [HI32, VInt(i)]:
			writeI32(ref.ptr, i);
		case [HBool, VBool(flag)]:
			var b = new Buffer(1);
			b.setUI8(0, flag ? 1 : 0);
			writeMem(ref.ptr, b, 1);
		default:
			if( ref.t.isPtr() ) {
				var ptr = getPtr(value);
				if( ptr != null || value.v == VNull ) {
					writePointer(ref.ptr, ptr);
					return value;
				}
			}
			throw "Don't know how to set "+ref.t.toString();
		}
		return value;
	}

	function getPtr( v : Value ) {
		return switch (v.v) {
		case VNull: Pointer.make(0,0);
		case VPointer(p), VString(_, p), VClosure(_,_,p), VFunction(_,p), VArray(_, _, _, p), VMap(_, _, _, _, p), VEnum(_, _, p), VBytes(_,_,p): p;
		default: null;
		}
	}

	function castTo( v : Value, t : HLType ) {
		if( v == null )
			return null;
		if( safeCastTo(v.t, t) )
			return v;
		if( v.v == VNull && t.isPtr() )
			return { v : VNull, t : t };
		throw "Don't know how to cast "+v.t.toString()+" to "+t.toString();
	}

	function safeCastTo( t : HLType, to : HLType ) {
		if( t == to )
			return true;
		// TODO
		return false;
	}

	public function ref( exprSrc : String ) {
		if( exprSrc == null || exprSrc == "" )
			return null;
		var expr = try parser.parseString(exprSrc) catch( e : hscript.Expr.Error ) throw hscript.Printer.errorToString(e);
		switch( expr ) {
		case EField(obj, f):
			var v = evalExpr(obj);
			var addr = readFieldAddress(v, f);
			if( addr == null || addr.ptr == null )
				throw "Can't reference " + exprSrc;
			return addr;
		case EIdent(i):
			return getVarAddress(i);
		default:
			throw "Can't get ref for " + hscript.Printer.toString(expr);
		}
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
			if( v == null ) return null;
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
			for( f in path ) {
				if( v == null ) break;
				v = readField(v, f);
			}
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

	function getVar( name : String ) : Value {
		return switch( name ) {
		case "true": { v : VBool(true), t : HBool };
		case "false": { v : VBool(false), t : HBool };
		case "null": { v : VNull, t : HDyn };
		default:
			fetch(getVarAddress(name));
		}
	}

	function getVarAddress( name : String ) {
		// locals
		var loc = module.getGraph(funIndex).getLocal(name, codePos);
		if( loc != null ) {
			var v = readRegAddress(loc.rid);
			if( loc.index != null ) {
				if( v.ptr == null )
					return { ptr : null, t : loc.t };
				var ptr = readPointer(v.ptr);
				return { ptr : ptr.offset(module.getEnumProto(loc.container)[0].params[loc.index].offset), t : loc.t };
			}
			return v;
		}

		// register
		if( ~/^\$[0-9]+$/.match(name) )
			return readRegAddress(Std.parseInt(name.substr(1)));

		// this variable
		if( module.getGraph(funIndex).getLocal("this", codePos) != null ) {
			var vthis = getVar("this");
			if( vthis != null ) {
				var f = readFieldAddress(vthis, name);
				if( f != null )
					return f;
				// static var
				switch( vthis.t ) {
				case HObj(o) if( o.globalValue != null ):
					var path = o.name.split(".");
					path.push(name);
					var f = getGlobalAddress(path);
					if( f != null )
						return f;
				default:
				}
			}
		}

		// static
		var ctx = module.getMethodContext(funIndex);
		if( ctx != null ) {
			var t = ctx.obj;
			if( t.globalValue != null )
				t = switch( module.code.globals[t.globalValue] ) {
				case HObj(p): p;
				default: null;
				}
			if( t != null && t.name.charCodeAt(0) == '$'.code ) {
				for( f in t.fields )
					if( f.name == name )
						return readFieldAddress(getVar(t.name.substr(1)), name);
			}
		}

		// global
		return getGlobalAddress([name]);
	}

	function evalPath( path : Array<String> ) {
		var v = getVar(path[0]);
		if( v != null ) {
			path.shift();
			return v;
		}
		return fetch(getGlobalAddress(path));
	}

	function getGlobalAddress( path : Array<String> ) {
		var g = module.resolveGlobal(path);
		if( g == null )
			return null;
		var addr = { ptr : jit.globals.offset(g.offset), t : g.type };
		while( path.length > 0 )
			addr = readFieldAddress(fetch(addr), path.shift());
		return addr;
	}

	function escape( s : String ) {
		s = s.split("\\").join("\\\\");
		s = s.split("\n").join("\\n");
		s = s.split("\r").join("\\r");
		s = s.split("\t").join("\\t");
		s = s.split('"').join('\\"');
		return s;
	}

	public function typeStr( t : HLType ) {
		switch( t ) {
		case HDynObj:
			return "{...}";
		default:
			return t.toString();
		}
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
		case VPointer(p):
			switch( v.t ) {
			case HVirtual(_): p.toString();
			case HBytes, HAbstract(_): typeStr(v.t)+"("+p.toString()+")";
			default: typeStr(v.t).split(".").pop();
			}
		case VString(s,_): "\"" + escape(s) + "\"";
		case VClosure(f, d, _): funStr(f) + "[" + valueStr(d) + "]";
		case VFunction(f,_): funStr(f);
		case VArray(_, length, read, _):
			if( length <= maxArrLength )
				"["+[for(i in 0...length) valueStr(read(i))].join(", ")+"]";
			else {
				var arr = [for(i in 0...maxArrLength) valueStr(read(i))];
				arr.push("...");
				"["+arr.join(",")+"]:"+length;
			}
		case VBytes(length, read, _):
			var blen = length < maxBytesLength ? length : maxBytesLength;
			var bytes = haxe.io.Bytes.alloc(blen);
			for( i in 0...blen )
				bytes.set(i, read(i));
			var str = length+":0x" + bytes.toHex().toUpperCase();
			if( length > maxBytesLength )
				str += "...";
			str;
		case VMap(_, nkeys, readKey, readValue, _):
			var max = nkeys < maxArrLength ? nkeys : maxArrLength;
			var content = [for( i in 0...max ) { var k = readKey(i); valueStr(k) + "=>" + valueStr(readValue(i)); }];
			if( max != nkeys ) {
				content.push("...");
				content.toString() + ":" + nkeys;
			} else
				content.toString();
		case VType(t):
			typeStr(t);
		case VEnum(c, values, _):
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

	function readRegAddress(index) {
		var r = module.getFunctionRegs(funIndex)[index];
		if( r == null )
			return null;
		if( !module.getGraph(funIndex).isRegisterWritten(index, codePos) )
			return { ptr : null, t : r.t };
		return { ptr : ebp.offset(r.offset), t : r.t };
	}

	function readReg(index) {
		return fetch(readRegAddress(index));
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
			case "haxe.ds.StringMap":
				v = makeMap(readPointer(p.offset(align.ptr)), HBytes);
			case "haxe.ds.IntMap":
				v = makeMap(readPointer(p.offset(align.ptr)), HI32);
			case "haxe.ds.ObjectMap":
				v = makeMap(readPointer(p.offset(align.ptr)), HDyn);
			case "haxe.io.Bytes":
				var length = readI32(p.offset(align.ptr));
				var bytes = readPointer(p.offset(align.ptr * 2));
				v = VBytes(length, function(i) return readMem(bytes.offset(i),1).getUI8(0), p);
			default:
			}
		case HVirtual(_):
			var v = readPointer(p.offset(align.ptr));
			if( !v.isNull() )
				return valueCast(v, HDyn);
		case HDyn:
			t = readType(p);
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
				v = VClosure(fval, value, p);
			} else
				v = VFunction(fval, p);
		case HType:
			v = VType(readType(p,true));
		case HBytes:
			var len = 0;
			var buf = new StringBuf();
			while( true ) {
				var c = try readI32(p.offset(len<<1)) & 0xFFFF catch( e : Dynamic ) 0;
				if( c == 0 ) break;
				buf.addChar(c);
				len++;
				if( len > 50 ) {
					buf.add("...");
					break;
				}
			}
			v = VString(buf.toString(), p);
		case HEnum(e):
			var index = readI32(p.offset(align.ptr));
			var c = module.getEnumProto(e)[index];
			v = VEnum(c.name,[for( a in c.params ) readVal(p.offset(a.offset),a.t)], p);
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

	function makeMap( p : Pointer, tkey : HLType ) {
		var cells = readPointer(p);
		var entries = readPointer(p.offset(align.ptr));
		var values = readPointer(p.offset(align.ptr * 2));
		var freelist_size = align.ptr + 4 + 4;
		var pos = align.ptr * 3 + freelist_size;
		var ncells = readI32(p.offset(pos));
		var nentries = readI32(p.offset(pos + 4));
		var content : Array<{ key : Value, value : Value }> = [];

		var curCell = 0;

		var keyInValue;
		var keyPos, valuePos, keyStride, valueStride;
		switch( tkey ) {
		case HBytes:
			keyInValue = true;
			keyPos = 0;
			valuePos = align.ptr;
			keyStride = 8;
			valueStride = align.ptr * 2;
		case HI32:
			keyInValue = false;
			keyPos = 0;
			valuePos = 0;
			keyStride = 8;
			valueStride = align.ptr;
		case HDyn:
			keyInValue = true;
			keyPos = 0;
			valuePos = align.ptr;
			keyStride = 4;
			valueStride = align.ptr * 2;
		default:
			throw "Unsupported map " + tkey.toString();
		}


		function fetch(k) {
			while( content.length <= k ) {
				if( curCell == ncells ) throw "assert";
				var c = readI32(cells.offset((curCell++) << 2));
				while( c >= 0 ) {
					var value = readVal(values.offset(c * valueStride + valuePos), HDyn);
					var keyPtr = keyInValue ? values.offset(c * valueStride + keyPos) : entries.offset(c * keyStride + keyPos);
					var key : Value = switch( tkey ) {
					case HBytes:
						{ v : VString(readUCSBytes(readPointer(keyPtr)), null), t : t_string };
					case HI32:
						{ v : VInt(readI32(keyPtr)), t : HI32 };
					case HDyn:
						readVal(keyPtr,HDyn);
					default:
						throw "Unsupported map " + tkey.toString();
					}
					content.push({ key : key, value : value });
					c = readI32(entries.offset(c * keyStride + keyStride - 4));
				}
			}
			return content[k];
		}

		function getKey(k) return k < 0 || k >= nentries ? { v : VUndef, t : tkey } : fetch(k).key;
		function getValue(k) return k < 0 || k >= nentries ? { v : VUndef, t : HDyn } : fetch(k).value;

		return VMap(tkey == HBytes ? t_string : tkey,nentries,getKey, getValue, p);
	}

	public function getFields( v : Value ) : Array<String> {
		var ptr = switch( v.v ) {
		case VPointer(p): p;
		case VEnum(_,values,_):
			// only list the pointer fields (others are displayed in enum anyway)
			return [for( i in 0...values.length ) if( values[i].t.isPtr() ) "$"+i];
		default:
			return null;
		}
		switch( v.t ) {
		case HObj(o):
			function getRec(o:format.hl.Data.ObjPrototype) {
				var fields = o.tsuper == null ? [] : getRec(switch( o.tsuper ) { case HObj(o): o; default: throw "assert"; });
				for( f in o.fields )
					if( f.name != "" )
						fields.push(f.name);
				return fields;
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

	public function readField( v : Value, name : String ) {
		switch( v.v ) {
		case VEnum(_,values,_) if( name.charCodeAt(0) == "$".code ):
			return values[Std.parseInt(name.substr(1))];
		default:
		}
		var a = readFieldAddress(v, name);
		return fetch(a);
	}

	public function fetch( addr : { ptr : Pointer, t : HLType } ) {
		if( addr == null )
			return null;
		if( addr.ptr == null )
			return { v : VUndef, t : addr.t };
		return readVal(addr.ptr, addr.t);
	}

	public function readFieldAddress( v : Value, name : String ) : { ptr : Null<Pointer>, t : HLType }  {
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
			return { ptr : ptr == null ? null : ptr.offset(f.offset), t : f.t };
		case HVirtual(fl):
			for( i in 0...fl.length )
				if( fl[i].name == name ) {
					var t = fl[i].t;
					if( ptr == null )
						return { ptr : null, t : t };
					var addr = readPointer(ptr.offset(align.ptr * (3 + i)));
					if( addr != null )
						return { ptr : addr, t : t };
				}
			if( ptr == null )
				return null;
			var realValue = readPointer(ptr.offset(align.ptr));
			if( realValue == null )
				return null;
			return readFieldAddress({ v : VPointer(realValue), t : HDyn }, name);
		case HDynObj:
			if( ptr == null )
				return { ptr : null, t : HDyn };
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
					return { ptr : t.isPtr() ? values.offset(offset * align.ptr) : raw_data.offset(offset), t : t };
				}
			}
			return null;
		case HDyn:
			if( ptr == null )
				return { ptr : null, t : HDyn };
			return readFieldAddress({ v : v.v, t : readType(ptr) }, name);
		default:
			return null;
		}
	}

	public function readPointer( p : Pointer ) {
		if( align.is64 ) {
			var m = readMem(p, 8);
			return m.getPointer(0,align);
		}
		return Pointer.make(readI32(p), 0);
	}

	public function writePointer( p : Pointer, v : Pointer ) {
		if( align.is64 ) {
			var buf = new Buffer(8);
			buf.setPointer(0, v, align);
			writeMem(p, buf, 8);
		} else
			writeI32(p, v.toInt());
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

	function readUCSBytes( ptr : Pointer ) {
		var len = 0;
		while( true ) {
			var v = readI32(ptr.offset(len << 1));
			var low = v & 0xFFFF;
			var high = v >>> 16;
			if( low == 0 ) break;
			len++;
			if( high == 0 ) break;
			len++;
		}
		return readUCS2(ptr, len);
	}

	public function readI32( p : Pointer ) {
		return readMem(p, 4).getI32(0);
	}

	public function writeI32( p : Pointer, v : Int ) {
		var buf = new Buffer(4);
		buf.setI32(0,v);
		writeMem(p, buf, 4);
	}

	function writeMem( p : Pointer, b : Buffer, size : Int ) {
		if( !api.write(p, b, size) )
			throw "Failed to write @" + p.toString() + ":" + size;
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

