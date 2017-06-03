package dx;

private typedef CursorPtr = hl.Abstract<"dx_cursor">;

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

//@:hlNative("dx")
abstract Cursor(CursorPtr) {

	//@:hlNative("dx", "cursor_create_system")
	public static function createSystem( kind : CursorKind ) : Cursor {
		return null;
	}

	public function free() {
		freeCursor(this);
	}

	public function set() {
		setCursor(this);
	}

	//@:hlNative("dx", "show_cursor")
	public static function show( v : Bool ) {
	}

	//@:hlNative("dx", "set_cursor")
	static function setCursor( k : CursorPtr ) {
	}

	static function createSystemCursor( kind : CursorKind ) : CursorPtr {
		return null;
	}

	static function freeCursor( ptr : CursorPtr ) {
	}

}
