package sdl;

@:hlNative("sdl")
class Sdl {

	static var initDone = false;
	static var isWin32 = false;
	static var sentinel : hl.UI.Sentinel;
	static var dismissErrors = false;

	public static function init() {
		if( initDone ) return;
		initDone = true;
		if( !initOnce() ) throw "Failed to init SDL";
		isWin32 = detectWin32();
	}

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

}