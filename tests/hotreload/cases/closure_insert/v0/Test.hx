class Test {
	static var a : Void -> String;
	static var b : Void -> String;
	static var named : Void -> String;
	static var arg : Void -> String;
	static var nested : Void -> String;
	public static function check( step : Int ) {
		if( a == null ) make(1);
		Sys.println("v0 step " + step + " a=" + a() + " b=" + b() + " named=" + named() + " arg=" + arg() + " nested=" + nested());
	}
	static function keep( f : Void -> String ) {
		arg = f;
	}
	static function make( x : Int ) {
		// a local function in a local function : maker/return
		var maker = function() return function() return "I" + x;
		nested = maker();
		a = function() return "A" + x;
		b = function() return "B" + x;
		function inc() return "N" + (x + 1);
		named = inc;
		keep(() -> "G" + x);
	}
}
