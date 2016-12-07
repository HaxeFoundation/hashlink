import HLReader;

@:enum abstract WaitResult(Int) {
	public var Timeout = -1;
	public var Exit = 0;
	public var Breakpoint = 1;
	public var SingleStep = 2;
	public var Error = 3;
}

@:enum abstract Register(Int) {
	public var Esp = 0;
	public var Ebp = 1;
	public var Eip = 2;
	public var EFlags = 3;
	public var Dr0 = 4;
	public var Dr1 = 5;
	public var Dr2 = 6;
	public var Dr3 = 7;
	public var Dr6 = 8;
	public var Dr7 = 9;
}

extern class DebugApi {
	@:hlNative("std", "debug_start") public static function startDebugProcess( pid : Int ) : Bool;
	@:hlNative("std", "debug_stop") public static function stopDebugProcess( pid : Int ) : Void;
	@:hlNative("std", "debug_breakpoint") public static function breakpoint( pid : Int ) : Bool;
	@:hlNative("std", "debug_read") public static function read( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_write") public static function write( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_flush") public static function flush( pid : Int, ptr : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_wait") public static function wait( pid : Int, threadId : hl.Ref<Int>, timeout : Int ) : WaitResult;
	@:hlNative("std", "debug_resume") public static function resume( pid : Int, tid : Int ) : Bool;
	@:hlNative("std", "debug_read_register") public static function readRegister( pid : Int, tid : Int, register : Register ) : hl.Bytes;
	@:hlNative("std", "debug_write_register") public static function writeRegister( pid : Int, tid : Int, register : Register, v : Pointer ) : Bool;
}

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

enum DebugFlag {
	Is64; // runs in 64 bit mode
	Bool4; // bool = 4 bytes (instead of 1)
}

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

abstract Pointer(hl.Bytes) to hl.Bytes {

	static var TMP = new hl.Bytes(8);

	public inline function new(b) {
		this = b;
	}

	public inline function offset(pos:Int) : Pointer {
		return new Pointer(this.offset(pos));
	}

	public inline function sub( p : Pointer ) {
		return this.subtract(p);
	}

	public inline function or( v : Int ) {
		return new Pointer(hl.Bytes.fromAddress(this.address() | v));
	}

	public inline function and( v : Int ) {
		return new Pointer(hl.Bytes.fromAddress(this.address() & v));
	}

	public function readByte( offset : Int ) {
		if( !DebugApi.read(Debugger.PID, this.offset(offset), TMP, 1) )
			throw "Failed to read @" + offset;
		return TMP.getUI8(0);
	}

	public function writeByte( offset : Int, value : Int ) {
		TMP.setUI8(0, value);
		if( !DebugApi.write(Debugger.PID, this.offset(offset), TMP, 1) )
			throw "Failed to write @" + offset;
	}

	public function flush( pos : Int ) {
		DebugApi.flush(Debugger.PID, this.offset(pos), 1);
	}

	public function toInt() {
		return this.address().low;
	}

	public function toString() {
		var i = this.address();
		if( i.high == 0 )
			return "0x" + StringTools.hex(i.low);
		return "0x" + StringTools.hex(i.high) + StringTools.hex(i.low, 8);
	}

	public static function ofPtr( p : hl.Bytes ) : Pointer {
		return cast p;
	}

	public static function make( low : Int, high : Int ) {
		return new Pointer(hl.Bytes.fromAddress(haxe.Int64.make(high, low)));
	}

}

typedef GlobalAccess = {
	var sub : Map<String,GlobalAccess>;
	var gid : Null<Int>;
}

class Debugger {

	public static var PID = 0;
	static inline var INT3 = 0xCC;

	var code : HLCode;
	var fileIndexes : Map<String, Int>;
	var functionsByFile : Map<Int, Array<{ f : HLFunction, ifun : Int, lmin : Int, lmax : Int }>>;
	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;

	var sock : sys.net.Socket;

	var flags : haxe.EnumFlags<DebugFlag>;
	var debugInfos : {
		var mainThread : Int;
		var stackTop : Pointer;
		var codeStart : Pointer;
		var globals : Pointer;
		var codeSize : Int;
		var allTypes : Pointer;
		var typeSize : Int;
		var functions : Array<{ start : Int, large : Bool, offsets : haxe.io.Bytes }>;
	};

	var globalTable : GlobalAccess;
	var globalsOffsets : Array<Int>;
	var currentFrame : Int;
	var currentStack : Array<{ fidx : Int, fpos : Int, codePos : Int, ebp : Pointer }>;
	var protoCache : Map<String,DebugObj>;
	var nextStep : Int = -1;
	var ptrSize : Int;
	var functionRegsCache : Array<Array<{ t : HLType, offset : Int }>>;
	var functionByCodePos : Map<Int,Int>;

	public var stoppedThread : Null<Int>;

	public function new() {
		breakPoints = [];
	}

	public function loadCode( file : String ) {
		var content = sys.io.File.getBytes(file);
		code = new HLReader(false).read(new haxe.io.BytesInput(content));

		// init files
		fileIndexes = new Map();
		for( i in 0...code.debugFiles.length ) {
			var f = code.debugFiles[i];
			fileIndexes.set(f, i);
			var low = f.split("\\").join("/").toLowerCase();
			fileIndexes.set(f, i);
			var fileOnly = low.split("/").pop();
			if( !fileIndexes.exists(fileOnly) ) {
				fileIndexes.set(fileOnly, i);
				if( StringTools.endsWith(fileOnly,".hx") )
					fileIndexes.set(fileOnly.substr(0, -3), i);
			}
		}

		functionsByFile = new Map();
		for( ifun in 0...code.functions.length ) {
			var f = code.functions[ifun];
			var files = new Map();
			for( i in 0...f.debug.length >> 1 ) {
				var ifile = f.debug[i << 1];
				var dline = f.debug[(i << 1) + 1];
				var inf = files.get(ifile);
				if( inf == null ) {
					inf = { f : f, ifun : ifun, lmin : 1000000, lmax : -1 };
					files.set(ifile, inf);
					var fl = functionsByFile.get(ifile);
					if( fl == null ) {
						fl = [];
						functionsByFile.set(ifile, fl);
					}
					fl.push(inf);
				}
				if( dline < inf.lmin ) inf.lmin = dline;
				if( dline > inf.lmax ) inf.lmax = dline;
			}
		}

		// init globals
		var globalsPos = 0;
		globalsOffsets = [];
		for( g in code.globals ) {
			globalsOffsets.push(globalsPos);
			var sz = typeSize(g);
			globalsPos = align(globalsPos, sz);
			globalsPos += sz;
		}

		globalTable = {
			sub : new Map(),
			gid : null,
		};
		function addGlobal( path : Array<String>, gid : Int ) {
			var t = globalTable;
			for( p in path ) {
				if( t.sub == null )
					t.sub = new Map();
				var next = t.sub.get(p);
				if( next == null ) {
					next = { sub : null, gid : null };
					t.sub.set(p, next);
				}
				t = next;
			}
			t.gid = gid;
		}
		for( t in code.types )
			switch( t ) {
			case HObj(o) if( o.globalValue != null ):
				addGlobal(o.name.split("."), o.globalValue);
			case HEnum(e) if( e.globalValue != null ):
				addGlobal(e.name.split("."), e.globalValue);
			default:
			}

		// init objects
		protoCache = new Map();
		functionRegsCache = [];
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

	function readDebugInfos() {
		if( sock.input.readString(3) != "HLD" )
			return false;
		var version = sock.input.readByte() - "0".code;
		if( version > 0 )
			return false;
		flags = haxe.EnumFlags.ofInt(sock.input.readInt32());
		ptrSize = flags.has(Is64) ? 8 : 4;

		if( flags.has(Is64) != hl.Api.is64() )
			return false;

		function readPointer() : Pointer {
			if( flags.has(Is64) )
				return Pointer.make(sock.input.readInt32(), sock.input.readInt32());
			return Pointer.make(sock.input.readInt32(),0);
		}

		debugInfos = {
			mainThread : sock.input.readInt32(),
			globals : readPointer(),
			stackTop : readPointer(),
			codeStart : readPointer(),
			codeSize : sock.input.readInt32(),
			allTypes : readPointer(),
			typeSize : sock.input.readInt32(),
			functions : [],
		};

		var nfunctions = sock.input.readInt32();
		if( nfunctions != code.functions.length )
			return false;


		functionByCodePos = new Map();
		for( i in 0...nfunctions ) {
			var nops = sock.input.readInt32();
			if( code.functions[i].debug.length >> 1 != nops )
				return false;
			var start = sock.input.readInt32();
			var large = sock.input.readByte() != 0;
			var offsets = sock.input.read((nops + 1) * (large ? 4 : 2));
			functionByCodePos.set(start, i);
			debugInfos.functions.push({
				start : start,
				large : large,
				offsets : offsets,
			});
		}
		return true;
	}

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

	public function startDebug( pid : Int, port : Int ) {

		PID = pid;
		sock = new sys.net.Socket();
		try {
			sock.connect(new sys.net.Host("127.0.0.1"), port);
		} catch( e : Dynamic ) {
			sock.close();
			return false;
		}

		if( !readDebugInfos() ) {
			sock.close();
			return false;
		}

		if( !DebugApi.startDebugProcess(pid) )
			return false;

		wait(); // wait first break

		return true;
	}

	public function run() {
		// unlock waiting thread
		if( sock != null ) {
			sock.close();
			sock = null;
		}
		if( stoppedThread != null )
			resume();
		return wait();
	}

	function wait() {
		var tid = 0;
		var cmd = Timeout;
		while( true ) {
			cmd = DebugApi.wait(PID, tid, 1000);
			switch( cmd ) {
			case Timeout:
				// continue
			case Breakpoint:
				var eip = getReg(tid, Eip);
				var codePos = eip.sub(debugInfos.codeStart) - 1;
				for( b in breakPoints ) {
					if( b.codePos == codePos ) {
						// restore code
						setCode(codePos, b.oldByte);
						// move backward
						setReg(tid, Eip, eip.offset(-1));
						// set EFLAGS to single step
						var r = getReg(tid, EFlags);
						setReg(tid, EFlags, r.or(256));
						nextStep = codePos;
						break;
					}
				}
				break;
			case SingleStep:
				// restore our breakpoint
				if( nextStep >= 0 ) {
					setCode(nextStep, 0xCC);
					nextStep = -1;
				}
				stoppedThread = tid;
				resume();
			default:
				break;
			}
		}
		stoppedThread = tid;
		currentFrame = 0;
		currentStack = (tid == debugInfos.mainThread) ? makeStack(tid) : [];
		return cmd;
	}

	public function resume() {
		if( !DebugApi.resume(PID, stoppedThread) )
			throw "Could not resume "+stoppedThread;
		stoppedThread = null;
	}

	public function getBackTrace() : Array<{ file : String, line : Int }> {
		return [for( e in currentStack ) resolveSymbol(e.fidx, e.fpos)];
	}

	public function getFrame() {
		var f = currentStack[currentFrame];
		return f == null ? {file:"???",line:0} : resolveSymbol(f.fidx, f.fpos);
	}

	function resolveSymbol( fidx : Int, fpos : Int ) {
		var f = code.functions[fidx];
		var fid = f.debug[fpos << 1];
		var fline = f.debug[(fpos << 1) + 1];
		return { file : code.debugFiles[fid], line : fline };
	}

	function makeStack(tid) {
		var stack = [];
		var esp = getReg(tid, Esp);
		var size = debugInfos.stackTop.sub(esp);
		var mem = readMem(esp, size);

		if( flags.has(Is64) ) throw "TODO";

		var codePos = getReg(tid, Eip).sub(debugInfos.codeStart);
		var e = resolvePos(codePos);

		if( e != null ) {
			e.ebp = getReg(tid, Ebp);
			stack.push(e);
		}

		// similar to module/module_capture_stack
		var stackBottom = esp.toInt();
		var stackTop = debugInfos.stackTop.toInt();
		var codeBegin = debugInfos.codeStart.toInt();
		var codeEnd = codeBegin + debugInfos.codeSize;
		var codeStart = codeBegin + debugInfos.functions[0].start;
		for( i in 1...size >> 2 ) {
			var val = mem.getI32(i << 2);
			if( val > stackBottom && val < stackTop ) {
				var prev = mem.getI32((i + 1) << 2);
				if( prev >= codeStart && prev < codeEnd ) {
					var codePos = prev - codeBegin;
					var e = resolvePos(codePos);
					if( e != null ) {
						e.ebp = esp.offset(i << 2);
						stack.push(e);
					}
				}
			}
		}
		return stack;
	}

	function resolvePos( codePos : Int ) {
		var absPos = codePos;
		var min = 0;
		var max = debugInfos.functions.length;
		while( min < max ) {
			var mid = (min + max) >> 1;
			var p = debugInfos.functions[mid];
			if( p.start <= codePos )
				min = mid + 1;
			else
				max = mid;
		}
		if( min == 0 )
			return null;
		var fidx = (min - 1);
		var dbg = debugInfos.functions[fidx];
		var fdebug = code.functions[fidx];
		min = 0;
		max = fdebug.debug.length>>1;
		codePos -= dbg.start;
		while( min < max ) {
			var mid = (min + max) >> 1;
			var offset = dbg.large ? dbg.offsets.getInt32(mid * 4) : dbg.offsets.getUInt16(mid * 2);
			if( offset <= codePos )
				min = mid + 1;
			else
				max = mid;
		}
		if( min == 0 )
			return null; // ???
		return { fidx : fidx, fpos : min - 1, codePos : absPos, ebp : null };
	}

	function getBreaks( file : String, line : Int ) {
		var ifile = fileIndexes.get(file);
		if( ifile == null )
			ifile = fileIndexes.get(file.split("\\").join("//").toLowerCase());

		var functions = functionsByFile.get(ifile);
		if( ifile == null || functions == null )
			return null;

		var breaks = [];
		for( f in functions ) {
			if( f.lmin > line || f.lmax < line ) continue;
			var ifun = f.ifun;
			var f = f.f;
			var i = 0;
			var len = f.debug.length >> 1;
			while( i < len ) {
				var dfile = f.debug[i << 1];
				if( dfile != ifile ) {
					i++;
					continue;
				}
				var dline = f.debug[(i << 1) + 1];
				if( dline != line ) {
					i++;
					continue;
				}
				breaks.push({ ifun : ifun, pos : i });
				// skip
				i++;
				while( i < len ) {
					var dfile = f.debug[i << 1];
					var dline = f.debug[(i << 1) + 1];
					if( dfile == ifile && dline != line )
						break;
					i++;
				}
			}
		}
		return breaks;
	}

	public function removeBreakpoint( file : String, line : Int ) {
		var breaks = getBreaks(file, line);
		if( breaks == null )
			return false;
		var rem = false;
		for( b in breaks )
			for( a in breakPoints )
				if( a.fid == b.ifun && a.pos == b.pos ) {
					rem = true;
					breakPoints.remove(a);
					setCode(a.codePos, a.oldByte);
					if( nextStep == a.codePos ) nextStep = -1;
					break;
				}
		return rem;
	}

	public function addBreakpoint( file : String, line : Int ) {
		var breaks = getBreaks(file, line);
		if( breaks == null )
			return false;
		// check already defined
		var set = false;
		for( b in breaks.copy() ) {
			var found = false;
			for( a in breakPoints ) {
				if( a.fid == b.ifun && a.pos == b.pos ) {
					found = true;
					break;
				}
			}
			if( found ) continue;

			var codePos = getCodePos(b.ifun, b.pos);
			var old = getCode(codePos);
			setCode(codePos, INT3);
			breakPoints.push({ fid : b.ifun, pos : b.pos, oldByte : old, codePos : codePos });
			set = true;
		}
		return set;
	}

	function getCodePos( fidx : Int, pos : Int ) {
		var dbg = debugInfos.functions[fidx];
		return dbg.start + (dbg.large ? dbg.offsets.getInt32(pos << 2) : dbg.offsets.getUInt16(pos << 1));
	}

	function getCode( pos : Int ) {
		return debugInfos.codeStart.readByte(pos);
	}

	function setCode( pos : Int, byte : Int ) {
		debugInfos.codeStart.writeByte(pos, byte);
		debugInfos.codeStart.flush(pos);
	}

	function getReg(tid, reg) {
		return Pointer.ofPtr(DebugApi.readRegister(PID, tid, reg));
	}

	function readReg(frame, index) {
		var c = currentStack[frame];
		if( c == null )
			return null;
		var regs = functionRegsCache[c.fidx];
		if( regs == null ) {
			var f = code.functions[c.fidx];
			var nargs = switch( f.t ) { case HFun(f): f.args.length; default: throw "assert"; };
			regs = [];
			var size = ptrSize * 2;
			for( i in 0...nargs ) {
				var t = f.regs[i];
				regs[i] = { t : t, offset : size };
				size += typeSize(t);
			}
			size = 0;
			for( i in nargs...f.regs.length ) {
				var t = f.regs[i];
				var sz = typeSize(t);
				size += sz;
				size = align(size, sz);
				regs[i] = { t : t, offset : -size };
			}
			functionRegsCache[c.fidx] = regs;
		}
		var r = regs[index];
		if( r == null )
			return null;
		return readVal(c.ebp.offset(r.offset), r.t);
	}

	function setReg(tid, reg, value) {
		if( !DebugApi.writeRegister(PID, tid, reg, value) )
			throw "Failed to set register " + reg;
	}

	function readMem( p : Pointer, size : Int ) {
		var mem = new hl.Bytes(size);
		if( !DebugApi.read(PID, p, mem, size) )
			throw "Failed to read memory @" + p.toString() + "[" + size+"]";
		return mem;
	}

	public function eval( name : String ) {
		if( name == null || name == "" )
			return null;
		var path = name.split(".");
		// TODO : look in locals

		var v;

		if( ~/^\$[0-9]+$/.match(path[0]) ) {

			// register
			v = readReg(currentFrame, Std.parseInt(path.shift().substr(1)));
			if( v == null )
				return null;

		} else {

			// global
			var g = globalTable;
			while( path.length > 0 ) {
				if( g.sub == null ) break;
				var p = path[0];
				var n = g.sub.get(p);
				if( n == null ) break;
				path.shift();
				g = n;
			}
			if( g.gid == null )
				return null;


			var gid = g.gid;
			var t = code.globals[gid];
			v = readVal(debugInfos.globals.offset(globalsOffsets[gid]), t);
		}

		for( p in path )
			v = readField(v, p);
		return v;
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
			var lookup = readMemPointer(ptr).offset(debugInfos.typeSize);
			var data = readMemPointer(ptr.offset(ptrSize));
			var nfields = readI32(ptr.offset(ptrSize * 2));
			// lookup, similar to hl_lookup_find
			var hash = HLTools.hash(name);
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
			var tid = Std.int( p.sub(debugInfos.allTypes) / debugInfos.typeSize );
			return code.types[tid];
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
			if( HLTools.isDynamic(t) )
				return valueCast(p, t);
			v = readVal(p.offset(8), t).v;
		case HNull(t):
			v = readVal(p.offset(8), t).v;
		case HRef(t):
			v = readVal(p, t).v;
		case HFun(_):
			var funPtr = readMemPointer(p.offset(ptrSize));
			var hasValue = readI32(p.offset(ptrSize * 2));
			var fidx = functionByCodePos.get(funPtr.sub(debugInfos.codeStart));
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

	function readI32( p : Pointer ) {
		return readMem(p, 4).getI32(0);
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

	// ---------------- hldebug commandline interface / GDB like ------------------------

	static function error( msg ) {
		Sys.stderr().writeString(msg + "\n");
		Sys.exit(1);
	}

	static function main() {
		var args = Sys.args();
		var debugPort = 5001;
		var file = null;
		var pid : Null<Int> = null;
		while( args.length > 0 && args[0].charCodeAt(0) == '-'.code ) {
			var param = args.shift();
			switch( param ) {
			case "-port":
				param = args.shift();
				if( param == null || (debugPort = Std.parseInt(param)) == 0 )
					error("Require port int value");
			case "-attach":
				if( param == null || (pid = Std.parseInt(param)) == null )
					error("Require attach process id value");
			default:
				error("Unsupported parameter " + param);
			}
		}
		file = args.shift();
		if( file == null ) {
			Sys.println("hldebug [-port <port>] [-path <path>] <file.hl> <args>");
			Sys.exit(1);
		}
		if( !sys.FileSystem.exists(file) )
			error(file+" not found");

		var process = null;
		if( pid == null ) {
			process = new sys.io.Process("hl", ["--debug", "" + debugPort, "--debug-wait", file]);
			pid = process.getPid();
		}

		function dumpProcessOut() {
			if( process == null ) return;
			if( process.exitCode(false) == null ) process.kill();
			Sys.print(process.stdout.readAll().toString());
			Sys.stderr().writeString(process.stderr.readAll().toString());
		}

		var dbg = new Debugger();
		var breaks = [];
		dbg.loadCode(file);

		if( !dbg.startDebug(pid, debugPort) ) {
			dumpProcessOut();
			error("Failed to access process #" + pid+" on port "+debugPort+" for debugging");
		}

		function frameStr( f : { file : String, line : Int } ) {
			return f.file+":" + f.line;
		}

		function clearBP() {
			var count = breaks.length;
			for( b in breaks )
				dbg.removeBreakpoint(b.file, b.line);
			breaks = [];
			Sys.println(count + " breakpoints removed");
		}

		var stdin = Sys.stdin();

		dbg.addBreakpoint("GlDriver", 748);
		dbg.run();

		while( true ) {

			if( process != null ) {
				var ecode = process.exitCode(false);
				if( ecode != null ) {
					dumpProcessOut();
					error("Process exit with code " + ecode);
				}
			}

			Sys.print("> ");
			var r = stdin.readLine();
			var args = r.split(" ");
			switch( args.shift() ) {
			case "q", "quit":
				dumpProcessOut();
				break;
			case "r", "run", "c", "continue":
				var r = dbg.run();
				switch( r ) {
				case Exit:
					dbg.resume();
				case Breakpoint:
					Sys.println("Thread "+dbg.stoppedThread+" paused");
				case Error:
					Sys.println("*** an error has occured, paused ***");
				default:
					throw "assert";
				}
			case "bt", "backtrace":
				for( f in dbg.getBackTrace() )
					Sys.println(frameStr(f));
			case "where":
				Sys.println(frameStr(dbg.getFrame()));
			case "frame","f":
				if( args.length == 1 )
					dbg.currentFrame = Std.parseInt(args[0]);
				Sys.println(frameStr(dbg.getFrame()));
			case "up":
				dbg.currentFrame += args.length == 0 ? 1 : Std.parseInt(args[0]);
				if( dbg.currentFrame >= dbg.currentStack.length )
					dbg.currentFrame = dbg.currentStack.length - 1;
				Sys.println(frameStr(dbg.getFrame()));
			case "down":
				dbg.currentFrame -= args.length == 0 ? 1 : Std.parseInt(args[0]);
				if( dbg.currentFrame < 0 )
					dbg.currentFrame = 0;
				Sys.println(frameStr(dbg.getFrame()));
			case "b", "break":
				var file = args.shift();
				var line = Std.parseInt(args.shift());
				if( dbg.addBreakpoint(file, line) ) {
					breaks.push({file:file, line:line});
					Sys.println("Breakpoint set");
				} else
					Sys.println("No breakpoint set");
			case "p", "print":
				var path = args.shift();
				if( path == null ) {
					Sys.println("Requires variable name");
					continue;
				}
				var v = dbg.eval(path);
				if( v == null ) {
					Sys.println("Unknown var " + path);
					continue;
				}
				Sys.println(dbg.valueStr(v) + " : " + dbg.typeStr(v.t));
			case "clear":
				switch( args.length ) {
				case 0:
					clearBP();
				case 1:
					var file = args[1];
					var line = Std.parseInt(file);
					var count = 0;
					for( b in breaks.copy() )
						if( b.file == file || (line != null && b.line == line) ) {
							dbg.removeBreakpoint(b.file, b.line);
							breaks.remove(b);
							count++;
						}
					Sys.println(count + " breakpoints removed");
				}

			case "delete", "d":
				clearBP();
			default:
				Sys.println("Unknown command " + r);
			}

		}
	}

}