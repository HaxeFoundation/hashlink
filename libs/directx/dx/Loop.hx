package dx;

class Loop {

	static var sentinel : hl.UI.Sentinel;
	static var dismissErrors = false;

	public static var defaultEventHandler : Event -> Void;

	/**
		Prevent the program from reporting timeout infinite loop.
	**/
	public static function tick() {
		sentinel.tick();
	}

	//@:hlNative TODO !
	static function eventLoop( e : Event ) return false;

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

	public static function run( callb : Void -> Void, ?onEvent : Event -> Void ) {
		var event = new Event();
		if( onEvent == null ) onEvent = defaultEventHandler;
		while( true ) {
			// process windows message
			while( true ) {
				switch( hl.UI.loop(false) ) {
				case NoMessage:
					break;
				case HandledMessage:
					continue;
				case Quit:
					return;
				}
			}
			// retreive captured events
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
			// callback our events handlers
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
	}

	static function onTimeout() {
		throw "Program timeout (infinite loop?)";
	}

	static function __init__() {
		hl.Api.setErrorHandler(function(e) reportError(e));
		sentinel = new hl.UI.Sentinel(30,onTimeout);
	}

}