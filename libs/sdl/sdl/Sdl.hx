package sdl;

@:hlNative("sdl")
class Sdl {

	static var initDone = false;
	static var isWin32 = false;
	static var sentinel : hl.UI.Sentinel;
	static var dismissErrors = false;

	public static var requiredGLMajor(default,null) = 3;
	public static var requiredGLMinor(default,null) = 2;

	public static function init() {
		if( initDone ) return;
		initDone = true;
		if( !initOnce() ) throw "Failed to init SDL";
		isWin32 = detectWin32();
	}

	public static function setGLOptions( major : Int = 3, minor : Int = 2, depth : Int = 24, stencil : Int = 8, flags : Int = 1 ) {
		requiredGLMajor = major;
		requiredGLMinor = minor;
		glOptions(major, minor, depth, stencil, flags);
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
		#if (haxe_ver >= "3.4.1") sentinel.setPause(true); #end
		hl.UI.dialog("OpenGL Error", msg, flags);
		Sys.exit( -1);
	}

	static function glOptions( major : Int, minor : Int, depth : Int, stencil : Int, flags : Int ) {}

	static function onTimeout() {
		throw "Program timeout (infinite loop?)";
	}

	static function __init__() {
		hl.Api.setErrorHandler(function(e) reportError(e));
		sentinel = new hl.UI.Sentinel(30,onTimeout);
	}

	static function initOnce() return false;
	static function eventLoop( e : Event ) return false;

	public static var defaultEventHandler : Event -> Void;

	/**
		Prevent the program from reporting timeout infinite loop.
	**/
	public static function tick() {
		sentinel.tick();
	}

	public static function processEvents() {
		var event = new Event();
		while( true ) {
			if( !eventLoop(event) )
				break;
			if( event.type == Quit )
				return false;
			defaultEventHandler(event);
		}
		return true;
	}

	public static function loop( callb : Void -> Void, ?onEvent : Event -> Void ) {
		var event = new Event();
		if( onEvent == null ) onEvent = defaultEventHandler;
		while( true ) {
			while( true ) {
				if( !eventLoop(event) )
					break;
				if( event.type == Quit )
					return;
				if( onEvent != null ) {
					try {
						onEvent(event);
					} catch( e : Dynamic ) {
						reportError(e);
					}
				}
			}
			try {
				callb();
			} catch( e : Dynamic ) {
				reportError(e);
			}
			tick();
        }
	}

	public dynamic static function reportError( e : Dynamic ) {

		var stack = haxe.CallStack.toString(haxe.CallStack.exceptionStack());
		var err = try Std.string(e) catch( _ : Dynamic ) "????";
		Sys.println(err + stack);

		if( dismissErrors )
			return;

		var wasFS = [];
		for( w in @:privateAccess Window.windows ) {
			switch( w.displayMode ) {
			case Windowed, Borderless:
			default:
				wasFS.push({ w : w, mode : w.displayMode });
				w.displayMode = Windowed;
			}
		}

		var f = new hl.UI.WinLog("Uncaught Exception", 500, 400);
		f.setTextContent(err+"\n"+stack);
		var but = new hl.UI.Button(f, "Continue");
		but.onClick = function() {
			hl.UI.stopLoop();
		};

		var but = new hl.UI.Button(f, "Dismiss all");
		but.onClick = function() {
			dismissErrors = true;
			hl.UI.stopLoop();
		};

		var but = new hl.UI.Button(f, "Exit");
		but.onClick = function() {
			Sys.exit(0);
		};

		while( hl.UI.loop(true) != Quit )
			tick();
		f.destroy();

		for( w in wasFS )
			w.w.displayMode = w.mode;
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

}