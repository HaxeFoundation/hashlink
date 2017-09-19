package dx;
import haxe.EntryPoint;

private typedef WinPtr = hl.Abstract<"dx_window">;

@:enum abstract DisplayMode(Int) {
	var Windowed = 0;
	var Fullscreen = 1;
	/**
		Fullscreen not exclusive.
	**/
	var Borderless = 2;
	var FullscreenResize = 3;
}

@:hlNative("directx")
class Window {

	static var windows : Array<Window> = [];

	var win : WinPtr;
	var savedSize : { x : Int, y : Int, width : Int, height : Int };
	public var title(default, set) : String;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var x(get, never) : Int;
	public var y(get, never) : Int;
	public var displayMode(default, set) : DisplayMode;
	public var vsync : Bool;

	public function new( title : String, width : Int, height : Int ) {
		win = winCreate(width, height);
		this.title = title;
		windows.push(this);
		vsync = true;
	}

	function set_title(name:String) {
		winSetTitle(win, @:privateAccess name.bytes);
		return title = name;
	}

	function set_displayMode(mode) {
		if( mode == displayMode )
			return mode;
		displayMode = mode;
		var fs = mode != Windowed;
		if( savedSize == null ) {
			if( !fs ) return mode;
			savedSize = { x : x, y : y, width : width, height : height };
			winSetFullscreen(win,true);
			Driver.fullScreen = mode == Fullscreen;
		} else {
			Driver.fullScreen = mode == Fullscreen;
			if( fs )
				return mode;
			winSetFullscreen(win, false);
			resize(savedSize.width, savedSize.height);
			setPosition(savedSize.x, savedSize.y);
			savedSize = null;
		}
		return mode;
	}

	public function resize( width : Int, height : Int ) {
		winSetSize(win, width, height);
	}

	public function setPosition( x : Int, y : Int ) {
		winSetPosition(win, x, y);
	}

	function get_width() {
		var w = 0;
		winGetSize(win, w, null);
		return w;
	}

	function get_height() {
		var h = 0;
		winGetSize(win, null, h);
		return h;
	}

	function get_x() {
		var x = 0;
		winGetPosition(win, x, null);
		return x;
	}

	function get_y() {
		var y = 0;
		winGetPosition(win, null, y);
		return y;
	}

	public function destroy() {
		winDestroy(win);
		win = null;
		windows.remove(this);
	}

	public function maximize() {
		winResize(win, 0);
	}

	public function minimize() {
		winResize(win, 1);
	}

	public function restore() {
		winResize(win, 2);
	}

	public function getNextEvent( e : Event ) : Bool {
		return winGetNextEvent(win, e);
	}

	public function clipCursor( enable : Bool ) : Void {
		winClipCursor(enable ? win : null);
	}

	static function winCreate( width : Int, height : Int ) : WinPtr {
		return null;
	}

	static function winSetTitle( win : WinPtr, title : hl.Bytes ) {
	}

	static function winSetFullscreen( win : WinPtr, fs : Bool ) {
	}

	static function winSetSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetPosition( win : WinPtr, x : Int, y : Int ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetPosition( win : WinPtr, x : hl.Ref<Int>, y : hl.Ref<Int> ) {
	}

	static function winDestroy( win : WinPtr ) {
	}

	public static function getScreenWidth() {
		return 0;
	}

	public static function getScreenHeight() {
		return 0;
	}

	static function winGetNextEvent( win : WinPtr, event : Dynamic ) : Bool {
		return false;
	}

	static function winClipCursor( win : WinPtr ) : Void {
	}

}