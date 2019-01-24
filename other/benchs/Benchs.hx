typedef Target = {
	var name : String;
	var out : String;
	var cmd : String;
	var args : Array<String>;
	@:optional var extraArgs : String;
	@:optional var startup : Float;
}

class Benchs {

	public static function result( v : Dynamic ) {
		#if sys
		Sys.println(v);
		#elseif flash

		var t = flash.Lib.getTimer();
		var f = new flash.filesystem.File(flash.filesystem.File.applicationDirectory.nativePath + "/flashout.txt");
		var o = new flash.filesystem.FileStream();
		o.open(f, flash.filesystem.FileMode.WRITE);
		o.endian = flash.utils.Endian.LITTLE_ENDIAN;
		o.writeFloat(t/1000.);
		o.writeUTFBytes(Std.string(v));
		o.close();

		flash.desktop.NativeApplication.nativeApplication.exit();
		#elseif js
		untyped console.log(v);
		#else
		trace(v);
		#end
	}

	#if neko

	static function main() {

		var isWin = Sys.systemName() == "Windows";
		var args = Sys.args();
		var is32 = false;
		var checkFlash = false;
		
		while( args.length > 0 ) {
			switch( args[0] ) {
			case "-32": 
				is32 = true;
			case "-swf":
				checkFlash = true;
			default:
				break;
			}
			args.shift();
		}
		
		var selected = args.shift();
		var targets : Array<Target> = [
			{ name : "hl", out : "bench.hl", cmd : "hl", args : ["bench.hl"] },
			{ name : "hlc", out : "hlc/bench.c", cmd : isWin ? "hlc.exe":"./hlc", args : [] },
			{ name : "js", out : "bench.js", cmd : "node", args : ["bench.js"] },
			{ name : "neko", out : "bench.n", cmd : "neko", args : ["bench.n"] },
			{ name : "cpp", out : "cpp", cmd : "cpp/$name" + (isWin ? ".exe" : ""), args : [], extraArgs : "-D HXCPP_SILENT -D HXCPP_GC_GENERATIONAL" },
		];
		if( checkFlash )
			targets.push({ name : "swf", out : "bench.swf", cmd : "adl", args : ["bench.air"], extraArgs : "-lib air3" });

		var all = [for( f in sys.FileSystem.readDirectory(".") ) if( StringTools.endsWith(f, ".hx") ) f];
		all.sort(Reflect.compare);
		while( all[all.length - 1].charAt(0) == "_" )
			all.unshift(all.pop());

		var gcc = "gcc";
		if( isWin && is32 )
			gcc = "i686-pc-cygwin-gcc";
		
		var useMSVC = isWin;
		if( useMSVC ) {
			var path = Sys.getEnv("PATH").split(";");
			var found = false;
			for( p in path )
				if( sys.FileSystem.exists(p+"/cl.exe") ) {
					if( Sys.getEnv("INCLUDE") == null ) {
						Sys.println("INCLUDE env var not set for MSVC, please set and restart");
						Sys.exit(1);
					}
					found = true;
					break;
				}
			if( !found )
				useMSVC = false;
		}

		for( f in all ) {

			if( f == "Benchs.hx" ) continue;
			var name = f.substr(0, -3);
			Sys.println(name);

			if( selected != null && selected.toLowerCase() != name.toLowerCase() ) {
				Sys.println("\tskip");
				continue;
			}

			var code = sys.io.File.getContent(f);
			var result = ~/@:result\(([^)]+)\)/;
			var result = result.match(code) ? result.matched(1) : "no result specified";

			for( t in targets ) {

				Sys.print("\t" + t.name+"...\r");

				var hargs = ["-" + (t.name == "hlc" ? "hl" : t.name), t.out, "-main", name, "-dce", "full"];
				if( t.extraArgs != null )
					for( a in t.extraArgs.split(" ") )
						hargs.push(a);
				var phaxe = new sys.io.Process("haxe", hargs);
				phaxe.stdout.readAll();
				if( phaxe.exitCode() != 0 ) {
					Sys.println(t.name+" failed to compile");
					continue;
				}

				if( t.name == "hlc" ) {
					// build
					var cmd;
					if( useMSVC )
						cmd = 'cl.exe /nologo /Ox /I hlc /I ../../src /Fehlc.exe hlc/bench.c ../../Release/libhl.lib';
					else
						cmd = '$gcc -O3 ${is32?'-m32':''} -std=c11 -o hlc -I hlc -I ../../src -L ../.. hlc/bench.c -lhl -lm';
					if( Sys.command(cmd) != 0 ) {
						Sys.println("Failed to run "+cmd);
						Sys.println(t.name+" failed to compile");
						continue;
					}
				}

				function run() {

					var totT = 0., count = 0;
					var r = null;

					var firstRun = true;
					while( totT < 1. ) {
						var t0 = haxe.Timer.stamp();
						var p = new sys.io.Process(t.cmd.split("$name").join(name),t.args);
						var code = try p.exitCode() catch( e : Dynamic ) -99;
						if( code != 0 ) {
							Sys.println(t.name+" errored with code "+code);
							return;
						}
						var et = haxe.Timer.stamp() - t0;
						if( t.name == "swf" ) {
							var bytes = sys.io.File.getBytes("flashout.txt");
							et = bytes.getFloat(0);
							r = StringTools.trim(bytes.sub(4,bytes.length-4).toString());
						} else
							r = StringTools.trim(p.stdout.readAll().toString());
						if( r != result ) {
							Sys.println(t.name+" result "+r+" but expected "+result);
							return;
						}
						if (firstRun)
							firstRun = false;
						else {
							totT += et;
							count++;
						}
					}

					var et = totT / count;
					if( t.startup == null )
						t.startup = et;
					else
						et -= t.startup;

					Sys.println("\t" + StringTools.rpad(t.name," ",5) + Std.int(et*100)/100);
				}
				run();
			}
		}
	}

	#end

}
