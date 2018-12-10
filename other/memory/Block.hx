abstract Pointer(haxe.Int64) {

	public var value(get,never) : haxe.Int64;

	inline function get_value() return this;

	inline function new(v) this = v;

	public inline function isNull() {
		return this == 0;
	}
	public inline function offset( i : Int ) : Pointer {
		return cast (this + i);
	}
	public inline function sub( p : Pointer ) : Int {
		return haxe.Int64.toInt(this - p.value);
	}
	public inline function pageAddress() : Pointer {
		return new Pointer(haxe.Int64.and(this,haxe.Int64.make(-1,~0xFFFF)));
	}
	public inline function toString() {
		return "0x"+(this.high == 0 ? StringTools.hex(this.high, 8) : "")+StringTools.hex(this.low,8);
	}
	public inline function shift( k : Int ) : haxe.Int64 {
		return haxe.Int64.shr(this,k);
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

	public inline function memHasPtr() {
		return kind == PDynamic || kind == PRaw;
	}

	public function isLiveBlock( bid : Int ) {
		if( sizes != null && sizes.get(bid) == 0 ) return false;
		return (bmp.get(bid >> 3) & (1 << (bid & 7))) != 0;
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

enum BlockTypeKind {
	KHeader;
	KRoot;
	KAbstractData;
	KDynObjData;
	KInferred( t : TType, k : BlockTypeKind );
}

class Block {
	public static var MARK_UID = 0;

	public var page : Page;
	public var bid : Int;
	public var owner : Block;
	public var type(default, set) : TType;
	public var typeKind : BlockTypeKind;

	public var depth : Int = -1;
	public var mark : Int = -1;

	public var subs : Array<Block>; // can be null
	public var parents : Array<Block>; // if multiple owners

	public function new() {
	}

	public inline function getPointer() {
		return page.getPointer(bid);
	}
	public inline function getMemSize() {
		return page.getBlockSize(bid) * page.blockSize;
	}

	function set_type( t : TType ) {
		if( t != null && t.t == HF64 && page.kind != PNoPtr )
			throw "!";
		return type = t;
	}

	public function addParent(b:Block) {
		if( owner == null ) {
			owner = b;
		} else {
			if( parents == null ) parents = [owner];
			parents.push(b);
		}
		if( b.subs == null ) b.subs = [];
		b.subs.push(this);
	}

	public function getParents() {
		return parents != null ? parents : owner == null ? [] : [owner];
	}

	public function markDepth() {
		var d = depth + 1;
		var all = subs;
		while( all.length > 0 ) {
			var out = [];
			for( b in all ) {
				if( b.depth < 0 || b.depth > d ) {
					b.depth = d;
					if( b.subs != null ) for( s in b.subs ) out.push(s);
				}
			}
			all = out;
			d++;
		}
	}

	public function finalize() {
		if( parents == null ) return;

		inline function getPriority(b:Block) {
			var d = -b.depth * 5;
			if( b.type == null ) return d-2;
			if( !b.type.hasPtr ) return d-1; // false positive
			return switch( b.type.t ) {
			case HFun(_): d;
			case HVirtual(_): d+1;
			default: d+2;
			}
		}

		parents.sort(function(p1, p2) return getPriority(p2) - getPriority(p1));
		owner = parents[0];
	}

	function removeParent( p : Block ) {
		if( parents != null ) {
			parents.remove(p);
			if( parents.length == 0 ) parents = null;
		}
		if( owner == p )
			owner = parents == null ? null : parents[0];
	}

	public function removeChildren() {
		if( subs != null ) {
			for( s in subs )
				s.removeParent(this);
			subs = null;
		}
	}

}

@:generic
class PointerMap<T> {

	var lookup : Map<Int,Map<Int,T>>;

	public function new() {
		lookup = new Map();
	}

	public function set( p : Pointer, v : T ) {
		var c = lookup.get(p.value.high);
		if( c == null ) {
			c = new Map();
			lookup.set(p.value.high, c);
		}
		c.set(p.value.low, v);
	}

	public function get( p : Pointer ) : T {
		if( p == null ) return null;
		var c = lookup.get(p.value.high);
		if( c == null ) return null;
		return c.get(p.value.low);
	}

}
