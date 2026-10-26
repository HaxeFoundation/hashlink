class Item {
	public var id : Int;
	public var label : String;
	public function new( id ) {
		this.id = id;
	}
	// called once on each live instance after the upgrade
	function onHotReload() {
		Test.calls++;
		if( id == 3 ) throw "bad item " + id;
		label = "L" + id;
	}
}

class Test {
	static var items : Array<Item>;
	public static var calls = 0;
	static function onHotReload() {
		Sys.println("Test.onHotReload");
	}
	public static function check( step : Int ) {
		if( items == null ) items = [for( i in 0...5 ) new Item(i)];
		Sys.println("v2 step " + step + " " + [for( it in items ) it.label].join(",") + " calls=" + calls);
	}
}
