package sdl;

private typedef JoystickPtr = hl.Abstract<"sdl_joystick">;

@:hlNative("sdl")
class Joystick {

	var ptr : JoystickPtr;

	public var id(get,never) : Int;
	public var name(get,never) : String;

	public function new( index : Int ){
		ptr = joyOpen( index );
	}

	public inline function getAxis( axisId : Int ){
		return joyGetAxis(ptr,axisId);
	}

	public inline function getHat( hatId : Int ){
		return joyGetHat(ptr,hatId);
	}

	public inline function getButton( btnId : Int ){
		return joyGetButton(ptr,btnId);
	}

	public inline function get_id() : Int {
		return joyGetId(ptr);
	}

	public inline function get_name() : String {
		return @:privateAccess String.fromUTF8( joyGetName(ptr) );
	}

	public function close(){
		joyClose( ptr );
		ptr = null;
	}

	static function joyCount() : Int {
		return 0;
	}

	static function joyOpen( idx : Int ) : JoystickPtr {
		return null;
	}

	static function joyClose( joystick : JoystickPtr ){
	}

	static function joyGetAxis( joystick : JoystickPtr, axisId : Int ) : Int {
		return 0;
	}

	static function joyGetHat( joystick : JoystickPtr, hatId : Int ) : Int {
		return 0;
	}

	static function joyGetButton( joystick : JoystickPtr, btnId : Int ) : Bool {
		return false;
	}

	static function joyGetId( joystick : JoystickPtr ) : Int {
		return -1;
	}

	static function joyGetName( joystick : JoystickPtr ) : hl.Bytes {
		return null;
	}


}
