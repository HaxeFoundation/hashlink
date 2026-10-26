class Test {
	static var foo : Foo;
	public static function check( step : Int ) {
		if( foo == null ) foo = new Foo();
		Sys.println("v1 step " + step + " " + foo.describe());
	}
}

class Foo {
	var value = 1;
	public function new() {}
	public function describe() return "foo" + value + " " + extra();
	function extra() return "extra" + (value + 1);
}
