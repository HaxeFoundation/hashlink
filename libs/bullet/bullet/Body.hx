package bullet;

@:hlNative("bullet")
class Body {

	var state : Native.MotionState;
	var inst : Native.RigidBody;
	var _pos = new h3d.col.Point();
	var _q = new h3d.Quat();
	var _tmp = new Array<Float>();

	public var shape(default,null) : Shape;
	public var mass(default,null) : Float;
	public var position(get,never) : h3d.col.Point;
	public var rotation(get,never) : h3d.Quat;

	public function new( shape : Shape, mass : Float ) {
		var inertia = new Native.Vector3(0, 0, 0);
		state = new Native.DefaultMotionState();
		var inf = new Native.RigidBodyConstructionInfo(mass, state, @:privateAccess shape.getInstance(), inertia);
		inst = new Native.RigidBody(inf);
		this.shape = shape;
		this.mass = mass;
		_tmp[6] = 0.;
	}

	public function setTransform( p : h3d.col.Point, ?q : h3d.Quat ) {
		var t = inst.getCenterOfMassTransform();
		t.setOrigin(new Native.Vector3(p.x, p.y, p.z));
		if( q != null ) t.setRotation(new Native.Quaternion(q.x, q.y, q.z, q.w));
		inst.setCenterOfMassTransform(t);
	}

	function get_position() {
		var t = inst.getCenterOfMassTransform();
		var p = t.getOrigin();
		_pos.set(p.x(), p.y(), p.z());
		return _pos;
	}

	function get_rotation() {
		var t = inst.getCenterOfMassTransform();
		var q : Native.QuadWord = t.getRotation();
		_q.set(q.x(), q.y(), q.z(), q.w());
		return _q;
	}

}
