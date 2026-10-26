class Test {
	public static function check( step : Int ) {
		Other.count += 10;
		Sys.println("v3 step " + step + " count=" + Other.count);
	}
}

class Other {
	public static var count : Int;
}
