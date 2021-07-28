class Log {
	static final T0 = haxe.Timer.stamp();

	public static function print( msg : String ) {
		Sys.println("["+Std.int((haxe.Timer.stamp() - T0) * 100)+"] "+msg);
	}
}