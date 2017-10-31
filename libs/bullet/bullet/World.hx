package bullet;

typedef WorldInst = hl.Abstract<"bworld">

@:hlNative("bullet")
class World {

	var inst : WorldInst;
	var bodies : Array<Body> = [];

	public function new() {
		inst = createWorld();
	}
	
	public function setGravity( x : Float, y : Float, z : Float ) {
		set_gravity(inst, x, y, z);
	}
	
	public function stepSimulation( time : Float, iterations : Int ) {
		step_simulation(inst, time, iterations);
	}
	
	public function addRigidBody( b : Body ) {
		if( bodies.indexOf(b) >= 0 ) throw "Body already in world";
		bodies.push(b);
		add_rigid_body(inst, @:privateAccess b.inst);
	}
	
	public function removeRigidBody( b : Body ) {
		if( !bodies.remove(b) ) return;
		remove_rigid_body(inst, @:privateAccess b.inst);
	}
	
	static function createWorld() : WorldInst {
		return null;
	}
	
	static function set_gravity( w : WorldInst, x : Float, y : Float, z : Float ) {
	}
	
	static function step_simulation( w : WorldInst, time : Float, iterations : Int ) {
	}
	
	static function add_rigid_body( w : WorldInst, b : Body.BodyInst ) {
	}

	static function remove_rigid_body( w : WorldInst, b : Body.BodyInst ) {
	}

}
