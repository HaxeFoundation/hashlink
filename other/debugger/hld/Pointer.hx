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

	var i64(get, never) : haxe.Int64;

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