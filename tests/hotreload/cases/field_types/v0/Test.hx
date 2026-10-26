typedef HasId = { var id : Int; }

class Base {
	public var id : Int;
	public function new( id ) { this.id = id; }
}

class Sub extends Base {
}

class Holder {
	public var count : Int = 7;
	public var ratio : Float = 2.75;
	public var maybe : Null<Int> = 5;
	public var plain : Int = 9;
	public var dyn : Dynamic = 11;
	public var flag : Bool = true;
	public var sub : Sub = new Sub(1);
	public var base1 : Base = new Sub(2);
	public var base2 : Base = new Base(3);
	public var obj : Base = new Base(4);
	public var text : String = "hi";
	public function new() {}
}

class Test {
	static var h : Holder;
	static var total : Int = 41;
	public static function check( step : Int ) {
		if( h == null ) h = new Holder();
		Sys.println("v0 step " + step + " count=" + h.count + " ratio=" + h.ratio + " maybe=" + h.maybe + " plain=" + h.plain + " dyn=" + h.dyn + " flag=" + h.flag + " sub=" + h.sub.id + " base1=" + h.base1.id + " base2=" + h.base2.id + " obj=" + h.obj.id + " text=" + h.text + " total=" + total);
	}
}
