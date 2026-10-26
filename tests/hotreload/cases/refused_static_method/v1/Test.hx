class Test {
	static var counter = 0;
	static var foo : Foo;
	public static function check( step : Int ) {
		if( foo == null ) foo = new Foo();
		counter++;
		Sys.println("v1 step " + step + " counter=" + counter + " " + foo.describe() + " " + compute(step));
	}
	static function compute( x : Int ) return twice(x);
	static function twice( x : Int ) return x * 2;
}

class Foo {
	var value = 1;
	public function new() {}
	public function describe() return "foo" + value;
}
