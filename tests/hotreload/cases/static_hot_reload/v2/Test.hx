class Test {
	@:hotReload static var GREETING = "bonjour";
	@:hotReload static final TABLE = [1, 2, 3, 4];
	// without @:hotReload : keeps its first value
	static final LIST = [0, 0];
	static var counter = 500;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v2 step " + step + " " + Reflect.field(Test, "GREETING") + " table=" + TABLE.length + " list=" + LIST.length + " counter=" + counter);
	}
}
