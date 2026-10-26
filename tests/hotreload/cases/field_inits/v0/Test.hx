class Base {
	public var name = "base";
	public function new() {}
}

class Sub extends Base {
	public var level = 1;
	public function new() {
		super();
	}
}

class Test {
	static var objs : Array<Base>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [new Base(), new Sub()];
		Sys.println("v0 step " + step + " " + [for( o in objs ) o.name].join(","));
	}
}
