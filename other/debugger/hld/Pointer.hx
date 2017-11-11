package hld;

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

	public inline function or( v : Int ) {
		return new Pointer(hl.Bytes.fromAddress(this.address() | v));
	}

	public inline function and( v : Int ) {
		return new Pointer(hl.Bytes.fromAddress(this.address() & v));
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
