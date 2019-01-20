package sdl;

private typedef CursorPtr = hl.Abstract<"sdl_cursor">;

@:enum abstract CursorKind(Int) {
	var Arrow = 0;
    var IBeam = 1;
    var Wait = 2;
    var CrossHair = 3;
    var WaitArrow = 4;
    var SizeNWSE = 5;
    var SizeNESW = 6;
    var SizeWE = 7;
    var SizeNS = 8;
    var SizeALL = 9;
    var No = 10;
    var Hand = 11;
}

@:hlNative("sdl")
abstract Cursor(CursorPtr) {


	@:hlNative("sdl", "cursor_create")
	public static function create( surface : Surface, hotX : Int, hotY : Int ) : Cursor {
		return null;
	}

	@:hlNative("sdl", "cursor_create_system")
	public static function createSystem( kind : CursorKind ) : Cursor {
		return null;
	}

	public function free() {
		freeCursor(this);
	}

	public function set() {
		setCursor(this);
	}

	@:hlNative("sdl", "show_cursor")
	public static function show( v : Bool ) {
	}

	@:hlNative("sdl", "is_cursor_visible")
	public static function isVisible() : Bool {
		return false;
	}

	@:hlNative("sdl", "set_cursor")
	static function setCursor( k : CursorPtr ) {
	}

	static function freeCursor( ptr : CursorPtr ) {
	}

}
