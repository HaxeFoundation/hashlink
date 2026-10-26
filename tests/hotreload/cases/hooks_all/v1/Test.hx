// the constructors move : the map is sorted again by its onHotReload()
enum Color {
	Rgb( r : Int, g : Int, b : Int );
	Blue;
	Green;
	Red;
}

class Watcher {
	public var reloads = 0;
	public function new() {}
	public function onHotReload() reloads++;
}

class Test {
	static var map : Map<Color, String>;
	static var watcher : Watcher;
	public static function check( step : Int ) {
		if( map == null ) {
			map = [Red => "red", Green => "green", Blue => "blue", Rgb(1, 2, 3) => "rgb123", Rgb(0, 0, 9) => "rgb009"];
			watcher = new Watcher();
		}
		Sys.println("v1 step " + step + " " + show() + " reloads=" + watcher.reloads);
	}
	static function show() {
		return [for( k in [Red, Green, Blue, Rgb(1, 2, 3), Rgb(0, 0, 9)] ) map.get(k)].join(",");
	}
}
