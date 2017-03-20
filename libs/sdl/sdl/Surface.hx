package sdl;

private typedef SurfacePtr = hl.Abstract<"sdl_surface">;

@:hlNative("sdl")
abstract Surface(SurfacePtr) {

	public inline function free() {
		freeSurface(this);
		this = null;
	}

	static function freeSurface(p:SurfacePtr) {
	}

	public static function fromBGRA( pixels, width, height ) {
		return from(pixels, width, height, 32, width * 4, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
	}

	@:hlNative("sdl","surface_from")
	public static function from( pixels : hl.Bytes,  width : Int, height : Int, depth : Int, pitch : Int, rmask : Int, gmask : Int, bmask : Int, amask : Int ) : Surface {
		return null;
	}

}
