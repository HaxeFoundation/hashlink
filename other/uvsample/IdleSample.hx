import hl.I64;
import hl.uv.Timer;
import hl.uv.Idle;
import sys.thread.Thread;

class IdleSample {
	public static function main() {
		var loop = Thread.current().events;
		var timer = Timer.init(loop);
		timer.start(() -> {
			timer.stop();
			timer.close();
		}, I64.ofInt(10), I64.ofInt(10));

		var idle = Idle.init(loop);
		idle.start(() -> {
			Log.print('Idle');
			idle.stop();
			idle.close();
		});
	}
}