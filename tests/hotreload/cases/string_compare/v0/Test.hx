class Test {
	static var name = "abc";
	public static function check( step : Int ) {
		Sys.println("v0 step " + step + " " + (name == "abc"));
	}
}
