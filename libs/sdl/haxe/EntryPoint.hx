package haxe;

private class Lock {
	public function new() {
	}
	public inline function release() {
	}
	public inline function wait( ?t : Float ) {
	}
}
private class Mutex {
	public function new() {
	}
	public inline function acquire() {
	}
	public inline function release() {
	}
}
private class Thread {
	public static function create( f : Void -> Void ) {
		f();
	}
}

class EntryPoint {

	static var sleepLock = new Lock();
	static var mutex = new Mutex();
	static var pending = new Array<Void->Void>();

	public static var threadCount(default,null) : Int = 0;

	public static function wakeup() {
		#if sys
		sleepLock.release();
		#end
	}

	public static function runInMainThread( f : Void -> Void ) {
		mutex.acquire();
		pending.push(f);
		mutex.release();
		wakeup();
	}

	public static function addThread( f : Void -> Void ) {
		mutex.acquire();
		threadCount++;
		mutex.release();
		Thread.create(function() {
			f();
			mutex.acquire();
			threadCount--;
			if( threadCount == 0 ) wakeup();
			mutex.release();
		});
	}

	static function processEvents() : Float {
		// flush all pending calls
		while( true ) {
			mutex.acquire();
			var f = pending.shift();
			mutex.release();
			if( f == null ) break;
			f();
		}
		if( !MainLoop.hasEvents() && threadCount == 0 )
			return -1;
		return @:privateAccess MainLoop.tick();
	}

	@:keep public static function run() @:privateAccess {
		sdl.Sdl.loop(function() processEvents());
		sdl.Sdl.quit();
	}

}
