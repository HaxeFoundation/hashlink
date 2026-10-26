class Game {
	public static function init( app : Main ) {
		var t = new h2d.Text(hxd.res.DefaultFont.get(), app.s2d);
		t.text = "hello";
		new h2d.Bitmap(h2d.Tile.fromColor(0xFF0000, 16, 16), app.s2d);
	}
	public static function status( app : Main ) : String {
		return "v0 objects=" + app.s2d.getObjectsCount();
	}
}
