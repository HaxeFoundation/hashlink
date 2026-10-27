interface Ser {
	var uid : Int;
}

interface Other {
	function other() : String;
}

// unchanged by every version : the classes below change, their runtime data must keep this one as parent
class Root implements Ser {
	public var uid : Int;
	public function new( uid : Int ) { this.uid = uid; }
}

class Base extends Root {
	public function kind() return "base";
}

class Mid extends Base implements Other {
	public var m : Int = 2;
	public function other() return "other" + uid;
}

class Leaf extends Mid {
	public var l : Int = 3;
}

class Test {
	// kept as Dynamic : every check converts them to the interfaces again (hl_to_virtual)
	static var objs : Array<Dynamic>;
	public static function check( step : Int ) {
		if( objs == null ) objs = [for( i in 0...300 ) i % 3 == 0 ? new Base(i) : i % 3 == 1 ? new Mid(i) : new Leaf(i)];
		var uids = 0, others = [], kinds = new Map<String, Int>();
		for( o in objs ) {
			var s : Ser = o;
			uids += s.uid;
			if( Std.isOfType(o, Other) ) {
				var t : Other = o;
				if( others.length < 3 ) others.push(t.other());
			}
			var k = (o : Base).kind();
			kinds.set(k, (kinds.exists(k) ? kinds.get(k) : 0) + 1);
		}
		var ks = [for( k => v in kinds ) k + ":" + v];
		ks.sort(Reflect.compare);
		Sys.println("v0 step " + step + " " + uids + " " + others.join(",") + " " + ks.join(","));
	}
}