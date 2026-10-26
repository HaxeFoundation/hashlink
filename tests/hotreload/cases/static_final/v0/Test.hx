class Test {
	static final GREETING = "hello";
	static final TABLE = [1, 2, 3];
	static var counter = 100;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v0 step " + step + " " + Reflect.field(Test, "GREETING") + " table=" + TABLE.length + " counter=" + counter);
	}
}
