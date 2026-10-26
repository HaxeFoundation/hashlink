class Base {
	public function new() {}
	public function name() return "base";
}

// an override added : the slots keep their methods, the running loop calls the new one
class Sub extends Base {
	override function name() return "sub";
}

class Leaf extends Sub {
	override function name() return "leaf";
}

class Test {
	public static function check( step : Int ) {
		Sys.println("v1 step " + step);
	}
}
