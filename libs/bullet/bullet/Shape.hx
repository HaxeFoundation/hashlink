package bullet;

abstract Shape(hl.Abstract<"bshape">) {
	
	@:hlNative("bullet","create_box_shape")
	public static function createBox( sizeX : Float, sizeY : Float, sizeZ : Float ) : Shape {
		return null;
	}

	@:hlNative("bullet","create_sphere_shape")
	public static function createSphere( radius : Float ) : Shape {
		return null;
	}

}
