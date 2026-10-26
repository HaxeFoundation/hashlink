class Base {
	public function new() {}
	public function name() return "base";
}

class Sub extends Base {
}

class Leaf extends Sub {
	override function name() return "leaf";
}

class Test {
	public static function check( step : Int ) {
		Sys.println("v0 step " + step);
	}
}
