enum Color {
	Red;
	Green;
}

class Test {
	static var colors : Array<Color>;
	static var fav : Color;
	public static function check( step : Int ) {
		if( colors == null ) {
			colors = [Red, Green];
			fav = Green;
		}
		Sys.println("v5 step " + step + " " + [for( c in colors ) name(c) + "#" + Type.enumIndex(c)].join(",") + " fav=" + (fav == Green) + " constructs=" + Type.getEnumConstructs(Color).join(","));
	}
	static function name( c : Color ) {
		return switch( c ) {
		case Red: "red";
		case Green: "green";
		}
	}
}
