import format.hl.Data;
using format.hl.Tools;

abstract Pointer(Int) {
	public inline function isNull() {
		return this == 0;
	}
	public inline function offset( i : Int ) : Pointer {
		return cast (this + i);
	}
	public inline function sub( p : Pointer ) : Int {
		return this - cast(p);
	}
	public inline function pageAddress() : Pointer {
		return cast (this & ~0xFFFF);
	}
	public inline function toString() {
		return "0x"+StringTools.hex(this, 8);
	}
}

@:enum abstract PageKind(Int) {
	var PDynamic = 0;
	var PRaw = 1;
	var PNoPtr = 2;
	var PFinalizer = 3;
}

class Page {
	var memory : Memory;
	public var addr : Pointer;
	public var kind : PageKind;
	public var size : Int;
	public var blockSize : Int;
	public var firstBlock : Int;
	public var maxBlocks : Int;
	public var nextBlock : Int;
	public var dataPosition : Int;
	public var bmp : haxe.io.Bytes;
	public var sizes : haxe.io.Bytes;

	public function new(m) {
		memory = m;
	}

	public function isLiveBlock( bid : Int ) {
		if( sizes != null && sizes.get(bid) == 0 ) return false;
		return (bmp.get(bid >> 3) >>> (bid & 7)) != 0;
	}

	public function getBlockSize( bid : Int ) {
		return sizes == null ? 1 : sizes.get(bid);
	}

	public function goto( bid : Int ) {
		memory.memoryDump.seek(dataPosition + bid * blockSize, SeekBegin);
	}

	public function getPointer( bid : Int ) {
		return addr.offset(bid * blockSize);
	}

}

class Stack {
	public var base : Pointer;
	public var contents : Array<Pointer>;
	public function new() {
	}
}

class DataSeg {
	public static inline var NO_OWNER = 0x80000000;
	public var page : Page;
	public var bid : Int;
	public var ownerType : Int = NO_OWNER;
	public function new() {
	}
	public inline function getPointer() {
		return page.getPointer(bid);
	}
	public inline function getMemSize() {
		return page.getBlockSize(bid) * page.blockSize;
	}
}

class Memory {

	public var memoryDump : sys.io.FileInput;
	var code : format.hl.Data;
	var is64 : Bool;
	var pages : Array<Page>;
	var roots : Array<Pointer>;
	var stacks : Array<Stack>;
	var types : Array<Pointer>;
	var closures : Array<Pointer>;
	var pointerType : Map<Pointer, HLType>;
	var pointerTypeIndex : Map<Pointer, Int>;
	var closuresType : Map<Pointer, HLType>;
	var pointerPage : Map<Pointer, Page>;
	var dataPointers : Array<DataSeg>;
	var ptrBits : Int;

	function new() {
	}

	function loadBytecode( arg : String ) {
		if( code != null ) throw "Duplicate code";
		code = new format.hl.Reader(false).read(new haxe.io.BytesInput(sys.io.File.getBytes(arg)));
		log(arg + " code loaded");
	}

	inline function readInt() {
		return memoryDump.readInt32();
	}

	inline function readPointer() : Pointer {
		return cast memoryDump.readInt32();
	}

	inline function readType() {
		return pointerType.get(readPointer());
	}

	static function MB( v : Float ) {
		if( v < 1024 * 1024 )
			return (Math.round(v * 10 / 1024) / 10)+"KB";
		return (Math.round(v * 10 / (1024 * 1024)) / 10)+"MB";
	}

	function loadMemory( arg : String ) {
		memoryDump = sys.io.File.read(arg);
		if( memoryDump.read(3).toString() != "HMD" )
			throw "Invalid memory dump file";

		var version = memoryDump.readByte() - "0".code;
		var flags = readInt();
		is64 = (flags & 1) != 0;
		ptrBits = is64 ? 3 : 2;
		if( is64 ) throw "64 bit not supported";

		// load pages
		var count = readInt();
		var totalSize = 0;
		pages = [];
		for( i in 0...count ) {
			while( true ) {
				var addr = readPointer();
				if( addr.isNull() ) break;
				var p = new Page(this);
				p.addr = addr;
				p.kind = cast readInt();
				p.size = readInt();
				p.blockSize = readInt();
				p.firstBlock = readInt();
				p.maxBlocks = readInt();
				p.nextBlock = readInt();

				p.dataPosition = memoryDump.tell();
				memoryDump.seek(p.size, SeekCur);

				var flags = readInt();
				if( flags & 1 != 0 )
					p.bmp = memoryDump.read((p.maxBlocks + 7) >> 3); // 1 bit per block
				if( flags & 2 != 0 )
					p.sizes = memoryDump.read(p.maxBlocks); // 1 byte per block
				pages.push(p);
				totalSize += p.size;
			}
		}
		log(pages.length + " pages, " + MB(totalSize) + " memory");

		// load roots
		roots = [for( i in 0...readInt() ) readPointer()];
		log(roots.length + " roots");

		// load stacks
		stacks = [];
		for( i in 0...readInt() ) {
			var s = new Stack();
			s.base = readPointer();
			s.contents = [for( i in 0...readInt() ) readPointer()];
			stacks.push(s);
		}
		log(stacks.length + " stacks");

		// load types
		types = [for( i in 0...readInt() ) readPointer()];
		closures = [for( i in 0...readInt() ) readPointer()];
		log(types.length + " types, " + closures.length + " closures");
	}

	function check() {
		if( code == null ) throw "Missing .hl file";
		if( memoryDump == null ) throw "Missing .dump file";
		if( code.types.length != this.types.length ) throw "Types count mismatch";

		pointerType = new Map();
		pointerTypeIndex = new Map();
		closuresType = new Map();
		var cid = 0;
		for( i in 0...types.length ) {
			pointerType.set(types[i], code.types[i]);
			pointerTypeIndex.set(types[i], i);
			switch( code.types[i] ) {
			case HFun(f):
				var ptr = closures[cid++];
				var args = f.args.copy();
				closuresType.set(ptr, args.shift());
				pointerType.set(ptr, HFun({ args : args, ret : f.ret }));
				pointerTypeIndex.set(ptr, -i);
			default:
			}
		}

		var count = 0, used = 0, free = 0;
		for( p in pages ) {
			var bid = p.firstBlock;
			while( bid < p.maxBlocks ) {
				if( !p.isLiveBlock(bid) ) {
					bid++;
					free += p.blockSize;
					continue;
				}
				var sz = p.getBlockSize(bid);
				bid += sz;
				count++;
				used += sz * p.blockSize;
			}
		}
		log(count + " live blocks " + MB(used) + " used, " + MB(free) + " free");

		pointerPage = new Map();
		for( p in pages ) {
			pointerPage.set(p.addr, p);
			for( i in 0...p.size >> 16 )
				pointerPage.set(p.addr.offset(i  << 16), p);
		}
	}

	inline function iterLiveBlocks( callb, withPtr = false ) {
		for( p in pages ) {
			if( withPtr && p.kind == PNoPtr ) continue;
			var bid = p.firstBlock;
			while( bid < p.maxBlocks ) {
				if( !p.isLiveBlock(bid) ) {
					bid++;
					continue;
				}
				var sz = p.getBlockSize(bid);
				callb(p, bid, sz);
				bid += sz;
			}
		}
	}

	function getStats(dataTID=0,doPrint=true) {

		var byT = new Map();
		var allT = [];

		dataPointers = [];

		iterLiveBlocks(function(p,bid,size) {
			p.goto(bid);

			var tid : Int = pointerTypeIndex.get(readPointer());
			if( tid == dataTID ) {
				var s = new DataSeg();
				s.bid = bid;
				s.page = p;
				dataPointers.push(s);
			}
			var inf = byT.get(tid);
			if( inf == null ) {
				inf = { t : code.types[tid < 0 ? -tid : tid], count : 0, mem : 0 };
				byT.set(tid, inf);
				allT.push(inf);
			}
			inf.count++;
			inf.mem += size * p.blockSize;
		});

		allT.sort(function(i1, i2) return i1.mem - i2.mem);


		if( doPrint )
			for( t in allT )
				log('${t.count} count ${MB(t.mem)} ${t.t.toString()}');
	}

	function locate( type : String ) {
		if( type == null ) {
			log("Require <type> parameter");
			return;
		}
		for( i in 0...code.types.length )
			if( code.types[i].toString() == type ) {
				getStats(i, false);
				log(dataPointers.length + " values found");
				lookupData();
				return;
			}
		log("Unknown type");
	}

	function printRoots() {
		var rid = 0;
		for( g in code.globals ) {
			if( !g.isPtr() ) continue;
			var r = roots[rid++];
			var ppage = r.pageAddress();
			var p = pointerPage.get(ppage);
			var bid = Std.int(r.sub(p.addr) / p.blockSize);
			p.goto(bid);
			var t = readPointer();
			Sys.println(r.toString()+" "+pointerType.get(t).toString()+" "+g.toString());
		}
	}

	function gotoAddress( val : Pointer ) {
		var ppage = val.pageAddress();
		var p = pointerPage.get(ppage);
		if( p == null )
			return false;
		var bid = Std.int(val.sub(p.addr) / p.blockSize);
		if( !p.isLiveBlock(bid) )
			return false;
		p.goto(bid);
		return true;
	}

	function lookupData() {
		if( dataPointers == null ) {
			Sys.println("No data pointers found, use <stats> first");
			return;
		}
		var data = new Map();
		for( d in dataPointers )
			data.set(d.getPointer(), d);

		var byT = [];

		iterLiveBlocks(function(p, bid, sz) {
			p.goto(bid);
			var type = 0;
			for( i in 0...Std.int((sz * p.blockSize) >> ptrBits) ) {
				var p = readPointer();
				if( i == 0 ) {
					type = pointerTypeIndex.get(p);
					if( type < 0 ) type = -type;
				}
				var d = data.get(p);
				if( d != null && d.ownerType == DataSeg.NO_OWNER ) {
					d.ownerType = type;
					var inf = byT[type];
					if( inf == null ) {
						inf = { t : code.types[type], mem : 0, count : 0 };
						byT[type] = inf;
					}
					inf.count++;
					inf.mem += d.getMemSize();
				}
			}
		}, true);

		var allT = [for( t in byT ) if( t != null ) t];
		allT.sort(function(t1, t2) return t1.mem - t2.mem);
		for( t in allT )
			Sys.println(t.count + " count " + MB(t.mem) + " " + t.t.toString());
	}

	function printPartition() {
		var part = sys.io.File.write("part.txt",false);
		for( p in pages ) {
			var bid = p.firstBlock;
			part.writeString("[" + p.blockSize+"]");
			while( bid < p.maxBlocks ) {
				if( !p.isLiveBlock(bid) ) {
					part.writeByte(".".code);
					bid++;
					continue;
				}
				var sz = p.getBlockSize(bid);
				for( i in 0...sz ) part.writeByte("#".code);
				bid += sz;
			}
			part.writeByte("\n".code);
		}
		part.close();
		log("Partition saved in part.txt");
	}

	function log(msg:String) {
		Sys.println(msg);
	}

	static function main() {
		var m = new Memory();

		//hl.Gc.dumpMemory(); Sys.command("cp memory.hl memory_test.hl");

		var args = Sys.args();
		while( args.length > 0 ) {
			var arg = args.shift();
			if( StringTools.endsWith(arg, ".hl") ) {
				m.loadBytecode(arg);
				continue;
			}
			m.loadMemory(arg);
		}
		m.check();

		var stdin = Sys.stdin();
		while( true ) {
			Sys.print("> ");
			var args = ~/ +/g.split(StringTools.trim(stdin.readLine()));
			var cmd = args.shift();
			switch( cmd ) {
			case "exit", "quit", "q":
				break;
			case "stats":
				m.getStats();
			case "roots":
				m.printRoots();
			case "data":
				m.lookupData();
			case "locate":
				m.locate(args.shift());
			case "part":
				m.printPartition();
			default:
				Sys.println("Unknown command " + cmd);
			}
		}
	}

}