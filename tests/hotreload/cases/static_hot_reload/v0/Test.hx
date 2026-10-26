class Test {
	@:hotReload static var GREETING = "hello";
	@:hotReload static final TABLE = [1, 2, 3];
	// without @:hotReload : keeps its first value
	static final LIST = [0];
	static var counter = 100;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v0 step " + step + " " + Reflect.field(Test, "GREETING") + " table=" + TABLE.length + " list=" + LIST.length + " counter=" + counter);
	}
}
