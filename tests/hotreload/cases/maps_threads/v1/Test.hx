import sys.thread.Thread;

enum W { WB(i:Int); }
enum E { C; A; B; }

class Key {
	public var id : Int;
	public function new(i) id = i;
}

class Moved {
	public var v : Int;
	public var added = 1;
	public function new(i) v = i;
}

class Worker {
	public static function start( main : Thread ) {
		return Thread.create(() -> run(main));
	}
	static function run( main : Thread ) {
		var keys = [for( i in 0...64 ) new Key(i)];
		var errors = 0, rounds = 0, seed = 1;
		while( true ) {
			var msg : Null<Int> = Thread.readMessage(false);
			if( msg != null ) {
				if( msg < 0 ) break;
				main.sendMessage("errors=" + errors + (rounds > 0 ? "" : " idle"));
			}
			var om = new haxe.ds.ObjectMap<Key,Int>();
			var em = new haxe.ds.EnumValueMap<W,Int>();
			var present = [for( i in 0...64 ) false];
			for( n in 0...500 ) {
				seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
				var i = seed % 64;
				if( present[i] ) {
					om.remove(keys[i]);
					em.remove(WB(i));
				} else {
					om.set(keys[i], i);
					em.set(WB(i), i);
				}
				present[i] = !present[i];
			}
			for( i in 0...64 ) {
				if( (om.get(keys[i]) == i) != present[i] ) errors++;
				if( (em.get(WB(i)) == i) != present[i] ) errors++;
			}
			rounds++;
		}
		main.sendMessage("done");
	}
}

class Test {
	static var moved : Array<Moved>;
	static var om : haxe.ds.ObjectMap<Moved,Int>;
	static var em : haxe.ds.EnumValueMap<E,Int>;

	public static function check( step : Int ) {
		if( moved == null ) {
			moved = [for( i in 0...1000 ) new Moved(i)];
			om = new haxe.ds.ObjectMap();
			for( m in moved ) om.set(m, m.v);
			em = new haxe.ds.EnumValueMap();
			em.set(A, 0);
			em.set(B, 1);
			em.set(C, 2);
		}
		var found = 0;
		for( m in moved ) if( om.get(m) == m.v ) found++;
		Sys.println("v1 main: om " + found + "/" + moved.length + " em A=" + em.get(A) + " B=" + em.get(B) + " C=" + em.get(C));
	}
}
