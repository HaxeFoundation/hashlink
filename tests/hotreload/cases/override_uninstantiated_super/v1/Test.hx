// Root and its subclasses are never instantiated before the reload : their prototypes were never built
class Root {
	public function new() {}
	public function id() return "root";
}

class Other extends Root {
	override function id() return "other";
}

class Win extends Root {
	public function base() return "win";
	public function open() return base();
}
class Menu extends Win {
	// Win.base becomes virtual : Win and Menu change, their superclass Root does not
	override function base() return "menu";
}

class Test {
	public static function check( step : Int ) {
		Sys.println("v1 step " + step + " " + new Menu().open() + " " + new Win().open() + " " + new Other().id());
	}
}