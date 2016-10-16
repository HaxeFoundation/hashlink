typedef Target = {
	var name : String;
	var out : String;
	var cmd : String;
	var args : Array<String>;
	@:optional var startup : Float;
}

class Benchs {

	public static function result( v : Dynamic ) {
		#if sys
		Sys.println(v);
		#else
		trace(v);
		#end
	}

	#if neko

	static function main() {


		var selected = Sys.args()[0];
		var targets : Array<Target> = [
			{ name : "cpp", out : "cpp -D HXCPP_SILENT", cmd : "cpp/$name.exe", args : [] },
			{ name : "hl", out : "bench.hl", cmd : "hl", args : ["bench.hl"] },
			{ name : "hlc", out : "bench.c", cmd : "hlc", args : [] },
			{ name : "js", out : "bench.js", cmd : "node", args : ["bench.js"] },
			{ name : "neko", out : "bench.n", cmd : "neko", args : ["bench.n"] },
		];

		var all = [for( f in sys.FileSystem.readDirectory(".") ) if( StringTools.endsWith(f, ".hx") ) f];
		all.sort(Reflect.compare);
		while( all[all.length - 1].charAt(0) == "_" )
			all.unshift(all.pop());

		var gcc = "gcc";
		if( Sys.systemName() == "Windows" )
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
				if( Sys.command('haxe -${t.name == "hlc" ? "hl" : t.name} ${t.out} -main $name -dce full') != 0 ) {
					Sys.println(t.name+" failed to compile");
					continue;
				}

				if( t.cmd == "hlc" ) {
					// build
					if( Sys.command('$gcc -O3 -m32 -std=c11 -o hlc -I ../../src -L ../.. -lhl bench.c ../../src/hlc_main.c') != 0 ) {
						Sys.println(t.name+" failed to compile");
						continue;
					}
				}

				function run() {

					var totT = 0., count = 0;
					var r = null;

					while( totT < 0.5 ) {
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

					Sys.println("\t" + t.name+"=" + Std.int(et*100)/100);
				}
				run();
			}
		}
	}

	#end

}