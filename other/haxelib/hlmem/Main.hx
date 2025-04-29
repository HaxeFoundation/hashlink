package hlmem;

class Main {
	static var args : Array<String>;
	static var analyzer : Analyzer;

	static var sortByCount : Bool = false;

	static function parseArgs( str: String ) {
		str = StringTools.trim(str);
		var i = 0;
		var tok = "";
		var args = [];
		var escape = false;
		while(i != str.length) {
			var c = str.charAt(i++);
			if(c == '"') {
				escape = !escape;
			}
			else {
				if(c == " " && !escape) {
					if(tok.length > 0) args.push(tok);
					tok = "";
				}
				else
					tok += c;
			}
		}
		if(tok.length > 0) args.push(tok);
		return args;
	}

	static function command() : Bool {
		Sys.print(Analyzer.withColor("> ", Red));
		var args = parseArgs(Main.args.length > 0 ? Main.args.shift() : Sys.stdin().readLine());
		var cmd = args.shift();
		var mem = analyzer.getMainMemory();
		switch( cmd ) {
		case "exit", "quit", "q":
			return false;
		case "stats":
			var res = mem.getMemStats();
			res.print();
		case "types":
			var res = mem.getBlockStatsByType();
			res.sort(sortByCount);
			res.print();
		case "false":
			var res = mem.getFalsePositives(args.shift());
			res.print();
		case "unknown":
			var res = mem.getUnknown();
			res.sort(sortByCount);
			res.print();
		case "locate":
			var lt = mem.resolveType(args.shift());
			if( lt != null ) {
				var res = mem.locate(lt, Std.parseInt(args.shift()));
				res.sort(sortByCount);
				res.print();
			}
		case "count":
			var lt = mem.resolveType(args.shift());
			if( lt != null ) {
				var res = mem.count(lt, args);
				res.sort(sortByCount);
				res.printWithSum = true;
				res.print();
			}
		case "parents":
			var lt = mem.resolveType(args.shift());
			if( lt != null ) {
				var res = mem.parents(lt);
				res.sort(sortByCount);
				res.print();
			}
		case "subs":
			var lt = mem.resolveType(args.shift());
			if( lt != null ) {
				var res = mem.subs(lt, Std.parseInt(args.shift()));
				res.sort(sortByCount);
				res.print();
			}
		case "sort":
			switch( args.shift() ) {
			case "mem":
				sortByCount = false;
			case "count":
				sortByCount = true;
			case mode:
				Sys.println("Unknown sort mode " + mode);
			}
		case "fields":
			switch( args.shift() ) {
			case "full":
				Analyzer.displayFields = Full;
			case "none":
				Analyzer.displayFields = None;
			case "parents":
				Analyzer.displayFields = Parents;
			case mode:
				Sys.println("Unknown fields mode " + mode);
			}
		case "filter":
			switch( args.shift() ) {
			case "n" | "none":
				mem.filterMode = None;
			case "i" | "intersect":
				mem.filterMode = Intersect;
			case "u" | "unique":
				mem.filterMode = Unique;
			case mode:
				Sys.println("Unknown filter mode " + mode);
			}
			mem.buildFilteredBlocks();
		case "nextDump":
			analyzer.nextDump();
		case "lines":
			var v = args.shift();
			if( v != null )
				Analyzer.maxLines = Std.parseInt(v);
			Sys.println(Analyzer.maxLines == 0 ? "Lines limit disabled" : Analyzer.maxLines + " maximum lines displayed");
		case null:
			// Ignore
		default:
			Sys.println("Unknown command " + cmd);
		}
		return true;
	}

	static function main() {
		analyzer = new Analyzer();
		args = Sys.args();

		//hl.Gc.dumpMemory(); Sys.command("cp memory.hl test.hl");

		var code = null, memory = null;
		while( args.length > 0 ) {
			var arg = args.shift();
			if( StringTools.endsWith(arg, ".hl") ) {
				code = arg;
				analyzer.loadBytecode(arg);
				continue;
			}
			if( arg == "-c" || arg == "--color" )
				continue;
			if( arg == "--no-color" ) {
				Analyzer.useColor = false;
				continue;
			}
			if( arg == "--args" ) {
				Analyzer.displayProgress = false;
				break;
			}
			if (memory == null) {
				memory = arg;
			}
			analyzer.loadMemoryDump(arg);
		}
		if( code != null && memory == null ) {
			var dir = new haxe.io.Path(code).dir;
			if( dir == null ) dir = ".";
			memory = dir+"/hlmemory.dump";
			if( sys.FileSystem.exists(memory) )
				analyzer.loadMemoryDump(memory);
		}
		analyzer.build();

		while( command() ) {
		}
	}
}
