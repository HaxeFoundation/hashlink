
@:result(9227465)
class Fib {

	static function fib( n : Int ) {
		if( n <= 1 ) return 1;
		return fib(n - 1) + fib(n - 2);
	}

	public static function main() {
		Benchs.result(fib(34));
	}

}