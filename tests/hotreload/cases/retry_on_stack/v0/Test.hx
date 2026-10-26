class Foo {
	public var x = 1;
	public function new() {}
}

class Test {
	public static var foo = new Foo();
	public static function check( step : Int ) {
		Sys.println("v0 step " + step + " x=" + foo.x);
	}
}
