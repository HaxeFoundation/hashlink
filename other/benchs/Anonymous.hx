
@:result(328272)
class Anonymous {

	public static function main() {
		var a = [for( i in 0...1000 ) { x : Math.sqrt(i), y : i * Math.PI }];
		var tot = 0.;
		for( k in 0...10 ) {
			for( i in 0...a.length )
				for( j in 0...a.length ) {
					var dx = a[i].x - a[j].x;
					var dy = a[i].y - a[j].y;
					tot += Math.sqrt(dx*dx+dy*dy);
				}
		}
		Benchs.result(Std.int(tot%1000000.));
	}

}