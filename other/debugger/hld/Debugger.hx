package hld;

enum StepMode {
	Out;
	Next;
	Into;
}

typedef Address = { ptr : Pointer, t : format.hl.Data.HLType };

typedef WatchPoint = {
	var addr : Address;
	var regs : Array<{ offset : Int, bits : Int, r : Api.Register }>;
	var forReadWrite : Bool;
}

typedef StackInfo = { file : String, line : Int, ebp : Pointer, ?context : { obj : format.hl.Data.ObjPrototype, field : String } };

class Debugger {

	static inline var INT3 = 0xCC;
	static var HW_REGS : Array<Api.Register> = [Dr0, Dr1, Dr2, Dr3];

	var sock : sys.net.Socket;

	var api : Api;
	var module : Module;
	var jit : JitInfo;

	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;
	var nextStep : Int = -1;
	var currentStack : Array<{ fidx : Int, fpos : Int, codePos : Int, ebp : hld.Pointer }>;
	var stepBreakData : { ptr : Pointer, old : Int };
	var watches : Array<WatchPoint>;

	public var is64(get, never) : Bool;

	public var eval : Eval;
	public var currentStackFrame : Int;
	public var stackFrameCount(get, never) : Int;
	public var stoppedThread : Null<Int>;

	public var customTimeout : Null<Float>;

	public var watchBreak : Address; // set if breakpoint occur on watch expression

	public function new() {
		breakPoints = [];
		watches = [];
	}

	function get_is64() {
		return jit.is64;
	}

	public function loadModule( content : haxe.io.Bytes ) {
		module = new Module();
		module.load(content);
	}

	public function connect( host : String, port : Int ) {
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
		return true;
	}

	public function init( api : Api ) {
		this.api = api;
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

	public function pause() {
		if( !api.breakpoint() )
			throw "Failed to break process";
		return wait();
	}

	function singleStep(tid) {
		var r = getReg(tid, EFlags).toInt() | 256;
		setReg(tid, EFlags, hld.Pointer.make(r,0));
	}

	public function getException() {
		var exc = @:privateAccess eval.readPointer(jit.debugExc);
		if( exc.isNull() )
			return null;
		return eval.readVal(exc, HDyn);
	}

	public function getCurrentVars( args : Bool ) {
		var s = currentStack[currentStackFrame];
		var g = module.getGraph(s.fidx);
		return args ? g.getArgs() : g.getLocals(s.fpos);
	}

	function wait( onStep = false ) : Api.WaitResult {
		var cmd = null;
		watchBreak = null;
		while( true ) {
			cmd = api.wait(customTimeout == null ? 1000 : Math.ceil(customTimeout * 1000));

			var tid = cmd.tid;
			switch( cmd.r ) {
			case Timeout, Handled:

				if( customTimeout != null )
					return cmd.r;

			case Breakpoint:
				var eip = getReg(tid, Eip);

				if( stepBreakData != null ) {
					var ptr = stepBreakData.ptr;
					api.writeByte(ptr, 0, stepBreakData.old);
					api.flush(ptr, 1);
					stepBreakData = null;
					// we triggered this break
					if( eip.sub(ptr) == 1 ) {
						setReg(tid, Eip, eip.offset(-1));
						stoppedThread = tid;
						if( !onStep ) throw "?";
						return SingleStep;
					}
				}

				var codePos = eip.sub(jit.codeStart) - 1;
				for( b in breakPoints ) {
					if( b.codePos == codePos ) {
						// restore code
						setAsm(codePos, b.oldByte);
						// move backward
						setReg(tid, Eip, eip.offset(-1));
						singleStep(tid);
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
				} else if( watches.length > 0 ) {

					// check if we have a break on a watchpoint
					var dr6 = api.readRegister(tid, Dr6);
					var watchBits = dr6.toInt() & 7;
					if( watchBits != 0 ) {
						for( w in watches )
							for( r in w.regs )
								if( watchBits & (1 << HW_REGS.indexOf(r.r)) != 0 ) {
									watchBreak = w.addr;
									break;
								}
						api.writeRegister(tid, Dr6, Pointer.make(0, 0));
						if( watchBreak != null ) {
							cmd.r = Watchbreak;
							break;
						}
					}

				}
				stoppedThread = tid;
				if( onStep )
					return SingleStep;
				resume();
			case Error if( cmd.tid != jit.mainThread ):
				stoppedThread = cmd.tid;
				resume();
			case Error, Exit, Watchbreak:
				break;
			}
		}
		stoppedThread = cmd.tid;
		currentStackFrame = 0;
		currentStack = (cmd.tid == jit.mainThread) ? makeStack(cmd.tid) : [];
		return cmd.r;
	}

	function smartStep( tid : Int, stepIntoCall : Bool ) {
		var eip = getReg(tid, Eip);
		var op = api.readByte(eip, 0);
		var mod_rm;
		switch( op ) {
		case 0xEA:
			// FAR JMP : this will elimate our single step flag
			// instead, put a tmp breakpoint on return eip
			// this fixes step-into continuing execution bug
			var esp = getReg(tid, Esp);
			var ptr = readMem(esp, jit.align.ptr).getPointer(0, jit.align);
			stepBreak(ptr);
		// skip CALL instruction
		case 0xFF if( !stepIntoCall && ((mod_rm = api.readByte(eip, 1)) >> 3) & 7 == 2 /* RM/2 */ ):
			var reg = mod_rm & 7;
			var mod = mod_rm >> 6;
			var size = 2; // 0xFF + mod/rm
			if( mod == 1 )
				size++; // single byte
			else if( mod == 2 )
				size += 4; // word
			if( reg == 4 )
				size++; // esp reg
			stepBreak(eip.offset(size));
		// skip CALL instruction
		case 0xE8 if( !stepIntoCall ):
			stepBreak(eip.offset(5));
		default:
			singleStep(tid);
		}
	}

	function stepBreak( ptr : Pointer ) {
		var old = api.readByte(ptr, 0);
		api.writeByte(ptr, 0, INT3);
		api.flush(ptr, 1);
		stepBreakData = { ptr : ptr, old : old };
	}

	public function debugTrace( stepIn ) {
		var tid = stoppedThread;
		var prevFun = -1, prevPos = -1, inC = false;
		var prevEbp = null, prevStack = 0;
		var tabs = "";
		function debugSt() {
			var s = currentStack[0];
			var eip = getReg(tid, Eip);
			var codePos = eip.sub(jit.codeStart);

			if( codePos < s.codePos || codePos > s.codePos + 1024 ) {
				if( s.fidx == prevFun && s.fpos == prevPos && s.ebp.sub(getReg(tid, Ebp)) > 0 ) {
					// in C function
					if( !inC ) {
						inC = true;
						Sys.println(tabs+"IN C CALL");
					}
				}
			}

			//trace(s.fidx + "@" + s.fpos + ":" + s.ebp.toString());

			if( s.fidx == prevFun && s.fpos == prevPos ) {
				if( s.ebp.sub(prevEbp) != 0 ) {
					throw "!!!EPB!!! " + s.ebp.toString() + "!=" + prevEbp.toString();
					prevEbp = s.ebp;
				}
				return;
			}

			inC = false;

			var calls = makeStack(tid);
			tabs = [for( c in calls ) ""].join("  ");

			var inf = module.resolveSymbol(s.fidx, s.fpos);
			var file = inf.file.split("\\").join("/").split("/").pop();
			var str = tabs;
			str += StringTools.hex(module.code.functions[s.fidx].findex) + "h @" + StringTools.hex(s.fpos);
			str += " " +file+":" + inf.line;
			str += " " + Std.string(module.code.functions[s.fidx].ops[s.fpos]).substr(1);

			if( s.fidx == prevFun && calls.length == prevStack ) {
				if( s.ebp.sub(prevEbp) != 0 )
					throw "!!!EPB!!! "+s.ebp.toString()+"!="+prevEbp.toString();
			}

			Sys.println(str);

			prevEbp = s.ebp;
			prevFun = s.fidx;
			prevPos = s.fpos;
			prevStack = calls.length;
		}
		debugSt();
		while( true ) {
			smartStep(tid,stepIn);
			resume();
			var r = wait(true);
			if( r != SingleStep )
				return r;
			currentStack = makeStack(tid, 1);
			if( currentStack.length == 0 )
				continue;
			debugSt();
		}
	}

	public function step( mode : StepMode ) : Api.WaitResult {
		var tid = stoppedThread;
		var s = currentStack[0];
		var line = module.resolveSymbol(s.fidx, s.fpos).line;
		while( true ) {
			smartStep(tid, mode == Into);
			resume();
			var r = wait(true);
			if( r != SingleStep )
				return r; // breakpoint
			var st = makeStack(tid, 1)[0];
			if( st == null )
				continue;
			var deltaEbp = st.ebp.sub(s.ebp);
			switch( mode ) {
			case Next:
				// same stack frame but different line
				if( st.fidx == s.fidx && deltaEbp == 0 && module.resolveSymbol(st.fidx, st.fpos).line != line )
					break;
			case Into:
				// different method or different line
				if( st.fidx != s.fidx || module.resolveSymbol(st.fidx, st.fpos).line != line )
					break;
			case Out:
				// only on ret
			}
			// returned !
			if( deltaEbp > 0 )
				break;
		}
		currentStackFrame = 0;
		currentStack = (stoppedThread == jit.mainThread) ? makeStack(stoppedThread) : [];
		return Breakpoint;
	}

	function makeStack(tid,max = 0) {
		var stack = [];
		var esp = getReg(tid, Esp);
		var size = jit.stackTop.sub(esp) + jit.align.ptr;
		if( size < 0 ) size = 0;
		var mem = readMem(esp.offset(-jit.align.ptr), size);

		var eip = getReg(tid, Eip);
		var asmPos = eip.sub(jit.codeStart);
		var e = jit.resolveAsmPos(asmPos);
		var inProlog = false;

		if( e == null && size > 0 ) {
			// we are on the first opcode of a C function ?
			// try again with immediate frame
			var ptr = mem.getPointer(jit.align.ptr, jit.align);
			if( ptr > jit.codeStart && ptr < jit.codeEnd ) {
				asmPos = ptr.sub(jit.codeStart);
				e = jit.resolveAsmPos(asmPos);
			}
		} else {
			// if we are on ret, our EBP is wrong, so let's ignore this stack part
			var op = api.readByte(eip, 0);
			if( op == 0xC3 ) // RET
				e = null;
		}

		if( e != null ) {
			var ebp = getReg(tid, Ebp);
			if( e.fpos < 0 ) {
				// we are in function prolog
				var delta = jit.getFunctionPos(e.fidx) - asmPos;
				e.fpos = 0;
				if( delta == 0 )
					e.ebp = esp.offset( -jit.align.ptr); // not yet pushed ebp
				else
					e.ebp = esp;
				inProlog = true;
			} else
				e.ebp = ebp;
			stack.push(e);
			if( max > 0 && stack.length >= max )
				return stack;
		}

		// similar to module/module_capture_stack
		if( is64 ) {
			for( i in 0...size >> 3 ) {
				var val = mem.getPointer(i << 3, jit.align);
				if( val > esp && val < jit.stackTop || (inProlog && i == 0) ) {
					var codePtr = mem.getPointer((i + 1) << 3, jit.align);
					if( codePtr < jit.codeStart || codePtr > jit.codeEnd )
						continue;
					var codePos = codePtr.sub(jit.codeStart);
					var e = jit.resolveAsmPos(codePos);
					if( e != null && e.fpos >= 0 ) {
						e.ebp = val;
						stack.push(e);
						if( max > 0 && stack.length >= max ) return stack;
					}
				}
			}
		} else {
			var stackBottom = esp.toInt();
			var stackTop = jit.stackTop.toInt();
			for( i in 0...size >> 2 ) {
				var val = mem.getI32(i << 2);
				if( val > stackBottom && val < stackTop || (inProlog && i == 0) ) {
					var codePos = mem.getI32((i + 1) << 2) - jit.codeStart.toInt();
					var e = jit.resolveAsmPos(codePos);
					if( e != null && e.fpos >= 0 ) {
						e.ebp = Pointer.make(val,0);
						stack.push(e);
						if( max > 0 && stack.length >= max ) return stack;
					}
				}
			}
		}
		return stack;
	}

	inline function get_stackFrameCount() return currentStack.length;

	public function getBackTrace() : Array<StackInfo> {
		return [for( e in currentStack ) stackInfo(e)];
	}

	public function getStackFrame( ?frame ) : StackInfo {
		if( frame == null ) frame = currentStackFrame;
		var f = currentStack[frame];
		if( f == null )
			return {file:"???", line:0, ebp:Pointer.make(0, 0)};
		return stackInfo(f);
	}

	function stackInfo( f ) {
		var s = module.resolveSymbol(f.fidx, f.fpos);
		return { file : s.file, line : s.line, ebp : f.ebp, context : module.getMethodContext(f.fidx) };
	}

	public function getValue( expr : String ) : Value {
		var cur = currentStack[currentStackFrame];
		if( cur == null ) return null;
		return eval.eval(expr, cur.fidx, cur.fpos, cur.ebp);
	}

	public function getRef( expr : String ) : Address {
		var cur = currentStack[currentStackFrame];
		if( cur == null ) return null;
		return eval.ref(expr, cur.fidx, cur.fpos, cur.ebp);
	}

	public function getWatches() {
		return [for( w in watches ) w.addr];
	}

	public function watch( a : Address, forReadWrite = false ) {
		var size = jit.align.typeSize(a.t);
		var availableRegs = HW_REGS.copy();
		for( w in watches )
			for( r in w.regs )
				availableRegs.remove(r.r);
		var w : WatchPoint = {
			addr : a,
			regs : [],
			forReadWrite : forReadWrite,
		};
		var offset = 0;
		var bitSize = [1, 2, 8, 4];
		while( size > 0 ) {
			var r = availableRegs.shift();
			if( r == null )
				throw "Not enough hardware register to watch: remove previous watches";
			var v = if( size >= 8 ) 2 else if( size >= 4 ) 3 else if( size >= 2 ) 1 else 0;
			w.regs.push({ r : r, offset : offset, bits : v });
			var delta = bitSize[v];
			size -= delta;
			offset += delta;
		}
		watches.push(w);
		syncDebugRegs();
		return w;
	}

	public function unwatch( a : Address ) {
		for( w in watches )
			if( w.addr == a ) {
				watches.remove(w);
				syncDebugRegs();
				return true;
			}
		return false;
	}


	function syncDebugRegs() {
		var wasPaused = false;
		if( stoppedThread == null ) {
			pause();
			wasPaused = true;
		}
		var dr7 = 0x100;
		for( w in watches ) {
			for( r in w.regs ) {
				var rid = HW_REGS.indexOf(r.r);
				dr7 |= 1 << (rid * 2);
				dr7 |= ((w.forReadWrite ? 3 : 1) | (r.bits << 2)) << (16 + rid * 4);
				api.writeRegister(stoppedThread, r.r, w.addr.ptr.offset(r.offset));
			}
		}
		api.writeRegister(stoppedThread, Dr7, Pointer.make(dr7, 0));
		if( wasPaused )
			resume();
	}

	public function resume() {
		if( stoppedThread == null )
			throw "No thread stopped";
		if( !api.resume(stoppedThread) )
			throw "Could not resume "+stoppedThread;
		stoppedThread = null;
		watchBreak = null;
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

	public function clearBreakpoints( file : String ) {
		var ffuns = module.getFileFunctions(file);
		if( ffuns == null )
			return;
		for( b in breakPoints.copy() )
			for( f in ffuns.functions )
				if( b.fid == f.ifun ) {
					removeBP(b);
					break;
				}
	}

	function removeBP( bp ) {
		breakPoints.remove(bp);
		setAsm(bp.codePos, bp.oldByte);
		if( nextStep == bp.codePos )
			nextStep = -1;
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
					removeBP(a);
					break;
				}
		return rem;
	}

}