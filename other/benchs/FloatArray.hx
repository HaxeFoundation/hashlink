
@:result(1000509)
class FloatArray {

	public static function main() {
		var a : Array<Float> = [for( i in 0...10000 ) 1 / (i + 1)];
		for( k in 0...400 ) {
			for( i in 0...a.length )
				a[i] += i;
			for( i in 1...a.length )
				a[i] /= i;
			for( i in 0...a.length )
				a[i] = Math.sqrt(a[i]);
		}
		var tot = 0.;
		for( v in a )
			tot += v;
		Benchs.result(Std.int(tot*100));
	}

}