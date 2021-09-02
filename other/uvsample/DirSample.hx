import hl.uv.DirSync;
import hl.uv.Misc;
import hl.uv.Dir;
import hl.uv.UVException;
import sys.thread.Thread;

class DirSample {
	public static function main() {
		var path = Misc.tmpDir();

		runSync(path);
		runAsync(path);
	}

	static inline function print(msg) {
		Log.print('DIR: $msg');
	}

	static function runSync(path:String) {
		print('SYNC functions:');
		var dir = DirSync.open(path);
		var entries = dir.sync.read(3);
		for(i in 0...entries.length)
			print('\t${entries[i]}');
		dir.sync.close();
	}

	static function runAsync(path:String) {
		var loop = Thread.current().events;
		print('ASYNC functions:');
		Dir.open(loop, path, (e, dir) -> switch e {
			case UV_NOERR:
				dir.read(loop, 3, (e, entries) -> switch e {
					case UV_NOERR:
						for(i in 0...entries.length)
							print('\t${entries[i]}');
						dir.close(loop, e -> switch e {
							case UV_NOERR: print('Done');
							case _: throw new UVException(e);
						});
					case _:
						throw new UVException(e);
				});
			case _:
				throw new UVException(e);
		});
	}
}