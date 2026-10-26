class Test {
	static var foo : Foo;
	static var method : Void -> String;
	static var local : Int -> String;
	public static function check( step : Int ) {
		if( foo == null ) {
			foo = new Foo();
			method = foo.describe;
			local = makeLocal();
		}
		Sys.println("v1 step " + step + " method=" + method() + " local=" + local(step) + " new=" + makeLocal()(step));
	}
	static function makeLocal() {
		var k = 2;
		return function( x : Int ) return "L2:" + (x * k);
	}
}

class Foo {
	var value = 1;
	public function new() {}
	public function describe() return "FOO" + value;
}
