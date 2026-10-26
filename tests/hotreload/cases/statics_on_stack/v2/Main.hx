class Main {
	static function main() {
		var files = Sys.args();
		Test.check(0);
		for( i in 0...files.length ) {
			var ok = reload(files[i]);
			// this loop keeps running its first version : it must not read the statics of a class whose layout changed
			Sys.println("reload " + (i + 1) + ": " + (ok ? "applied" : "not applied") + " counter=" + Test.counter);
			Test.check(i + 1);
		}
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
