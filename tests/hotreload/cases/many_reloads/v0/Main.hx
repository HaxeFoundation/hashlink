class Main {
	static function main() {
		var files = Sys.args();
		Test.check(0);
		var applied = 0;
		for( i in 0...120 )
			if( reload(files[i % 2]) ) {
				applied++;
				Test.check(i + 1);
			}
		Sys.println("applied " + applied);
		Test.report();
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
