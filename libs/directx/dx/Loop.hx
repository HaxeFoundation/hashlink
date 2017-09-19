package dx;

class Loop {

	static var sentinel : hl.UI.Sentinel;
	static var dismissErrors = false;

	public static var defaultEventHandler : Event -> Bool;

	/**
		Prevent the program from reporting timeout infinite loop.
	**/
	public static function tick() {
		sentinel.tick();
	}

	static function eventLoop( e : Event ) {
		for( w in @:privateAccess Window.windows )
			if( w.getNextEvent(e) )
				return true;
		return false;
	}

	public static function processEvents() {
		var event = new Event();
		while( true ) {
			if( !eventLoop(event) )
				break;
			var ret = defaultEventHandler(event);
			if( event.type == Quit && ret )
				return false;
		}
		return true;
	}

	public static function run( callb : Void -> Void, ?onEvent : Event -> Bool ) {
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
				var ret = true;
				if( onEvent != null ) {
					try {
						ret = onEvent(event);
					} catch( e : Dynamic ) {
						reportError(e);
					}
				}
				if( event.type == Quit && ret )
				 	return;
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