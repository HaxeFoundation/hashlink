class Base {
	public var name = "base";
	// added, with constant initializers : the live instances get them
	public var count = 3;
	public var ratio = 0.5;
	public var flag = true;
	public var label = "new";
	public var maybe : Null<Int> = 8;
	// not constants : the default value
	public var list = [1, 2];
	public var computed = Std.int(Math.sqrt(16));
	public function new() {}
}

class Sub extends Base {
	public var level = 1;
	public var power = 10;
	public var later : Int;
	public function new() {
		super();
		// after a call, still straight code : a constant
		later = 4;
		if( Math.random() < 2 ) power = 20;
	}
}

class Test {
	static var objs : Array<Base>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [new Base(), new Sub()];
		Sys.println("v1 step " + step + " " + [for( o in objs ) o.name + " count=" + o.count + " ratio=" + o.ratio + " flag=" + o.flag + " label=" + o.label + " maybe=" + o.maybe + " list=" + o.list + " computed=" + o.computed + (Std.isOfType(o, Sub) ? " power=" + (cast o : Sub).power + " later=" + (cast o : Sub).later : "")].join(", "));
	}
}
