class Bench {

	static var BENCH_COUNT = 2;

	static function bestTime( cmd : String, args : Array<String>, out ) {
		var bestTime = 1e9;
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
			if( out != o ) {
				Sys.println("Bad ouput : " + o + " should be " + out);
				return null;
			}
			if( t < bestTime )
				bestTime = t;
			p.close();
		}
		return { t : bestTime, out : out };
	}

	public static function main() {
		var count = 0;
		for( f in sys.FileSystem.readDirectory(".") ) {
			if( !sys.FileSystem.isDirectory(f) ) continue;
			if( !sys.FileSystem.exists(f + "/out.txt") || !sys.FileSystem.exists(f + "/Main.hx") ) continue;
			Sys.println(f);
			
			var out = StringTools.trim(sys.io.File.getContent(f+"/out.txt"));
			var c32 = null, c64 = null;

			if( sys.FileSystem.exists(f+"/main.c") ) {
				// MSVC32
				var p = new sys.io.Process("cl.exe", ["/nologo", "/Ot", "/arch:SSE2", "/fp:fast", f + "/main.c"]);
				var code = p.exitCode();
				if( code != 0 ) {
					Sys.println("MSVC32 Compilation failed with code " + code);
					continue;
				}
				c32 = bestTime("main.exe", [], out);
				if( c32 != null )
					Sys.println("  MSVC-32 " + c32.t);
	
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
				c64 = bestTime("main.exe", [], out);
				if( c64 != null )
					Sys.println("  MSVC-64 " + c64.t);

				// GCC32
				var p = new sys.io.Process("i686-pc-cygwin-gcc", ["-m32", "-msse2", "-mfpmath=sse", "-O3", f + "/main.c"]);
				var code = p.exitCode();
				if( code != 0 ) {
					Sys.println("GCC32 Compilation failed with code " + code);
					continue;
				}
				var gc32 = bestTime("a.exe", [], out);
				if( gc32 != null ) {
					Sys.println("  GCC-32 " + gc32.t);
					if( c32 == null || c32.t > gc32.t ) c32 = gc32;
				}
				
				// GCC64
				var p = new sys.io.Process("gcc", ["-m64", "-msse2", "-mfpmath=sse", "-O3", f + "/main.c"]);
				var code = p.exitCode();
				if( code != 0 ) {
					Sys.println("GCC64 Compilation failed with code " + code);
					continue;
				}
				var gc64 = bestTime("a.exe", [], out);
				if( gc64 != null ) {
					Sys.println("  GCC-64 " + gc64.t);
					if( c64 == null || c64.t > gc64.t ) c64 = gc64;
				}
			}

			// HAXE
			var p = new sys.io.Process("haxe", ["-cp", f, "-dce", "full", "-hl", "main.hl", "-main", "Main"]);
			var code = p.exitCode();
			if( code != 0 ) {
				Sys.println("Haxe Compilation failed with code " + code);
				Sys.println(p.stderr.readAll().toString());
				continue;
			}

			// HL32
			var h32 = bestTime("../Release/hl.exe", ["main.hl"], out);
			if( h32 != null )
				Sys.println("  HL-32 " + h32.t + (c32 == null ? "" : " " + Math.round(c32.t * 100 / h32.t) + "%"));

			// HL64
			var h64 = bestTime("../x64/Release/hl.exe", ["main.hl"], out);
			if( h64 != null )
				Sys.println("  HL-64 " + h64.t + (c64 == null ? "" : " " + Math.round(c64.t * 100 / h64.t) + "%"));

			count++;
		}
		Sys.println(count+" DONE");
	}

}