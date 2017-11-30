package hld;
import hld.Api;

class NodeDebugApi implements Api {

	var pid : Int;
	var tmp : Buffer;
	var is64 : Bool;

	public function new( pid : Int, is64 : Bool ) {
		this.pid = pid;
		this.is64 = is64;
		tmp = new Buffer(1);
	}

	public function start() {
		throw "TODO";
		return false;
	}

	public function stop() {
		throw "TODO";
	}

	public function breakpoint() {
		throw "TODO";
		return false;
	}

	public function read( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		throw "TODO";
	}

	public function readByte( ptr : Pointer, pos : Int ) : Int {
		throw "TODO";
		return tmp.getUI8(0);
	}

	public function write( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		throw "TODO";
	}

	public function writeByte( ptr : Pointer, pos : Int, value : Int ) : Void {
		throw "TODO";
	}

	public function flush( ptr : Pointer, size : Int ) : Bool {
		throw "TODO";
	}

	public function wait( timeout : Int ) : { r : WaitResult, tid : Int } {
		throw "TODO";
	}

	public function resume( tid : Int ) : Bool {
		throw "TODO";
	}

	public function readRegister( tid : Int, register : Register ) : Pointer {
		throw "TODO";
	}

	public function writeRegister( tid : Int, register : Register, v : Pointer ) : Bool {
		throw "TODO";
	}

}