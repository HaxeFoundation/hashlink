package hld;

class Align {

	public var is64(default, null) : Bool;
	public var ptr(get, never) : Int;

	var ptrSize : Int;
	var boolSize : Int;
	var structSizes : Array<Int>;

	public function new( is64, boolSize ) {
		this.is64 = is64;
		this.ptrSize = is64 ? 8 : 4;
		this.boolSize = boolSize;
	}

	inline function get_ptr() {
		return ptrSize;
	}

	public function typeSize( t : HLType ) {
		return switch( t ) {
		case HVoid: 0;
		case HUi8: 1;
		case HUi16: 2;
		case HI32, HF32: 4;
		case HI64, HF64: 8;
		case HBool: boolSize;
		default: ptrSize;
		}
	}

	public function stackSize( t : HLType ) {
		return switch( t ) {
		case HUi8, HUi16, HBool: ptrSize;
		case HI32, HF32 if( is64 ): ptrSize;
		default: typeSize(t);
		}
	}

	inline function rest( v : Int, size : Int ) {
		return ( -v) & (size - 1);
	}

	public function padSize( v : Int, t : HLType ) {
		return t == HVoid ? 0 : rest(v, typeSize(t));
	}

	public function padStruct( v : Int, t : HLType ) {
		return rest(v, structSize(t));
	}

	function structSize( t : HLType ) : Int {
		if( t.isPtr() )
			return structSizes[structSizes.length - 1];
		return structSizes[t.getIndex()];
	}

}