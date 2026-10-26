class Main {
	static function main() {
		var files = Sys.args();
		Test.check(0);
		// a file still being written : nothing is patched
		var bytes = sys.io.File.getBytes(files[0]);
		sys.io.File.saveBytes("partial.hl", bytes.sub(0, bytes.length >> 1));
		Sys.println("reload partial: " + (reload("partial.hl") ? "applied" : "not applied"));
		Test.check(1);
		Sys.println("reload complete: " + (reload(files[0]) ? "applied" : "not applied"));
		Test.check(2);
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
