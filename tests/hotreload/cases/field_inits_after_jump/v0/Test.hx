class Obj {
	// a dynamic method : the constructor starts with "if( this.onEvent == null ) this.onEvent = ..."
	public dynamic function onEvent() return "event";
	// an initializer with a test
	public var b : Int = Sys.args().length > 100 ? 1 : 2;
	public var x : Int = 5;
	public function new() {}
}

class Test {
	static var o : Obj;
	public static function check( step : Int ) {
		if( o == null ) o = new Obj();
		Sys.println("v0 step " + step + " " + o.onEvent() + " b=" + o.b + " x=" + o.x);
	}
}