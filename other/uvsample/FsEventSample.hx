import sys.FileSystem;
import haxe.Timer;
import sys.io.File;
import hl.uv.Misc;
import hl.uv.FsEvent;
import hl.uv.UVException;
import sys.thread.Thread;

class FsEventSample {
	public static function main() {
		var loop = Thread.current().events;
		var event = FsEvent.init(loop);
		var path = Misc.tmpDir() + '/test-file-fsevent';
		File.saveContent(path, 'Hello, world');
		event.start(path, null, (e, path, events) -> switch e {
			case UV_NOERR:
				Log.print('FS event monitors ${event.getPath()}');
				Log.print('FS event {change:${events.change}, rename:${events.rename}} on $path');
				event.stop();
				event.close();
			case _:
				throw new UVException(e);
		});
		Timer.delay(FileSystem.deleteFile.bind(path), 50);
	}
}