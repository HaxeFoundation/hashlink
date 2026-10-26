typedef HasName = { function name() : String; }

class Base {
	public function new() {}
	public function name() return "base";
	public function describe() return "[" + name() + "]";
}

class Sub extends Base {
}

class Leaf extends Sub {
	override function name() return "leaf";
}

class Test {
	static var objs : Array<Base>;
	static var virt : HasName;
	static var bound : Void -> String;
	public static function check( step : Int ) {
		if( objs == null ) {
			objs = [new Base(), new Sub(), new Leaf()];
			virt = objs[1];
			bound = objs[1].name;
		}
		Sys.println("v0 step " + step + " " + [for( o in objs ) o.describe()].join(",") + " virtual=" + virt.name() + " dynamic=" + (objs[1] : Dynamic).name() + " bound=" + bound());
	}
}
