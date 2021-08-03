import hl.uv.Misc;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.File;

class FileSample {
	public static function main() {
		var loop = Thread.current().events;
		var dir = Misc.tmpDir();
		File.open(loop, '$dir/test', [O_CREAT(511), O_WRONLY], (e, file) -> switch e {
			case UV_NOERR:
				trace(file);
			case _:
				throw new UVException(e);
		});
	}
}