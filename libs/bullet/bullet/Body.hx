package bullet;

typedef BodyInst = hl.Abstract<"bbody">

@:hlNative("bullet")
class Body {

	var inst : BodyInst;
	var _pos = new h3d.col.Point();
	var _q = new h3d.Quat();
	var _tmp = new Array<Float>();
	
	public var shape(default,null) : Shape;
	public var mass(default,null) : Float;
	public var position(get,never) : h3d.col.Point;
	public var rotation(get,never) : h3d.Quat;

	public function new( shape : Shape, mass : Float ) {
		inst = createBody(shape, mass);
		this.shape = shape;
		this.mass = mass;
		_tmp[6] = 0.;
	}
	
	public function setTransform( p : h3d.col.Point, ?q : h3d.Quat ) {
		set_body_position(inst, p.x, p.y, p.z);
		if( q != null ) set_body_rotation(inst, q.x, q.y, q.z, q.w) else set_body_rotation(inst, 0, 0, 0, 1);
	}
	
	function get_position() {
		get_body_transform(inst, hl.Bytes.getArray(_tmp));
		_pos.set(_tmp[0],_tmp[1],_tmp[2]);
		return _pos;
	}

	function get_rotation() {
		get_body_transform(inst, hl.Bytes.getArray(_tmp));
		_q.set(_tmp[3],_tmp[4],_tmp[5],_tmp[6]);
		return _q;
	}

	static function set_body_position( b : BodyInst, x : Float, y : Float, z : Float ) {
	}

	static function set_body_rotation( b : BodyInst, x : Float, y : Float, z : Float, w : Float ) {
	}

	static function get_body_transform( b : BodyInst, bytes : hl.Bytes ) {
	}
	
	static function createBody( s : Shape, mass : Float ) : BodyInst {
		return null;
	}
	
}
