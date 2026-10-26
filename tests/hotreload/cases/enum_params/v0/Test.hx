enum Shape {
	Rgb( r : Int, g : Int, b : Int );
	Named( name : String, n : Int );
}

class Test {
	static var shapes : Array<Shape>;
	public static function check( step : Int ) {
		if( shapes == null ) shapes = [Rgb(1, 2, 3), Named("a", 7)];
		Sys.println("v0 step " + step + " " + [for( s in shapes ) str(s)].join(","));
	}
	static function str( s : Shape ) {
		return switch( s ) {
		case Rgb(r,g,b): "rgb r=" + r + " g=" + g + " b=" + b;
		case Named(name,n): "named name=" + name + " n=" + n;
		}
	}
}
