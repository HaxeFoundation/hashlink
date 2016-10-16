
@:result(-169096)
class NBodies {

	var bodies : Array<Body>;

	public function new() {
		this.bodies = [
			Body.Sun(),
			Body.Jupiter(),
			Body.Saturn(),
			Body.Uranus(),
			Body.Neptune()
		];

		var px = 0.0;
		var py = 0.0;
		var pz = 0.0;

		var size = this.bodies.length;
		for( b in bodies ){
			var m = b.mass;
			px += b.vx * m;
			py += b.vy * m;
			pz += b.vz * m;
		}
		this.bodies[0].offsetMomentum(px,py,pz);
	}

	public function advance(dt: Float) {
		var size = this.bodies.length;
		for( i in 0...size ) {
			var a = bodies[i];
			for( j in i+1...size ) {
				var b = bodies[j];
				var dx = a.x - b.x;
				var dy = a.y - b.y;
				var dz = a.z - b.z;

				var distance = Math.sqrt(dx*dx + dy*dy + dz*dz);
				var mag = dt / (distance * distance * distance);

				a.vx -= dx * b.mass * mag;
				a.vy -= dy * b.mass * mag;
				a.vz -= dz * b.mass * mag;

				b.vx += dx * a.mass * mag;
				b.vy += dy * a.mass * mag;
				b.vz += dz * a.mass * mag;
			}
		}

		for( body in bodies ) {
			body.x += dt * body.vx;
			body.y += dt * body.vy;
			body.z += dt * body.vz;
		}
	}

	public function energy() {
		var e = 0.0;
		for( i in 0...bodies.length ) {
			var a = bodies[i];
			e += 0.5 * a.mass * ( a.vx * a.vx + a.vy * a.vy	+ a.vz * a.vz );
			for( j in i + 1...bodies.length ) {
				var b = bodies[j];
				var dx = b.x - a.x;
				var dy = b.y - a.y;
				var dz = b.z - a.z;
				var dist = Math.sqrt(dx*dx + dy*dy + dz*dz);
				e -= (a.mass * b.mass) / dist;
			}
		}
		return e;
	}

	public static function main() {
		var nbodies = new NBodies();
		for( i in 0...500000 )
			nbodies.advance(0.01);
		Benchs.result(Std.int(nbodies.energy() * 1000000));
	}

}

class Body {
	private static inline var PI = 3.141592653589793;
	private static inline var SOLAR_MASS = 4 * Body.PI * Body.PI;
	private static inline var DAYS_PER_YEAR = 365.24;


	public var x: Float;
	public var y: Float;
	public var z: Float;
	public var vx: Float;
	public var vy: Float;
	public var vz: Float;
	public var mass: Float;


	public function new(x,y,z,vx,vy,vz,mass) {
		this.x = x;
		this.y = y;
		this.z = z;
		this.vx = vx;
		this.vy = vy;
		this.vz = vz;
		this.mass = mass;
	}

	public static function Jupiter(){
	return new Body(
	4.84143144246472090e+00,
	-1.16032004402742839e+00,
	-1.03622044471123109e-01,
	1.66007664274403694e-03 * Body.DAYS_PER_YEAR,
	7.69901118419740425e-03 * Body.DAYS_PER_YEAR,
	-6.90460016972063023e-05 * Body.DAYS_PER_YEAR,
	9.54791938424326609e-04 * Body.SOLAR_MASS
	);
	}

	public static function Saturn(){
	return new Body(
	8.34336671824457987e+00,
	4.12479856412430479e+00,
	-4.03523417114321381e-01,
	-2.76742510726862411e-03 * Body.DAYS_PER_YEAR,
	4.99852801234917238e-03 * Body.DAYS_PER_YEAR,
	2.30417297573763929e-05 * Body.DAYS_PER_YEAR,
	2.85885980666130812e-04 * Body.SOLAR_MASS
	);
	}

	public static function Uranus(){
	return new Body(
	1.28943695621391310e+01,
	-1.51111514016986312e+01,
	-2.23307578892655734e-01,
	2.96460137564761618e-03 * Body.DAYS_PER_YEAR,
	2.37847173959480950e-03 * Body.DAYS_PER_YEAR,
	-2.96589568540237556e-05 * Body.DAYS_PER_YEAR,
	4.36624404335156298e-05 * Body.SOLAR_MASS
	);
	}

	public static function Neptune(){
	return new Body(
	1.53796971148509165e+01,
	-2.59193146099879641e+01,
	1.79258772950371181e-01,
	2.68067772490389322e-03 * Body.DAYS_PER_YEAR,
	1.62824170038242295e-03 * Body.DAYS_PER_YEAR,
	-9.51592254519715870e-05 * Body.DAYS_PER_YEAR,
	5.15138902046611451e-05 * Body.SOLAR_MASS
	);
	}

	public static function Sun(){
	return new Body(
	0,
	0,
	0,
	0,
	0,
	0,
	Body.SOLAR_MASS
	);
	}

	public function offsetMomentum(px:Float, py:Float, pz:Float): Body {
		this.vx = -px / Body.SOLAR_MASS;
		this.vy = -py / Body.SOLAR_MASS;
		this.vz = -pz / Body.SOLAR_MASS;
		return this;
	}

}
