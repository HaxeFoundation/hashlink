class Test {
	static final GREETING = "bonjour";
	static final TABLE = [1, 2, 3, 4];
	static var counter = 500;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v1 step " + step + " " + Reflect.field(Test, "GREETING") + " table=" + TABLE.length + " counter=" + counter);
	}
}
