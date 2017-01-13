import format.hl.Data;
using format.hl.Tools;
import DebugApi;
import DebugValue;


typedef GlobalAccess = {
	var sub : Map<String,GlobalAccess>;
	var gid : Null<Int>;
}

class Debugger extends DebugValue.DebugValueReader {

	static inline var INT3 = 0xCC;

	var pid : Int;
	var fileIndexes : Map<String, Int>;
	var functionsByFile : Map<Int, Array<{ f : HLFunction, ifun : Int, lmin : Int, lmax : Int }>>;
	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;

	var sock : sys.net.Socket;
	var debugInfos : {
		var mainThread : Int;
		var stackTop : Pointer;
		var codeStart : Pointer;
		var globals : Pointer;
		var codeSize : Int;
		var allTypes : Pointer;
		var functions : Array<{ start : Int, large : Bool, offsets : haxe.io.Bytes }>;
	};

	var globalTable : GlobalAccess;
	var globalsOffsets : Array<Int>;
	var currentFrame : Int;
	var currentStack : Array<{ fidx : Int, fpos : Int, codePos : Int, ebp : Pointer }>;
	var nextStep : Int = -1;
	var functionRegsCache : Array<Array<{ t : HLType, offset : Int }>>;
	var functionByCodePos : Map<Int,Int>;

	public var stoppedThread : Null<Int>;

	public function new() {
		breakPoints = [];
	}

	public function loadCode( file : String ) {
		var content = sys.io.File.getBytes(file);
		code = new format.hl.Reader(false).read(new haxe.io.BytesInput(content));

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

	public function startDebug( pid : Int, port : Int ) {

		this.pid = pid;
		Pointer.PID = pid;
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
			cmd = DebugApi.wait(pid, tid, 1000);
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
		if( !DebugApi.resume(pid, stoppedThread) )
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
		return Pointer.ofPtr(DebugApi.readRegister(pid, tid, reg));
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
		if( !DebugApi.writeRegister(pid, tid, reg, value) )
			throw "Failed to set register " + reg;
	}

	override function typeFromAddr( p : Pointer ) : HLType {
		var tid = Std.int( p.sub(debugInfos.allTypes) / typeStructSize() );
		return code.types[tid];
	}

	override function functionFromAddr( p : Pointer ) : Null<Int> {
		return functionByCodePos.get(p.sub(debugInfos.codeStart));
	}

	override function readMem( p : Pointer, size : Int ) {
		var mem = new hl.Bytes(size);
		if( !DebugApi.read(pid, p, mem, size) )
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
				param = args.shift();
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
				Sys.println(dbg.valueStr(v) + " : " + v.t.toString());
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