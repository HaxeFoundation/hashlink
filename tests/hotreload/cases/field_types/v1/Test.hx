typedef HasId = { var id : Int; }

class Base {
	public var id : Int;
	public function new( id ) { this.id = id; }
}

class Sub extends Base {
}

// the type of each field changes : the values are converted
class Holder {
	public var count : Float = 0;			// Int -> Float
	public var ratio : Int = 0;				// Float -> Int
	public var maybe : Int = 0;				// Null<Int> -> Int
	public var plain : Null<Int> = 0;		// Int -> Null<Int>
	public var dyn : Int = 0;				// Dynamic -> Int
	public var flag : Null<Bool> = false;	// Bool -> Null<Bool>
	public var sub : Base = null;			// to the superclass
	public var base1 : Sub = null;			// to a subclass : the value is a Sub
	public var base2 : Sub = null;			// to a subclass : the value is a Base, lost
	public var obj : HasId = null;			// to a structure
	public var text : Dynamic = null;		// String -> Dynamic
	public function new() {}
}

class Test {
	static var h : Holder;
	static var total : Float = 0;			// a static : Int -> Float
	public static function check( step : Int ) {
		if( h == null ) h = new Holder();
		Sys.println("v1 step " + step + " count=" + h.count + " ratio=" + h.ratio + " maybe=" + h.maybe + " plain=" + h.plain + " dyn=" + h.dyn + " flag=" + h.flag + " sub=" + h.sub.id + " base1=" + h.base1.id + " base2=" + h.base2 + " obj=" + h.obj.id + " text=" + h.text + " total=" + (total + 0.5));
	}
}
