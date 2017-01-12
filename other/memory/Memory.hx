import format.hl.Data;

abstract Pointer(Int) {
	public inline function isNull() {
		return this == 0;
	}
}

class Page {
	public var addr : Pointer;
	public var kind : Int;
	public var size : Int;
	public var blockSize : Int;
	public var firstBlock : Int;
	public var maxBlocks : Int;
	public var nextBlock : Int;
	public var dataPosition : Int;
	public var bmp : haxe.io.Bytes;
	public var sizes : haxe.io.Bytes;

	public function new() {
	}

	public function isLiveBlock( bid : Int ) {
		if( bmp == null ) return true;
		return bmp.get(bid >> 3)
	}
}

class Stack {
	public var base : Pointer;
	public var contents : Array<Pointer>;
	public function new() {
	}
}

class Memory {

	var memoryDump : sys.io.FileInput;
	var code : format.hl.Data;
	var is64 : Bool;
	var pages : Array<Page>;
	var roots : Array<Pointer>;
	var stacks : Array<Stack>;
	var types : Array<Pointer>;
	var pointerType : Map<Pointer, HLType>;

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

	function loadMemory( arg : String ) {
		memoryDump = sys.io.File.read(arg);
		if( memoryDump.read(3).toString() != "HMD" )
			throw "Invalid memory dump file";
		var version = memoryDump.readByte() - "0".code;
		var flags = readInt();
		is64 = (flags & 1) != 0;
		if( is64 ) throw "64 bit not supported";

		// load pages
		var count = readInt();
		var totalSize = 0;
		pages = [];
		for( i in 0...count ) {
			while( true ) {
				var addr = readPointer();
				if( addr.isNull() ) break;
				var p = new Page();
				p.addr = addr;
				p.kind = readInt();
				p.size = readInt();
				p.blockSize = readInt();
				p.firstBlock = readInt();
				p.maxBlocks = readInt();
				p.nextBlock = readInt();

				var flags = readInt();
				p.dataPosition = memoryDump.tell();
				memoryDump.seek(p.size, SeekCur);
				if( flags & 1 != 0 )
					p.bmp = memoryDump.read((p.maxBlocks + 7) >> 3); // 1 bit per block
				if( flags & 2 != 0 )
					p.sizes = memoryDump.read(p.maxBlocks); // 1 byte per block
				pages.push(p);
				totalSize += p.size;
			}
		}
		log(pages.length + " pages, " + (totalSize >> 10) + "KB memory");

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
		log(types.length + " types");
	}

	function check() {
		if( code == null ) throw "Missing .hl file";
		if( memoryDump == null ) throw "Missing .dump file";
		if( code.types.length != this.types.length ) throw "Types count mismatch";

		pointerType = new Map();
		for( i in 0...types.length )
			pointerType.set(types[i], code.types[i]);

		for( p in pages ) {
			var bid = p.firstBlock;
			while( bid < p.maxBlocks ) {
				if( !p.isBlockLive(bid) ) {
					bid++;
					continue;
				}
				count++;
			}
		}
		log(count + " live blocks");
	}

	function log(msg:String) {
		Sys.println(msg);
	}

	static function main() {
		var m = new Memory();

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
	}

}