class Test {
	static var counter = 0;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v0 step " + step + " counter=" + counter);
	}
}
