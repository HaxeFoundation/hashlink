class Test {
	public static function check( step : Int ) {
		Keep.total += 2;
		Keep.calls++;
	}
	public static function report() {
		Sys.println("total=" + Keep.total + " calls=" + Keep.calls);
	}
}

class Keep {
	public static var total : Int;
	public static var calls : Int;
}
