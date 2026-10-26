enum Color {
	Red;
	Green;
	Rgb( r : Int, g : Int, b : Int );
}

class Test {
	static var color : Color;
	public static function check( step : Int ) {
		if( color == null ) color = Rgb(1, 2, 3);
		Sys.println("v0 step " + step + " " + name(color));
	}
	static function name( c : Color ) {
		return switch( c ) {
		case Red: "red";
		case Green: "green";
		case Rgb(r, g, b): "rgb" + r + g + b;
		}
	}
}
