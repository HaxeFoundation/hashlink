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
	public static var IGNORED_ROOTS = [
		"hl",
		"sys",
		"haxe",
		"Date",
		"EReg",
		"Math",
		"Reflect",
		"Std",
		"String",
		"StringBuf",
		"Sys",
		"Type",
		"Xml",
		"ArrayObj" // special
	];

	var sock : sys.net.Socket;

	var api : Api;
	var module : Module;
	var jit : JitInfo;
	var processExit : Bool;
	var ignoredRoots : Map<String,Bool>;

	var breakPoints : Array<{ fid : Int, pos : Int, codePos : Int, oldByte : Int }>;
	var nextStep : Int = -1;
	var currentStack : Array<{ fidx : Int, fpos : Int, codePos : Int, ebp : hld.Pointer }>;
	var stepBreakData : { ptr : Pointer, old : Int };
	var watches : Array<WatchPoint>;
	var threads : Map<Int,{ id : Int, stackTop : Pointer, exception : Pointer }>;

	public var is64(get, never) : Bool;

	public var eval : Eval;
	public var currentStackFrame : Int;
	public var breakOnThrow(default, set) : Bool;
	public var stackFrameCount(get, never) : Int;
	public var mainThread(default, null) : Int = 0;
	public var stoppedThread(default,set) : Null<Int>;
	public var currentThread(default,null) : Null<Int>;

	public var customTimeout : Null<Float>;

	public var watchBreak : Address; // set if breakpoint occur on watch expression

	public function new() {
		breakPoints = [];
		watches = [];
	}

	function set_stoppedThread(v) {
		return stoppedThread = currentThread = v;
	}

	function get_is64() {
		return jit.is64;
	}

	public function loadModule( content : haxe.io.Bytes ) {
		module = new Module();
		module.load(content);
	}

	public function connect( host : String, port : Int, retries = 10 ) {
		sock = new sys.net.Socket();

		var connected = false;
		for( i in 0...retries ) {
			try {
				sock.connect(new sys.net.Host(host), port);
				connected = true;
				break;
			} catch( e : Dynamic ) {
				Sys.sleep(0.1);
			}
		}
		if( !connected ) {
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

	public function getThreads() {
		var tl = [for( t in threads ) t.id];
		tl.sort(Reflect.compare);
		return tl;
	}

	public function setCurrentThread(tid) {
		currentThread = tid;
		prepareStack();
	}

	public function pause() {
		if( !api.breakpoint() )
			throw "Failed to break process";
		var r = wait();
		// if we have stopped on a not HL thread, let's switch on main thread
		var found = false;
		for( t in threads )
			if( t.id == stoppedThread ) {
				found = true;
				break;
			}
		if( !found ) {
			currentThread = mainThread;
			prepareStack();
		}
		return r;
	}

	function singleStep(tid,set=true) {
		var r = getReg(tid, EFlags).toInt();
		if( set ) r |= 256 else r &= ~256;
		setReg(tid, EFlags, hld.Pointer.make(r,0));
	}

	public function getException() {
		var t = threads.get(currentThread);
		if( t == null )
			return null;
		var exc = t.exception;
		if( exc.isNull() )
			return null;
		return eval.readVal(exc, HDyn);
	}

	public function getCurrentVars( args : Bool ) {
		var s = currentStack[currentStackFrame];
		var g = module.getGraph(s.fidx);
		return args ? g.getArgs() : g.getLocals(s.fpos);
	}

	public function getCurrentClass() {
		var s = currentStack[currentStackFrame];
		var ctx = module.getMethodContext(s.fidx);
		if( ctx == null )
			return null;
		var name = ctx.obj.name;
		return name.split("$").join("");
	}

	public function getClassStatics( cl : String ) {
		var v = getValue(cl);
		if( v == null )
			throw "No such class "+cl;
		var fields = eval.getFields(v);
		fields.remove("__name__");
		fields.remove("__type__");
		fields.remove("__meta__");
		fields.remove("__implementedBy__");
		fields.remove("__constructor__");
		return fields;
	}

	function wait( onStep = false ) : Api.WaitResult {
		var cmd = null;
		watchBreak = null;
		while( true ) {
			cmd = api.wait(customTimeout == null ? 1000 : Math.ceil(customTimeout * 1000));

			if( cmd.r == Breakpoint && nextStep >= 0 ) {
				// On Linux, singlestep is not reset
				cmd.r = SingleStep;
				singleStep(cmd.tid,false);
			}

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
			case Exit:
				processExit = true;
				break;
			case Error, Watchbreak:
				break;
			}
		}
		stoppedThread = cmd.tid;

		// in thread-disabled we don't know the main thread id in HL:
		// first stop is on a special thread in windows
		// wait for second stop with is user-specific
		if( jit.oldThreadInfos != null )
			mainThread = jit.oldThreadInfos.id;
		else if( mainThread == 0 )
			mainThread = -1;
		else if( mainThread == -1 )
			mainThread = stoppedThread;

		readThreads();
		prepareStack();
		return cmd.r;
	}

	function readThreads() {
		var old = jit.oldThreadInfos;
		threads = new Map();
		if( old != null ) {
			threads.set(old.id, { id : old.id, stackTop : old.stackTop, exception : eval.readPointer(old.debugExc) });
			return;
		}
		var count = eval.readI32(jit.threads);
		var tinfos = eval.readPointer(jit.threads.offset(8));
		var flagsPos = jit.align.ptr * 6 + 8;
		var excPos = jit.align.ptr * 5 + 8;
		for( i in 0...count ) {
			var tinf = eval.readPointer(tinfos.offset(jit.align.ptr * i));
			var tid = eval.readI32(tinf);
			var flags = eval.readI32(tinf.offset(flagsPos));
			if( flags & 16 != 0 ) continue; // invisible
			if( tid == 0 )
				tid = mainThread;
			else if( mainThread <= 0 )
				mainThread = tid;
			var t = {
				id : tid,
				stackTop : eval.readPointer(tinf.offset(8)),
				exception : flags & 4 == 0 ? null : tinf.offset(excPos),
			};
			threads.set(tid, t);
		}
	}

	function prepareStack() {
		currentStackFrame = 0;
		currentStack = makeStack(currentThread);
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
		var tid = currentThread;
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

	function skipFunction( fidx : Int ) {
		var ctx = module.getMethodContext(fidx);
		var name = ctx == null ? new haxe.io.Path(module.resolveSymbol(fidx, 0).file).file : ctx.obj.name.split(".")[0];
		if( name.charCodeAt(0) == "$".code ) name = name.substr(1);
		if( ignoredRoots == null ) {
			ignoredRoots = new Map();
			for( r in IGNORED_ROOTS )
				ignoredRoots.set(r,true);
		}
		return ignoredRoots.exists(name);
	}

	public function step( mode : StepMode ) : Api.WaitResult {
		var tid = currentThread;
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
				if( st.fidx != s.fidx ) {
					if( deltaEbp < 0 && skipFunction(st.fidx) )
						continue;
					break;
				}
				if( module.resolveSymbol(st.fidx, st.fpos).line != line )
					break;
			case Out:
				// only on ret
			}
			// returned !
			if( deltaEbp > 0 )
				break;
		}
		prepareStack();
		return Breakpoint;
	}

	function makeStack(tid,max = 0) {
		var stack = [];
		var tinf = threads.get(tid);
		if( tinf == null )
			return stack;
		var esp = getReg(tid, Esp);
		var size = tinf.stackTop.sub(esp) + jit.align.ptr;
		if( size < 0 ) size = 0;
		var mem = readMem(esp.offset(-jit.align.ptr), size);

		var eip = getReg(tid, Eip);
		var asmPos = eip.sub(jit.codeStart);
		var e = jit.resolveAsmPos(asmPos);
		var inProlog = false;

		// if we are on ret, our EBP is wrong, so let's ignore this stack part
		if( e != null ) {
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
		}

		// when requiring only top level stack, do not look further if we are in a C function
		// because we need to step so we don't want false positive
		if( max == 1 ) return stack;

		// similar to module/module_capture_stack
		if( is64 ) {
			for( i in 0...size >> 3 ) {
				var val = mem.getPointer(i << 3, jit.align);
				if( val > esp && val < tinf.stackTop || (inProlog && i == 0) ) {
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
			var stackTop = tinf.stackTop.toInt();
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
		eval.setContext(cur.fidx, cur.fpos, cur.ebp);
		return eval.eval(expr);
	}

	public function setValue( expr : String, value : String ) : Value {
		var cur = currentStack[currentStackFrame];
		if( cur == null ) return null;
		eval.setContext(cur.fidx, cur.fpos, cur.ebp);
		return eval.setValue(expr, value);
	}

	public function getRef( expr : String ) : Address {
		var cur = currentStack[currentStackFrame];
		if( cur == null ) return null;
		eval.setContext(cur.fidx, cur.fpos, cur.ebp);
		return eval.ref(expr);
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
		if( currentThread == null ) {
			pause();
			wasPaused = true;
		}
		var dr7 = 0x100;
		for( w in watches ) {
			for( r in w.regs ) {
				var rid = HW_REGS.indexOf(r.r);
				dr7 |= 1 << (rid * 2);
				dr7 |= ((w.forReadWrite ? 3 : 1) | (r.bits << 2)) << (16 + rid * 4);
				api.writeRegister(currentThread, r.r, w.addr.ptr.offset(r.offset));
			}
		}
		api.writeRegister(currentThread, Dr7, Pointer.make(dr7, 0));
		if( wasPaused )
			resume();
	}

	public function resume() {
		if( stoppedThread == null )
			throw "No thread stopped";
		if( !api.resume(stoppedThread) && !processExit )
			throw "Could not resume "+stoppedThread;
		stoppedThread = null;
		watchBreak = null;
	}

	public function end() {
		if( stoppedThread != null ) resume();
		api.stop();
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
			return -1;
		// check already defined
		var set = false;
		for( b in breaks.breaks ) {
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
		return breaks.line;
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
		for( b in breaks.breaks )
			for( a in breakPoints )
				if( a.fid == b.ifun && a.pos == b.pos ) {
					rem = true;
					removeBP(a);
					break;
				}
		return rem;
	}

	function set_breakOnThrow(b) {
		var count = eval.readI32(jit.threads);
		var tinfos = eval.readPointer(jit.threads.offset(8));
		var flagsPos = jit.align.ptr * 6 + 8;
		for( i in 0...count ) {
			var tinf = eval.readPointer(tinfos.offset(jit.align.ptr * i));
			var flags = eval.readI32(tinf.offset(flagsPos));
			if( b ) flags |= 2 else flags &= ~2;
			eval.writeI32(tinf.offset(flagsPos), flags);
		}
		return breakOnThrow = b;
	}


}