
@:result(11000)
class IntArray {

	public static function main() {
		var a : Array<Int> = [for( i in 0...10000 ) i];
		for( k in 0...1000 ) {
			for( i in 0...a.length )
				a[i] += i;
			for( i in 1...a.length )
				a[i] = Std.int(a[i] / i);
		}
		var tot = 0;
		for( v in a )
			tot += v;
		Benchs.result(tot);
	}

}