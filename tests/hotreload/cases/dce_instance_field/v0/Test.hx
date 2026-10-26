class Test {
	public static function check( step : Int ) {
		Sys.println("v0 step " + step + " " + compute(step));
	}
	static function compute( x : Int ) : Int {
		return x * 2;
	}
}
