import sys.FileSystem;
import haxe.Timer;
import sys.io.File;
import hl.uv.Misc;
import hl.uv.FsPoll;
import hl.uv.UVException;
import sys.thread.Thread;

class FsPollSample {
	public static function main() {
		var loop = Thread.current().events;
		var poll = FsPoll.init(loop);
		var path = Misc.tmpDir() + '/test-file';
		File.saveContent(path, 'Hello, world');
		poll.start(path, 1000, (e, previous, current) -> switch e {
			case UV_NOERR:
				Log.print('FS Poll at $path:');
				Log.print('\tprev: $previous');
				Log.print('\tcurr: $current');
				FileSystem.deleteFile(path);
				poll.stop();
				poll.close();
			case _:
				throw new UVException(e);
		});
		Timer.delay(File.saveContent.bind(path, 'Bye, world'), 50);
	}
}