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
	public var addr : Pointer;
	public var kind : PageKind;
	public var size : Int;
	public var reserved : Int;
	public var dataPosition : Int = -1;

	public function new() {
	}

	public inline function memHasPtr() {
		return kind == PDynamic || kind == PRaw;
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

class BlockSub {
	public var b : Block;
	public var fid : Int;
	public function new(b,fid) {
		this.b = b;
		this.fid = fid;
	}
}

class Block {
	public static var MARK_UID = 0;

	public var page : Page;
	public var addr : Pointer;
	public var size : Int;
	public var typePtr : Pointer;
	public var owner : Block;
	public var type(default, set) : TType;
	public var typeKind : BlockTypeKind;

	public var depth : Int = -1;
	public var mark : Int = -1;

	public var subs : Array<BlockSub>; // can be null
	public var parents : Array<Block>; // if multiple owners

	public function new() {
	}

	function set_type( t : TType ) {
		if( t != null && t.t == HF64 && page.kind != PNoPtr )
			throw "!";
		return type = t;
	}

	public function addParent(b:Block,fid:Int=0) {
		if( owner == null ) {
			owner = b;
		} else {
			if( parents == null ) parents = [owner];
			parents.push(b);
		}
		if( b.subs == null ) b.subs = [];
		b.subs.push(new BlockSub(this,fid));
	}

	public function makeTID( prev : Block, withField : Bool ) {
		if( type == null )
			return 0;
		if( withField )
			for( s in subs )
				if( s.b == prev )
					return type.tid | (s.fid << 24);
		return type.tid;
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
				var b = b.b;
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
				s.b.removeParent(this);
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
