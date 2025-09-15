class Build {

	var output : String;
	var name : String;
	var sourcesDir : String;
	var targetDir : String;
	var dataPath : String;
	var config : {
		var version : Int;
		var libs : Array<String>;
		var defines : haxe.DynamicAccess<String>;
		var files : Array<String>;
	};

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


	public function generate() {
		var tpl = config.defines.get("hlgen.makefile");
		if( tpl != null )
			generateTemplates(tpl);
		log('Code generated in $output');
	}

	public function compile() {
		var tpl = config.defines.get("hlgen.makefile");
		return switch tpl {
			case "make":
				Sys.command("make", ["-C", targetDir]);
			case "hxcpp":
				Sys.command("haxelib", ["--cwd", targetDir, "run", "hxcpp", "Build.xml"].concat(config.defines.exists("debug") ? ["-Ddebug"] : []));
			case "vs2019", "vs2022":
				var vswhereProc = new sys.io.Process("C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe", ["-requires", "Microsoft.Component.MSBuild", "-find", "MSBuild",
					"-version", tpl == "vs2019" ? "[16.0,17.0]" : "[17.0,18.0]"
				]);
				var code = 0;
				if( vswhereProc.exitCode() == 0 ) {
					var msbuildPath = StringTools.trim(try vswhereProc.stdout.readLine().toString() catch (e:haxe.io.Eof) "");
					if( msbuildPath.length > 0 ) {
						var prevCwd = Sys.getCwd();
						var msbuild = '$msbuildPath\\Current\\Bin\\MSBuild.exe';
						var msbuildArgs = ['$name.sln', '-t:$name', "-nologo", "-verbosity:minimal", "-property:Configuration=Release", "-property:Platform=x64"];
						log('"$msbuild"' + " " + msbuildArgs.join(" "));
						Sys.setCwd(targetDir);
						code = Sys.command(msbuild, msbuildArgs);
						Sys.setCwd(prevCwd);
					} else {
						log('Failed to find a valid MSbuild installation for template $tpl.');
						code = 1;
					}
				} else {
					log("vswhere error: " + vswhereProc.stdout.readAll().toString() + vswhereProc.stderr.readAll().toString());
					code = vswhereProc.exitCode();
				}
				vswhereProc.close();
				code;
			case null:
				var suggestion = (Sys.systemName() == "Windows") ? "vs2019" : "make";
				log('Set -D hlgen.makefile=${suggestion} for automatic native compilation');
				0;
			case unimplemented:
				log('Automatic native compilation not yet implemented for $unimplemented');
				0;
		};
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
			final build = new Build(haxelibPath,output,config);
			build.generate();
			Sys.exit(build.compile());
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
