package hld;

#if debug
@:jsRequire('source-map-support')
extern class SMS {
	public static function install() : Void;
}
class SMSInstall {
	static function __init__() : Void {
		SMS.install();
	}
}
#end

/**
	Commandline interface - GDB like
**/
class Main {

	var args = Sys.args();
	var debugPort = 5001;
	var file = null;
	var pid : Null<Int>;
	var breaks = [];
	var dbg : hld.Debugger;
	var stdin = Sys.stdin();

	#if nodejs
	var process : js.node.child_process.ChildProcess;
	#else
	var process : sys.io.Process;
	#end

	function new() {
	}

	function error( msg ) {
		Sys.stderr().writeString(msg + "\n");
		Sys.exit(1);
	}

	function init() {
		var cmd = "hl";
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
			case "--cmd":
				cmd = args.shift();
			case "--cwd":
				Sys.setCwd(args.shift());
			default:
				error("Unsupported parameter " + param);
			}
		}
		file = args.shift();
		if( file == null ) {
			Sys.println("hldebug [-port <port>] [--cwd <path>] <file.hl> [--args <args>] or [<commands>]");
			Sys.exit(1);
		}
		if( !sys.FileSystem.exists(file) )
			error(file+" not found");

		var hlArgs = [];
		if( args[0] == "--args" ) {
			args.shift();
			while( true ) {
				var a = args.shift();
				if( a == null || a == "--" ) break;
				hlArgs.push(a);
			}
		}

		if( pid == null ) {
			var args = ["--debug", "" + debugPort, "--debug-wait", file].concat(hlArgs);
			#if nodejs
			process = js.node.ChildProcess.spawn(cmd, args);
			process.stdout.on("data", function(data:String) Sys.print(data));
			process.stderr.on("data", function(data:String) Sys.stderr().writeString(data));
			pid = process.pid;
			#else
			process = new sys.io.Process(cmd, args);
			pid = process.getPid();
			#end
		}

		dbg = new hld.Debugger();
		dbg.loadModule(sys.io.File.getBytes(file));

		function getAPI() : hld.Api {
			#if hl
			return new hld.HLDebugApi(pid, dbg.is64);
			#elseif nodejs
			switch( Sys.systemName() ) {
			case "Windows":
				return new hld.NodeDebugApi(pid, dbg.is64);
			default:
				return new hld.NodeDebugApiLinux(pid,dbg.is64);
			}
			#else
			throw "This platform does not have a debug API";
			return null;
			#end
		}

		if( !dbg.connect("127.0.0.1", debugPort) || !dbg.init(getAPI()) ) {
			dumpProcessOut();
			error("Failed to access process #" + pid + " on port " + debugPort + " for debugging");
			return;
		}

		while( command() ) {
		}

	}

	function frameStr( f : Debugger.StackInfo, ?debug ) {
		if( f == null ) return "???";
		return f.file+":" + f.line + (f.context == null ? "" : " ("+f.context.obj.name+"::"+f.context.field+")") + (debug ? " @"+f.ebp.toString():"");
	}

	function dumpProcessOut() {
		if( process == null ) return;
		#if nodejs
		process.kill();
		#else
		if( process.exitCode(false) == null ) process.kill();
		Sys.print(process.stdout.readAll().toString());
		Sys.stderr().writeString(process.stderr.readAll().toString());
		#end
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
			#if nodejs
			Sys.println("Process has exit");
			Sys.exit(0);
			#end
			dbg.resume();
		case Breakpoint:
			Sys.println("Thread " + dbg.stoppedThread + " paused " + frameStr(dbg.getStackFrame()));
			var exc = dbg.getException();
			if( exc != null )
				Sys.println("Exception: "+dbg.eval.valueStr(exc));
		case Error:
			Sys.println("*** an error has occured, paused ***");
		case Watchbreak:
			var w = dbg.watchBreak;
			Sys.println("Watch change " + w.ptr.toString() + ":" + w.t.toString() + " = " + dbg.eval.valueStr(dbg.eval.fetch(w)) + " at "+frameStr(dbg.getStackFrame()));
		case Timeout:
			dbg.customTimeout = null;
			handleResult(dbg.pause());
		default:
			throw "assert "+r;
		}
	}

	function command() {
		#if !nodejs
		if( process != null ) {
			var ecode = process.exitCode(false);
			if( ecode != null ) {
				dumpProcessOut();
				error("Process exit with code " + ecode);
			}
		}
		#end

		Sys.print("> ");
		var r = args.shift();
		if( r == null )
			r = stdin.readLine();
		else
			Sys.println(r);
		var args = ~/[ \t\r\n]+/g.split(r);
		var cmd = args.shift();
		switch( cmd ) {
		case "q", "quit":
			dumpProcessOut();
			return false;
		case "r", "run", "c", "continue":
			var time = args.shift();
			dbg.customTimeout = time == null ? null : Std.parseFloat(time);
			while( true ) {
				var r = dbg.run();
				if( r == Handled ) continue;
				handleResult(r);
				break;
			}
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
			line = dbg.addBreakpoint(file, line);
			if( line >= 0 ) {
				breaks.push({file:file, line:line});
				Sys.println("Breakpoint set line "+line);
			} else
				Sys.println("No breakpoint set");
		case "p", "print":
			var expr = args.shift();
			if( expr == null ) {
				Sys.println("Requires expression");
				return true;
			}
			var v = try dbg.getValue(expr) catch( e : Dynamic ) {
				Sys.println("Error " + e + haxe.CallStack.toString(haxe.CallStack.exceptionStack()));
				return true;
			}
			if( v == null ) {
				Sys.println("Unknown var " + expr);
				return true;
			}
			Sys.println(dbg.eval.valueStr(v) + " : " + v.t.toString());
			var fields = dbg.eval.getFields(v);
			if( fields != null )
				for( f in fields ) {
					var fv = dbg.eval.readField(v, f);
					Sys.println("  " + f + " = " + dbg.eval.valueStr(fv) + " : " + fv.t.toString());
				}
		case "watch", "rwatch":
			var expr = args.shift();
			var v = try dbg.getRef(expr) catch( e : Dynamic ) {
				Sys.println("Error " + e + haxe.CallStack.toString(haxe.CallStack.exceptionStack()));
				return true;
			};
			if( v == null ) {
				Sys.println("Unknown var " + expr);
				return true;
			}
			if( v.ptr == null ) {
				Sys.println("Can't watch undefined var");
				return true;
			}
			try {
				dbg.watch(v, cmd == "rwatch");
			} catch( e : Dynamic ) {
				Sys.println("Error " + e + haxe.CallStack.toString(haxe.CallStack.exceptionStack()));
				return true;
			}
			Sys.println("Watching " + v.ptr.toString() + ":" + v.t.toString() + " " + dbg.eval.valueStr(dbg.eval.fetch(v)));
		case "unwatch":
			var param = args.shift();
			var count = 0;
			for( w in dbg.getWatches() )
				if( param == null || w.ptr.toString() == param ) {
					dbg.unwatch(w);
					count++;
				}
			Sys.println("Unwatch " + count + " addresses");
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
		case "thread":
			var arg = args.shift();
			if( arg != null ) {
				var tid = Std.parseInt(args.shift());
				if( tid != null ) tid = dbg.getThreads()[tid];
				if( tid != null ) dbg.setCurrentThread(tid);
			}
			Sys.println("Thread "+dbg.getThreads().indexOf(dbg.currentThread));
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
			case "threads":
				var cur = dbg.currentThread;
				var stack = dbg.currentStackFrame;
				var index = 0;
				for( tid in dbg.getThreads() ) {
					dbg.setCurrentThread(tid);
					Sys.println((tid == cur ? "*" : " ")+" Thread "+(index++)+"("+tid+") "+frameStr(dbg.getBackTrace()[0]));
				}
				dbg.setCurrentThread(cur);
				dbg.currentStackFrame = stack;
			case "statics":
				var cl = dbg.getCurrentClass();
				if( cl == null )
					Sys.println("Class not found");
				else {
					Sys.println("Class "+cl);
					var fields = dbg.getClassStatics(cl);
					for( f in fields )
						printVar(cl+"."+f);
				}
			default:
				Sys.println("Unknown info request");
			}
		case "catch":
			var arg = args.shift();
			switch( arg ) {
			case "throw":
				dbg.breakOnThrow = true;
			case "uncaught":
				dbg.breakOnThrow = false;
			default:
				Sys.println("Invalid catch mode");
			}
		case "set":
			var all = args.join(" ").split("=");
			var expr = all.shift();
			var value = all.join("=");
			if( expr == "" || value == null || value == "" ) {
				Sys.println("Syntax: set <expr>=<value>");
				return true;
			}
			var value = dbg.setValue(expr,value);
			if( value == null )
				Sys.println("Failed to set expression");
			else
				Sys.println(dbg.eval.valueStr(value) + " : " + value.t.toString());
		case "cd":
			try Sys.setCwd(args.shift()) catch( e : Dynamic ) Sys.println(""+e);
		default:
			Sys.println("Unknown command " + r);
		}
		return true;
	}

	static function main() {
		new Main().init();
		Sys.exit(0);
	}

}