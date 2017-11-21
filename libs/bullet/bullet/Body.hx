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
		inertia.delete();
		inf.delete();
		this.shape = shape;
		this.mass = mass;
		_tmp[6] = 0.;
	}

	public function setTransform( p : h3d.col.Point, ?q : h3d.Quat ) {
		var t = inst.getCenterOfMassTransform();
		var v = new Native.Vector3(p.x, p.y, p.z);
		t.setOrigin(v);
		v.delete();
		if( q != null ) {
			var qv = new Native.Quaternion(q.x, q.y, q.z, q.w);
			t.setRotation(qv);
			qv.delete();
		}
		inst.setCenterOfMassTransform(t);
		t.delete();
	}

	public function delete() {
		inst.delete();
		state.delete();
	}

	function get_position() {
		var t = inst.getCenterOfMassTransform();
		var p = t.getOrigin();
		_pos.set(p.x(), p.y(), p.z());
		t.delete();
		p.delete();
		return _pos;
	}

	function get_rotation() {
		var t = inst.getCenterOfMassTransform();
		var q = t.getRotation();
		var qw : Native.QuadWord = q;
		_q.set(qw.x(), qw.y(), qw.z(), qw.w());
		q.delete();
		t.delete();
		return _q;
	}

}
