package sdl;

private typedef GameControllerPtr = hl.Abstract<"sdl_gamecontroller">;
private typedef HapticPtr = hl.Abstract<"sdl_haptic">;

@:hlNative("sdl")
class GameController {

	var ptr : GameControllerPtr;
	var haptic : HapticPtr;

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

	public function rumbleInit() : Bool {
		if( haptic == null ){
			haptic = hapticOpen(ptr);
			if( haptic == null )
			 	return false;
			if( hapticRumbleInit(haptic) != 0 ){
				hapticClose(haptic);
				haptic = null;
				return false;
			}
		}
		return true;
	}

	public function rumble( strength : Float, length : Int ) : Bool {
		if( haptic == null ) return false;
		return hapticRumblePlay( haptic, strength, length ) == 0;
	}

	public function close(){
		if( haptic != null )
			hapticClose(haptic);
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

	static function hapticOpen( controller : GameControllerPtr ) : HapticPtr {
		return null;
	}

	static function hapticClose( haptic : HapticPtr ) : Void {
	}

	static function hapticRumbleInit( haptic : HapticPtr ) : Int {
		return -1;
	}
	
	static function hapticRumblePlay( haptic : HapticPtr, strength : Float, length : Int ) : Int {
		return -1;
	}


}
