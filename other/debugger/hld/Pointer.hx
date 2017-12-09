package hld;

#if hl
abstract Pointer(hl.Bytes) to hl.Bytes {

	public inline function new(b) {
		this = b;
	}

	public inline function offset(pos:Int) : Pointer {
		return new Pointer(this.offset(pos));
	}

	public inline function sub( p : Pointer ) {
		return this.subtract(p);
	}

	public function toInt() {
		return this.address().low;
	}

	inline function addr() {
		return this.address();
	}

	public inline function isNull() {
		return this == null;
	}

	@:op(a > b) static function opGt( a : Pointer, b : Pointer ) : Bool {
		return a.addr() > b.addr();
	}
	@:op(a < b) static function opLt( a : Pointer, b : Pointer ) : Bool {
		return a.addr() < b.addr();
	}

	public function toString() {
		var i = this.address();
		if( i.high == 0 )
			return "0x" + StringTools.hex(i.low);
		return "0x" + StringTools.hex(i.high) + StringTools.hex(i.low, 8);
	}

	public static function ofPtr( p : hl.Bytes ) : Pointer {
		return cast p;
	}

	public static function make( low : Int, high : Int ) {
		return new Pointer(hl.Bytes.fromAddress(haxe.Int64.make(high, low)));
	}

}
#else
abstract Pointer(haxe.Int64) to haxe.Int64 {

	public var i64(get, never) : haxe.Int64;

	public inline function new(b) {
		this = b;
	}

	inline function get_i64() return this;

	public inline function offset(pos:Int) : Pointer {
		return new Pointer(this + pos);
	}

	public inline function sub( p : Pointer ) : Int {
		return haxe.Int64.toInt(this - p.i64);
	}

	public function toInt() {
		return this.low;
	}

	public inline function isNull() {
		return this == null || (this.low == 0 && this.high == 0);
	}

	@:op(a > b) static function opGt( a : Pointer, b : Pointer ) : Bool {
		return a.i64 > b.i64;
	}
	@:op(a < b) static function opLt( a : Pointer, b : Pointer ) : Bool {
		return a.i64 < b.i64;
	}

	public function toString() {
		if( this.high == 0 )
			return "0x" + StringTools.hex(this.low);
		return "0x" + StringTools.hex(this.high) + StringTools.hex(this.low, 8);
	}

	public static function ofPtr( p : haxe.Int64 ) : Pointer {
		return cast p;
	}

	public static function make( low : Int, high : Int ) {
		return new Pointer(haxe.Int64.make(high, low));
	}

}
#end