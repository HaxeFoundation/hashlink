class Test {
	public static function check( step : Int ) {
		try {
			Sys.println("v1 step " + step + " " + compute(step));
		} catch( e : haxe.Exception ) {
			var stack = e.stack.toString().split("\n").filter(l -> l.indexOf("Test.hx") >= 0);
			Sys.println("v1 step " + step + " caught " + e.message + stack.join(" |"));
		}
	}
	static function compute( x : Int ) : Int {
		if( x > 0 )
			throw "error" + x;
		return x * 2;
	}
}
