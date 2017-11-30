package hld;
import hld.Api;

@:hlNative("std")
class HLDebugApi implements Api {

	// HL low level api
	static function debugStart( pid : Int ) : Bool { return false; }
	static function debugStop( pid : Int ) : Void {}
	static function debugBreakpoint( pid : Int ) : Bool { return false; }
	static function debugRead( pid : Int, ptr : hl.Bytes, buffer : Buffer, size : Int ) : Bool { return false; }
	static function debugWrite( pid : Int, ptr : hl.Bytes, buffer : Buffer, size : Int ) : Bool { return false; }
	static function debugFlush( pid : Int, ptr : hl.Bytes, size : Int ) : Bool { return false; }
	static function debugWait( pid : Int, threadId : hl.Ref<Int>, timeout : Int ) : Int { return 0; }
	static function debugResume( pid : Int, tid : Int ) : Bool { return false; }
	static function debugReadRegister( pid : Int, tid : Int, register : Register, is64 : Bool ) : Pointer { return null; }
	static function debugWriteRegister( pid : Int, tid : Int, register : Register, v : Pointer, is64 : Bool ) : Bool { return false; }

	var pid : Int;
	var tmp : Buffer;
	var is64 : Bool;

	public function new( pid : Int, is64 : Bool ) {
		if( is64 && !hl.Api.is64() )
			throw "You can't debug a 64 bit process from a 32 bit HL";
		this.pid = pid;
		this.is64 = is64;
		tmp = new Buffer(1);
	}

	public function start() {
		return debugStart(pid);
	}

	public function stop() {
		debugStop(pid);
	}

	public function breakpoint() {
		return debugBreakpoint(pid);
	}

	public function read( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		return debugRead(pid, ptr, buffer, size);
	}

	public function readByte( ptr : Pointer, pos : Int ) : Int {
		if( !read(ptr.offset(pos), cast tmp, 1) )
			throw "Failed to read @" + ptr.toString();
		return tmp.getUI8(0);
	}

	public function write( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		return debugWrite(pid, ptr, buffer, size);
	}

	public function writeByte( ptr : Pointer, pos : Int, value : Int ) : Void {
		tmp.setUI8(0, value & 0xFF);
		if( !write(ptr.offset(pos), cast tmp, 1) )
			throw "Failed to write @" + ptr.toString();
	}

	public function flush( ptr : Pointer, size : Int ) : Bool {
		return debugFlush(pid, ptr, size);
	}

	public function wait( timeout : Int ) : { r : WaitResult, tid : Int } {
		var tid = 0;
		var r = debugWait(pid, tid, timeout);
		return { r : cast r, tid : tid };
	}

	public function resume( tid : Int ) : Bool {
		return debugResume(pid, tid);
	}

	public function readRegister( tid : Int, register : Register ) : Pointer {
		return debugReadRegister(pid, tid, register, is64);
	}

	public function writeRegister( tid : Int, register : Register, v : Pointer ) : Bool {
		return debugWriteRegister(pid, tid, register, v, is64);
	}

}