class Statics {

	static var BASE_PATH = "../../";

	static function loopRec( dir : String ) {
		for( f in sys.FileSystem.readDirectory(BASE_PATH+dir) ) {
			var path = dir + "/" + f;
			if( sys.FileSystem.isDirectory(BASE_PATH+path) ) {
				loopRec(path);
				continue;
			}
			var ext = f.split(".").pop().toLowerCase();
			if( ext != "c" && ext != "cpp" ) continue;
			
			var lineNum = 0;
			for( l in sys.io.File.getContent(BASE_PATH+path).split("\n") ) {
				lineNum++;
				var l = StringTools.rtrim(l);
				if( l == "" ) continue;
				switch( l.charCodeAt(0) ) {
				case '\t'.code, '*'.code, '#'.code, '}'.code:
					continue;
				default:
					if( !StringTools.endsWith(l,";") )
						continue;
					// function decl
					if( StringTools.endsWith(l,");") )
						continue;
					if( l.indexOf(" const ") > 0 )
						continue;
					for( w in ["HL_PRIM ","HL_API ","DEFINE_PRIM","typedef","struct","//","/*"," *"," "] )
						if( StringTools.startsWith(l,w) ) {
							l = null;
							break;
						}
					if( l == null )
						continue;
					Sys.println('$path:$lineNum: $l');
				}
			}
		}
	}

	static function main() {
		for( dir in ["src","libs"] )
			loopRec(dir);
	}

}