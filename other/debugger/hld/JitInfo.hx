package hld;

private enum DebugFlag {
	Is64; // runs in 64 bit mode
	Bool4; // bool = 4 bytes (instead of 1)
}

class JitInfo {

	public var is64(default, null) : Bool;
	public var align(default,null) : Align;


	var flags : haxe.EnumFlags<DebugFlag>;
	var input : haxe.io.Input;

	public var mainThread : Int;
	public var globals : Pointer;
	public var stackTop : Pointer;
	public var codeStart : Pointer;
	public var codeEnd : Pointer;
	public var debugExc : Pointer;
	var codeSize : Int;
	var allTypes : Pointer;

	var functions : Array<{ start : Int, large : Bool, offsets : haxe.io.Bytes }>;
	var functionByCodePos : Map<Int,Int>;
	var module : Module;

	public function new() {
	}

	function readPointer() : Pointer {
		if( is64 )
			return Pointer.make(input.readInt32(), input.readInt32());
		return Pointer.make(input.readInt32(),0);
	}

	public function read( input : haxe.io.Input, module : Module ) {
		this.input = input;
		this.module = module;

		if( input.readString(3) != "HLD" )
			return false;
		var version = input.readByte() - "0".code;
		if( version > 0 )
			return false;
		flags = haxe.EnumFlags.ofInt(input.readInt32());
		is64 = flags.has(Is64);
		align = new Align(is64, flags.has(Bool4)?4:1);

		mainThread = input.readInt32();
		globals = readPointer();
		debugExc = readPointer();
		stackTop = readPointer();
		codeStart = readPointer();
		codeSize = input.readInt32();
		codeEnd = codeStart.offset(codeSize);
		allTypes = readPointer();
		functions = [];

		var structSizes = [0];
		for( i in 1...9 )
			structSizes[i] = input.readInt32();
		@:privateAccess align.structSizes = structSizes;

		var nfunctions = input.readInt32();
		if( nfunctions != module.code.functions.length )
			return false;

		functionByCodePos = new Map();
		for( i in 0...nfunctions ) {
			var nops = input.readInt32();
			if( module.code.functions[i].debug.length >> 1 != nops )
				return false;
			var start = input.readInt32();
			var large = input.readByte() != 0;
			var offsets = input.read((nops + 1) * (large ? 4 : 2));
			functionByCodePos.set(start, i);
			functions.push({
				start : start,
				large : large,
				offsets : offsets,
			});
		}
		return true;
	}

	public function getFunctionPos( fidx : Int ) {
		return functions[fidx].start;
	}

	public function getCodePos( fidx : Int, pos : Int ) {
		var dbg = functions[fidx];
		return dbg.start + (dbg.large ? dbg.offsets.getInt32(pos << 2) : dbg.offsets.getUInt16(pos << 1));
	}

	public function resolveAsmPos( asmPos : Int ) {
		if( asmPos < 0 || asmPos > codeSize )
			return null;
		var min = 0;
		var max = functions.length;
		while( min < max ) {
			var mid = (min + max) >> 1;
			var p = functions[mid];
			if( p.start <= asmPos )
				min = mid + 1;
			else
				max = mid;
		}
		if( min == 0 )
			return null;
		var fidx = (min - 1);
		var dbg = functions[fidx];
		var fdebug = module.code.functions[fidx];
		min = 0;
		max = fdebug.debug.length>>1;
		var relPos = asmPos - dbg.start;
		while( min < max ) {
			var mid = (min + max) >> 1;
			var offset = dbg.large ? dbg.offsets.getInt32(mid * 4) : dbg.offsets.getUInt16(mid * 2);
			if( offset <= relPos )
				min = mid + 1;
			else
				max = mid;
		}
		return { fidx : fidx, fpos : min - 1, codePos : asmPos, ebp : null };
	}

	public function functionFromAddr( p : Pointer ) {
		return functionByCodePos.get(p.sub(codeStart));
	}

}