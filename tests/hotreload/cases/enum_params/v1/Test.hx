enum Shape {
	// the same types, another order : the values follow their names
	Rgb( b : Int, g : Int, r : Int );
	// a parameter inserted, one moved
	Named( n : Int, scale : Float, name : String );
}

class Test {
	static var shapes : Array<Shape>;
	public static function check( step : Int ) {
		if( shapes == null ) shapes = [Rgb(3, 2, 1), Named(7, 0., "a")];
		Sys.println("v1 step " + step + " " + [for( s in shapes ) str(s)].join(","));
	}
	static function str( s : Shape ) {
		return switch( s ) {
		case Rgb(b,g,r): "rgb r=" + r + " g=" + g + " b=" + b;
		case Named(n,scale,name): "named name=" + name + " n=" + n + " scale=" + scale;
		}
	}
}
