class Threads {

	static var WAIT = [];
	
	@:hlNative("std","thread_create") static function thread_create( f : Void -> Void ) : hl.Abstract<"hl_thread"> {
		return null;
	}

	static function run(i,k) {
		for( i in 0...Math.ceil(1000000/k) )
			new hl.Bytes(1);
		if( i>=0 ) WAIT[i] = false; 
	}
	
    public static function main() {

		hl.Gc.enable(false); // disable major gc

		var flags = hl.Gc.flags;
		flags.set(NoThreads);
		hl.Gc.flags = flags;
		
		var t0 = Sys.time();
		run(-1,1);
		trace("Threads disable "+(Sys.time() - t0));
		
		flags.unset(NoThreads);
		hl.Gc.flags = flags;
	
		var t0 = Sys.time();
		run(-1,1);
		trace("Single thread "+(Sys.time() - t0));
		
		for( COUNT in [2,4,10] ) {
			for( i in 0...COUNT ) 
				WAIT[i] = true;
			t0 = Sys.time();
			for( i in 0...COUNT ) 			
				thread_create(run.bind(i,COUNT));
			var i = 0;
			while( i < COUNT ) {
				if( WAIT[i] ) {
					i = 0;
					Sys.sleep(0);
				} else i++;
			}
			trace(COUNT+" threads "+(Sys.time() - t0));
		}
    }
	
}