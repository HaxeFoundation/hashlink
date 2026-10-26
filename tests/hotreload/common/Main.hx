/**
	Drives the steps of a case : at step i (i >= 1), reloads the code from the i-th file given
	on the command line, then calls Test.check(i), which every version defines.
	A case can replace this file by its own Main.hx.
**/
class Main {
	static function main() {
		var files = Sys.args();
		Test.check(0);
		for( i in 0...files.length ) {
			var ok = reload(files[i]);
			Sys.println("reload " + (i + 1) + ": " + (ok ? "applied" : "not applied"));
			Test.check(i + 1);
		}
	}

	public static function reload( file : String ) : Bool {
		// the VM reads a native path : UTF-16 on Windows, UTF-8 elsewhere
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
