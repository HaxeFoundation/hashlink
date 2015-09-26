class Main {

	static function fib(n) {
		if( n <= 1 ) return 1;
		return fib(n-1) + fib(n-2);
	}

	static function main() {
		Sys.println(fib(39));
	}

}