class Point {
	public var y : Int;
	public var x : Int;
	public function new( x ) {
		this.x = x;
	}
}

class Test {
	static var points : Array<Point>;
	static var map : Map<Point, Int>;
	public static function check( step : Int ) {
		if( points == null ) {
			points = [for( i in 0...10 ) new Point(i)];
			map = [for( p in points ) p => p.x * 10];
		}
		var s = 0;
		for( p in points ) {
			s += p.x + p.y + map.get(p);
			p.y = 1;
		}
		Sys.println("v2 step " + step + " sum=" + s);
	}
}
