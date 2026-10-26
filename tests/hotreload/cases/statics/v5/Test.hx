class Test {
	static var counter = 0;
	static var added = "retyped";
	static var name = "kept";
	public static function check( step : Int ) {
		counter++;
		Sys.println("v5 step " + step + " counter=" + counter + " name=" + name + " added=" + added);
	}
}
