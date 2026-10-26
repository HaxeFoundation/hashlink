class Test {
	static var counter = 0;
	static var name = "kept";
	public static function check( step : Int ) {
		counter++;
		Sys.println("v2 step " + step + " counter=" + counter + " name=" + name);
	}
}
