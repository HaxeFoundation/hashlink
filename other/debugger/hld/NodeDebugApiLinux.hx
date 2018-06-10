package hld;
import hld.Api;
import hld.NodeFFI;
import hld.NodeFFI.makeDecl in FDecl;

@:enum abstract PTraceReq(Int) {
	var PeekData = 2;
	var PeekUser = 3;
	var PokeData = 5;
	var PokeUser = 6;
	var Attach = 16;
	var Detach = 17;
	var Cont = 7;
}

class NodeDebugApiLinux implements Api {

	var pid : Int;
	var tmp : Buffer;
	var tmpByte : Buffer;
	var is64 : Bool;
	var isNode64 : Bool;
	var context : js.node.Buffer;
	var libc : {
		function ptrace(req:PTraceReq,pid:Int,addr:CValue,data:CValue) : CValue;
		function waitpid(pid:Int,status:Buffer,options:Int) : Int;
	};

	// indexes of registers in "struct user", see <sys/user.h>
	static var REG_ADDR = [
		19, // Esp
		4, // Ebp
		16, // Eip
		18, // EFlags
		// DR0-7
		106,
		107,
		108,
		109,
		110,
		111,
		112,
		113,
	];

	public function new( pid : Int, is64 : Bool ) {
		this.pid = pid;
		this.is64 = is64;
		this.isNode64 = (untyped process.arch) == 'x64';

		if( !isNode64 || !is64 )
			throw "Can't debug when HL or Node is 32 bit";

		tmp = new Buffer(8);
		tmpByte = new Buffer(4);
		libc = NodeFFI.Library("libc",{
			ptrace : FDecl(pointer,[int,int,pointer,pointer]),
			waitpid : FDecl(int,[int,pointer,int]),
		});
	}

	public function start() : Bool {
		return libc.ptrace(Attach,pid,null,null).address() >= 0;
	}

	public function stop() {
		libc.ptrace(Detach,pid,null,null);
	}

	public function breakpoint() {
		throw "BREAK";
		return false;
	}

	function makePointer( ptr : Pointer ) : CValue {
		tmp.setI32(0, ptr.i64.low);
		tmp.setI32(4, ptr.i64.high);
		return Ref.readPointer(tmp, 0);
	}

	function intPtr( i : Int ) : CValue {
		return makePointer(Pointer.ofPtr(i));
	}

	public function read( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		var pos = 0;
		while( size > 0 ) {
			var v = libc.ptrace(PeekData,pid,makePointer(ptr.offset(pos)),null);
			if( size >= 8 ) {
				var a = v.ref();
				buffer.setI32(pos, a.readInt32LE(0));
				buffer.setI32(pos + 4, a.readInt32LE(4));
				pos += 8;
			} else {
				var a = v.ref();
				var apos = 0;
				if( size >= 4 ) {
					buffer.setI32(pos, a.readInt32LE(apos));
					pos += 4;
					apos += 4;
					size -= 4;
				}
				if( size >= 2 ) {
					buffer.setUI16(pos, a.readUInt16LE(apos));
					pos += 2;
					apos += 2;
					size -= 2;
				}
				if( size >= 1 )
					buffer.setUI8(pos, a.readUInt8(apos));
				break;
			}
			size -= 8;
		}
		return true;
	}

	public function readByte( ptr : Pointer, pos : Int ) : Int {
		if( !read(ptr.offset(pos), tmpByte, 1) )
			throw "Failed to read process memory";
		return tmpByte.getUI8(0);
	}

	public function write( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		var pos = 0;
		while( size > 0 ) {
			var sz = size >= 8 ? 8 : size;
			var v = null;
			var addr = makePointer(ptr.offset(pos));
			if( sz < 8 ) {
				v = libc.ptrace(PeekData,pid,addr,null);
				v = v.ref();
				for( i in 0...sz )
					v.writeUInt8(buffer.getUI8(pos++),i);
				v = v.deref();
			} else {
				v = makePointer(Pointer.make(buffer.getI32(pos),buffer.getI32(pos+4)));
			}
			if( libc.ptrace(PokeData,pid,addr,v).address() < 0 )
				return false;
			pos += sz;
			size -= sz;
		}
		return true;
	}

	public function writeByte( ptr : Pointer, pos : Int, value : Int ) : Void {
		tmpByte.setI32(0, value);
		if( !write(ptr.offset(pos), tmpByte, 1) )
			throw "Failed to write process memory";
	}

	public function flush( ptr : Pointer, size : Int ) : Bool {
		return true;
	}

	static function WIFEXITED(status:Int) {
		return (status & 0x7F) == 0;
	}

	static function WIFSTOPPED(status:Int) {
		return (status & 0x7F) == 0x7F;
	}

	static function WSTOPSIG(status:Int) {
		return status>>8;
	}

	public function wait( timeout : Int ) : { r : WaitResult, tid : Int } {
		var tid = libc.waitpid(pid,tmp,0);
		var status = tmp.getI32(0);
		var kind : WaitResult = Error;
		if( WIFEXITED(status) )
			kind = Exit;
		else if( WIFSTOPPED(status) ) {
			var sig = WSTOPSIG(status);
			if( sig == 19/*SIGSTOP*/ || sig == 5/*SIGTRAP*/ )
				kind = Breakpoint;
		}
		return { r : kind, tid : tid };
	}

	public function resume( tid : Int ) : Bool {
		return libc.ptrace(Cont,pid,null,null).address() >= 0;
	}

	public function readRegister( tid : Int, register : Register ) : Pointer {
		var v = libc.ptrace(PeekUser,tid,intPtr(REG_ADDR[register.toInt()] * 8),null);
		var a = v.ref();
		return Pointer.make(a.readInt32LE(0), a.readInt32LE(4));
	}

	public function writeRegister( tid : Int, register : Register, v : Pointer ) : Bool {
		return libc.ptrace(PokeUser,tid,intPtr(REG_ADDR[register.toInt()]*8),makePointer(v)).address() >= 0;
	}

}