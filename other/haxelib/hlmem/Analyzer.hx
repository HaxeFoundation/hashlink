package hlmem;

import hlmem.Memory;
using format.hl.Tools;

// A list of ansi colors is available at
// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797#8-16-colors
enum abstract TextColor(Int) {
	var Black = 30;
	var Red = 31;
	var Green = 32;
	var Yellow = 33;
	var Blue = 34;
	var Magenta = 35;
	var Cyan = 36;
	var White = 37;
}

class Analyzer {

	public static var displayProgress = true;
	public static var displayFields : FieldsMode = Full;
	public static var maxLines : Int = 100;
	public static var useColor : Bool = true;

	public var code : format.hl.Data;
	var mem : Memory;
	var otherMems : Array<Memory> = [];

	public function new() {
	}

	public function loadBytecode( file : String ) {
		if( code != null ) throw "Duplicate code";
		code = new format.hl.Reader(false).read(new haxe.io.BytesInput(sys.io.File.getBytes(file)));
		Analyzer.log(file + " code loaded");
	}

	public function loadMemoryDump( file : String ) : Memory {
		var m = new Memory(this);
		m.load(file);
		if( mem == null ) {
			mem = m;
		} else {
			otherMems.push(m);
		}
		return m;
	}

	public function build( filter : FilterMode = None ) {
		mem.build();
		for( m2 in otherMems ) {
			m2.buildBlockTypes();
		}
		mem.otherMems = [for (i in otherMems) i];
		mem.filterMode = filter;
		mem.buildFilteredBlocks();
	}

	public function nextDump() : Memory {
		otherMems.push(mem);
		mem = otherMems.shift();
		mem.otherMems = [for (i in otherMems) i];
		mem.build();
		mem.buildFilteredBlocks();
		var ostr = otherMems.length > 0 ? (" (others are " + otherMems.map(m -> m.memFile) + ")") : "";
		Analyzer.log("Using dump " + mem.memFile + ostr);
		return mem;
	}

	public inline function getMainMemory() {
		return mem;
	}

	public function getMemStats() : Array<hlmem.Result.MemStats> {
		var objs = [mem.getMemStats()];
		for( m2 in otherMems ) {
			objs.push(m2.getMemStats());
		}
		return objs;
	}

	public static function mb( v : Float ) : String {
		if( v < 1000 )
			return Std.int(v) + "B";
		if( v < 1024 * 1000 )
			return (Math.round(v * 10 / 1024) / 10)+"KB";
		return (Math.round(v * 10 / (1024 * 1024)) / 10)+"MB";
	}

	public static inline function logProgress( current : Int, max : Int ) {
		if( displayProgress && current % 1000 == 0 )
			Sys.print(Std.int((current * 1000.0 / max) / 10) + "%  \r");
		if( displayProgress && current == max )
			Sys.print("       \r");
	}

	public static inline function log( msg : String ) {
		Sys.println(msg);
	}

	public static inline function withColor( str : String, textColor : TextColor ) {
		if( !useColor )
			return str;
		return '\x1B[${textColor}m${str}\x1B[0m';
	}

}
