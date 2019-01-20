package dx;

private typedef CursorPtr = hl.Abstract<"dx_cursor">;

@:enum abstract CursorKind(Int) {
	var Arrow = 32512;
    var IBeam = 32513;
    var Wait = 32514;
    var CrossHair = 32515;
    var WaitArrow = 32650;
    var No = 32648;
    var Hand = 32649;
	var SizeALL = 32646;
}

abstract Cursor(CursorPtr) {

	@:hlNative("directx","load_cursor")
	public static function createSystem( kind : CursorKind ) : Cursor {
		return null;
	}

	@:hlNative("directx","create_cursor")
	public static function createCursor( width : Int, height : Int, pixels : hl.Bytes, hotX : Int, hotY : Int ) : Cursor {
		return null;
	}

	public function destroy() {
		destroyCursor(this);
	}

	public function set() {
		setCursor(this);
	}

	@:hlNative("directx","show_cursor")
	public static function show( v : Bool ) {
	}

	@:hlNative("directx","is_cursor_visible")
	public static function isVisible() : Bool {
		return false;
	}

	@:hlNative("directx","set_cursor")
	static function setCursor( k : CursorPtr ) {
	}

	@:hlNative("directx","destroy_cursor")
	static function destroyCursor( ptr : CursorPtr ) {
	}

}
