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

		var f = new flash.filesystem.File(flash.filesystem.File.applicationDirectory.nativePath + "/flashout.txt");
		var o = new flash.filesystem.FileStream();
		o.open(f, flash.filesystem.FileMode.WRITE);
		o.writeUTFBytes(Std.string(v));
		o.close();

		flash.desktop.NativeApplication.nativeApplication.exit();
		#else
		trace(v);
		#end
	}

	#if neko

	static function main() {


		var selected = Sys.args()[0];
		var targets : Array<Target> = [
			{ name : "cpp", out : "cpp", cmd : "cpp/$name.exe", args : [], extraArgs : "-D HXCPP_SILENT" },
			{ name : "hl", out : "bench.hl", cmd : "hl", args : ["bench.hl"] },
			{ name : "hlc", out : "bench.c", cmd : "hlc", args : [] },
			{ name : "js", out : "bench.js", cmd : "node", args : ["bench.js"] },
			{ name : "neko", out : "bench.n", cmd : "neko", args : ["bench.n"] },
			{ name : "swf", out : "bench.swf", cmd : "adl", args : ["bench.air"], extraArgs : "-lib air3" },
		];

		var all = [for( f in sys.FileSystem.readDirectory(".") ) if( StringTools.endsWith(f, ".hx") ) f];
		all.sort(Reflect.compare);
		while( all[all.length - 1].charAt(0) == "_" )
			all.unshift(all.pop());

		var gcc = "gcc";
		var is32 = false;
		if( Sys.systemName() == "Windows" && is32 )
			gcc = "i686-pc-cygwin-gcc";

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

				if( t.cmd == "hlc" ) {
					// build
					if( Sys.command('$gcc -O3 ${is32?'-m32':''} -std=c11 -o hlc -I ../../src -L ../.. -lhl bench.c ../../src/hlc_main.c') != 0 ) {
						Sys.println(t.name+" failed to compile");
						continue;
					}
				}

				function run() {

					var totT = 0., count = 0;
					var r = null;

					while( totT < 1. ) {
						var t0 = haxe.Timer.stamp();
						var p = new sys.io.Process(t.cmd.split("$name").join(name),t.args);
						var code = p.exitCode();
						if( code != 0 ) {
							Sys.println(t.name+" errored with code "+code);
							return;
						}
						var et = haxe.Timer.stamp() - t0;
						totT += et;
						count++;
						if( t.name == "swf" )
							r = StringTools.trim(sys.io.File.getContent("flashout.txt"));
						else
							r = StringTools.trim(p.stdout.readAll().toString());
						if( r != result ) {
							Sys.println(t.name+" result "+r+" but expected "+result);
							return;
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