class Test {
	static var live : Void -> String;
	public static function check( step : Int ) {
		if( live == null ) live = make(1, 2);
		Sys.println("v1 step " + step + " live=" + live() + " new=" + make(3, 4)());
	}
	static function make( a : Int, b : Int ) {
		var x = a * 10;
		var name = "y" + b;
		// captures another variable : the live closure keeps its body and its context
		return function() return name + ":" + x;
	}
}
