class Item {
	public var id : Int;
	public function new( id ) {
		this.id = id;
	}
	public function label() return "item" + id;
}

class Test {
	static var items : Array<Item>;
	static var names : Map<Item, String>;
	static var captured : Void -> Int;
	static var method : Void -> String;
	static var holder : { item : Item };
	public static function check( step : Int ) {
		if( items == null ) {
			items = [for( i in 0...50 ) new Item(i)];
			names = new Map();
			for( it in items ) names.set(it, "n" + it.id);
			var it7 = items[7];
			captured = () -> it7.id;
			method = items[9].label;
			holder = { item : items[3] };
		}
		var sum = 0;
		for( it in items ) sum += it.id;
		var found = 0;
		for( it in items ) if( names.get(it) == "n" + it.id ) found++;
		Sys.println("v0 step " + step + " sum=" + sum + " map=" + found + " captured=" + captured() + " method=" + method() + " holder=" + holder.item.id + " same=" + (holder.item == items[3]) + " is=" + Std.isOfType(items[0], Item));
	}
}
