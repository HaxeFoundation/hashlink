package hld;

@:enum abstract WaitResult(Int) {
	public var Timeout = -1;
	public var Exit = 0;
	public var Breakpoint = 1;
	public var SingleStep = 2;
	public var Error = 3;
	public var Handled = 4;
}

@:enum abstract Register(Int) {
	public var Esp = 0;
	public var Ebp = 1;
	public var Eip = 2;
	public var EFlags = 3;
	public inline function toInt() return this;
}

interface Api {

	function start() : Bool;
	function stop() : Void;
	function breakpoint() : Bool;
	function read( ptr : Pointer, buffer : Buffer, size : Int ) : Bool;
	function write( ptr : Pointer, buffer : Buffer, size : Int ) : Bool;

	function readByte( ptr : Pointer, pos : Int ) : Int;
	function writeByte( ptr : Pointer, pos : Int, value : Int ) : Void;

	function flush( ptr : Pointer, size : Int ) : Bool;
	function wait( timeout : Int ) : { r : WaitResult, tid : Int };
	function resume( tid : Int ) : Bool;
	function readRegister( tid : Int, register : Register ) : Pointer;
	function writeRegister( tid : Int, register : Register, v : Pointer ) : Bool;

}
