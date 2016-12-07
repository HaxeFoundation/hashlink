@:enum abstract WaitResult(Int) {
	public var Timeout = -1;
	public var Exit = 0;
	public var Breakpoint = 1;
	public var SingleStep = 2;
	public var Error = 3;
}

@:enum abstract Register(Int) {
	public var Esp = 0;
	public var Ebp = 1;
	public var Eip = 2;
	public var EFlags = 3;
	public var Dr0 = 4;
	public var Dr1 = 5;
	public var Dr2 = 6;
	public var Dr3 = 7;
	public var Dr6 = 8;
	public var Dr7 = 9;
}

extern class DebugApi {
	@:hlNative("std", "debug_start") public static function startDebugProcess( pid : Int ) : Bool;
	@:hlNative("std", "debug_stop") public static function stopDebugProcess( pid : Int ) : Void;
	@:hlNative("std", "debug_breakpoint") public static function breakpoint( pid : Int ) : Bool;
	@:hlNative("std", "debug_read") public static function read( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_write") public static function write( pid : Int, ptr : hl.Bytes, buffer : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_flush") public static function flush( pid : Int, ptr : hl.Bytes, size : Int ) : Bool;
	@:hlNative("std", "debug_wait") public static function wait( pid : Int, threadId : hl.Ref<Int>, timeout : Int ) : WaitResult;
	@:hlNative("std", "debug_resume") public static function resume( pid : Int, tid : Int ) : Bool;
	@:hlNative("std", "debug_read_register") public static function readRegister( pid : Int, tid : Int, register : Register ) : hl.Bytes;
	@:hlNative("std", "debug_write_register") public static function writeRegister( pid : Int, tid : Int, register : Register, v : Pointer ) : Bool;
}

abstract Pointer(hl.Bytes) to hl.Bytes {

	static var TMP = new hl.Bytes(8);
	public static var PID : Int;

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

	public function readByte( offset : Int ) {
		if( !DebugApi.read(PID, this.offset(offset), TMP, 1) )
			throw "Failed to read @" + offset;
		return TMP.getUI8(0);
	}

	public function writeByte( offset : Int, value : Int ) {
		TMP.setUI8(0, value);
		if( !DebugApi.write(PID, this.offset(offset), TMP, 1) )
			throw "Failed to write @" + offset;
	}

	public function flush( pos : Int ) {
		DebugApi.flush(PID, this.offset(pos), 1);
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
