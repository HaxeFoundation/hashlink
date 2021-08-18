import hl.uv.Timer;
import hl.uv.Check;
import sys.thread.Thread;

class CheckSample {
	public static function main() {
		var loop = Thread.current().events;
		var timer = Timer.init(loop);
		timer.start(() -> {
			timer.stop();
			timer.close();
		}, 10, 10);

		var check = Check.init(loop);
		check.start(() -> {
			Log.print('Check');
			check.stop();
			check.close();
		});
	}
}