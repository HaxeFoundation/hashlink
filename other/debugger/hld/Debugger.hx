package hld;

class Debugger {

	static inline var INT3 = 0xCC;

	var sock : sys.net.Socket;

	var api : Api;
	var module : Module;
	var jit : JitInfo;

	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;
	var nextStep : Int = -1;
	var currentStack : Array<{ fidx : Int, fpos : Int, codePos : Int, ebp : hld.Pointer }>;

	public var eval : Eval;
	public var currentStackFrame : Int;
	public var stackFrameCount(get, never) : Int;
	public var stoppedThread : Null<Int>;

	public function new() {
		breakPoints = [];
	}

	public function loadModule( content : haxe.io.Bytes ) {
		module = new Module();
		module.load(content);
	}

	public function connect( api : Api, host : String, port : Int ) {
		this.api = api;
		sock = new sys.net.Socket();
		try {
			sock.connect(new sys.net.Host(host), port);
		} catch( e : Dynamic ) {
			sock.close();
			return false;
		}

		jit = new JitInfo();
		if( !jit.read(sock.input, module) ) {
			sock.close();
			return false;
		}
		module.init(jit.align);
		eval = new Eval(module, api, jit);

		if( !api.start() )
			return false;

		wait(); // wait first break

		return true;
	}

	public function run() {
		// closing the socket will unlock waiting thread
		if( sock != null ) {
			sock.close();
			sock = null;
		}
		if( stoppedThread != null )
			resume();
		return wait();
	}

	public function wait() {
		var cmd = null;
		while( true ) {
			cmd = api.wait(1000);
			var tid = cmd.tid;
			switch( cmd.r ) {
			case Timeout:
				// continue
			case Breakpoint:
				var eip = getReg(tid, Eip);
				var codePos = eip.sub(jit.codeStart) - 1;
				for( b in breakPoints ) {
					if( b.codePos == codePos ) {
						// restore code
						setAsm(codePos, b.oldByte);
						// move backward
						setReg(tid, Eip, eip.offset(-1));
						// set EFLAGS to single step
						var r = getReg(tid, EFlags);
						setReg(tid, EFlags, r.or(256));
						nextStep = codePos;
						break;
					}
				}
				break;
			case SingleStep:
				// restore our breakpoint
				if( nextStep >= 0 ) {
					setAsm(nextStep, INT3);
					nextStep = -1;
				}
				stoppedThread = tid;
				resume();
			default:
				break;
			}
		}
		stoppedThread = cmd.tid;
		currentStackFrame = 0;
		currentStack = (cmd.tid == jit.mainThread) ? makeStack(cmd.tid) : [];
		return cmd.r;
	}

	function makeStack(tid) {
		var stack = [];
		var esp = getReg(tid, Esp);
		var size = jit.stackTop.sub(esp);
		var mem = readMem(esp, size);

		var asmPos = getReg(tid, Eip).sub(jit.codeStart);
		var e = jit.resolveAsmPos(asmPos);

		if( e != null ) {
			e.ebp = getReg(tid, Ebp);
			stack.push(e);
		}

		if( jit.is64 ) throw "TODO : use int64 calculus";

		// similar to module/module_capture_stack
		var stackBottom = esp.toInt();
		var stackTop = jit.stackTop.toInt();
		for( i in 1...size >> 2 ) {
			var val = mem.getI32(i << 2);
			if( val > stackBottom && val < stackTop ) {
				var codePos = mem.getI32((i + 1) << 2) - jit.codeStart.toInt();
				var e = jit.resolveAsmPos(codePos);
				if( e != null ) {
					e.ebp = Pointer.make(val,0);
					stack.push(e);
				}
			}
		}
		return stack;
	}

	inline function get_stackFrameCount() return currentStack.length;

	public function getBackTrace() : Array<{ file : String, line : Int }> {
		return [for( e in currentStack ) module.resolveSymbol(e.fidx, e.fpos)];
	}

	public function getStackFrame( ?frame ) {
		if( frame == null ) frame = currentStackFrame;
		var f = currentStack[frame];
		return f == null ? {file:"???",line:0} : module.resolveSymbol(f.fidx, f.fpos);
	}

	public function getValue( path : String ) {
		var cur = currentStack[currentStackFrame];
		if( cur == null ) return null;
		return eval.eval(path, cur.fidx, cur.fpos, cur.ebp);
	}

	public function resume() {
		if( stoppedThread == null )
			throw "No thread stopped";
		if( !api.resume(stoppedThread) )
			throw "Could not resume "+stoppedThread;
		stoppedThread = null;
	}

	function readMem( addr : Pointer, size : Int ) {
		var mem = new Buffer(size);
		if( !api.read(addr, mem, size) )
			throw "Failed to read memory @" + addr.toString() + "[" + size+"]";
		return mem;
	}

	function getAsm( pos : Int ) {
		return api.readByte(jit.codeStart,pos);
	}

	function setAsm( pos : Int, byte : Int ) {
		api.writeByte(jit.codeStart, pos, byte);
		api.flush(jit.codeStart.offset(pos),1);
	}

	function getReg(tid, reg) {
		return Pointer.ofPtr(api.readRegister(tid, reg));
	}

	function setReg(tid, reg, value) {
		if( !api.writeRegister(tid, reg, value) )
			throw "Failed to set register " + reg;
	}

	public function addBreakpoint( file : String, line : Int ) {
		var breaks = module.getBreaks(file, line);
		if( breaks == null )
			return false;
		// check already defined
		var set = false;
		for( b in breaks.copy() ) {
			var found = false;
			for( a in breakPoints ) {
				if( a.fid == b.ifun && a.pos == b.pos ) {
					found = true;
					break;
				}
			}
			if( found ) continue;

			var codePos = jit.getCodePos(b.ifun, b.pos);
			var old = getAsm(codePos);
			setAsm(codePos, INT3);
			breakPoints.push({ fid : b.ifun, pos : b.pos, oldByte : old, codePos : codePos });
			set = true;
		}
		return set;
	}

	public function removeBreakpoint( file : String, line : Int ) {
		var breaks = module.getBreaks(file, line);
		if( breaks == null )
			return false;
		var rem = false;
		for( b in breaks )
			for( a in breakPoints )
				if( a.fid == b.ifun && a.pos == b.pos ) {
					rem = true;
					breakPoints.remove(a);
					setAsm(a.codePos, a.oldByte);
					if( nextStep == a.codePos ) nextStep = -1;
					break;
				}
		return rem;
	}

}