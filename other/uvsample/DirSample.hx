import hl.uv.Misc;
import hl.uv.Dir;
import hl.uv.UVException;
import sys.thread.Thread;

class DirSample {
	public static function main() {
		var loop = Thread.current().events;
		var path = Misc.tmpDir();
		inline function print(msg)
			Log.print('DIR: $msg');
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