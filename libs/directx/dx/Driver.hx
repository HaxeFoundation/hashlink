package dx;

enum DriverInitFlag {
	SingleThread;
	DebugLayer;
	SwitchToRef;
	PreventInternalThreadingOptimizations;
	Unused1;
	BgraSupport;
	Debuggable;
	PreventAlteringLayerSettingsFromRegistry;
	DisableGpuTimeout;
	Unused2;
	Unused3;
	VideoSupport;
}

typedef DriverInstance = hl.Abstract<"dx_driver">;

@:hlNative("directx","dx_")
class Driver {

	public static function create( win : Window, ?flags : haxe.EnumFlags<DriverInitFlag> ) {
		return dxCreate(@:privateAccess win.win, flags == null ? 0 : flags.toInt());
	}

	public static function clearColor( r : Float, g : Float, b : Float, a : Float ) {
	}

	public static function present() {
	}

	public static function getDeviceName() {
		return @:privateAccess String.fromUCS2(dxGetDeviceName());
	}

	public static function getScreenWidth() {
		return 0;
	}

	public static function getScreenHeight() {
		return 0;
	}

	public static function getSupportedVersion() : Float {
		return 0.;
	}

	@:hlNative("directx","dx_create")
	static function dxCreate( win : hl.Abstract < "dx_window" > , flags : Int ) : DriverInstance { return null; }
	@:hlNative("directx","dx_get_device_name")
	static function dxGetDeviceName() : hl.Bytes { return null; }

}