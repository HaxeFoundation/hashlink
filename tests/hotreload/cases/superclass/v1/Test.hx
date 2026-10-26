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

// its superclass changes : the fields of A are lost, the ones of C are new
class B extends C {
	public var b = 2;
	override function who() return "B<" + super.who();
}

class D extends B {
	public var d = 4;
	override function who() return "D<" + super.who() + " d=" + d;
}

class Test {
	static var objs : Array<B>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [new B(), new D()];
		Sys.println("v1 step " + step + " " + [for( o in objs ) o.who() + " b=" + o.b + " c=" + o.c + " isA=" + Std.isOfType(o, A) + " isC=" + Std.isOfType(o, C)].join(", "));
	}
}
