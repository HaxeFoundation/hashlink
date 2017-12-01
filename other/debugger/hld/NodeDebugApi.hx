package hld;
import hld.Api;
import hld.NodeFFI;
import hld.NodeFFI.makeDecl in FDecl;

class NodeDebugApi implements Api {

	var pid : Int;
	var tmp : Buffer;
	var is64 : Bool;
	var isNode64 : Bool;

	var winApi : Dynamic;
	var debugEvent : CValue;
	var context : js.node.Buffer;

	var hthreads : Map<Int,CValue> = new Map();
	var regPositions : Array<Int>;
	var phandle : CValue;

	public function new( pid : Int, is64 : Bool ) {
		this.pid = pid;
		this.is64 = is64;
		this.isNode64 = (untyped process.arch) == 'x64';

		if( !isNode64 && is64 )
			throw "Can't debug HL 64 bit process with Node 32 bit host";

		tmp = new Buffer(8);
		winApi = NodeFFI.Library("Kernel32.dll", {
			DebugActiveProcess : FDecl(bool, [int]),
			DebugActiveProcessStop : FDecl(bool, [int]),
			DebugBreakProcess : FDecl(bool,[pointer]),
			WaitForDebugEvent : FDecl(bool, [pointer, int]),
			ContinueDebugEvent : FDecl(bool, [int, int, int]),
			GetThreadContext : FDecl(bool, [pointer, pointer]),
			SetThreadContext : FDecl(bool, [pointer, pointer]),
			OpenThread : FDecl(pointer, [int, int, int]),
			OpenProcess : FDecl(pointer, [int, int, int]),
			ReadProcessMemory : FDecl(bool,[pointer,pointer,pointer,size_t,pointer]),
			WriteProcessMemory : FDecl(bool, [pointer, pointer, pointer, size_t, pointer]),
			FlushInstructionCache : FDecl(bool, [pointer, pointer, size_t]),
		});

		phandle = winApi.OpenProcess(0xF0000 | 0x100000 | 0xFFFF, 0, pid);

		// DEBUG_EVENT
		var dev = new CStruct();
		dev.defineProperty('debugEventCode', int);
		dev.defineProperty('processId', int);
		dev.defineProperty('threadId', int);

		// exception record
		if( isNode64 )
			dev.defineProperty('_align', int);
		dev.defineProperty('exceptionCode', int);

		// 176 bytes in x86-64
		for( i in 0...44 - 5 )
			dev.defineProperty('pad' + i, int);

		context = new js.node.Buffer(2048);
		var flagsPos = isNode64 ? 48 : 0;
		var flags = 1 | 2 | 8;
		if( isNode64 )
			flags |= 0x00100000;
		context.writeUInt32LE(flags, flagsPos);

		var baseRegs = 8 + 9;
		inline function R(index) return (baseRegs + index) << 3;
		regPositions = [R(4), R(5), R(16), 68];

		debugEvent = dev.alloc({});
	}

	public function start() : Bool {
		return winApi.DebugActiveProcess(pid);
	}

	public function stop() {
		winApi.DebugActiveProcessStop(pid);
	}

	public function breakpoint() {
		return winApi.DebugBreakProcess(phandle);
	}

	function makePointer( ptr : Pointer ) : CValue {
		tmp.setI32(0, ptr.i64.low);
		tmp.setI32(4, ptr.i64.high);
		return Ref.readPointer(tmp, 0);
	}

	public function read( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		return winApi.ReadProcessMemory(phandle, makePointer(ptr), buffer, size, null);
	}

	public function readByte( ptr : Pointer, pos : Int ) : Int {
		if( !read(ptr.offset(pos), tmp, 1) )
			throw "Failed to read process memory";
		return tmp.getUI8(0);
	}

	public function write( ptr : Pointer, buffer : Buffer, size : Int ) : Bool {
		return winApi.WriteProcessMemory(phandle, makePointer(ptr), buffer, size, null);
	}

	public function writeByte( ptr : Pointer, pos : Int, value : Int ) : Void {
		tmp.setI32(0, value);
		if( !write(ptr.offset(pos), tmp, 1) )
			throw "Failed to write process memory";
	}

	public function flush( ptr : Pointer, size : Int ) : Bool {
		return winApi.FlushInstructionCache(phandle, makePointer(ptr), size);
	}

	public function wait( timeout : Int ) : { r : WaitResult, tid : Int } {
		var e = debugEvent;
		if( !winApi.WaitForDebugEvent(e.ref(), timeout) )
			return { r : Timeout, tid : 0 };
		var tid = e.threadId;
		var result : WaitResult = switch( e.debugEventCode ) {
		case 1://EXCEPTION_DEBUG_EVENT
			switch( e.exceptionCode ) {
			case 0x80000003: //EXCEPTION_BREAKPOINT
				Breakpoint;
			case 0x80000004: //EXCEPTION_SINGLE_STEP
				SingleStep;
			default:
				Error;
			}
		case 5://EXIT_PROCESS_DEBUG_EVENT:
			Exit;
		default:
			resume(tid);
			Timeout;
		}
		return { r : result, tid : tid };
	}

	function openThread( tid : Int ) {
		var th = hthreads.get(tid);
		if( th != null )
			return th;
		th = winApi.OpenThread(0xF0000 | 0x100000 | 0xFFFF, 0, tid);
		hthreads.set(tid, th);
		return th;
	}

	public function resume( tid : Int ) : Bool {
		return winApi.ContinueDebugEvent(pid, tid, 0x00010002/*DBG_CONTINUE*/);
	}

	public function readRegister( tid : Int, register : Register ) : Pointer {
		if( !winApi.GetThreadContext(openThread(tid), context) )
			throw "Failed to read registers for thread " + tid;
		var pos = regPositions[register.toInt()];
		if( register == EFlags )
			return Pointer.make(context.readUInt16LE(pos), 0);
		if( !is64 )
			return Pointer.make(context.readInt32LE(pos), 0);
		return Pointer.make(context.readInt32LE(pos), context.readInt32LE(pos + 4));
	}

	public function writeRegister( tid : Int, register : Register, v : Pointer ) : Bool {
		if( !winApi.GetThreadContext(openThread(tid), context) )
			return false;
		var pos = regPositions[register.toInt()];
		if( register == EFlags )
			context.writeUInt16LE(v.toInt(), pos);
		context.writeInt32LE(v.i64.low, pos);
		if( is64 ) context.writeInt32LE(v.i64.high, pos + 4);
		return winApi.SetThreadContext(openThread(tid), context);
	}

}