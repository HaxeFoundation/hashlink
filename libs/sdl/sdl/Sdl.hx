package sdl;

@:hlNative("sdl")
class Sdl {

	static var initDone = false;
	static var isWin32 = false;
	public static function init() {
		if( initDone ) return;
		initDone = true;
		if( !initOnce() ) throw "Failed to init SDL";
		isWin32 = detectWin32();
	}
	
	static function __init__() {
		hl.types.Api.setErrorHandler(function(e) reportError(e));
	}

	static function initOnce() return false;
	static function eventLoop( e : Event ) return false;
	
	public static var defaultEventHandler : Event -> Void;

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
        }
	}
	
	public dynamic static function reportError( e : Dynamic ) {
		var f = new hl.UI.WinLog("Uncaught Exception", 400, 300);
		var stack = haxe.CallStack.toString(haxe.CallStack.exceptionStack());
		var err = try Std.string(e) catch( _ : Dynamic ) "????";
		Sys.println(err + stack);
		f.setTextContent(err+"\n"+stack);
		var but = new hl.UI.Button(f, "Continue");
		but.onClick = function() {
			hl.UI.stopLoop();
		};
		var but = new hl.UI.Button(f, "Exit");
		but.onClick = function() {
			Sys.exit(0);
		};
		while( hl.UI.loop(true) != Quit ) {};
		f.destroy();
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

	static function messageBox( title : hl.types.Bytes, text : hl.types.Bytes, error : Bool ) {
	}

	static function detectWin32() {
		return false;
	}

}