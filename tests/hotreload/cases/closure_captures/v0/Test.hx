class Test {
	static var live : Void -> String;
	public static function check( step : Int ) {
		if( live == null ) live = make(1, 2);
		Sys.println("v0 step " + step + " live=" + live() + " new=" + make(3, 4)());
	}
	static function make( a : Int, b : Int ) {
		var x = a * 10;
		return function() return "x" + x;
	}
}
