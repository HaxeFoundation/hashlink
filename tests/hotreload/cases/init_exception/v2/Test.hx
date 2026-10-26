class Test {
	public static function check( step : Int ) {
		Sys.println("v2 step " + step + " " + Bad.value + " " + Good.value);
	}
}

class Bad {
	public static var value : Null<Int> = fail();
	static function fail() : Int throw "init failed";
}

class Good {
	public static var value = 42;
}
