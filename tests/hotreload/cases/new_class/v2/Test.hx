class Test {
	public static function check( step : Int ) {
		Other.count++;
		Sys.println("v2 step " + step + " count=" + Other.count);
	}
}

class Other {
	public static var count : Int;
}
