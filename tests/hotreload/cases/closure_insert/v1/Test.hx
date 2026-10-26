class Test {
	static var a : Void -> String;
	static var b : Void -> String;
	static var c : Void -> String;
	static var named : Void -> String;
	static var arg : Void -> String;
	public static function check( step : Int ) {
		if( a == null ) make(1);
		Sys.println("v1 step " + step + " a=" + a() + " b=" + b() + " named=" + named() + " arg=" + arg());
	}
	static function keep( f : Void -> String ) {
		arg = f;
	}
	static function make( x : Int ) {
		// local functions inserted before the others, of the same type and context :
		// the live closures run the new body of their own function, found by its name
		c = function() return "C" + x;
		function dec() return "D" + (x - 1);
		c = dec;
		a = function() return "A2:" + x;
		b = function() return "B2:" + x;
		function inc() return "N2:" + (x + 1);
		named = inc;
		keep(() -> "G2:" + x);
	}
}
