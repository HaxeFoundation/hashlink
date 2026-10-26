/**
	A Heaps app that runs until a file named "quit" appears : every frame, it prints the status
	returned by Game.status when it changes. The driver edits Game.hx and Heaps between reloads.
**/
class Main extends hxd.App {

	override function init() {
		Game.init(this);
		log("ready");
	}

	var last : String;

	override function update( dt : Float ) {
		var s = try Game.status(this) catch( e : Dynamic ) "exception " + e;
		if( s != last ) {
			last = s;
			log("status " + s);
		}
		if( sys.FileSystem.exists("quit") ) hxd.System.exit();
	}

	public static function log( msg : String ) {
		Sys.println(msg);
		Sys.stdout().flush();
	}

	static function main() {
		hxd.System.onReload = () -> log("onReload");
		new Main();
	}
}
