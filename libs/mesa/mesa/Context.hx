package mesa;

abstract Context(hl.Abstract<"mesa_ctx">) {

//	public static var COLOR_INDEX = GL.COLOR_INDEX;
	public static var RGBA = GL.RGBA;
	public static var BGRA = 0x1;
	public static var ARGB = 0x2;
	public static var RGB = GL.RGB;
	public static var BGR = 0x4;
	public static var RGB_565 = 0x5;

	public static var ROW_LENGTH = 0x10;
	public static var Y_UP = 0x11;

	public static var WIDTH	= 0x20;
	public static var HEIGHT = 0x21;
	public static var FORMAT = 0x22;
	public static var TYPE = 0x23;
	public static var MAX_WIDTH = 0x24;
	public static var MAX_HEIGHT = 0x25;

	public static var DEPTH_BITS            = 0x30;
	public static var STENCIL_BITS          = 0x31;
	public static var ACCUM_BITS            = 0x32;
	public static var PROFILE               = 0x33;
	public static var CORE_PROFILE          = 0x34;
	public static var COMPAT_PROFILE        = 0x35;
	public static var CONTEXT_MAJOR_VERSION = 0x36;
	public static var CONTEXT_MINOR_VERSION = 0x37;

	@:hlNative("mesa","create_context")
	public static function create( attribs : hl.Bytes, ?shared : Context ) : Context {
		return null;
	}

	@:hlNative("mesa","destroy_context")
	public function destroy() {
	}

	@:hlNative("mesa","make_current")
	public function makeCurrent( buffer : hl.Bytes, type : Int, width : Int, height : Int ) : Bool {
		return false;
	}

}