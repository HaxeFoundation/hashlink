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

	@:hlNative("directx","dx_create")
	static function dxCreate( win : hl.Abstract < "dx_window" > , flags : Int ) : DriverInstance { return null; }

}