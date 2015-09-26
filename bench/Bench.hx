class Bench {

	static var BENCH_COUNT = 2;

	static function bestTime( cmd : String, args : Array<String> ) {
		var out = null, bestTime = 1e9;
		var targs = args.copy();
		targs.unshift("./"+cmd);
		targs.unshift("%e");
		targs.unshift("-f");
		for( i in 0...BENCH_COUNT ) {
			var t = haxe.Timer.stamp();
			var p = new sys.io.Process("times", targs);
			var code = p.exitCode();
			if( code != 0 ) {
				Sys.println(cmd + " has exit with code " + code);
				return null;
			}
			var o = StringTools.trim(p.stdout.readAll().toString());
			var t = Std.parseFloat(p.stderr.readAll().toString());
			if( out == null ) {
				out = o;
				bestTime = t;
			} else {
				if( out != o )
					Sys.println("Varying output " + o + " != " + out);
				if( t < bestTime )
					bestTime = t;
			}
			p.close();
		}
		return { t : bestTime, out : out };
	}

	public static function main() {
		var count = 0;
		for( f in sys.FileSystem.readDirectory(".") ) {
			if( !sys.FileSystem.isDirectory(f) ) continue;
			if( !sys.FileSystem.exists(f + "/main.c") || !sys.FileSystem.exists(f + "/Main.hx") ) continue;
			Sys.println(f);

			// C32
			var p = new sys.io.Process("cl.exe", ["/nologo", "/Ot", "/arch:SSE2", "/fp:fast", f + "/main.c"]);
			var code = p.exitCode();
			if( code != 0 ) {
				Sys.println("C32 Compilation failed with code " + code);
				continue;
			}
			var c32 = bestTime("main.exe", []);
			if( c32 == null ) continue;
			Sys.println("  C-32 " + c32.t);

			// C64
			var lib = Sys.getEnv("LIB");
			Sys.putEnv("LIB", "C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\lib\\amd64;C:\\Program Files\\Microsoft SDKs\\Windows\\v7.1\\Lib\\x64;" + lib);
 			var p = new sys.io.Process("C:\\Program Files (x86)\\Microsoft Visual Studio 10.0\\VC\\bin\\amd64\\cl.exe", ["/nologo", "/Ot", "/fp:fast", f + "/main.c"]);
			var code = p.exitCode();
			Sys.putEnv("LIB", lib); // restore
			if( code != 0 ) {
				Sys.println("C64 Compilation failed with code " + code);
				continue;
			}
			var c64 = bestTime("main.exe", []);
			if( c64 == null ) continue;
			if( c32.out != c64.out )
				Sys.println("32/64 output vary : " + c32.out + " != " + c64.out);
			Sys.println("  C-64 " + c64.t);

			// HAXE
			var p = new sys.io.Process("haxe", ["-cp", f, "-dce", "full", "-hl", "main.hl", "-main", "Main"]);
			var code = p.exitCode();
			if( code != 0 ) {
				Sys.println("Haxe Compilation failed with code " + code);
				Sys.println(p.stderr.readAll().toString());
				continue;
			}

			// HL32
			var h32 = bestTime("../Release/hl.exe", ["main.hl"]);
			if( h32 == null ) continue;
			if( h32.out != c32.out )
				Sys.println("Bad HL ouput : " + h32.out + " should be " + c32.out);
			Sys.println("  HL-32 " + h32.t + " " + Math.round(c32.t * 100 / h32.t) + "%");

			// HL64
			var h64 = bestTime("../x64/Release/hl.exe", ["main.hl"]);
			if( h64 == null ) continue;
			if( h64.out != c64.out )
				Sys.println("Bad HL ouput : " + h64.out + " should be " + c64.out);
			Sys.println("  HL-64 " + h64.t + " " + Math.round(c64.t * 100 / h64.t) + "%");

			count++;
		}
		Sys.println(count+" DONE");
	}

}