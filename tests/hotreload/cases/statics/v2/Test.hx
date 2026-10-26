class Test {
	static var counter = 0;
	static var added = 40;
	static var list = [1, 2, 3];
	static var name = "kept";
	public static function check( step : Int ) {
		counter++;
		added++;
		Sys.println("v2 step " + step + " counter=" + counter + " name=" + name + " added=" + added + " list=" + list.length + " " + twice(step));
	}
	static function twice( x : Int ) return x * 2;
}
