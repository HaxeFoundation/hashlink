import haxe.Constraints.Function;
import hl.I64;
import haxe.io.Bytes;
import hl.uv.UVError;
import haxe.PosInfos;
import hl.uv.Misc;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.File;
import hl.uv.FileSync;
import FileSample.Actions;

class FileSyncSample {
	public static function main() {
		var actions:Actions = [
			createWriteSyncReadUnlink,
			mkdirRenameRmdir,
			mkdtempRmdir,
			mkstempUnlink,
			statFStat,
			statFs,
			truncate,
			copyFile,
			// sendFile, // Throws EBADF and idk why
			access,
			chmod,
			utime,
			linkSymlinkReadLinkRealPath,
			chown,
		];
		actions.next();
	}

	static function createFile(path:String, content:Bytes, ?pos:PosInfos) {
		try {
			var file = FileSync.open(path, [O_CREAT(420),O_TRUNC,O_WRONLY]);
			file.sync.write(content.getData(), content.length, I64.ofInt(0));
			file.sync.close();
		} catch(e:UVException) {
			throw new UVException(e.error, pos.fileName + ':' + pos.lineNumber + ': ' + e.error.toString(), e);
		}
	}

	static function readFile(path:String):Bytes {
		var file = FileSync.open(path, [O_RDONLY]);
		var buf = new hl.Bytes(10240);
		var bytesRead = file.sync.read(buf, 10240, I64.ofInt(0));
		file.sync.close();
		return buf.toBytes(bytesRead.toInt());
	}

	static function deleteFiles(files:Array<String>) {
		for(path in files)
			FileSync.unlink(path);
	}

	static function createWriteSyncReadUnlink(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		Log.print('Creating $path for writing...');
		var file = FileSync.open(path, [O_CREAT(420), O_WRONLY]);
		Log.print('Writing...');
		var data = Bytes.ofString('Hello, world!');
		var bytesWritten = file.sync.write(data.getData(), data.length, I64.ofInt(0));
		Log.print('$bytesWritten bytes written: $data');
		Log.print('fsync...');
		file.sync.fsync();
		Log.print('fdatasync...');
		file.sync.fdataSync();
		file.sync.close();
		Log.print('closed $path');
		readUnlink(path, actions);
	}

	static function readUnlink(path:String, actions:Actions) {
		Log.print('Opening $path for reading...');
		var file = FileSync.open(path, [O_RDONLY]);
		Log.print('Reading...');
		var buf = new hl.Bytes(1024);
		var bytesRead = file.sync.read(buf, 1024, I64.ofInt(0));
		Log.print('$bytesRead bytes read: ' + buf.toBytes(bytesRead.toInt()));
		file.sync.close();
		Log.print('closed $path');
		unlink(path, actions);
	}

	static function unlink(path:String, actions:Actions) {
		Log.print('Unlinking $path...');
		FileSync.unlink(path);
		actions.next();
	}

	static function mkdirRenameRmdir(actions:Actions) {
		var path = Misc.tmpDir() + '/test-dir';
		var newPath = Misc.tmpDir() + '/test-dir2';
		Log.print('Creating directory $path...');
		FileSync.mkdir(path, 511);
		Log.print('Renaming $path to $newPath...');
		FileSync.rename(path, newPath);
		Log.print('Removing directory $newPath...');
		FileSync.rmdir(newPath);
		Log.print('Done');
		actions.next();
	}

	static function mkdtempRmdir(actions:Actions) {
		var tpl = Misc.tmpDir() + '/test-dir-XXXXXX';
		Log.print('Creating temp directory with tpl $tpl...');
		var path = FileSync.mkdtemp(tpl);
		Log.print('Removing directory $path...');
		FileSync.rmdir(path);
		Log.print('Done');
		actions.next();
	}

	static function mkstempUnlink(actions:Actions) {
		var tpl = Misc.tmpDir() + '/test-file-XXXXXX';
		Log.print('Creating temp file with tpl $tpl...');
		var tmp = FileSync.mkstemp(tpl);
		Log.print('Closing ${tmp.path}...');
		tmp.file.sync.close();
		Log.print('Unlinking ${tmp.path}...');
		FileSync.unlink(tmp.path);
		Log.print('Done');
		actions.next();
	}

	static function statFStat(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		Log.print('fstat on $path...');
		var file = FileSync.open(path, [O_CREAT(420)]);
		var fstat = file.sync.fstat();
		Log.print('got fstat: $fstat');
		file.sync.close();
		Log.print('stat on $path');
		var stat = FileSync.stat(path);
		Log.print('got stat: $stat');
		// TODO: jit error on I64 == I64
		// var ok = stat.dev == fstat.dev;
		// 	&& stat.mode == fstat.mode
		// 	&& stat.nlink == fstat.nlink
		// 	&& stat.uid == fstat.uid
		// 	&& stat.gid == fstat.gid
		// 	&& stat.rdev == fstat.rdev
		// 	&& stat.ino == fstat.ino
		// 	&& stat.size == fstat.size
		// 	&& stat.blksize == fstat.blksize
		// 	&& stat.blocks == fstat.blocks
		// 	&& stat.flags == fstat.flags
		// 	&& stat.gen == fstat.gen;
		// Log.print('fstat equals stat: $ok');
		deleteFiles([path]);
		Log.print('Done');
		actions.next();
	}

	static function statFs(actions:Actions) {
		Log.print('statfs on .');
		var stat = FileSync.statFs('.');
		Log.print('got statfs: $stat');
		Log.print('Done');
		actions.next();
	}

	static function truncate(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-truncate';
		var content = '1234567890';
		Log.print('Writing content for truncation at $path: $content');
		createFile(path, Bytes.ofString(content));
		var file = FileSync.open(path, [O_WRONLY]);
		Log.print('truncating at 5...');
		file.sync.ftruncate(I64.ofInt(5));
		file.sync.close();
		var data = readFile(path);
		Log.print('Content after truncation (length=${data.length}): $data');
		deleteFiles([path]);
		Log.print('Done');
		actions.next();
	}

	static function copyFile(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-copy';
		var newPath = '$path-copy';
		createFile(path, Bytes.ofString('123'));
		Log.print('Copy $path to $newPath');
		FileSync.copyFile(path, newPath, [EXCL]);
		deleteFiles([path, newPath]);
		Log.print('Done');
		actions.next();
	}

	static function sendFile(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-send';
		var newPath = '$path-copy';
		createFile(path, Bytes.ofString('12345678'));
		var src = FileSync.open(path, [O_RDONLY]);
		var dst = FileSync.open(newPath, [O_CREAT(420), O_WRONLY]);
		Log.print('sendFile from $path to $newPath...');
		var outOffset = src.sync.sendFile(dst, I64.ofInt(0), I64.ofInt(20));
		Log.print('sendfile stopped at $outOffset');
		src.sync.close();
		dst.sync.close();
		deleteFiles([path, newPath]);
		Log.print('Done');
		actions.next();
	}

	static function access(actions:Actions) {
		var path = Misc.tmpDir();
		Log.print('Checking write permissions on $path...');
		FileSync.access(path, [W_OK]);
		Log.print('Done');
		actions.next();
	}

	static function chmod(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-chmod';
		createFile(path, Bytes.ofString('123'));
		Log.print('chmod on $path...');
		FileSync.chmod(path, 420);
		deleteFiles([path]);
		Log.print('Done');
		actions.next();
	}

	static function utime(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-utime';
		createFile(path, Bytes.ofString('123'));
		Log.print('utime on $path...');
		FileSync.utime(path, Date.now().getTime(), Date.now().getTime());
		deleteFiles([path]);
		Log.print('Done');
		actions.next();
	}

	static function linkSymlinkReadLinkRealPath(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file-l';
		var newPath = Misc.tmpDir() + '/test-file-link';
		createFile(path, Bytes.ofString('123'));
		Log.print('link $path to $newPath...');
		FileSync.link(path, newPath);
		deleteFiles([newPath]);
		Log.print('symlink $path to $newPath...');
		FileSync.symlink(path, newPath, [SYMLINK_JUNCTION]);
		Log.print('readlink at $newPath...');
		var target = FileSync.readLink(newPath);
		Log.print('Link content: $target');
		var real = FileSync.readLink(newPath);
		Log.print('Real path of $newPath: $real');
		deleteFiles([path, newPath]);
		Log.print('Done');
		actions.next();
	}

	static function chown(actions:Actions) {
		if(Sys.systemName() == 'Windows') {
			actions.next();
			return;
		}

		var path = Misc.tmpDir() + '/test-file-chown';
		createFile(path, Bytes.ofString(''));
		Log.print('chown on $path...');
		FileSync.chown(path, -1, -1);
		deleteFiles([path]);
		Log.print('Done');
		actions.next();
	}
}