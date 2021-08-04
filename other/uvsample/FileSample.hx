import hl.uv.Loop;
import haxe.Constraints.Function;
import haxe.xml.Access;
import hl.I64;
import haxe.io.Bytes;
import hl.uv.UVError;
import haxe.PosInfos;
import hl.uv.Misc;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.File;

typedef Actions = Array<Function>;

class FileSample {
	static final loop = Thread.current().events;

	public static function main() {
		var actions:Actions = [
			createWriteReadUnlink,
			mkdirRmdir,
			mkdtempRmdir,
			mkstempUnlink,
			stat,
		];
		runNext(actions);
	}

	static function handle(success:()->Void, ?p:PosInfos):(e:UVError)->Void {
		return e -> switch e {
			case UV_NOERR: success();
			case _: throw new UVException(e, p.fileName + ':' + p.lineNumber + ': ' + e.toString());
		}
	}

	static function runNext(actions:Actions) {
		var fn = actions.shift();
		if(fn != null) {
			Log.print('-----------');
			fn(actions);
		}
	}

	static function createWriteReadUnlink(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		Log.print('Creating $path for writing...');
		File.open(loop, path, [O_CREAT(511), O_WRONLY], (e, file) -> handle(() -> {
			Log.print('Writing...');
			var data = Bytes.ofString('Hello, world!');
			file.write(loop, data.getData(), data.length, I64.ofInt(0), (e, bytesWritten) -> handle(() -> {
				Log.print('$bytesWritten bytes written: $data');
				file.close(loop, handle(() -> {
					Log.print('closed $path');
					readUnlink(path, actions);
				}));
			})(e));
		})(e));
	}

	static function readUnlink(path:String, actions:Actions) {
		Log.print('Opening $path for reading...');
		File.open(loop, path, [O_RDONLY], (e, file) -> handle(() -> {
			Log.print('Reading...');
			var buf = new hl.Bytes(1024);
			file.read(loop, buf, 1024, I64.ofInt(0), (e, bytesRead) -> handle(() -> {
				Log.print('$bytesRead bytes read: ' + buf.toBytes(bytesRead.toInt()));
				file.close(loop, handle(() -> {
					Log.print('closed $path');
					unlink(path, actions);
				}));
			})(e));
		})(e));
	}

	static function unlink(path:String, actions:Actions) {
		Log.print('Unlinking $path...');
		File.unlink(loop, path, handle(() -> {
			runNext(actions);
		}));
	}

	static function mkdirRmdir(actions:Actions) {
		var path = Misc.tmpDir() + '/test-dir';
		Log.print('Creating directory $path...');
		File.mkdir(loop, path, 511, handle(() -> {
			Log.print('Removing directory $path...');
			File.rmdir(loop, path, handle(() -> {
				Log.print('Done');
				runNext(actions);
			}));
		}));
	}

	static function mkdtempRmdir(actions:Actions) {
		var tpl = Misc.tmpDir() + '/test-dir-XXXXXX';
		Log.print('Creating temp directory with tpl $tpl...');
		File.mkdtemp(loop, tpl, (e, path) -> handle(() -> {
			Log.print('Removing directory $path...');
			File.rmdir(loop, path, handle(() -> {
				Log.print('Done');
				runNext(actions);
			}));
		})(e));
	}

	static function mkstempUnlink(actions:Actions) {
		var tpl = Misc.tmpDir() + '/test-file-XXXXXX';
		Log.print('Creating temp file with tpl $tpl...');
		File.mkstemp(loop, tpl, (e, file, path) -> handle(() -> {
			Log.print('Closing $path...');
			file.close(loop, handle(() -> {
				Log.print('Unlinking $path...');
				File.unlink(loop, path, handle(() -> {
					Log.print('Done');
					runNext(actions);
				}));
			}));
		})(e));
	}

	static function stat(actions:Actions) {
		var path = Misc.tmpDir();
		Log.print('Stat on $path...');
		File.stat(loop, path, (e, stat) -> handle(() -> {
			Log.print('Got stat: $stat');
			Log.print('Done');
			runNext(actions);
		})(e));
	}
}