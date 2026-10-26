class Item {
	public var id : Int;
	public function new( id ) {
		this.id = id;
	}
}

class Test {
	static var items : Array<Item>;
	public static function check( step : Int ) {
		if( items == null ) items = [for( i in 0...5 ) new Item(i)];
		Sys.println("v0 step " + step + " " + [for( it in items ) it.id].join(","));
	}
}
