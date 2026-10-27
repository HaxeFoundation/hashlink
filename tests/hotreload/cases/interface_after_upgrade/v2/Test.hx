interface Marked {
	var mark : Int;
}

class Base implements Marked {
	public var mark : Int = 0;
	public var z : Int = 3;
	public function new() {}
}

class Leaf extends Base {
	public var x : Int = 1;
	public var y : Int = 2;
}

class Test {
	// kept as Dynamic : every check converts them to the interface again (hl_to_virtual)
	static var objs : Array<Dynamic>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [new Base(), new Leaf(), new Leaf()];
		var marks = [];
		for( o in objs ) {
			var m : Marked = o;
			m.mark++;
			marks.push(m.mark);
		}
		Sys.println("v2 step " + step + " " + marks.join(","));
	}
}