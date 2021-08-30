import hl.I64;
import hl.uv.Timer;
import hl.uv.Prepare;
import sys.thread.Thread;

class PrepareSample {
	public static function main() {
		var loop = Thread.current().events;
		var timer = Timer.init(loop);
		timer.start(() -> {
			timer.stop();
			timer.close();
		}, I64.ofInt(10), I64.ofInt(10));

		var prepare = Prepare.init(loop);
		prepare.start(() -> {
			Log.print('Prepare');
			prepare.stop();
			prepare.close();
		});
	}
}