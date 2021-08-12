import hl.uv.File;
import hl.uv.Tty;
import hl.uv.UVException;
import sys.thread.Thread;

class TtySample {
	static function print(msg:String) {
		Log.print('TtySample: $msg');
	}

	public static function main() {
		print('opening tty...');
		var tty = Tty.init(Thread.current().events, File.stdout);
		print('setting mode...');
		tty.setMode(TTY_MODE_NORMAL);
		print('window size: ' + tty.getWinSize());
		Tty.resetMode();
		tty.close(() -> print('Done'));
	}
}