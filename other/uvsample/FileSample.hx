import hl.uv.Dir;
import sys.io.Process;
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

abstract Actions(Array<Function>) from Array<Function> {
	public function next() {
		var fn = this.shift();
		if(fn != null) {
			Log.print('-----------');
			fn(this);
		}
	}
}

class FileSample {
	static final loop = Thread.current().events;

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
			openReadCloseDir,
		];
		actions.next();
	}

	static function handle(success:()->Void, ?p:PosInfos):(e:UVError)->Void {
		return e -> switch e {
			case UV_NOERR: success();
			case _: throw new UVException(e, p.fileName + ':' + p.lineNumber + ': ' + e.toString());
		}
	}

	static function createFile(path:String, content:Bytes, callback:()->Void) {
		File.open(loop, path, [O_CREAT(420),O_TRUNC,O_WRONLY], (e, file) -> handle(() -> {
			file.write(loop, content.getData(), content.length, I64.ofInt(0), (e, bytesWritten) -> handle(() -> {
				file.close(loop, handle(callback));
			})(e));
		})(e));
	}

	static function readFile(path:String, callback:(data:Bytes)->Void) {
		File.open(loop, path, [O_RDONLY], (e, file) -> handle(() -> {
			var buf = new hl.Bytes(10240);
			file.read(loop, buf, 10240, I64.ofInt(0), (e, bytesRead) -> handle(() -> {
				file.close(loop, handle(() -> {
					callback(buf.toBytes(bytesRead.toInt()));
				}));
			})(e));
		})(e));
	}

	static function deleteFiles(files:Array<String>, callback:()->Void) {
		var finished = 0;
		for(path in files) {
			File.unlink(loop, path, handle(() -> {
				++finished;
				if(finished == files.length)
					callback();
			}));
		}
	}

	static function createWriteSyncReadUnlink(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		Log.print('Creating $path for writing...');
		File.open(loop, path, [O_CREAT(420), O_WRONLY], (e, file) -> handle(() -> {
			Log.print('Writing...');
			var data = Bytes.ofString('Hello, world!');
			file.write(loop, data.getData(), data.length, I64.ofInt(0), (e, bytesWritten) -> handle(() -> {
				Log.print('$bytesWritten bytes written: $data');
				Log.print('fsync...');
				file.fsync(loop, handle(() -> {
					Log.print('fdatasync...');
					file.fdataSync(loop, handle(() -> {
						file.close(loop, handle(() -> {
							Log.print('closed $path');
							readUnlink(path, actions);
						}));
					}));
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
			actions.next();
		}));
	}

	static function mkdirRenameRmdir(actions:Actions) {
		var path = Misc.tmpDir() + '/test-dir';
		var newPath = Misc.tmpDir() + '/test-dir2';
		Log.print('Creating directory $path...');
		File.mkdir(loop, path, 511, handle(() -> {
			Log.print('Renaming $path to $newPath...');
			File.rename(loop, path, newPath, handle(() -> {
				Log.print('Removing directory $newPath...');
				File.rmdir(loop, newPath, handle(() -> {
					Log.print('Done');
					actions.next();
				}));
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
				actions.next();
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
					actions.next();
				}));
			}));
		})(e));
	}

	static function statFStat(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		Log.print('fstat on $path...');
		File.open(loop, path, [O_CREAT(420)], (e, file) -> handle(() -> {
			file.fstat(loop, (e, fstat) -> handle(() -> {
				Log.print('got fstat');
				file.close(loop, handle(() -> {
					Log.print('stat on $path');
					File.stat(loop, path, (e, stat) -> handle(() -> {
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
						Log.print('Done');
						actions.next();
					})(e));
				}));
			})(e));
		})(e));
	}

	static function statFs(actions:Actions) {
		Log.print('statfs on .');
		File.statFs(loop, '.', (e, stat) -> handle(() -> {
			Log.print('got statfs: $stat');
			Log.print('Done');
			actions.next();
		})(e));
	}

	static function truncate(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		var content = '1234567890';
		Log.print('Writing content for truncation at $path: $content');
		createFile(path, Bytes.ofString(content), () -> {
			File.open(loop, path, [O_WRONLY], (e, file) -> handle(() -> {
				Log.print('truncating at 5...');
				file.ftruncate(loop, I64.ofInt(5), handle(() -> {
					file.close(loop, handle(() -> {
						readFile(path, data -> {
							Log.print('Content after truncation (length=${data.length}): $data');
							Log.print('Done');
							actions.next();
						});
					}));
				}));
			})(e));
		});
	}

	static function copyFile(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		var newPath = '$path-copy';
		createFile(path, Bytes.ofString('123'), () -> {
			Log.print('Copy $path to $newPath');
			File.copyFile(loop, path, newPath, [EXCL], handle(() -> {
				deleteFiles([path, newPath], () -> {
					Log.print('Done');
					actions.next();
				});
			}));
		});
	}

	static function sendFile(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		var newPath = '$path-copy';
		createFile(path, Bytes.ofString('12345678'), () -> {
			File.open(loop, path, [O_RDONLY], (e, src) -> handle(() -> {
				File.open(loop, newPath, [O_CREAT(420), O_WRONLY], (e, dst) -> handle(() -> {
					Log.print('sendFile from $path to $newPath...');
					src.sendFile(loop, dst, I64.ofInt(0), I64.ofInt(20), (e, outOffset) -> handle(() -> {
						Log.print('sendfile stopped at $outOffset');
						src.close(loop, handle(() -> {
							dst.close(loop, handle(() -> {
								deleteFiles([path, newPath], () -> {
									Log.print('Done');
									actions.next();
								});
							}));
						}));
					})(e));
				})(e));
			})(e));
		});
	}

	static function access(actions:Actions) {
		var path = Misc.tmpDir();
		Log.print('Checking write permissions on $path...');
		File.access(loop, path, [W_OK], handle(() -> {
			Log.print('Done');
			actions.next();
		}));
	}

	static function chmod(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		createFile(path, Bytes.ofString('123'), () -> {
			Log.print('chmod on $path...');
			File.chmod(loop, path, 420, handle(() -> {
				Log.print('Done');
				actions.next();
			}));
		});
	}

	static function utime(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		createFile(path, Bytes.ofString('123'), () -> {
			Log.print('utime on $path...');
			File.utime(loop, path, Date.now().getTime(), Date.now().getTime(), handle(() -> {
				Log.print('Done');
				actions.next();
			}));
		});
	}

	static function linkSymlinkReadLinkRealPath(actions:Actions) {
		var path = Misc.tmpDir() + '/test-file';
		var newPath = Misc.tmpDir() + '/test-file-link';
		createFile(path, Bytes.ofString('123'), () -> {
			Log.print('link $path to $newPath...');
			File.link(loop, path, newPath, handle(() -> {
				deleteFiles([newPath], () -> {
					Log.print('symlink $path to $newPath...');
					File.symlink(loop, path, newPath, [SYMLINK_JUNCTION], handle(() -> {
						Log.print('readlink at $newPath...');
						File.readLink(loop, newPath, (e, target) -> handle(() -> {
							Log.print('Link content: $target');
							File.readLink(loop, newPath, (e, real) -> handle(() -> {
								Log.print('Real path of $newPath: $real');
								deleteFiles([path, newPath], () -> {
									Log.print('Done');
									actions.next();
								});
							})(e));
						})(e));
					}));
				});
			}));
		});
	}

	static function chown(actions:Actions) {
		if(Sys.systemName() == 'Windows') {
			actions.next();
			return;
		}

		var path = Misc.tmpDir() + '/test-file';
		createFile(path, Bytes.ofString(''), () -> {
			Log.print('chown on $path...');
			File.chown(loop, path, -1, -1, handle(() -> {
				Log.print('Done');
				actions.next();
			}));
		});
	}

	static function openReadCloseDir(actions:Actions) {
		var path = Misc.tmpDir();
		Log.print('Reading 3 entries from dir $path...');
		Dir.open(loop, path, (e, dir) -> handle(() -> {
			dir.read(loop, 3, (e, entries) -> handle(() -> {
				for(i in 0...entries.length)
					Log.print('\t${entries[i]}');
				dir.close(loop, handle(() -> {
					Log.print('Done');
					actions.next();
				}));
			})(e));
		})(e));
	}
}