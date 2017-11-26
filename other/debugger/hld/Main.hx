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

		function frameStr( f : { file : String, line : Int, ebp : Pointer }, ?debug ) {
			return f.file+":" + f.line + (debug ? " @"+f.ebp.toString():"");
		}

		function clearBP() {
			var count = breaks.length;
			for( b in breaks )
				dbg.removeBreakpoint(b.file, b.line);
			breaks = [];
			Sys.println(count + " breakpoints removed");
		}

		function handleResult( r : hld.Api.WaitResult ) {
			switch( r ) {
			case Exit:
				dbg.resume();
			case Breakpoint:
				Sys.println("Thread " + dbg.stoppedThread + " paused " + frameStr(dbg.getStackFrame()));
				var exc = dbg.getException();
				if( exc != null )
					Sys.println("Exception: "+dbg.eval.valueStr(exc));
			case Error:
				Sys.println("*** an error has occured, paused ***");
			default:
				throw "assert "+r;
			}
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
			var r = args.shift();
			if( r == null )
				r = stdin.readLine();
			else
				Sys.println(r);
			var args = ~/[ \t\r\n]+/g.split(r);
			switch( args.shift() ) {
			case "q", "quit":
				dumpProcessOut();
				break;
			case "r", "run", "c", "continue":
				handleResult(dbg.run());
			case "bt", "backtrace":
				for( f in dbg.getBackTrace() )
					Sys.println(frameStr(f));
			case "btdebug":
				for( f in dbg.getBackTrace() )
					Sys.println(frameStr(f,true));
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
				var fileLine = args.shift().split(":");
				var line = Std.parseInt(fileLine.pop());
				var file = fileLine.join(":");
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
				var v = try dbg.getValue(path) catch( e : Dynamic ) {
					Sys.println("Error " + e + haxe.CallStack.toString(haxe.CallStack.exceptionStack()));
					continue;
				}
				if( v == null ) {
					Sys.println("Unknown var " + path);
					continue;
				}
				Sys.println(dbg.eval.valueStr(v) + " : " + v.t.toString());
				var fields = dbg.eval.getFields(v);
				if( fields != null )
					for( f in fields ) {
						var fv = dbg.eval.readField(v, f);
						Sys.println("  " + f + " = " + dbg.eval.valueStr(fv) + " : " + fv.t.toString());
					}
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
			case "next", "n":
				handleResult(dbg.step(Next));
			case "step", "s":
				handleResult(dbg.step(Into));
			case "finish":
				handleResult(dbg.step(Out));
			case "debug":
				handleResult(dbg.debugTrace(args.shift() == "step"));
			case "info":
				function printVar( name : String ) {
					var v = dbg.getValue(name);
					Sys.println(" " + name+" = " + (v == null ? "???" : dbg.eval.valueStr(v) + " : " + v.t.toString()));
				}
				switch( args.shift() ) {
				case "args":
					for( name in dbg.getCurrentVars(true) )
						printVar(name);
				case "locals":
					for( name in dbg.getCurrentVars(false) )
						printVar(name);
				case "variables":
					for( name in dbg.getCurrentVars(true).concat(dbg.getCurrentVars(false)) )
						printVar(name);
				}
			default:
				Sys.println("Unknown command " + r);
			}

		}
	}

}