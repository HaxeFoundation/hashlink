import hl.uv.Version;
import hl.uv.UVException;
import sys.thread.Thread;

class VersionSample {
	static function print(msg:String) {
		Log.print('VersionSample: $msg');
	}

	public static function main() {
		print('string ' + Version.string());
		print('major ' + Version.major);
		print('minor ' + Version.minor);
		print('patch ' + Version.patch);
		print('isRelease ' + Version.isRelease);
		print('suffix "${Version.suffix}"');
		print('hex ' + Version.hex);
	}
}