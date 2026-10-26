class Test {
	static var name = "abc";
	public static function check( step : Int ) {
		// String has a __compare : the JIT calls it, and its prototype is already initialized
		var other = "ab" + "c".toUpperCase().toLowerCase();
		Sys.println("v1 step " + step + " " + (name == other) + " " + (name != "abd") + " " + (name < "abd"));
	}
}
