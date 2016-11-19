package sdl;

private typedef GameControllerPtr = hl.Abstract<"sdl_gamecontroller">;

@:hlNative("sdl")
class GameController {

	var ptr : GameControllerPtr;

	public var id(get,never) : Int;
	public var name(get,never) : String;

	public function new( index : Int ){
		ptr = gctrlOpen( index );
	}

	public inline function getAxis( axisId : Int ){
		return gctrlGetAxis(ptr,axisId);
	}

	public inline function getButton( btnId : Int ){
		return gctrlGetButton(ptr,btnId);
	}

	public inline function get_id() : Int {
		return gctrlGetId(ptr);
	}

	public inline function get_name() : String {
		return @:privateAccess String.fromUTF8( gctrlGetName(ptr) );
	}

	public function close(){
		gctrlClose( ptr );
		ptr = null;
	}

	static function gctrlCount() : Int {
		return 0;
	}

	static function gctrlOpen( idx : Int ) : GameControllerPtr {
		return null;
	}

	static function gctrlClose( controller : GameControllerPtr ){
	}

	static function gctrlGetAxis( controller : GameControllerPtr, axisId : Int ) : Int {
		return 0;
	}

	static function gctrlGetButton( controller : GameControllerPtr, btnId : Int ) : Bool {
		return false;
	}

	static function gctrlGetId( controller : GameControllerPtr ) : Int {
		return -1;
	}

	static function gctrlGetName( controller : GameControllerPtr ) : hl.Bytes {
		return null;
	}

}
