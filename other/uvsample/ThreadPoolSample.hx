import hl.uv.ThreadPool;
import hl.Bytes;
import hl.uv.UVException;
import sys.thread.Thread;
import hl.uv.Misc;

class ThreadPoolSample {
	public static function main() {
		function print(msg:String)
			Log.print('ThreadPoolSample: $msg');
		ThreadPool.queueWork(Thread.current().events, () -> print('message from pool'), e -> switch e {
			case UV_NOERR: print('Done');
			case _: throw new UVException(e);
		});
	}
}