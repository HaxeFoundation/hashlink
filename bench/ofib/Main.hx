
class Main {

    static function main() {
		Sys.println(new Sub().mfib(40));
    }
	
	var one = 1;
	var two = 2;
	var mfib3 : Int -> Int;

	function new() {
		mfib3 = mfib;
	}

    function mfib(x) {
		if( x <= 2 ) return 1;
		return mfib2(x-one) + mfib3(x-two);
    }
	function mfib2(x) {
		return mfib(x);
	}

}

class Sub extends Main {

	override function mfib2(x) {
		return mfib(x);
	}

}

