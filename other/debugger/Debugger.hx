@:enum abstract WaitResult(Int) {
	public var Timeout = -1;
	public var Exit = 0;
	public var Breakpoint = 1;
	public var SingleStep = 2;
	public var Error = 3;
}

extern class DebugApi {
	@:hlNative("std", "debug_start") public static function startDebugProcess( pid : Int ) : Bool;
	@:hlNative("std", "debug_stop") public static function stopDebugProcess( pid : Int ) : Void;
	@:hlNative("std", "debug_breakpoint") public static function breakpoint( pid : Int ) : Bool;
	@:hlNative("std", "debug_read") public static function read( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_write") public static function write( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_address") public static function address( low : Int, high : Int ) : hl.Bytes;
	@:hlNative("std", "debug_flush") public static function flush( pid : Int, ptr : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_wait") public static function wait( pid : Int, threadId : hl.Ref<Int>, timeout : Int ) : WaitResult;
	@:hlNative("std", "debug_resume") public static function resume( pid : Int, tid : Int ) : Bool;
}

enum DebugFlag {
	Is64; // runs in 64 bit mode
	Bool4; // bool = 4 bytes (instead of 1)
}

abstract Pointer(hl.Bytes) {

	static var TMP = new hl.Bytes(8);

	public inline function new(low, high) {
		this = DebugApi.address(low, high);
	}

	public inline function offset(pos:Int) : Pointer {
		return cast this.offset(pos);
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

}

class Debugger {

	public static var PID = 0;
	static inline var INT3 = 0xCC;

	var code : HLReader.HLCode;
	var fileIndexes : Map<String, Int>;
	var functionsByFile : Map<Int, Array<{ f : HLReader.HLFunction, ifun : Int, lmin : Int, lmax : Int }>>;
	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;

	var sock : sys.net.Socket;

	var flags : haxe.EnumFlags<DebugFlag>;
	var debugInfos : {
		var stackTop : Pointer;
		var codeStart : Pointer;
		var codeSize : Int;
		var functions : Array<{ start : Int, large : Bool, offsets : haxe.io.Bytes }>;
	};

	public var stoppedThread : Null<Int>;

	public function new() {
		breakPoints = [];
	}

	public function loadCode( file : String ) {
		var content = sys.io.File.getBytes(file);
		code = new HLReader(false).read(new haxe.io.BytesInput(content));

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
	}

	function readPointer() : Pointer {
		if( flags.has(Is64) )
			return new Pointer(sock.input.readInt32(), sock.input.readInt32());
		return new Pointer(sock.input.readInt32(),0);
	}

	function readDebugInfos() {
		if( sock.input.readString(3) != "HLD" )
			return false;
		var version = sock.input.readByte() - "0".code;
		if( version > 0 )
			return false;
		flags = haxe.EnumFlags.ofInt(sock.input.readInt32());

		if( flags.has(Is64) != hl.Api.is64() )
			return false;

		debugInfos = {
			stackTop : readPointer(),
			codeStart : readPointer(),
			codeSize : sock.input.readInt32(),
			functions : [],
		};
		var nfunctions = sock.input.readInt32();
		if( nfunctions != code.functions.length )
			return false;
		for( i in 0...nfunctions ) {
			var nops = sock.input.readInt32();
			if( code.functions[i].debug.length >> 1 != nops )
				return false;
			var start = sock.input.readInt32();
			var large = sock.input.readByte() != 0;
			var offsets = sock.input.read((nops + 1) * (large ? 4 : 2));
			debugInfos.functions.push({
				start : start,
				large : large,
				offsets : offsets,
			});
		}
		return true;
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
			if( cmd != Timeout ) break;
		}
		stoppedThread = tid;
		return cmd;
	}

	public function resume() {
		if( !DebugApi.resume(PID, stoppedThread) )
			throw "Could not resume "+stoppedThread;
		stoppedThread = null;
	}

	public function getBackTrace() : Array<{ file : String, line : Int }> {
		throw "TODO";
	}

	public function addBreakPoint( file : String, line : Int ) {
		var ifile = fileIndexes.get(file);
		if( ifile == null )
			ifile = fileIndexes.get(file.split("\\").join("//").toLowerCase());

		var functions = functionsByFile.get(ifile);
		if( ifile == null || functions == null )
			throw "File not part of compiled code: " + file;

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

		// check already defined
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
		}
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
		dbg.loadCode(file);

		if( !dbg.startDebug(pid, debugPort) ) {
			dumpProcessOut();
			error("Failed to access process #" + pid+" on port "+debugPort+" for debugging");
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
				for( b in dbg.getBackTrace() )
					Sys.println(b.file+":" + b.line);
			case "b", "break":
				var file = args.shift();
				var line = Std.parseInt(args.shift());
				try {
					dbg.addBreakPoint(file, line);
					Sys.println("Breakpoint set");
				} catch( e : Dynamic ) {
					Sys.println(e);
				}
			default:
				Sys.println("Unknown command " + r);
			}

		}
	}

}