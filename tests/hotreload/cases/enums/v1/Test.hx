enum Color {
	Rgb( r : Int, g : Int, b : Int );
	Red;
	Blue;
	Green;
}

class Test {
	static var colors : Array<Color>;
	static var fav : Color;
	public static function check( step : Int ) {
		if( colors == null ) {
			colors = [Red, Green, Rgb(1, 2, 3)];
			fav = Green;
		}
		Sys.println("v1 step " + step + " " + [for( c in colors ) name(c) + "#" + Type.enumIndex(c)].join(",") + " fav=" + (fav == Green) + " constructs=" + Type.getEnumConstructs(Color).join(",") + " blue=" + name(Type.createEnum(Color, "Blue")));
	}
	static function name( c : Color ) {
		return switch( c ) {
		case Red: "red";
		case Blue: "blue";
		case Green: "green";
		case Rgb(r,g,b): "rgb" + r + g + b;
		}
	}
}
