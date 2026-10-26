class Test {
	static var added = 5;
	public static var counter = 0;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v1 step " + step + " " + added);
	}
}
