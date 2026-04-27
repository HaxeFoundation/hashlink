package sdl;

typedef Display = {
	var handle : Window.DisplayHandle;
	var name : String;
	var left : Int;
	var top : Int;
	var right : Int;
	var bottom : Int;
};

typedef ScreenMode = {
	var format : Int;
	var width : Int;
	var height : Int;
	var framerate : Int;
};

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

	public static function setGLVersion( major : Int, minor : Int) {
		setGLOptions(major, minor);
	}

	public static function setHint(name:String, value:String) {
		return @:privateAccess hintValue(name.toUtf8(), value.toUtf8());
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
	static function eventLoop( e : Dynamic ) return false;
	static function hintValue( name : hl.Bytes, value : hl.Bytes ) return false;

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

	public static function getScreenWidth(?win : sdl.Window) : Int {
		return
			if(win == null)
				get_screen_width();
			else
				get_screen_width_of_window(@:privateAccess win.win);
	}

	public static function getScreenHeight(?win : sdl.Window) : Int {
		return
			if(win == null)
				get_screen_height();
			else
				get_screen_height_of_window(@:privateAccess win.win);
	}

	@:hlNative("?sdl", "get_framerate")
	public static function getFramerate(win : sdl.Window.WinPtr) : Int {
		return 0;
	}

	public static function message( title : String, text : String, error = false ) {
		@:privateAccess messageBox(title.toUtf8(), text.toUtf8(), error);
	}

	public static function getDisplayModes(display : Window.DisplayHandle) : Array<ScreenMode> {
		var modes = get_display_modes(display);
		if(modes == null)
			return [];
		return [ for(m in modes) m ];
	}

	public static function getCurrentDisplayMode(display : Window.DisplayHandle, registry : Bool = false) : ScreenMode {
		return get_current_display_mode(display, registry);
	}

	public static function getDisplays() : Array<Display> {
		var displays = get_displays();
		if(displays == null)
			return [];
		var i = 0;
		return [ for(d in get_displays() ) @:privateAccess { handle: d.handle, name: '${String.fromUTF8(d.name)} (${++i})', left: d.left, top: d.top, right: d.right, bottom: d.bottom } ];
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

	public static function setClipboardText( text : String ) : Bool {
		if( text == null )
			return false;
		return @:privateAccess _setClipboardText( text.toUtf8() );
	}

	public static function getClipboardText() : String {
		var t = _getClipboardText();
		if( t == null )
			return null;
		else
			return @:privateAccess String.fromUTF8(t);
	}

	@:hlNative("?sdl", "get_screen_width")
	static function get_screen_width() : Int {
		return 0;
	}

	@:hlNative("?sdl", "get_screen_height")
	static function get_screen_height() : Int {
		return 0;
	}

	@:hlNative("?sdl", "get_screen_width_of_window")
	static function get_screen_width_of_window(win: sdl.Window.WinPtr) : Int {
		return 0;
	}

	@:hlNative("?sdl", "get_screen_height_of_window")
	static function get_screen_height_of_window(win: sdl.Window.WinPtr) : Int {
		return 0;
	}

	static function messageBox( title : hl.Bytes, text : hl.Bytes, error : Bool ) {
	}

	static function detectWin32() {
		return false;
	}

	@:hlNative("?sdl", "get_display_modes")
	static function get_display_modes(displayId : Int) : hl.NativeArray<Dynamic> {
		return null;
	}

	@:hlNative("?sdl", "get_current_display_mode")
	static function get_current_display_mode(displayId : Int, registry : Bool) : Dynamic {
		return null;
	}

	@:hlNative("?sdl", "get_displays")
	static function get_displays() : hl.NativeArray<Dynamic> {
		return null;
	}

	static function get_devices() : hl.NativeArray<hl.Bytes> {
		return null;
	}

	public static function getRelativeMouseMode() : Bool {
		return false;
	}

	public static function warpMouseGlobal( x : Int, y : Int ) : Int {
		return 0;
	}

	@:hlNative("?sdl", "get_global_mouse_state")
	public static function getGlobalMouseState( x : hl.Ref<Int>, y : hl.Ref<Int> ) : Int {
		return 0;
	}

	static function detect_keyboard_layout() : hl.Bytes {
		return null;
	}

	public static function detectKeyboardLayout(){
		return @:privateAccess String.fromUTF8(detect_keyboard_layout());
	}

	@:hlNative("?sdl", "set_clipboard_text")
	private static function _setClipboardText( text : hl.Bytes ) : Bool {
		return false;
	}

	@:hlNative("?sdl", "get_clipboard_text")
	private static function _getClipboardText() : hl.Bytes {
		return null;
	}

	@:hlNative("?sdl", "set_drag_and_drop_enabled")
	public static function setDragAndDropEnabled( v : Bool ): Void {
	}

	@:hlNative("?sdl", "get_drag_and_drop_enabled")
	public static function getDragAndDropEnabled(): Bool {
		return false;
	}
}

enum abstract SDLHint(String) from String to String {

	var SDL_HINT_FRAMEBUFFER_ACCELERATION =                 "SDL_FRAMEBUFFER_ACCELERATION";
	var SDL_HINT_RENDER_DRIVER =                            "SDL_RENDER_DRIVER";
	var SDL_HINT_RENDER_OPENGL_SHADERS =                    "SDL_RENDER_OPENGL_SHADERS";
	var SDL_HINT_RENDER_DIRECT3D_THREADSAFE =               "SDL_RENDER_DIRECT3D_THREADSAFE";
	var SDL_HINT_RENDER_DIRECT3D11_DEBUG =                  "SDL_RENDER_DIRECT3D11_DEBUG";
	var SDL_HINT_RENDER_SCALE_QUALITY =                     "SDL_RENDER_SCALE_QUALITY";
	var SDL_HINT_RENDER_VSYNC =                             "SDL_RENDER_VSYNC";
	var SDL_HINT_VIDEO_ALLOW_SCREENSAVER =                  "SDL_VIDEO_ALLOW_SCREENSAVER";
	var SDL_HINT_VIDEO_X11_XVIDMODE =                       "SDL_VIDEO_X11_XVIDMODE";
	var SDL_HINT_VIDEO_X11_XINERAMA =                       "SDL_VIDEO_X11_XINERAMA";
	var SDL_HINT_VIDEO_X11_XRANDR =                         "SDL_VIDEO_X11_XRANDR";
	var SDL_HINT_VIDEO_X11_NET_WM_PING =                    "SDL_VIDEO_X11_NET_WM_PING";
	var SDL_HINT_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN =  "SDL_WINDOW_FRAME_USABLE_WHILE_CURSOR_HIDDEN";
	var SDL_HINT_WINDOWS_ENABLE_MESSAGELOOP =               "SDL_WINDOWS_ENABLE_MESSAGELOOP";
	var SDL_HINT_GRAB_KEYBOARD =                            "SDL_GRAB_KEYBOARD";
	var SDL_HINT_MOUSE_RELATIVE_MODE_WARP =                 "SDL_MOUSE_RELATIVE_MODE_WARP";
	var SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS =             "SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS";
	var SDL_HINT_IDLE_TIMER_DISABLED =                      "SDL_IOS_IDLE_TIMER_DISABLED";
	var SDL_HINT_ORIENTATIONS =                             "SDL_IOS_ORIENTATIONS";
	var SDL_HINT_ACCELEROMETER_AS_JOYSTICK =                "SDL_ACCELEROMETER_AS_JOYSTICK";
	var SDL_HINT_XINPUT_ENABLED =                           "SDL_XINPUT_ENABLED";
	var SDL_HINT_XINPUT_USE_OLD_JOYSTICK_MAPPING =          "SDL_XINPUT_USE_OLD_JOYSTICK_MAPPING";
	var SDL_HINT_GAMECONTROLLERCONFIG =                     "SDL_GAMECONTROLLERCONFIG";
	var SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS =         "SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS";
	var SDL_HINT_ALLOW_TOPMOST =                            "SDL_ALLOW_TOPMOST";
	var SDL_HINT_TIMER_RESOLUTION =                         "SDL_TIMER_RESOLUTION";
	var SDL_HINT_THREAD_STACK_SIZE =                        "SDL_THREAD_STACK_SIZE";
	var SDL_HINT_VIDEO_HIGHDPI_DISABLED =                   "SDL_VIDEO_HIGHDPI_DISABLED";
	var SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK =       "SDL_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK";
	var SDL_HINT_VIDEO_WIN_D3DCOMPILER =                    "SDL_VIDEO_WIN_D3DCOMPILER";
	var SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT =          "SDL_VIDEO_WINDOW_SHARE_PIXEL_FORMAT";
	var SDL_HINT_WINRT_PRIVACY_POLICY_URL =                 "SDL_WINRT_PRIVACY_POLICY_URL";
	var SDL_HINT_WINRT_PRIVACY_POLICY_LABEL =               "SDL_WINRT_PRIVACY_POLICY_LABEL";
	var SDL_HINT_WINRT_HANDLE_BACK_BUTTON =                 "SDL_WINRT_HANDLE_BACK_BUTTON";
	var SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES =              "SDL_VIDEO_MAC_FULLSCREEN_SPACES";
	var SDL_HINT_MAC_BACKGROUND_APP =                       "SDL_MAC_BACKGROUND_APP";
	var SDL_HINT_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION =  "SDL_ANDROID_APK_EXPANSION_MAIN_FILE_VERSION";
	var SDL_HINT_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION = "SDL_ANDROID_APK_EXPANSION_PATCH_FILE_VERSION";
	var SDL_HINT_IME_INTERNAL_EDITING =                     "SDL_IME_INTERNAL_EDITING";
	var SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH =         "SDL_ANDROID_SEPARATE_MOUSE_AND_TOUCH";
	var SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT =              "SDL_EMSCRIPTEN_KEYBOARD_ELEMENT";
	var SDL_HINT_NO_SIGNAL_HANDLERS =                       "SDL_NO_SIGNAL_HANDLERS";
	var SDL_HINT_WINDOWS_NO_CLOSE_ON_ALT_F4 =               "SDL_WINDOWS_NO_CLOSE_ON_ALT_F4";
}

enum abstract SDLKeyCode(Int) from Int to Int {

	var SDLK_UNKNOWN = 0;

	var SDLK_RETURN = 13;
	var SDLK_ESCAPE = 27;
	var SDLK_BACKSPACE = 8;
	var SDLK_TAB = 9;
	var SDLK_SPACE = 32;
	var SDLK_EXCLAIM = 33;
	var SDLK_QUOTEDBL = 34;
	var SDLK_HASH = 35;
	var SDLK_PERCENT = 37;
	var SDLK_DOLLAR = 36;
	var SDLK_AMPERSAND = 38;
	var SDLK_QUOTE = 39;
	var SDLK_LEFTPAREN = 40;
	var SDLK_RIGHTPAREN = 41;
	var SDLK_ASTERISK = 42;
	var SDLK_PLUS = 43;
	var SDLK_COMMA = 44;
	var SDLK_MINUS = 45;
	var SDLK_PERIOD = 46;
	var SDLK_SLASH = 47;
	var SDLK_0 = 48;
	var SDLK_1 = 49;
	var SDLK_2 = 50;
	var SDLK_3 = 51;
	var SDLK_4 = 52;
	var SDLK_5 = 53;
	var SDLK_6 = 54;
	var SDLK_7 = 55;
	var SDLK_8 = 56;
	var SDLK_9 = 57;
	var SDLK_COLON = 58;
	var SDLK_SEMICOLON = 59;
	var SDLK_LESS = 60;
	var SDLK_EQUALS = 61;
	var SDLK_GREATER = 62;
	var SDLK_QUESTION = 63;
	var SDLK_AT = 64;

  /* Skip uppercase letters */

	var SDLK_LEFTBRACKET = 91;
	var SDLK_BACKSLASH = 92;
	var SDLK_RIGHTBRACKET = 93;
	var SDLK_CARET = 94;
	var SDLK_UNDERSCORE = 95;
	var SDLK_BACKQUOTE = 96;
	var SDLK_a = 97;
	var SDLK_b = 98;
	var SDLK_c = 99;
	var SDLK_d = 100;
	var SDLK_e = 101;
	var SDLK_f = 102;
	var SDLK_g = 103;
	var SDLK_h = 104;
	var SDLK_i = 105;
	var SDLK_j = 106;
	var SDLK_k = 107;
	var SDLK_l = 108;
	var SDLK_m = 109;
	var SDLK_n = 110;
	var SDLK_o = 111;
	var SDLK_p = 112;
	var SDLK_q = 113;
	var SDLK_r = 114;
	var SDLK_s = 115;
	var SDLK_t = 116;
	var SDLK_u = 117;
	var SDLK_v = 118;
	var SDLK_w = 119;
	var SDLK_x = 120;
	var SDLK_y = 121;
	var SDLK_z = 122;

	var SDLK_CAPSLOCK = 1073741881;

	var SDLK_F1 = 1073741882;
	var SDLK_F2 = 1073741883;
	var SDLK_F3 = 1073741884;
	var SDLK_F4 = 1073741885;
	var SDLK_F5 = 1073741886;
	var SDLK_F6 = 1073741887;
	var SDLK_F7 = 1073741888;
	var SDLK_F8 = 1073741889;
	var SDLK_F9 = 1073741890;
	var SDLK_F10 = 1073741891;
	var SDLK_F11 = 1073741892;
	var SDLK_F12 = 1073741893;

	var SDLK_PRINTSCREEN = 1073741894;
	var SDLK_SCROLLLOCK = 1073741895;
	var SDLK_PAUSE = 1073741896;
	var SDLK_INSERT = 1073741897;
	var SDLK_HOME = 1073741898;
	var SDLK_PAGEUP = 1073741899;
	var SDLK_DELETE = 127;
	var SDLK_END = 1073741901;
	var SDLK_PAGEDOWN = 1073741902;
	var SDLK_RIGHT = 1073741903;
	var SDLK_LEFT = 1073741904;
	var SDLK_DOWN = 1073741905;
	var SDLK_UP = 1073741906;

	var SDLK_NUMLOCKCLEAR = 1073741907;
	var SDLK_KP_DIVIDE = 1073741908;
	var SDLK_KP_MULTIPLY = 1073741909;
	var SDLK_KP_MINUS = 1073741910;
	var SDLK_KP_PLUS = 1073741911;
	var SDLK_KP_ENTER = 1073741912;
	var SDLK_KP_1 = 1073741913;
	var SDLK_KP_2 = 1073741914;
	var SDLK_KP_3 = 1073741915;
	var SDLK_KP_4 = 1073741916;
	var SDLK_KP_5 = 1073741917;
	var SDLK_KP_6 = 1073741918;
	var SDLK_KP_7 = 1073741919;
	var SDLK_KP_8 = 1073741920;
	var SDLK_KP_9 = 1073741921;
	var SDLK_KP_0 = 1073741922;
	var SDLK_KP_PERIOD = 1073741923;

	var SDLK_APPLICATION = 1073741925;
	var SDLK_POWER = 1073741926;
	var SDLK_KP_EQUALS = 1073741927;
	var SDLK_F13 = 1073741928;
	var SDLK_F14 = 1073741929;
	var SDLK_F15 = 1073741930;
	var SDLK_F16 = 1073741931;
	var SDLK_F17 = 1073741932;
	var SDLK_F18 = 1073741933;
	var SDLK_F19 = 1073741934;
	var SDLK_F20 = 1073741935;
	var SDLK_F21 = 1073741936;
	var SDLK_F22 = 1073741937;
	var SDLK_F23 = 1073741938;
	var SDLK_F24 = 1073741939;
	var SDLK_EXECUTE = 1073741940;
	var SDLK_HELP = 1073741941;
	var SDLK_MENU = 1073741942;
	var SDLK_SELECT = 1073741943;
	var SDLK_STOP = 1073741944;
	var SDLK_AGAIN = 1073741945;
	var SDLK_UNDO = 1073741946;
	var SDLK_CUT = 1073741947;
	var SDLK_COPY = 1073741948;
	var SDLK_PASTE = 1073741949;
	var SDLK_FIND = 1073741950;
	var SDLK_MUTE = 1073741951;
	var SDLK_VOLUMEUP = 1073741952;
	var SDLK_VOLUMEDOWN = 1073741953;
	var SDLK_KP_COMMA = 1073741957;
	var SDLK_KP_EQUALSAS400 = 1073741958;

	var SDLK_ALTERASE = 1073741977;
	var SDLK_SYSREQ = 1073741978;
	var SDLK_CANCEL = 1073741979;
	var SDLK_CLEAR = 1073741980;
	var SDLK_PRIOR = 1073741981;
	var SDLK_RETURN2 = 1073741982;
	var SDLK_SEPARATOR = 1073741983;
	var SDLK_OUT = 1073741984;
	var SDLK_OPER = 1073741985;
	var SDLK_CLEARAGAIN = 1073741986;
	var SDLK_CRSEL = 1073741987;
	var SDLK_EXSEL = 1073741988;

	var SDLK_KP_00 = 1073742000;
	var SDLK_KP_000 = 1073742001;
	var SDLK_THOUSANDSSEPARATOR = 1073742002;
	var SDLK_DECIMALSEPARATOR = 1073742003;
	var SDLK_CURRENCYUNIT = 1073742004;
	var SDLK_CURRENCYSUBUNIT = 1073742005;
	var SDLK_KP_LEFTPAREN = 1073742006;
	var SDLK_KP_RIGHTPAREN = 1073742007;
	var SDLK_KP_LEFTBRACE = 1073742008;
	var SDLK_KP_RIGHTBRACE = 1073742009;
	var SDLK_KP_TAB = 1073742010;
	var SDLK_KP_BACKSPACE = 1073742011;
	var SDLK_KP_A = 1073742012;
	var SDLK_KP_B = 1073742013;
	var SDLK_KP_C = 1073742014;
	var SDLK_KP_D = 1073742015;
	var SDLK_KP_E = 1073742016;
	var SDLK_KP_F = 1073742017;
	var SDLK_KP_XOR = 1073742018;
	var SDLK_KP_POWER = 1073742019;
	var SDLK_KP_PERCENT = 1073742020;
	var SDLK_KP_LESS = 1073742021;
	var SDLK_KP_GREATER = 1073742022;
	var SDLK_KP_AMPERSAND = 1073742023;
	var SDLK_KP_DBLAMPERSAND = 1073742024;
	var SDLK_KP_VERTICALBAR = 1073742025;
	var SDLK_KP_DBLVERTICALBAR = 1073742026;
	var SDLK_KP_COLON = 1073742027;
	var SDLK_KP_HASH = 1073742028;
	var SDLK_KP_SPACE = 1073742029;
	var SDLK_KP_AT = 1073742030;
	var SDLK_KP_EXCLAM = 1073742031;
	var SDLK_KP_MEMSTORE = 1073742032;
	var SDLK_KP_MEMRECALL = 1073742033;
	var SDLK_KP_MEMCLEAR = 1073742034;
	var SDLK_KP_MEMADD = 1073742035;
	var SDLK_KP_MEMSUBTRACT = 1073742036;
	var SDLK_KP_MEMMULTIPLY = 1073742037;
	var SDLK_KP_MEMDIVIDE = 1073742038;
	var SDLK_KP_PLUSMINUS = 1073742039;
	var SDLK_KP_CLEAR = 1073742040;
	var SDLK_KP_CLEARENTRY = 1073742041;
	var SDLK_KP_BINARY = 1073742042;
	var SDLK_KP_OCTAL = 1073742043;
	var SDLK_KP_DECIMAL = 1073742044;
	var SDLK_KP_HEXADECIMAL = 1073742045;

	var SDLK_LCTRL = 1073742048;
	var SDLK_LSHIFT = 1073742049;
	var SDLK_LALT = 1073742050;
	var SDLK_LGUI = 1073742051;
	var SDLK_RCTRL = 1073742052;
	var SDLK_RSHIFT = 1073742053;
	var SDLK_RALT = 1073742054;
	var SDLK_RGUI = 1073742055;

	var SDLK_MODE = 1073742081;

	var SDLK_AUDIONEXT = 1073742082;
	var SDLK_AUDIOPREV = 1073742083;
	var SDLK_AUDIOSTOP = 1073742084;
	var SDLK_AUDIOPLAY = 1073742085;
	var SDLK_AUDIOMUTE = 1073742086;
	var SDLK_MEDIASELECT = 1073742087;
	var SDLK_WWW = 1073742088;
	var SDLK_MAIL = 1073742089;
	var SDLK_CALCULATOR = 1073742090;
	var SDLK_COMPUTER = 1073742091;
	var SDLK_AC_SEARCH = 1073742092;
	var SDLK_AC_HOME = 1073742093;
	var SDLK_AC_BACK = 1073742094;
	var SDLK_AC_FORWARD = 1073742095;
	var SDLK_AC_STOP = 1073742096;
	var SDLK_AC_REFRESH = 1073742097;
	var SDLK_AC_BOOKMARKS = 1073742098;

	var SDLK_BRIGHTNESSDOWN = 1073742099;
	var SDLK_BRIGHTNESSUP = 1073742100;
	var SDLK_DISPLAYSWITCH = 1073742101;
	var SDLK_KBDILLUMTOGGLE = 1073742102;
	var SDLK_KBDILLUMDOWN = 1073742103;
	var SDLK_KBDILLUMUP = 1073742104;
	var SDLK_EJECT = 1073742105;
	var SDLK_SLEEP = 1073742106;
	var SDLK_APP1 = 1073742107;
	var SDLK_APP2 = 1073742108;

	var SDLK_AUDIOREWIND = 1073742109;
	var SDLK_AUDIOFASTFORWARD = 1073742110;
}

enum abstract SDLKeymod(Int) from Int to Int {

	var KMOD_NONE = 0;
	var KMOD_LSHIFT = 1;
	var KMOD_RSHIFT = 2;
	var KMOD_LCTRL = 64;
	var KMOD_RCTRL = 128;
	var KMOD_LALT = 256;
	var KMOD_RALT = 512;
	var KMOD_LGUI = 1024;
	var KMOD_RGUI = 2048;
	var KMOD_NUM = 4096;
	var KMOD_CAPS = 8192;
	var KMOD_MODE = 16384;
	var KMOD_SCROLL = 32768;

	var KMOD_CTRL = 192;
	var KMOD_SHIFT = 3;
	var KMOD_ALT = 768;
	var KMOD_GUI = 3072;

	var KMOD_RESERVED = 32768;
}

enum abstract SDLScanCode(Int) from Int to Int {
  var SDL_SCANCODE_UNKNOWN = 0;

  /**
   *  \name Usage page 0x07
   *
   *  These values are from usage page 0x07 (USB keyboard page).
   */
  /* @{ */

  var SDL_SCANCODE_A = 4;
  var SDL_SCANCODE_B = 5;
  var SDL_SCANCODE_C = 6;
  var SDL_SCANCODE_D = 7;
  var SDL_SCANCODE_E = 8;
  var SDL_SCANCODE_F = 9;
  var SDL_SCANCODE_G = 10;
  var SDL_SCANCODE_H = 11;
  var SDL_SCANCODE_I = 12;
  var SDL_SCANCODE_J = 13;
  var SDL_SCANCODE_K = 14;
  var SDL_SCANCODE_L = 15;
  var SDL_SCANCODE_M = 16;
  var SDL_SCANCODE_N = 17;
  var SDL_SCANCODE_O = 18;
  var SDL_SCANCODE_P = 19;
  var SDL_SCANCODE_Q = 20;
  var SDL_SCANCODE_R = 21;
  var SDL_SCANCODE_S = 22;
  var SDL_SCANCODE_T = 23;
  var SDL_SCANCODE_U = 24;
  var SDL_SCANCODE_V = 25;
  var SDL_SCANCODE_W = 26;
  var SDL_SCANCODE_X = 27;
  var SDL_SCANCODE_Y = 28;
  var SDL_SCANCODE_Z = 29;

  var SDL_SCANCODE_1 = 30;
  var SDL_SCANCODE_2 = 31;
  var SDL_SCANCODE_3 = 32;
  var SDL_SCANCODE_4 = 33;
  var SDL_SCANCODE_5 = 34;
  var SDL_SCANCODE_6 = 35;
  var SDL_SCANCODE_7 = 36;
  var SDL_SCANCODE_8 = 37;
  var SDL_SCANCODE_9 = 38;
  var SDL_SCANCODE_0 = 39;

  var SDL_SCANCODE_RETURN = 40;
  var SDL_SCANCODE_ESCAPE = 41;
  var SDL_SCANCODE_BACKSPACE = 42;
  var SDL_SCANCODE_TAB = 43;
  var SDL_SCANCODE_SPACE = 44;

  var SDL_SCANCODE_MINUS = 45;
  var SDL_SCANCODE_EQUALS = 46;
  var SDL_SCANCODE_LEFTBRACKET = 47;
  var SDL_SCANCODE_RIGHTBRACKET = 48;
  var SDL_SCANCODE_BACKSLASH = 49; /**< Located at the lower left of the return
                                  *   key on ISO keyboards and at the right end
                                  *   of the QWERTY row on ANSI keyboards.
                                  *   Produces REVERSE SOLIDUS (backslash) and
                                  *   VERTICAL LINE in a US layout, REVERSE
                                  *   SOLIDUS and VERTICAL LINE in a UK Mac
                                  *   layout, NUMBER SIGN and TILDE in a UK
                                  *   Windows layout, DOLLAR SIGN and POUND SIGN
                                  *   in a Swiss German layout, NUMBER SIGN and
                                  *   APOSTROPHE in a German layout, GRAVE
                                  *   ACCENT and POUND SIGN in a French Mac
                                  *   layout, and ASTERISK and MICRO SIGN in a
                                  *   French Windows layout.
                                  */
  var SDL_SCANCODE_NONUSHASH = 50; /**< ISO USB keyboards actually use this code
                                  *   instead of 49 for the same key, but all
                                  *   OSes I've seen treat the two codes
                                  *   identically. So, as an implementor, unless
                                  *   your keyboard generates both of those
                                  *   codes and your OS treats them differently,
                                  *   you should generate SDL_SCANCODE_BACKSLASH
                                  *   instead of this code. As a user, you
                                  *   should not rely on this code because SDL
                                  *   will never generate it with most (all?)
                                  *   keyboards.
                                  */
  var SDL_SCANCODE_SEMICOLON = 51;
  var SDL_SCANCODE_APOSTROPHE = 52;
  var SDL_SCANCODE_GRAVE = 53; /**< Located in the top left corner (on both ANSI
                              *   and ISO keyboards). Produces GRAVE ACCENT and
                              *   TILDE in a US Windows layout and in US and UK
                              *   Mac layouts on ANSI keyboards, GRAVE ACCENT
                              *   and NOT SIGN in a UK Windows layout, SECTION
                              *   SIGN and PLUS-MINUS SIGN in US and UK Mac
                              *   layouts on ISO keyboards, SECTION SIGN and
                              *   DEGREE SIGN in a Swiss German layout (Mac:
                              *   only on ISO keyboards), CIRCUMFLEX ACCENT and
                              *   DEGREE SIGN in a German layout (Mac: only on
                              *   ISO keyboards), SUPERSCRIPT TWO and TILDE in a
                              *   French Windows layout, COMMERCIAL AT and
                              *   NUMBER SIGN in a French Mac layout on ISO
                              *   keyboards, and LESS-THAN SIGN and GREATER-THAN
                              *   SIGN in a Swiss German, German, or French Mac
                              *   layout on ANSI keyboards.
                              */
  var SDL_SCANCODE_COMMA = 54;
  var SDL_SCANCODE_PERIOD = 55;
  var SDL_SCANCODE_SLASH = 56;

  var SDL_SCANCODE_CAPSLOCK = 57;

  var SDL_SCANCODE_F1 = 58;
  var SDL_SCANCODE_F2 = 59;
  var SDL_SCANCODE_F3 = 60;
  var SDL_SCANCODE_F4 = 61;
  var SDL_SCANCODE_F5 = 62;
  var SDL_SCANCODE_F6 = 63;
  var SDL_SCANCODE_F7 = 64;
  var SDL_SCANCODE_F8 = 65;
  var SDL_SCANCODE_F9 = 66;
  var SDL_SCANCODE_F10 = 67;
  var SDL_SCANCODE_F11 = 68;
  var SDL_SCANCODE_F12 = 69;

  var SDL_SCANCODE_PRINTSCREEN = 70;
  var SDL_SCANCODE_SCROLLLOCK = 71;
  var SDL_SCANCODE_PAUSE = 72;
  var SDL_SCANCODE_INSERT = 73; /**< insert on PC, help on some Mac keyboards (but
                                  does send code 73, not 117) */
  var SDL_SCANCODE_HOME = 74;
  var SDL_SCANCODE_PAGEUP = 75;
  var SDL_SCANCODE_DELETE = 76;
  var SDL_SCANCODE_END = 77;
  var SDL_SCANCODE_PAGEDOWN = 78;
  var SDL_SCANCODE_RIGHT = 79;
  var SDL_SCANCODE_LEFT = 80;
  var SDL_SCANCODE_DOWN = 81;
  var SDL_SCANCODE_UP = 82;

  var SDL_SCANCODE_NUMLOCKCLEAR = 83; /**< num lock on PC, clear on Mac keyboards
                                    */
  var SDL_SCANCODE_KP_DIVIDE = 84;
  var SDL_SCANCODE_KP_MULTIPLY = 85;
  var SDL_SCANCODE_KP_MINUS = 86;
  var SDL_SCANCODE_KP_PLUS = 87;
  var SDL_SCANCODE_KP_ENTER = 88;
  var SDL_SCANCODE_KP_1 = 89;
  var SDL_SCANCODE_KP_2 = 90;
  var SDL_SCANCODE_KP_3 = 91;
  var SDL_SCANCODE_KP_4 = 92;
  var SDL_SCANCODE_KP_5 = 93;
  var SDL_SCANCODE_KP_6 = 94;
  var SDL_SCANCODE_KP_7 = 95;
  var SDL_SCANCODE_KP_8 = 96;
  var SDL_SCANCODE_KP_9 = 97;
  var SDL_SCANCODE_KP_0 = 98;
  var SDL_SCANCODE_KP_PERIOD = 99;

  var SDL_SCANCODE_NONUSBACKSLASH = 100; /**< This is the additional key that ISO
                                        *   keyboards have over ANSI ones,
                                        *   located between left shift and Y.
                                        *   Produces GRAVE ACCENT and TILDE in a
                                        *   US or UK Mac layout, REVERSE SOLIDUS
                                        *   (backslash) and VERTICAL LINE in a
                                        *   US or UK Windows layout, and
                                        *   LESS-THAN SIGN and GREATER-THAN SIGN
                                        *   in a Swiss German, German, or French
                                        *   layout. */
  var SDL_SCANCODE_APPLICATION = 101; /**< windows contextual menu, compose */
  var SDL_SCANCODE_POWER = 102; /**< The USB document says this is a status flag,
                              *   not a physical key - but some Mac keyboards
                              *   do have a power key. */
  var SDL_SCANCODE_KP_EQUALS = 103;
  var SDL_SCANCODE_F13 = 104;
  var SDL_SCANCODE_F14 = 105;
  var SDL_SCANCODE_F15 = 106;
  var SDL_SCANCODE_F16 = 107;
  var SDL_SCANCODE_F17 = 108;
  var SDL_SCANCODE_F18 = 109;
  var SDL_SCANCODE_F19 = 110;
  var SDL_SCANCODE_F20 = 111;
  var SDL_SCANCODE_F21 = 112;
  var SDL_SCANCODE_F22 = 113;
  var SDL_SCANCODE_F23 = 114;
  var SDL_SCANCODE_F24 = 115;
  var SDL_SCANCODE_EXECUTE = 116;
  var SDL_SCANCODE_HELP = 117;
  var SDL_SCANCODE_MENU = 118;
  var SDL_SCANCODE_SELECT = 119;
  var SDL_SCANCODE_STOP = 120;
  var SDL_SCANCODE_AGAIN = 121;   /**< redo */
  var SDL_SCANCODE_UNDO = 122;
  var SDL_SCANCODE_CUT = 123;
  var SDL_SCANCODE_COPY = 124;
  var SDL_SCANCODE_PASTE = 125;
  var SDL_SCANCODE_FIND = 126;
  var SDL_SCANCODE_MUTE = 127;
  var SDL_SCANCODE_VOLUMEUP = 128;
  var SDL_SCANCODE_VOLUMEDOWN = 129;
  /* not sure whether there's a reason to enable these */
  /*var  SDL_SCANCODE_LOCKINGCAPSLOCK = 130;  */
  /*var  SDL_SCANCODE_LOCKINGNUMLOCK = 131; */
  /*var  SDL_SCANCODE_LOCKINGSCROLLLOCK = 132; */
  var SDL_SCANCODE_KP_COMMA = 133;
  var SDL_SCANCODE_KP_EQUALSAS400 = 134;

  var SDL_SCANCODE_INTERNATIONAL1 = 135; /**< used on Asian keyboards, see
                                            footnotes in USB doc */
  var SDL_SCANCODE_INTERNATIONAL2 = 136;
  var SDL_SCANCODE_INTERNATIONAL3 = 137; /**< Yen */
  var SDL_SCANCODE_INTERNATIONAL4 = 138;
  var SDL_SCANCODE_INTERNATIONAL5 = 139;
  var SDL_SCANCODE_INTERNATIONAL6 = 140;
  var SDL_SCANCODE_INTERNATIONAL7 = 141;
  var SDL_SCANCODE_INTERNATIONAL8 = 142;
  var SDL_SCANCODE_INTERNATIONAL9 = 143;
  var SDL_SCANCODE_LANG1 = 144; /**< Hangul/English toggle */
  var SDL_SCANCODE_LANG2 = 145; /**< Hanja conversion */
  var SDL_SCANCODE_LANG3 = 146; /**< Katakana */
  var SDL_SCANCODE_LANG4 = 147; /**< Hiragana */
  var SDL_SCANCODE_LANG5 = 148; /**< Zenkaku/Hankaku */
  var SDL_SCANCODE_LANG6 = 149; /**< reserved */
  var SDL_SCANCODE_LANG7 = 150; /**< reserved */
  var SDL_SCANCODE_LANG8 = 151; /**< reserved */
  var SDL_SCANCODE_LANG9 = 152; /**< reserved */

  var SDL_SCANCODE_ALTERASE = 153; /**< Erase-Eaze */
  var SDL_SCANCODE_SYSREQ = 154;
  var SDL_SCANCODE_CANCEL = 155;
  var SDL_SCANCODE_CLEAR = 156;
  var SDL_SCANCODE_PRIOR = 157;
  var SDL_SCANCODE_RETURN2 = 158;
  var SDL_SCANCODE_SEPARATOR = 159;
  var SDL_SCANCODE_OUT = 160;
  var SDL_SCANCODE_OPER = 161;
  var SDL_SCANCODE_CLEARAGAIN = 162;
  var SDL_SCANCODE_CRSEL = 163;
  var SDL_SCANCODE_EXSEL = 164;

  var SDL_SCANCODE_KP_00 = 176;
  var SDL_SCANCODE_KP_000 = 177;
  var SDL_SCANCODE_THOUSANDSSEPARATOR = 178;
  var SDL_SCANCODE_DECIMALSEPARATOR = 179;
  var SDL_SCANCODE_CURRENCYUNIT = 180;
  var SDL_SCANCODE_CURRENCYSUBUNIT = 181;
  var SDL_SCANCODE_KP_LEFTPAREN = 182;
  var SDL_SCANCODE_KP_RIGHTPAREN = 183;
  var SDL_SCANCODE_KP_LEFTBRACE = 184;
  var SDL_SCANCODE_KP_RIGHTBRACE = 185;
  var SDL_SCANCODE_KP_TAB = 186;
  var SDL_SCANCODE_KP_BACKSPACE = 187;
  var SDL_SCANCODE_KP_A = 188;
  var SDL_SCANCODE_KP_B = 189;
  var SDL_SCANCODE_KP_C = 190;
  var SDL_SCANCODE_KP_D = 191;
  var SDL_SCANCODE_KP_E = 192;
  var SDL_SCANCODE_KP_F = 193;
  var SDL_SCANCODE_KP_XOR = 194;
  var SDL_SCANCODE_KP_POWER = 195;
  var SDL_SCANCODE_KP_PERCENT = 196;
  var SDL_SCANCODE_KP_LESS = 197;
  var SDL_SCANCODE_KP_GREATER = 198;
  var SDL_SCANCODE_KP_AMPERSAND = 199;
  var SDL_SCANCODE_KP_DBLAMPERSAND = 200;
  var SDL_SCANCODE_KP_VERTICALBAR = 201;
  var SDL_SCANCODE_KP_DBLVERTICALBAR = 202;
  var SDL_SCANCODE_KP_COLON = 203;
  var SDL_SCANCODE_KP_HASH = 204;
  var SDL_SCANCODE_KP_SPACE = 205;
  var SDL_SCANCODE_KP_AT = 206;
  var SDL_SCANCODE_KP_EXCLAM = 207;
  var SDL_SCANCODE_KP_MEMSTORE = 208;
  var SDL_SCANCODE_KP_MEMRECALL = 209;
  var SDL_SCANCODE_KP_MEMCLEAR = 210;
  var SDL_SCANCODE_KP_MEMADD = 211;
  var SDL_SCANCODE_KP_MEMSUBTRACT = 212;
  var SDL_SCANCODE_KP_MEMMULTIPLY = 213;
  var SDL_SCANCODE_KP_MEMDIVIDE = 214;
  var SDL_SCANCODE_KP_PLUSMINUS = 215;
  var SDL_SCANCODE_KP_CLEAR = 216;
  var SDL_SCANCODE_KP_CLEARENTRY = 217;
  var SDL_SCANCODE_KP_BINARY = 218;
  var SDL_SCANCODE_KP_OCTAL = 219;
  var SDL_SCANCODE_KP_DECIMAL = 220;
  var SDL_SCANCODE_KP_HEXADECIMAL = 221;

  var SDL_SCANCODE_LCTRL = 224;
  var SDL_SCANCODE_LSHIFT = 225;
  var SDL_SCANCODE_LALT = 226; /**< alt, option */
  var SDL_SCANCODE_LGUI = 227; /**< windows, command (apple), meta */
  var SDL_SCANCODE_RCTRL = 228;
  var SDL_SCANCODE_RSHIFT = 229;
  var SDL_SCANCODE_RALT = 230; /**< alt gr, option */
  var SDL_SCANCODE_RGUI = 231; /**< windows, command (apple), meta */

  var SDL_SCANCODE_MODE = 257;    /**< I'm not sure if this is really not covered
                                *   by any of the above, but since there's a
                                *   special KMOD_MODE for it I'm adding it here
                                */

    /* @} *//* Usage page 0x07 */

    /**
    *  \name Usage page 0x0C
    *
    *  These values are mapped from usage page 0x0C (USB consumer page).
    */
    /* @{ */

  var SDL_SCANCODE_AUDIONEXT = 258;
  var SDL_SCANCODE_AUDIOPREV = 259;
  var SDL_SCANCODE_AUDIOSTOP = 260;
  var SDL_SCANCODE_AUDIOPLAY = 261;
  var SDL_SCANCODE_AUDIOMUTE = 262;
  var SDL_SCANCODE_MEDIASELECT = 263;
  var SDL_SCANCODE_WWW = 264;
  var SDL_SCANCODE_MAIL = 265;
  var SDL_SCANCODE_CALCULATOR = 266;
  var SDL_SCANCODE_COMPUTER = 267;
  var SDL_SCANCODE_AC_SEARCH = 268;
  var SDL_SCANCODE_AC_HOME = 269;
  var SDL_SCANCODE_AC_BACK = 270;
  var SDL_SCANCODE_AC_FORWARD = 271;
  var SDL_SCANCODE_AC_STOP = 272;
  var SDL_SCANCODE_AC_REFRESH = 273;
  var SDL_SCANCODE_AC_BOOKMARKS = 274;

    /* @} *//* Usage page 0x0C */

    /**
    *  \name Walther keys
    *
    *  These are values that Christian Walther added (for mac keyboard?).
    */
    /* @{ */

  var SDL_SCANCODE_BRIGHTNESSDOWN = 275;
  var SDL_SCANCODE_BRIGHTNESSUP = 276;
  var SDL_SCANCODE_DISPLAYSWITCH = 277; /**< display mirroring/dual display
                                          switch, video mode switch */
  var SDL_SCANCODE_KBDILLUMTOGGLE = 278;
  var SDL_SCANCODE_KBDILLUMDOWN = 279;
  var SDL_SCANCODE_KBDILLUMUP = 280;
  var SDL_SCANCODE_EJECT = 281;
  var SDL_SCANCODE_SLEEP = 282;

  var SDL_SCANCODE_APP1 = 283;
  var SDL_SCANCODE_APP2 = 284;

    /* @} *//* Walther keys */

    /**
    *  \name Usage page 0x0C (additional media keys)
    *
    *  These values are mapped from usage page 0x0C (USB consumer page).
    */
    /* @{ */

  var SDL_SCANCODE_AUDIOREWIND = 285;
  var SDL_SCANCODE_AUDIOFASTFORWARD = 286;

    /* @} *//* Usage page 0x0C (additional media keys) */

    /* Add any other keys here. */

  var SDL_NUM_SCANCODES = 512; /**< not a key; just marks the number of scancodes
                                for array bounds */
}