package dx;
import haxe.EntryPoint;

private typedef WinPtr = hl.Abstract<"dx_window">;
typedef MonitorHandle = String;

typedef Monitor = {
	name : MonitorHandle,
	left : Int,
	right : Int,
	top : Int,
	bottom : Int
}

typedef DisplaySetting = {
	width : Int,
	height : Int,
	framerate : Int
}

@:enum abstract DisplayMode(Int) {
	var Windowed = 0;
	var Fullscreen = 1;
	var Borderless = 2;
}

@:hlNative("directx")
class Window {

	static var windows : Array<Window> = [];

	public static inline var CW_USEDEFAULT = 0x80000000;

	public static inline var HIDDEN    = 0x000001;
	public static inline var RESIZABLE = 0x000002;

	var win : WinPtr;
	public var title(default, set) : String;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var minWidth(get, never) : Int;
	public var minHeight(get, never) : Int;
	public var maxWidth(get, never) : Int;
	public var maxHeight(get, never) : Int;
	public var x(get, never) : Int;
	public var y(get, never) : Int;
	public var displayMode(default, set) : DisplayMode;
	public var visible(default, set) : Bool = true;
	public var opacity(get, set) : Float;
	public var displaySetting : DisplaySetting;
	public var selectedMonitor : MonitorHandle;
	public var vsync : Bool;

	public function new( title : String, width : Int, height : Int, x : Int = CW_USEDEFAULT, y : Int = CW_USEDEFAULT, windowFlags : Int = RESIZABLE ) {
		win = winCreateEx(x, y, width, height, windowFlags);
		this.title = title;
		windows.push(this);
		vsync = true;
	}

	function set_title(name:String) {
		winSetTitle(win, @:privateAccess name.bytes);
		return title = name;
	}

	function set_displayMode(mode) {
		displayMode = mode;
		if(mode == Windowed) {
			dx.Window.winChangeDisplaySetting(selectedMonitor != null ? @:privateAccess selectedMonitor.bytes : null, null);
			winSetFullscreen(win, false);
		}
		else if(mode == Borderless) {
			dx.Window.winChangeDisplaySetting(selectedMonitor != null ? @:privateAccess selectedMonitor.bytes : null, null);
			winSetFullscreen(win,true);
		}
		else {
			var r = dx.Window.winChangeDisplaySetting(selectedMonitor != null ? @:privateAccess selectedMonitor.bytes : null, displaySetting);
			winSetFullscreen(win,true);
		}
		return mode;
	}

	function set_visible(b) {
		if( visible == b )
			return b;
		winResize(win, b ? 4 : 3);
		return visible = b;
	}

	public function resize( width : Int, height : Int ) {
		winSetSize(win, width, height);
	}

	public function setMinSize( width : Int, height : Int ) {
		winSetMinSize(win, width, height);
	}

	public function setMaxSize( width : Int, height : Int ) {
		winSetMaxSize(win, width, height);
	}

	public function setPosition( x : Int, y : Int ) {
		winSetPosition(win, x, y);
	}

	public function center( centerPrimary : Bool = true ) {
		winCenter(win, centerPrimary);
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

	public static function getDisplaySettings(monitor : MonitorHandle) : Array<DisplaySetting> {
		var a : Array<DisplaySetting> = [for(s in winGetDisplaySettings(monitor != null ? @:privateAccess monitor.bytes : null)) s];
		a.sort((a, b) -> {
			if(b.width > a.width) 1;
			else if(b.width < a.width) -1;
			else if(b.height > a.height) 1;
			else if(b.height < a.height) -1;
			else if(b.framerate > a.framerate) 1;
			else if(b.framerate < a.framerate) -1;
			else 0;
		});
		var last = null;
		return a.filter(function(e) {
			if(last == null) {
				last = e;
				return true;
			}
			else if(last.width == e.width && last.height == e.height && last.framerate == e.framerate) {
				return false;
			}
			last = e;
			return true;
		});
	}

	public static function getCurrentDisplaySetting(monitor : MonitorHandle, registry : Bool = false) : DisplaySetting {
		return winGetCurrentDisplaySetting(monitor != null ? @:privateAccess monitor.bytes : null, registry);
	}

	public static function getMonitors() : Array<Monitor> {
		var last = null;
		return [for(m in winGetMonitors()) @:privateAccess { name: String.fromUCS2(m.name), left: m.left, right: m.right, top: m.top, bottom: m.bottom } ];
	}

	public function getCurrentMonitor() : MonitorHandle {
		return @:privateAccess String.fromUCS2(winGetMonitorFromWindow(win));
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

	function get_minWidth() {
		var w = 0;
		winGetMinSize(win, w, null);
		return w;
	}

	function get_minHeight() {
		var h = 0;
		winGetMinSize(win, null, h);
		return h;
	}

	function get_maxWidth() {
		var w = 0;
		winGetMaxSize(win, w, null);
		return w;
	}

	function get_maxHeight() {
		var h = 0;
		winGetMaxSize(win, null, h);
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

	function get_opacity() {
		return winGetOpacity(win);
	}

	function set_opacity(v) {
		winSetOpacity(win, v);
		return v;
	}

	@:hlNative("?directx", "win_get_display_settings")
	static function winGetDisplaySettings(monitor : hl.Bytes) : hl.NativeArray<Dynamic> {
		return null;
	}

	@:hlNative("?directx", "win_get_current_display_setting")
	static function winGetCurrentDisplaySetting(monitor : hl.Bytes, registry : Bool) : Dynamic {
		return null;
	}

	@:hlNative("?directx", "win_change_display_setting")
	public static function winChangeDisplaySetting(monitor : hl.Bytes, ds : Dynamic) : Int {
		return 0;
	}

	@:hlNative("?directx", "win_get_monitors")
	static function winGetMonitors() : hl.NativeArray<Dynamic> {
		return null;
	}

	@:hlNative("?directx", "win_get_monitor_from_window")
	static function winGetMonitorFromWindow( win : WinPtr ) : hl.Bytes {
		return null;
	}

	static function winCreateEx( x : Int, y : Int, width : Int, height : Int, windowFlags : Int ) : WinPtr {
		return null;
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

	static function winSetMinSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetMaxSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winSetPosition( win : WinPtr, x : Int, y : Int ) {
	}

	static function winCenter( win : WinPtr, centerPrimary : Bool ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMinSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetMaxSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winGetPosition( win : WinPtr, x : hl.Ref<Int>, y : hl.Ref<Int> ) {
	}

	static function winGetOpacity( win : WinPtr ) : Float {
		return 0.0;
	}

	static function winSetOpacity( win : WinPtr, opacity : Float ) : Bool {
		return false;
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