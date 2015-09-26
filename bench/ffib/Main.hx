class Main {

	static function ffib(n) {
		if( n <= 1. ) return 1.;
		return ffib(n-1.) + ffib(n-2.);
	}

	static function main() {
		Sys.println(ffib(39.));
	}

}