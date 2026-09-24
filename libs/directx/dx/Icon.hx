package dx;

private typedef IconPtr = hl.Abstract<"dx_icon">;

abstract Icon(IconPtr) {
	@:hlNative("?directx","create_icon")
	public static function createIcon( width : Int, height : Int, pixels : hl.Bytes) : Icon {
		return null;
	}

	@:hlNative("?directx", "load_icon")
	/**
		Loads a .ico file from the given path
		Pass -1 to width and height to get the preferred icon size for the os, 0
		to load the highest resolution in the icon file, or the actual wanted size.
	**/
	public static function loadIcon(path: hl.Bytes, width: Int, height: Int) : Icon {
		return null;
	}

	public function destroy() {
		destroyIcon(this);
	}

	@:hlNative("?directx","destroy_icon")
	static function destroyIcon( ptr : IconPtr) {
	}

}
