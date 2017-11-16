package bullet;

@:hlNative("bullet")
class World {

	var config : Native.DefaultCollisionConfiguration;
	var dispatch : Native.Dispatcher;
	var broad : Native.BroadphaseInterface;
	var solver : Native.ConstraintSolver;
	var inst : Native.DiscreteDynamicsWorld;
	var bodies : Array<Body> = [];

	public function new() {
		config = new Native.DefaultCollisionConfiguration();
		dispatch = new Native.CollisionDispatcher(config);
		broad = new Native.DbvtBroadphase();
		solver = new Native.SequentialImpulseConstraintSolver();
		inst = new Native.DiscreteDynamicsWorld(dispatch, broad, solver, config);
	}

	public function setGravity( x : Float, y : Float, z : Float ) {
		inst.setGravity(new Native.Vector3(x, y, z));
	}

	public function stepSimulation( time : Float, iterations : Int ) {
		inst.stepSimulation(time, iterations);
	}

	public function addRigidBody( b : Body ) {
		if( bodies.indexOf(b) >= 0 ) throw "Body already in world";
		bodies.push(b);
		inst.addRigidBody(@:privateAccess b.inst,1,1);
	}

	public function removeRigidBody( b : Body ) {
		if( !bodies.remove(b) ) return;
		inst.removeRigidBody(@:privateAccess b.inst);
	}

}
