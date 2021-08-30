import hl.uv.Signal;
import sys.thread.Thread;

class SignalSample {
	public static function main() {
		var loop = Thread.current().events;
		var signal = Signal.init(loop);
		Log.print('SignalSample: waiting for Ctrl^C...');
		signal.start(SIGINT, () -> {
			Log.print('SignalSample: got Ctrl^C');
			signal.stop();
			signal.close();
		});
	}
}