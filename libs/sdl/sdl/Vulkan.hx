package sdl;

enum abstract ShaderKind(Int) {
	var Vertex = 0;
	var Fragment = 1;
}

@:hlNative("?sdl","vk_")
class Vulkan {
	public static function clearColorImage( r : Float, g : Float, b : Float, a : Float ) {
	}

	public static function compileShader( source : String, mainFunction : String, kind : ShaderKind ) {
		var outSize = -1;
		var fileName = "";
		var bytes = @:privateAccess compile_shader(source.toUtf8(), fileName.toUtf8(), mainFunction.toUtf8(), kind, outSize);
		if( outSize < 0 ) {
			var error = @:privateAccess String.fromUTF8(bytes);
			var lines = source.split("\n");
			throw error+"\n\nin\n\n"+[for( i => l in lines ) StringTools.rpad((i+1)+":"," ",8)+l].join("\n");
		}
		return @:privateAccess new haxe.io.Bytes(bytes, outSize);
	}

	static function compile_shader( source : hl.Bytes, shaderFile : hl.Bytes, mainFunction : hl.Bytes, kind : ShaderKind, outSize : hl.Ref<Int> ) : hl.Bytes {
		return null;
	}

}


@:hlNative("?sdl","vk_")
abstract VKContext(hl.Abstract<"vk_context">) {

	public function initSwapchain( width : Int, height : Int ) : Bool {
		return false;
	}

	public function beginFrame() : Bool {
		return false;
	}

	public function setCurrent() {
	}

	public function endFrame() {
	}

}