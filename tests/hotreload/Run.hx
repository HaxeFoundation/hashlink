import sys.FileSystem;
import sys.io.File;
import sys.io.Process;

/**
	Hot reload tests runner.

		haxe --cwd tests/hotreload --run Run [--hl <hl>] [--haxe <haxe>] [--port <port>] [--no-server] [case...]

	For every case in cases/, the versions v0, v1... are copied one after the other to the same
	directory and compiled through a compilation server, as an editor would, then the program runs
	with `hl --hot-reload v0.hl v1.hl ...` and its output is compared with expected.txt.
**/
class Run {

	static var hl = "hl";
	static var haxe = "haxe";
	static var port = 6789;
	static var useServer = true;
	static var server : Process;

	static function main() {
		var args = Sys.args();
		var filter = [];
		var i = 0;
		while( i < args.length ) {
			var a = args[i++];
			switch( a ) {
			case "--hl": hl = args[i++];
			case "--haxe": haxe = args[i++];
			case "--port": port = Std.parseInt(args[i++]);
			case "--no-server": useServer = false;
			default: filter.push(a);
			}
		}
		var root = Sys.getCwd();
		var cases = FileSystem.readDirectory(root + "cases").filter(c -> FileSystem.isDirectory(root + "cases/" + c));
		cases.sort(Reflect.compare);
		if( filter.length > 0 ) cases = cases.filter(c -> filter.indexOf(c) >= 0);

		if( useServer ) startServer();
		var failed = [], known = [];
		for( c in cases ) {
			var result = try runCase(root, c) catch( e : Dynamic ) { Sys.println("  " + e); false; };
			var knownFailure = FileSystem.exists(root + "cases/" + c + "/KNOWN_FAILURE");
			if( result )
				Sys.println("[ OK ] " + c + (knownFailure ? " (marked KNOWN_FAILURE, remove the marker)" : ""));
			else if( knownFailure ) {
				Sys.println("[KNOWN] " + c + " : " + StringTools.trim(File.getContent(root + "cases/" + c + "/KNOWN_FAILURE")));
				known.push(c);
			} else {
				Sys.println("[FAIL] " + c);
				failed.push(c);
			}
		}
		if( server != null ) server.kill();
		Sys.println('${cases.length - failed.length - known.length} passed, ${known.length} known failures, ${failed.length} failed');
		Sys.exit(failed.length == 0 ? 0 : 1);
	}

	static function startServer() {
		server = new Process(haxe, ["--wait", "" + port]);
	}

	static function compile( args : Array<String> ) {
		// the server may not listen yet
		for( i in 0...50 ) {
			var r = command(haxe, args);
			if( !useServer || r.code == 0 || (r.out + r.err).indexOf("Couldn't connect") < 0 ) return r;
			Sys.sleep(0.2);
		}
		throw "Could not connect to the compilation server on port " + port;
	}

	static function command( cmd : String, args : Array<String>, ?cwd : String ) {
		var old = Sys.getCwd();
		if( cwd != null ) Sys.setCwd(cwd);
		var p = new Process(cmd, args);
		Sys.setCwd(old);
		var out = p.stdout.readAll().toString();
		var err = p.stderr.readAll().toString();
		var code = p.exitCode();
		p.close();
		return { code : code, out : out, err : err };
	}

	static function clearDir( dir : String ) {
		if( !FileSystem.exists(dir) ) {
			FileSystem.createDirectory(dir);
			return;
		}
		for( f in FileSystem.readDirectory(dir) ) {
			var p = dir + "/" + f;
			if( FileSystem.isDirectory(p) ) {
				clearDir(p);
				FileSystem.deleteDirectory(p);
			} else
				FileSystem.deleteFile(p);
		}
	}

	static function copyDir( from : String, to : String ) {
		if( !FileSystem.exists(to) ) FileSystem.createDirectory(to);
		for( f in FileSystem.readDirectory(from) ) {
			var p = from + "/" + f;
			if( FileSystem.isDirectory(p) )
				copyDir(p, to + "/" + f);
			else
				File.saveBytes(to + "/" + f, File.getBytes(p));
		}
	}

	static function normalize( s : String ) {
		s = StringTools.replace(s, "\r\n", "\n");
		return StringTools.trim(s);
	}

	// a * in the expected line matches any word
	static function sameLine( actual : String, expected : String ) {
		if( expected.indexOf("*") < 0 ) return actual == expected;
		var parts = expected.split("*").map(p -> ~/[\^\$\.\|\?\+\(\)\[\]\{\}\\]/g.map(p, r -> "\\" + r.matched(0)));
		return new EReg("^" + parts.join("\\S+") + "$", "").match(actual);
	}

	static function runCase( root : String, name : String ) {
		var dir = root + "cases/" + name;
		var versions = [];
		while( FileSystem.exists(dir + "/v" + versions.length) )
			versions.push("v" + versions.length);
		if( versions.length == 0 ) throw "No version in " + dir;
		var work = root + "out/" + name;
		clearDir(work);
		var src = work + "/src";
		var hls = [];
		for( v in versions ) {
			// same source directory for every version, as when editing the files
			clearDir(src);
			copyDir(dir + "/" + v, src);
			var args = useServer ? ["--connect", "" + port] : [];
			// the last class path has the priority : a case can replace Main.hx
			// the entry point initializes the statics of new classes from HL 2.0
			args = args.concat(["--cwd", work, "-cp", root + "common", "-cp", "src", "-main", "Main", "-hl", v + ".hl", "-D", "hl-ver=2.0.0"]);
			if( FileSystem.exists(dir + "/build.txt") )
				for( a in File.getContent(dir + "/build.txt").split("\n") ) {
					a = StringTools.trim(a);
					if( a != "" ) args.push(a);
				}
			// a warm compilation must give the same bytecode as a cold one
			if( useServer ) args = args.concat(["-D", "hl-memo-check"]);
			var r = compile(args);
			if( r.code != 0 ) {
				Sys.println("  compilation of " + v + " failed:\n" + r.out + r.err);
				return false;
			}
			if( (r.out + r.err).indexOf("hl memo check") >= 0 ) {
				Sys.println("  compilation of " + v + ": " + StringTools.trim(r.out + r.err));
				return false;
			}
			hls.push(v + ".hl");
		}
		if( FileSystem.exists(dir + "/env.txt") )
			for( l in File.getContent(dir + "/env.txt").split("\n") ) {
				var kv = StringTools.trim(l).split("=");
				if( kv.length == 2 ) Sys.putEnv(kv[0], kv[1]);
			}
		var r = command(hl, ["--hot-reload"].concat(hls), work);
		if( FileSystem.exists(dir + "/env.txt") )
			for( l in File.getContent(dir + "/env.txt").split("\n") ) {
				var kv = StringTools.trim(l).split("=");
				if( kv.length == 2 ) Sys.putEnv(kv[0], "");
			}
		var output = normalize(r.out + r.err);
		File.saveContent(work + "/output.txt", output + "\n");
		if( r.code != 0 ) output += "\nexit code " + r.code;
		var expected = normalize(File.getContent(dir + "/expected.txt"));
		var lo = output.split("\n"), le = expected.split("\n");
		if( lo.length == le.length && [for( i in 0...lo.length ) sameLine(lo[i], le[i])].indexOf(false) < 0 ) return true;
		for( i in 0...(lo.length > le.length ? lo.length : le.length) )
			if( lo[i] == null || le[i] == null || !sameLine(lo[i], le[i]) ) {
				Sys.println('  line ${i + 1}:\n    expected: ${le[i]}\n    actual:   ${lo[i]}');
				break;
			}
		Sys.println("  full output in " + work + "/output.txt");
		return false;
	}
}
