package bullet;

@:enum abstract Axis(Int) {
	var X = 0;
	var Y = 1;
	var Z = 1;
}

abstract Shape(hl.Abstract<"bshape">) {

	@:hlNative("bullet","create_box_shape")
	public static function createBox( sizeX : Float, sizeY : Float, sizeZ : Float ) : Shape {
		return null;
	}

	@:hlNative("bullet","create_sphere_shape")
	public static function createSphere( radius : Float ) : Shape {
		return null;
	}

	@:hlNative("bullet","create_capsule_shape")
	public static function createCapsule( axis : Axis, radius : Float, length : Float ) : Shape {
		return null;
	}

	@:hlNative("bullet","create_cylinder_shape")
	public static function createCylinder( axis : Axis, sizeX : Float, sizeY : Float, sizeZ : Float ) : Shape {
		return null;
	}

	@:hlNative("bullet","create_cone_shape")
	public static function createCone( axis : Axis, radius : Float, length : Float ) : Shape {
		return null;
	}

	public static function createCompound( shapes : Array<{ shape : Shape, mass : Float, position : h3d.col.Point, rotation : h3d.Quat }> ) {
		var svalues = new hl.NativeArray<Shape>(shapes.length);
		var arr = new Array<Float>();
		var pos = 0;
		for( i in 0...shapes.length ) {
			var s = shapes[i];
			svalues[i] = s.shape;
			arr[pos++] = s.position.x;
			arr[pos++] = s.position.y;
			arr[pos++] = s.position.z;
			arr[pos++] = s.rotation.x;
			arr[pos++] = s.rotation.y;
			arr[pos++] = s.rotation.z;
			arr[pos++] = s.rotation.w;
			arr[pos++] = s.mass;
		}
		var shape = _createCoumpound(svalues, hl.Bytes.getArray(arr));
		return { shape : shape, rotation : new h3d.Quat(arr[3],arr[4],arr[5],arr[6]), position : new h3d.col.Point(arr[0], arr[1], arr[2]) };
	}

	@:hlNative("bullet", "create_compound_shape")
	static function _createCoumpound( shapes : hl.NativeArray<Shape>, data : hl.Bytes ) : Shape {
		return null;
	}

}
