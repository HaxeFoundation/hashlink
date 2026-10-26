enum Color {
	Red;
	Blue;
	Green;
	Rgb( r : Int, g : Int, b : Int );
}

class Test {
	static var color : Color;
	public static function check( step : Int ) {
		if( color == null ) color = Rgb(1, 2, 3);
		Sys.println("v1 step " + step + " " + name(color));
	}
	static function name( c : Color ) {
		return switch( c ) {
		case Red: "red";
		case Blue: "blue";
		case Green: "green";
		case Rgb(r, g, b): "rgb" + r + g + b;
		}
	}
}
