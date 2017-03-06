package sdl;

private typedef WinPtr = hl.Abstract<"sdl_window">;
private typedef GLContext = hl.Abstract<"sdl_gl">;

@:enum abstract DisplayMode(Int) {
	var Windowed = 0;
	var Fullscreen = 1;
	/**
		Fullscreen not exclusive.
	**/
	var Borderless = 2;
	var FullscreenResize = 3;
}

@:hlNative("sdl")
class Window {

	static var windows : Array<Window> = [];

	var win : WinPtr;
	var glctx : GLContext;
	var lastFrame : Float;
	public var vsync(default, set) : Bool;
	public var width(get, never) : Int;
	public var height(get, never) : Int;
	public var displayMode(default, set) : DisplayMode;

	public function new( title : String, width : Int, height : Int ) {
		while( true ) {
			win = winCreate(@:privateAccess title.toUtf8(), width, height);
			if( win == null ) throw "Failed to create window";
			glctx = winGetGLContext(win);
			if( glctx == null || !GL.init() || !testGL() ) {
				destroy();
				if( Sdl.onGlContextRetry() ) continue;
				Sdl.onGlContextError();
			}
			break;
		}
		windows.push(this);
		vsync = true;
	}

	function testGL() {
		try {

			var reg = ~/[0-9]+\.[0-9]+/;
			var v : String = GL.getParameter(GL.SHADING_LANGUAGE_VERSION);
			var shaderVersion = 130;
			if( reg.match(v) )
				shaderVersion = hxd.Math.imin( 150, Math.round( Std.parseFloat(reg.matched(0)) * 100 ) );

			var vertex = GL.createShader(GL.VERTEX_SHADER);
			GL.shaderSource(vertex, ["#version " + shaderVersion, "void main() { gl_Position = vec4(1.0); }"].join("\n"));
			GL.compileShader(vertex);
			if( GL.getShaderParameter(vertex, GL.COMPILE_STATUS) != 1 ) throw "Failed to compile VS ("+GL.getShaderInfoLog(vertex)+")";

			var fragment = GL.createShader(GL.FRAGMENT_SHADER);
			GL.shaderSource(fragment, ["#version " + shaderVersion, "out vec4 color; void main() { color = vec4(1.0); }"].join("\n"));
			GL.compileShader(fragment);
			if( GL.getShaderParameter(fragment, GL.COMPILE_STATUS) != 1 ) throw "Failed to compile FS ("+GL.getShaderInfoLog(fragment)+")";

			var p = GL.createProgram();
			GL.attachShader(p, vertex);
			GL.attachShader(p, fragment);
			GL.linkProgram(p);

			if( GL.getProgramParameter(p, GL.LINK_STATUS) != 1 ) throw "Failed to link ("+GL.getProgramInfoLog(p)+")";

			GL.deleteShader(vertex);
			GL.deleteShader(fragment);

		} catch( e : Dynamic ) {

			return false;
		}
		return true;
	}

	function set_displayMode(mode) {
		if( mode == displayMode )
			return mode;
		if( winSetFullscreen(win, cast mode) )
			displayMode = mode;
		return displayMode;
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

	static function winCreate( title : hl.Bytes, width : Int, height : Int ) : WinPtr {
		return null;
	}

	static function winGetGLContext( win : WinPtr ) : GLContext {
		return null;
	}

	static function winSwapWindow( win : WinPtr ) {
	}

	static function winSetFullscreen( win : WinPtr, mode : DisplayMode ) {
		return false;
	}

	static function winSetSize( win : WinPtr, width : Int, height : Int ) {
	}

	static function winResize( win : WinPtr, mode : Int ) {
	}

	static function winGetSize( win : WinPtr, width : hl.Ref<Int>, height : hl.Ref<Int> ) {
	}

	static function winRenderTo( win : WinPtr, gl : GLContext ) {
	}

	static function winDestroy( win : WinPtr, gl : GLContext ) {
	}

	static function setVsync( b : Bool ) {
	}

}