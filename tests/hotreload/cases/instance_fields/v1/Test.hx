typedef Named = { var name : String; var level : Int; }

interface Leveled {
	function describe() : String;
}

class Base implements Leveled {
	public var level : Int;
	public var added : Array<Int>;
	public var name : String;
	public function new( name : String, level : Int ) {
		this.name = name;
		this.level = level;
	}
	public function describe() return name + ":" + level + (added == null ? "" : "+" + added.length);
}

class Sub extends Base {
	public var power : Float;
	public function new( name, level, power ) {
		super(name, level);
		this.power = power;
	}
	override function describe() return super.describe() + "/" + power;
}

class Test {
	static var all : Array<Base>;
	static var named : Named;
	static var leveled : Leveled;
	public static function check( step : Int ) {
		if( all == null ) {
			all = [new Base("a", 1), new Sub("b", 2, 1.5)];
			named = all[1];
			leveled = all[1];
		}
		Sys.println("v1 step " + step + " " + [for( b in all ) b.describe()].join(",") + " named=" + named.name + named.level + " leveled=" + leveled.describe() + " reflect=" + Reflect.field(all[1], "power"));
	}
}
