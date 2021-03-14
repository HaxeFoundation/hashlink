package sdl;

@:hlNative("?sdl","vk_")
class Vulkan {
	public static function clearColorImage( r : Float, g : Float, b : Float, a : Float ) {
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