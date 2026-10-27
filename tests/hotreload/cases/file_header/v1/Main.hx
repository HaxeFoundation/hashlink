class Main {
	static function main() {
		var file = Sys.args()[0];
		var data = sys.io.File.getBytes(file);
		// next to the file : a relative path has no directory part
		var copy = haxe.io.Path.join([haxe.io.Path.directory(file), "copy.hl"]);
		Test.check(0);
		// the compiler is still writing : shorter than the length in its header, not read
		sys.io.File.saveBytes(copy, data.sub(0, data.length >> 1));
		Sys.println("partial: " + (reload(copy) ? "applied" : "not applied"));
		Test.check(1);
		// complete
		sys.io.File.saveBytes(copy, data);
		Sys.println("complete: " + (reload(copy) ? "applied" : "not applied"));
		Test.check(2);
		// written again with the same code : same build id, not read
		Sys.sleep(0.01);
		sys.io.File.saveBytes(copy, data);
		Sys.println("same code: " + (reload(copy) ? "applied" : "not applied"));
		Test.check(3);
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
