import sys.thread.Thread;

class Acc {
	public var total = 0;
	public var added = 0;
	public function new() {}
}

class Worker {
	public static function start( main : Thread ) {
		return Thread.create(() -> run(main));
	}
	// runs as long as the program : it keeps its first version
	static function run( main : Thread ) {
		var acc = new Acc();
		while( true ) {
			var step : Int = Thread.readMessage(true);
			if( step < 0 ) break;
			acc.total += step;
			main.sendMessage(compute(step) + " total=" + acc.total);
		}
		main.sendMessage("done");
	}
	static function compute( step : Int ) return "v2:" + (step * 4);
}
