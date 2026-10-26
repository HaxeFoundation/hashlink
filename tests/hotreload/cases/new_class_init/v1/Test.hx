class Test {
	static var counter = 0;
	public static function check( step : Int ) {
		counter++;
		Sys.println("v1 step " + step + " counter=" + counter + " " + Fresh.name + " " + Fresh.list.length + " " + Fresh.inits);
	}
}

class Fresh {
	public static var inits = 0;
	public static var name = "fresh" + (1 + 2);
	public static var list = [for( i in 0...4 ) i];
	static function __init__() {
		Sys.println("Fresh.__init__");
		inits++;
	}
}
