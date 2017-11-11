package hld;

class Size {

	public var ptr(get, never) : Int;

	var ptrSize : Int;
	var boolSize : Int;

	public function new( ptrSize, boolSize ) {
		this.ptrSize = ptrSize;
		this.boolSize = boolSize;
	}

	inline function get_ptr() return ptrSize;

	public function type( t : HLType ) {
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

	inline function rest( v : Int, size : Int ) {
		return ( -v) & (size - 1);
	}

	public inline function pad( v : Int,  t : HLType ) {
		return rest(v, type(t));
	}


}