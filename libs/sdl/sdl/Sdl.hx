package sdl;

@:hlNative("sdl")
class Sdl {

	static var initDone = false;
	static var isWin32 = false;

	public static var requiredGLMajor(default,null) = 3;
	public static var requiredGLMinor(default,null) = 2;

	public static function init() {
		if( initDone ) return;
		initDone = true;
		if( !initOnce() ) throw "Failed to init SDL";
		isWin32 = detectWin32();
	}

	public static function setGLOptions( major : Int = 3, minor : Int = 2, depth : Int = 24, stencil : Int = 8, flags : Int = 1, samples : Int = 1 ) {
		requiredGLMajor = major;
		requiredGLMinor = minor;
		glOptions(major, minor, depth, stencil, flags, samples);
	}


	public static dynamic function onGlContextRetry() {
		return false;
	}

	public static dynamic function onGlContextError() {
		var devices = Sdl.getDevices();
		var device = devices[0];
		if( device == null ) device = "Unknown";
		var flags = new haxe.EnumFlags<hl.UI.DialogFlags>();
		flags.set(IsError);
		var msg = 'The application was unable to create an OpenGL context\nfor your $device video card.\nOpenGL $requiredGLMajor.$requiredGLMinor+ is required, please update your driver.';
		hl.UI.dialog("OpenGL Error", msg, flags);
		Sys.exit( -1);
	}

	public static inline var DOUBLE_BUFFER            = 1 << 0;
	public static inline var GL_CORE_PROFILE          = 1 << 1;
	public static inline var GL_COMPATIBILITY_PROFILE = 1 << 2;
	public static inline var GL_ES                    = 1 << 3;

	static function glOptions( major : Int, minor : Int, depth : Int, stencil : Int, flags : Int, samples : Int ) {}

	static function initOnce() return false;
	static function eventLoop( e : Event ) return false;

	static var event = new Event();
	public static function processEvents( onEvent : Event -> Bool ) {
		while( true ) {
			if( !eventLoop(event) )
				break;
			var ret = onEvent(event);
			if( event.type == Quit && ret )
				return false;
		}
		return true;
	}

	public static function quit() {
	}

	public static function delay(time:Int) {
	}


	public static function getScreenWidth() : Int {
		return 0;
	}

	public static function getScreenHeight() : Int {
		return 0;
	}

	public static function message( title : String, text : String, error = false ) {
		@:privateAccess messageBox(title.toUtf8(), text.toUtf8(), error);
	}

	static function messageBox( title : hl.Bytes, text : hl.Bytes, error : Bool ) {
	}

	static function detectWin32() {
		return false;
	}

	static function get_devices() : hl.NativeArray<hl.Bytes> {
		return null;
	}

	public static function getDevices() {
		var a = [];
		var arr = get_devices();
		var dnames = new Map();
		for( v in arr ) {
			if( v == null ) break;
			var d = StringTools.trim(@:privateAccess String.fromUCS2(v));
			if( dnames.exists(d) || StringTools.startsWith(d,"RDP") /* RemoteDesktop */ ) continue;
			dnames.set(d, true);
			a.push(d);
		}
		return a;
	}

	public static function setRelativeMouseMode( enable : Bool ) : Int {
		return 0;
	}

	static function detect_keyboard_layout() : hl.Bytes {
		return null;
	}

	public static function detectKeyboardLayout(){
		return @:privateAccess String.fromUTF8(detect_keyboard_layout());
	}

}