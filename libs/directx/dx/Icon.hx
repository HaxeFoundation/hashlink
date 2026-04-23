package dx;

private typedef IconPtr = hl.Abstract<"dx_icon">;

abstract Icon(IconPtr) {
	@:hlNative("?directx","create_icon")
	public static function createIcon( width : Int, height : Int, pixels : hl.Bytes) : Icon {
		return null;
	}

	public function destroy() {
		destroyIcon(this);
	}

	@:hlNative("?directx","destroy_icon")
	static function destroyIcon( ptr : IconPtr) {
	}

}
