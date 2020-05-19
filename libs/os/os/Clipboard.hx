package os;

import haxe.Exception;

typedef ClipboardImageSpec = {
	width : Int,
	height : Int,
	bitsPerPixel : Int,
	bytesPerRow : Int,
	redMask : Int,
	greenMask : Int,
	blueMask : Int,
	alphaMask : Int,
	redShift : Int,
	greenShift : Int,
	blueShift : Int,
	alphaShift : Int
}

class ClipboardImage {
	public var spec : ClipboardImageSpec;
	public var data : hl.Bytes;

	public function new (data, spec) {
		this.data = data;
		this.spec = spec;
	}

	public inline function getI32ByteLength () {
		return spec.width * spec.height * 4;
	}

	public function convertDataToI32 () {
		if(spec.bitsPerPixel == 32) {
			return;
		}

		var bpr = spec.width * 4;

		var bytes = new hl.Bytes(spec.height * bpr);

		for(y in 0...spec.height) {
			for(x in 0...spec.width) {
				var pos = y * bpr + x * 4;

				bytes.setI32(pos, getPixel(x, y));
			}
		}

		spec.bitsPerPixel = 32;
		spec.bytesPerRow = bpr;

		this.data = bytes;
	}

	public function getPixel (x : Int, y : Int) : Int {
		var b = Std.int(spec.bitsPerPixel / 8);
		switch (this.spec.bitsPerPixel) {
			case 32:
				return data.getI32(y * spec.bytesPerRow + x * b);
			case 24:
				var pos = y * spec.bytesPerRow + x * 3;
				return (0xFF000000 | data.getI32(pos));
			case _:
				throw new Exception("Unhandled bitdepth.");
		}
	}
	
	public function toClipboard () {
		Clipboard.setClipboardImage(this.data, this.spec);
	}
}

@:hlNative("os")
class Clipboard {
	static function get_clipboard_text():hl.Bytes {
		return null;
	}

	public static function getClipboardText() {
		return @:privateAccess String.fromUTF8(get_clipboard_text());
	}

	static function set_clipboard_text(text:hl.Bytes):Bool {
		return false;
	}

	public static function setClipboardText(text:String) {
		return @:privateAccess set_clipboard_text(text.toUtf8());
	}

	@:hlNative("os", "get_clipboard_image_data")
	static function getClipboardImageData() : hl.Bytes {
		return null;
	}

	public static function getClipboardImage () : ClipboardImage {
		var imgData = getClipboardImageData();
		var imgSpec = getClipboardImageSpec();
		
		if(imgData == null || imgSpec == null) {
			return null;
		}

		return new ClipboardImage(imgData, imgSpec);
	}
	
	static function set_clipboard_image(data : hl.Bytes, w : Int, h : Int, bpp : Int, bpr : Int, rmask : Int, gmask : Int, bmask : Int, amask : Int, rshift : Int, gshift : Int, bshift : Int, ashift : Int) : Bool {
		return false;
	}

	public static function setClipboardImage(imageData:hl.Bytes, imageSpec : ClipboardImageSpec) {
		return set_clipboard_image(
			imageData, imageSpec.width, imageSpec.height,
			imageSpec.bitsPerPixel, imageSpec.bytesPerRow,
			imageSpec.redMask, imageSpec.greenMask, imageSpec.blueMask, imageSpec.alphaMask,
			imageSpec.redShift, imageSpec.greenShift, imageSpec.blueShift, imageSpec.alphaShift
		);
	}

	static function get_clipboard_image_spec (w : hl.Ref<Int>, h : hl.Ref<Int>, bpp : hl.Ref<Int>, bpr : hl.Ref<Int>, rmask : hl.Ref<Int>, gmask : hl.Ref<Int>, bmask : hl.Ref<Int>, amask : hl.Ref<Int>, rshift : hl.Ref<Int>, gshift : hl.Ref<Int>, bshift : hl.Ref<Int>, ashift : hl.Ref<Int>) : Bool {
		return false;
	}

	public static function getClipboardImageSpec() : ClipboardImageSpec {
		var width = 0, height = 0, bitsPerPixel = 0, bytesPerRow = 0, redMask = 0, greenMask = 0, blueMask = 0, alphaMask = 0, 
			redShift = 0, greenShift = 0, blueShift = 0, alphaShift = 0;

		if(get_clipboard_image_spec(width, height, bitsPerPixel, bytesPerRow, redMask, greenMask, blueMask, alphaMask, redShift, greenShift, blueShift, alphaShift)) {

			return {
				width : width, height : height,
				bitsPerPixel : bitsPerPixel, bytesPerRow : bytesPerRow,
				redMask : redMask, greenMask : greenMask, blueMask : blueMask, alphaMask : alphaMask,
				redShift : redShift, greenShift : greenShift, blueShift : blueShift, alphaShift : alphaShift
			};
		} else {
			return null;
		}
	}

}
