interface Named {
	function name() : String;
}

class Base implements Named {
	public function new() {}
	public function name() return "base";
}

class Sub extends Base {
	override function name() return "sub(" + super.name() + ")";
}

class Test {
	static var obj : Base;
	public static function check( step : Int ) {
		if( obj == null ) obj = new Sub();
		var named : Named = obj;
		var dyn : Dynamic = obj;
		var refl = Reflect.callMethod(obj, Reflect.field(obj, "name"), []);
		Sys.println("v0 step " + step + " virtual=" + obj.name() + " interface=" + named.name() + " dynamic=" + dyn.name() + " reflect=" + refl);
	}
}
