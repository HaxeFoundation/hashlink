class Base {
	public function new() {}
	// a virtual method before name : the slots move, the running loop would call another method
	public function first() return 0;
	public function name() return "base";
}

class Sub extends Base {
	override function name() return "sub";
}

class Leaf extends Sub {
	override function first() return 1;
	override function name() return "leaf";
}

class Test {
	public static function check( step : Int ) {
		Sys.println("v2 step " + step);
	}
}
