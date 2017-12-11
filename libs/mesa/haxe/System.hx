package haxe;

class System {

	public static var width = 800;
	public static var height = 600;
	public static var vsync = true;

	public static var buffer : haxe.io.Bytes = null;
	public static var ctx : mesa.Context;

	public static var name = "OSMesa";

	public static function init() {
		var attribs : Array<Int> = [
			mesa.Context.FORMAT, mesa.Context.RGBA,
			mesa.Context.DEPTH_BITS, 16,
			mesa.Context.STENCIL_BITS, 8,
			mesa.Context.ACCUM_BITS, 0,
			mesa.Context.PROFILE, mesa.Context.CORE_PROFILE,
			mesa.Context.CONTEXT_MAJOR_VERSION, 3,
			mesa.Context.CONTEXT_MINOR_VERSION, 2,
			0, 0,
		];
		ctx = mesa.Context.create(hl.Bytes.getArray(attribs), null);
		if( ctx == null )
			throw "Failed to create Mesa context";
		if( buffer == null )
			buffer = haxe.io.Bytes.alloc(width*height*4);
		if( !ctx.makeCurrent(buffer, mesa.GL.UNSIGNED_BYTE, width, height) )
			throw "Failed to make Mesa current context";
		if( !mesa.GL.init() )
			throw "Failed to init GL API";
		return true;
	}
	
	@:extern public static inline function beginFrame() {
	}
	
	public static function emitEvents(_) {
		return true;
	}

	public static function present() {
	}

}