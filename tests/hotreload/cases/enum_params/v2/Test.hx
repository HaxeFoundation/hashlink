enum Shape {
	// a parameter renamed : a new one, its value is lost
	Rgb( b : Int, g : Int, red : Int );
	Named( n : Int, scale : Float, name : String );
}

class Test {
	static var shapes : Array<Shape>;
	public static function check( step : Int ) {
		if( shapes == null ) shapes = [Rgb(3, 2, 1), Named(7, 0., "a")];
		Sys.println("v2 step " + step + " " + [for( s in shapes ) str(s)].join(","));
	}
	static function str( s : Shape ) {
		return switch( s ) {
		case Rgb(b,g,red): "rgb red=" + red + " g=" + g + " b=" + b;
		case Named(n,scale,name): "named name=" + name + " n=" + n + " scale=" + scale;
		}
	}
}
