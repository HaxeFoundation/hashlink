import sys.io.File;
import haxe.io.Path;
using StringTools;

class NinjaGenerator {
	var buf:StringBuf;

	public function new() {
		buf = new StringBuf();
	}

	function comment(value:String, empty_line = false) {
		buf.add('# $value \n');
		if (empty_line) buf.add('\n');
	}

	function bind(name:String, value:String) {
		buf.add('$name = $value\n\n');
	}

	function rule(name:String, args:Map<String, String>) {
		buf.add('rule $name\n');
		for (key => value in args) {
			buf.add('  $key = $value\n');
		}
		buf.add('\n');
	}

	function build(out:Array<String>, rule:String, input:Array<String>, ?args:Map<String, String>) {
		if(args == null) args = [];
		buf.add('build ${out.join(' ')}: $rule ${input.join(' ')}\n');
		for (key => value in args) {
			buf.add('  $key = $value\n');
		}
		buf.add('\n');
	}

	function save(path:String) {
		var str = this.buf.toString();
		File.saveContent(path, str);
	}

	public static function gen(config: HlcConfig, output: String) {
		var gen = new NinjaGenerator();
		gen.comment('Automatically generated file, do not edit', true);
		gen.bind('ninja_required_version', '1.2');

		var compiler_flavor: CCFlavor = switch Sys.systemName() {
			case "Windows": MSVC;
			case _: GCC;
		}

		switch compiler_flavor {
			case GCC:
				var prefix = "/usr/local";
				if (Sys.systemName() == "Mac") {
					var proc = new sys.io.Process("brew", ["--prefix", "hashlink"]);
					proc.stdin.close();
					if (proc.exitCode(true) == 0) {
						var path = proc.stdout.readAll().toString().trim();
						if (sys.FileSystem.exists(path)) {
							prefix = path;
						}
					}
				}
				var opt_flag = config.defines.exists("debug") ? "-g" : '-O2';
				var rpath = switch Sys.systemName() {
					case "Mac": '-rpath @executable_path -rpath $prefix/lib';
					case _: '-Wl,-rpath,$$ORIGIN:$prefix/lib';
				};
				gen.bind('cflags', '$opt_flag -std=c11 -DHL_MAKE -Wall -I. -pthread');
				final libflags = config.libs.map((lib) -> switch lib {
					case "std": "-lhl";
					case "uv": '$prefix/lib/$lib.hdll -luv';
					case var lib: '$prefix/lib/$lib.hdll';
				}).join(' ');
				gen.bind('ldflags', '-pthread -lm -L$prefix/lib $libflags $rpath');
				gen.rule('cc', [
					"command" => "cc -MD -MF $out.d $cflags -c $in -o $out",
					"deps" => "gcc",
					"depfile" => "$out.d",
				]);
				gen.rule('ld', [
					"command" => "cc $in -o $out $ldflags"
				]);
			case MSVC:
				gen.bind('hashlink', Sys.getEnv('HASHLINK'));
				gen.bind('cflags', "/DHL_MAKE /std:c11 /I. /I$hashlink\\include");
				final libflags = config.libs.map((lib) -> switch lib {
					case "std": "libhl.lib";
					case var lib: '$lib.lib';
				});
				gen.bind('ldflags', "/LIBPATH:$hashlink " + libflags);
				gen.rule('cc', [
					"command" => "cl.exe /nologo /showIncludes $cflags /c $in /Fo$out",
					"deps" => "msvc",
				]);
				gen.rule('ld', [
					"command" => "link.exe /nologo /OUT:$out $ldflags @$out.rsp",
					"rspfile" => "$out.rsp",
					"rspfile_content" => "$in"
				]);
		}

		final objects = [];

		for (file in config.files) {
			final out_path = haxe.io.Path.withExtension(file, 'o');
			objects.push(out_path);
			gen.build([out_path.toString()], "cc", [file], []);
		}

		final exe_path = Path.withExtension(Path.withoutDirectory(output), switch compiler_flavor {
			case MSVC: "exe";
			case GCC: null;
		});
		gen.build([exe_path], 'ld', objects, []);

		gen.save(Path.join([Path.directory(output), 'build.ninja']));
	}

	public static function run(dir:String) {
		switch Sys.systemName() {
			case "Windows":
				var devcmd = findVsDevCmdScript();
				var devcmd = haxe.SysTools.quoteWinArg(devcmd, true);
				Sys.command('$devcmd && ninja -C $dir');
			case _:
				Sys.command("ninja", ["-C", dir]);
		}
	}

	private static function findVsDevCmdScript(): Null<String> {
		var proc = new sys.io.Process('C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe', [
			"-latest",
			"-products", "*",
			"-requires",
			"Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
			"-property",
			"installationPath"
		]);
		proc.stdin.close();
		var stdout = proc.stdout.readAll();
		if (proc.exitCode(true) == 0) {
			var instPath = stdout.toString().trim();
			return '$instPath\\VC\\Auxiliary\\Build\\vcvars64.bat';
		} else {
			return null;
		}
	}
}

enum abstract CCFlavor(String) {
	var MSVC = "msvc";
	/**
	*	GCC, Clang, etc 
	**/
	var GCC = "gcc";
}

typedef HlcConfig = {
	var version:Int;
	var libs:Array<String>;
	var defines:haxe.DynamicAccess<String>;
	var files:Array<String>;
};

class Build {

	var output : String;
	var name : String;
	var sourcesDir : String;
	var targetDir : String;
	var dataPath : String;
	var config : HlcConfig;

	public function new(dataPath,output,config) {
		this.output = output;
		this.config = config;
		this.dataPath = dataPath;
		var path = new haxe.io.Path(output);
		this.name = path.file;
		this.sourcesDir = path.dir+"/";
		this.targetDir = this.sourcesDir;
	}

	function log(message:String) {
		if( config.defines.get("hlgen.silent") == null )
			Sys.println(message);
	}


	public function run() {
		var tpl = config.defines.get("hlgen.makefile");
		switch tpl {
			case "ninja":
				NinjaGenerator.gen(config, output);
			case var tpl:
				if( tpl != null )
					generateTemplates(tpl);
		}
		log('Code generated in $output');
		switch tpl {
			case "make":
				Sys.command("make", ["-C", targetDir]);
			case "hxcpp":
				Sys.command("haxelib", ["--cwd", targetDir, "run", "hxcpp", "Build.xml"].concat(config.defines.exists("debug") ? ["-Ddebug"] : []));
			case "vs2019", "vs2022":
				Sys.command("make", ["-C", targetDir]);
			case "ninja":
				NinjaGenerator.run(Path.directory(output));
			case null:
				var suggestion = (Sys.systemName() == "Windows") ? "vs2019" : "make";
				log('Set -D hlgen.makefile=${suggestion} for automatic native compilation');
			case unimplemented:
				log('Automatic native compilation not yet implemented for $unimplemented');
		}
	}

	function isAscii( bytes : haxe.io.Bytes ) {
		// BOM CHECK
		if( bytes.length > 3 && bytes.get(0) == 0xEF && bytes.get(1) == 0xBB && bytes.get(2) == 0xBF )
			return true;
		var i = 0;
		var len = bytes.length;
		while( i < len ) {
			var c = bytes.get(i++);
			if( c == 0 || c >= 0x80 ) return false;
		}
		return true;
	}

	function generateTemplates( ?tpl ) {
		if( tpl == null || tpl == "1" )
			tpl = "vs2015";
		var srcDir = tpl;
		var jumboBuild = config.defines.get("hlgen.makefile.jumbo");
		var relDir = "";
		if( config.defines["hlgen.makefilepath"] != null ) {
			targetDir = config.defines.get("hlgen.makefilepath");
			if( !StringTools.endsWith(targetDir,"/") && !StringTools.endsWith(targetDir,"\\") )
				targetDir += "/";
			var sourcesAbs = sys.FileSystem.absolutePath(sourcesDir);
			var targetAbs = sys.FileSystem.absolutePath(targetDir);
			if( !StringTools.startsWith(sourcesAbs, targetAbs+"/") )
				relDir = sourcesAbs+"/"; // absolute
			else
				relDir = sourcesAbs.substr(targetAbs.length+1);
			relDir = relDir.split("\\").join("/");
			if( !(relDir == "" || StringTools.endsWith(relDir, "/")) )
				relDir += "/";
		}
		if( !sys.FileSystem.exists(srcDir) ) {
			srcDir = dataPath + "templates/"+tpl;
			if( !sys.FileSystem.exists(srcDir) )
				throw "Failed to find make template '"+tpl+"'";
		}
		if( StringTools.contains(sys.FileSystem.absolutePath(targetDir), sys.FileSystem.absolutePath(srcDir)) ) {
			throw "Template "+tpl+" contains "+targetDir+", can cause recursive generation";
		}

		var allFiles = config.files.copy();
		for( f in config.files )
			if( StringTools.endsWith(f,".c") ) {
				var h = f.substr(0,-2) + ".h";
				if( sys.FileSystem.exists(sourcesDir+h) )
					allFiles.push(h);
			}
		allFiles.sort(Reflect.compare);

		var files = [for( f in allFiles ) { path : f, directory : new haxe.io.Path(f).dir }];
		var directories = new Map();
		for( f in files )
			if( f.directory != null )
				directories.set(f.directory, true);
		for( k in directories.keys() ) {
			var dir = k.split("/");
			dir.pop();
			while( dir.length > 0 ) {
				var p = dir.join("/");
				directories.set(p, true);
				dir.pop();
			}
		}

		var directories = [for( k in directories.keys() ) { path : k }];
		directories.sort(function(a,b) return Reflect.compare(a.path,b.path));

		function genRec( path : String ) {
			var dir = srcDir + "/" + path;
			for( f in sys.FileSystem.readDirectory(dir) ) {
				var srcPath = dir + "/" + f;
				var parts = f.split(".");
				var isBin = parts[parts.length-2] == "bin"; // .bin.xxx file
				var isOnce = isBin || parts[parts.length-2] == "once"; // .once.xxxx file - don't overwrite existing
				var f = (isBin || isOnce) ? { parts.splice(parts.length-2,1); parts.join("."); } : f;
				var targetPath = targetDir + path + "/" + f.split("__file__").join(name);
				if( sys.FileSystem.isDirectory(srcPath) ) {
					try sys.FileSystem.createDirectory(targetPath) catch( e : Dynamic ) {};
					genRec(path+"/"+f);
					continue;
				}
				var bytes = sys.io.File.getBytes(srcPath);
				if( !isAscii(bytes) ) {
					isBin = true;
					isOnce = true;
				}
				if( isOnce && sys.FileSystem.exists(targetPath) )
					continue;
				if( isBin ) {
					sys.io.File.copy(srcPath,targetPath);
					continue;
				}
				var content = sys.io.File.getContent(srcPath);
				var tpl = new haxe.Template(content);
				var context = {
					name : this.name,
					libraries : [for( l in config.libs ) if( l != "std" ) { name : l }],
					files : files,
					relDir : relDir,
					directories : directories,
					cfiles : [for( f in files ) if( StringTools.endsWith(f.path,".c") ) f],
					hfiles : [for( f in files ) if( StringTools.endsWith(f.path,".h") ) f],
					jumboBuild : jumboBuild,
				};
				var macros = {
					makeUID : function(_,s:String) {
						var sha1 = haxe.crypto.Sha1.encode(s);
						sha1 = sha1.toUpperCase();
						return sha1.substr(0,8)+"-"+sha1.substr(8,4)+"-"+sha1.substr(12,4)+"-"+sha1.substr(16,4)+"-"+sha1.substr(20,12);
					},
					makePath : function(_,dir:String) {
						return dir == "" ? "." : (StringTools.endsWith(dir,"/") || StringTools.endsWith(dir,"\\")) ? dir : dir + "/";
					},
					upper : function(_,s:String) {
						return s.charAt(0).toUpperCase() + s.substr(1);
					},
					winPath : function(_,s:String) return s.split("/").join("\\"),
					getEnv : function(_,s:String) return Sys.getEnv(s),
					setDefaultJumboBuild: function(_, b:String) {
						context.jumboBuild ??= b;
						return "";
					}
				};
				content = tpl.execute(context, macros);
				var prevContent = try sys.io.File.getContent(targetPath) catch( e : Dynamic ) null;
				if( prevContent != content )
					sys.io.File.saveContent(targetPath, content);
			}
		}
		genRec(".");
	}

}

class Run {
	static function main() {
		var args = Sys.args();
		var originalPath = args.pop();
		var haxelibPath = Sys.getCwd()+"/";
		Sys.setCwd(originalPath);

		switch( args.shift() ) {
		case "build":
			var output = args.shift();
			var path = new haxe.io.Path(output);
			path.file = "hlc";
			path.ext = "json";
			var config = haxe.Json.parse(sys.io.File.getContent(path.toString()));
			new Build(haxelibPath,output,config).run();
		case "run":
			var output = args.shift();
			if( StringTools.endsWith(output,".c") ) return;
			Sys.command("hl "+output);
		case cmd:
			Sys.println("Unknown command "+cmd);
			Sys.exit(1);
		}
	}
}
