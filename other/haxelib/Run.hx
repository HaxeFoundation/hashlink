class Run {
	static function main() {
		var args = Sys.args();
		var originalPath = args.pop();
		Sys.setCwd(originalPath);
		
		switch( args.shift() ) {
		case "build":		
			var output = args.shift();
			Sys.println("Code generated in "+output+" automatic native compilation not yet implemented");
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