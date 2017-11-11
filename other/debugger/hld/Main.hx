package hld;

/**
	Commandline interface - GDB like
**/
class Main {

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

		var api = new hld.HLDebugApi(pid);
		var dbg = new hld.Debugger();
		var breaks = [];
		dbg.loadModule(sys.io.File.getBytes(file));

		if( !dbg.connect(api, "127.0.0.1", debugPort) ) {
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
				Sys.println(frameStr(dbg.getStackFrame()));
			case "frame","f":
				if( args.length == 1 )
					dbg.currentStackFrame = Std.parseInt(args[0]);
				Sys.println(frameStr(dbg.getStackFrame()));
			case "up":
				dbg.currentStackFrame += args.length == 0 ? 1 : Std.parseInt(args[0]);
				if( dbg.currentStackFrame >= dbg.stackFrameCount )
					dbg.currentStackFrame = dbg.stackFrameCount - 1;
				Sys.println(frameStr(dbg.getStackFrame()));
			case "down":
				dbg.currentStackFrame -= args.length == 0 ? 1 : Std.parseInt(args[0]);
				if( dbg.currentStackFrame < 0 )
					dbg.currentStackFrame = 0;
				Sys.println(frameStr(dbg.getStackFrame()));
			case "b", "break":
				var file = args.shift();
				var line = Std.parseInt(args.shift());
				if( dbg.addBreakpoint(file, line) ) {
					breaks.push({file:file, line:line});
					Sys.println("Breakpoint set");
				} else
					Sys.println("No breakpoint set");
/*			case "p", "print":
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
*/			case "clear":
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