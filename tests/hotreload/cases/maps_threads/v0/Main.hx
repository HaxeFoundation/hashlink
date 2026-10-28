import sys.thread.Thread;

class Main {
	static function main() {
		var files = Sys.args();
		var worker = Test.Worker.start(Thread.current());
		check(worker, 0);
		for( i in 0...files.length ) {
			var ok = reload(files[i]);
			Sys.println("reload " + (i + 1) + ": " + (ok ? "applied" : "not applied"));
			check(worker, i + 1);
		}
		worker.sendMessage(-1);
		Thread.readMessage(true);
	}

	static function check( worker : Thread, step : Int ) {
		Test.check(step);
		worker.sendMessage(step);
		Sys.println("step " + step + " worker: " + Thread.readMessage(true));
	}

	public static function reload( file : String ) : Bool {
		var path : hl.Bytes = Sys.systemName() == "Windows" ? @:privateAccess file.bytes : @:privateAccess file.toUtf8();
		return hl.Api.checkReload(path);
	}
}
