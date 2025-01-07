package hlmem;

abstract class Result {
	abstract public function print() : Void;
}

@:structInit
class MemStats extends Result {
	public var memFile : String;
	public var free : Int;
	public var used : Int;
	public var filterUsed : Int;
	public var totalAllocated : Int;
	public var gc : Int;
	public var pagesCount : Int;
	public var pagesSize : Int;
	public var rootsCount : Int;
	public var stackCount : Int;
	public var typesCount : Int;
	public var closuresCount : Int;
	public var blockCount : Int;
	public var filterMode : Memory.FilterMode;
	public var filteredBlockCount : Int;

	public function toString() {
		var str = Analyzer.withColor("--- " + memFile + " ---", Cyan);
		str += "\n" + pagesCount + " pages, " + Analyzer.mb(pagesSize) + " memory";
		str += "\n" + rootsCount + " roots, "+ stackCount + " stacks";
		str += "\n" + typesCount + " types, " + closuresCount + " closures";
		str += "\n" + blockCount + " live blocks, " + Analyzer.mb(used) + " used, " + Analyzer.mb(free) + " free, "+ Analyzer.mb(gc) + " gc";
		if( filterMode != None )
			str += "\n" + filteredBlockCount + " blocks in filter, " + Analyzer.mb(filterUsed) + " used";
		return str;
	}

	public function print() {
		Analyzer.log(this.toString());
	}
}

class FalsePositiveStats extends Result {
	public var falses : Array<TType>;
	public function new() {
	}
	public function add( t : TType ) {
		falses.push(t);
	}
	public function sort() {
		falses.sort((t1, t2) -> t1.falsePositive - t2.falsePositive);
	}
	public function print() {
		for( f in falses )
			Analyzer.log(f.falsePositive + " count " + f + " " + f.falsePositiveIndexes + "\n    "+ [for( f in f.memFields ) format.hl.Tools.toString(f.t)]);
	}
}

class BlockStatsElement {
	var mem : Memory;
	public var tl : Array<Int>;
	public var count : Int = 0;
	public var size : Int = 0;
	public function new( mem : Memory, tl : Array<Int> ) {
		this.mem = mem;
		this.tl = tl;
	}
	public function add( size : Int ) {
		this.count ++;
		this.size += size;
	}
	public function getTypes() {
		var ttypes = [];
		for( tid in tl )
			ttypes.push(mem.getTypeById(tid));
		return ttypes;
	}
	public function getNames( withTstr : Bool = true, withId : Bool = true, withField : Bool = true ) {
		var tpath = [];
		for( tid in tl )
			tpath.push(mem.getTypeString(tid, withTstr, withId, withField));
		return tpath;
	}
}

class BlockStats extends Result {
	var mem : Memory;
	var byT : Map<String, BlockStatsElement>;
	public var allT : Array<BlockStatsElement>;
	public var printWithSum : Bool;

	public function new( mem : Memory ) {
		this.mem = mem;
		this.byT = [];
		this.allT = [];
		this.printWithSum = false;
	}

	public function add( t : TType, size : Int ) {
		return addPath([t == null ? 0 : t.tid], size);
	}

	public function addPath( tl : Array<Int>, size : Int ) {
		var key = tl.join(" ");
		var inf = byT.get(key);
		if( inf == null ) {
			inf = new BlockStatsElement(mem, tl);
			byT.set(key, inf);
			allT.push(inf);
		}
		inf.add(size);
	}

	public function get( t : TType ) : BlockStatsElement {
		return getPath([t == null ? 0 : t.tid]);
	}

	public function getPath( tl : Array<Int> ) : BlockStatsElement {
		var key = tl.join(" ");
		return byT.get(key);
	}

	public function sort( byCount = true , asc = true) {
		if( byCount )
			allT.sort(function(i1, i2) return asc ? i1.count - i2.count : i2.count - i1.count);
		else
			allT.sort(function(i1, i2) return asc ? i1.size - i2.size : i2.size - i1.size);
	}

	/**
	 * Create a copy of the current BlockStats from `pos` to `end`.
	 * (If call add on the copy, a separate entry will be created.)
	 */
	public function slice( pos : Int, ?end : Int ) : BlockStats {
		var newS = new BlockStats(mem);
		newS.allT = allT.slice(pos, end);
		// Do not set byT to reduce memory usage and prevent overwrite parent's elements
		newS.printWithSum = printWithSum;
		return newS;
	}

	public function print() {
		var totCount = 0;
		var totSize = 0;
		var max = Analyzer.maxLines;
		if( max > 0 && allT.length > max ) {
			Analyzer.log("<ignore "+(allT.length - max)+" lines>");
			allT = allT.slice(allT.length - max);
		}
		for( i in allT ) {
			totCount += i.count;
			totSize += i.size;
			Analyzer.log(Analyzer.withColor(i.count + " count, " + Analyzer.mb(i.size) + " ", Yellow) + i.getNames().join(Analyzer.withColor(' > ', Cyan)));
		}
		if( printWithSum )
			Analyzer.log("Total: " + totCount + " count, " + Analyzer.mb(totSize));
	}

}
