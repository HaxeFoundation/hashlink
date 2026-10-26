class A {
	public var a = 1;
	public function new() {}
	public function who() return "A";
}

class C {
	public var c = 3;
	public function new() {}
	public function who() return "C";
}

// not a leaf : D extends it
class B extends A {
	public var b = 2;
	override function who() return "B<" + super.who();
}

class D extends B {
	public var d = 4;
	override function who() return "D<" + super.who();
}

class Test {
	static var objs : Array<B>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [new B(), new D()];
		Sys.println("v2 step " + step + " " + [for( o in objs ) o.who() + " b=" + o.b + " a=" + o.a + " isA=" + Std.isOfType(o, A) + " isC=" + Std.isOfType(o, C)].join(", "));
	}
}
