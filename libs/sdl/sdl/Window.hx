package sdl;

private typedef WinPtr = hl.types.NativeAbstract<"sdl_window">;
private typedef GLContext = hl.types.NativeAbstract<"sdl_gl">;

@:hlNative("sdl")
class Window {

	static var windows : Array<Window> = [];

	var win : WinPtr;
	var glctx : GLContext;
	var lastFrame : Float;
	public var vsync(default, set) : Bool;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var fullScreen(default, set) : Bool;

	public function new( title : String, width : Int, height : Int ) {
		win = winCreate(@:privateAccess title.toUtf8(), width, height);
		if( win == null ) throw "Failed to create window";
		glctx = winGetGLContext(win);
		if( glctx == null ) throw "Failed to init GL Context (OpenGL 2.1 required)";
		if( !GL.init() ) throw "Failed to init GL API";
		windows.push(this);
		vsync = true;
	}

	function set_fullScreen(b) {
		if( b == fullScreen )
			return b;
		if( winSetFullscreen(win, b) )
			fullScreen = b;
		return fullScreen;
	}

	public function resize( width : Int, height : Int ) {
		winSetSize(win, width, height);
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

	function set_vsync(v) {
		setVsync(v);
		return vsync = v;
	}

	/**
		Set the current window you will render to (in case of multiple windows)
	**/
	public function renderTo() {
		winRenderTo(win, glctx);
	}

	public function present() {
		if( vsync && @:privateAccess Sdl.isWin32 ) {
			// NVIDIA OpenGL windows driver does implement vsync as an infinite loop, causing high CPU usage
			// make sure to sleep a bit here based on how much time we spent since the last frame
			var spent = haxe.Timer.stamp() - lastFrame;
			if( spent < 0.005 ) Sys.sleep(0.005 - spent);
		}
		GL.finish();
		winSwapWindow(win);
		lastFrame = haxe.Timer.stamp();
	}

	public function destroy() {
		winDestroy(win, glctx);
		win = null;
		glctx = null;
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

	static function winCreate( title : hl.types.Bytes, width : Int, height : Int ) : WinPtr {
		return null;
	}

	static function winGetGLContext( win : WinPtr ) : GLContext {
		return null;
	}

	static function winSwapWindow( win : WinPtr ) {
	}

	static function winSetFullscreen( win : WinPtr, b : Bool ) {
		return false;
	}

	static function winSetSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.types.Ref<Int>, height : hl.types.Ref<Int> ) {
	}

	static function winRenderTo( win : WinPtr, gl : GLContext ) {
	}

	static function winDestroy( win : WinPtr, gl : GLContext ) {
	}

	static function setVsync( b : Bool ) {
	}

}