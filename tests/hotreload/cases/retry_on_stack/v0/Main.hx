class Main {
	static function main() {
		var files = Sys.args();
		Test.check(0);
		update(files[0]);
		// update returned : the same file is tried again
		Sys.println("after update: " + (reload(files[0]) ? "applied" : "not applied"));
		Test.check(1);
		Sys.println("again: " + (reload(files[0]) ? "applied" : "not applied"));
	}

	// uses Foo while the reload changes its layout : the reload is delayed
	static function update( file : String ) {
		var f = Test.foo;
		var ok = reload(file);
		Sys.println("in update: " + (ok ? "applied" : "not applied") + " x=" + f.x);
		ok = reload(file);
		Sys.println("in update, again: " + (ok ? "applied" : "not applied"));
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
