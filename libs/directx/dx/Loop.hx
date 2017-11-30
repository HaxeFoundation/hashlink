package dx;

class Loop {

	static function eventLoop( e : Event ) {
		for( w in @:privateAccess Window.windows )
			if( w.getNextEvent(e) )
				return true;
		return false;
	}

	static var event = new Event();

	public static function processEvents( onEvent : Event -> Bool ) {
		while( true ) {
			switch( hl.UI.loop(false) ) {
			case NoMessage:
				break;
			case HandledMessage:
				continue;
			case Quit:
				return false;
			}
		}
		while( true ) {
			if( !eventLoop(event) )
				break;
			var ret = onEvent(event);
			if( event.type == Quit && ret )
				return false;
		}
		return true;
	}

}