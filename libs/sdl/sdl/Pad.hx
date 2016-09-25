package sdl;

private typedef PadPtr = hl.types.NativeAbstract<"sdl_gamecontroller">;

@:hlNative("sdl")
class Pad {
	
	var ptr : PadPtr;
	
	public var id(get,never) : Int;
	public var name(get,never) : String;
	
	public function new( index : Int ){
		ptr = padOpen( index );
	}
	
	public inline function getAxis( axisId : Int ){
		return padGetAxis(ptr,axisId);
	}
	
	public inline function getButton( btnId : Int ){
		return padGetButton(ptr,btnId);
	}
	
	public inline function get_id() : Int {
		return padGetId(ptr);
	}
	
	public inline function get_name() : String {
		return @:privateAccess String.fromUTF8( padGetName(ptr) );
	}
	
	public function close(){
		padClose( ptr );
		ptr = null;
	}
	
	static function padCount() : Int {
		return 0;
	}
	
	static function padOpen( idx : Int ) : PadPtr {
		return null;
	}
	
	static function padClose( controller : PadPtr ){
	}
	
	static function padGetAxis( controller : PadPtr, axisId : Int ) : Int {
		return 0;
	}
	
	static function padGetButton( controller : PadPtr, btnId : Int ) : Bool {
		return false;
	}
	
	static function padGetId( controller : PadPtr ) : Int {
		return -1;
	}
	
	static function padGetName( controller : PadPtr ) : hl.types.Bytes {
		return null;
	}

}
