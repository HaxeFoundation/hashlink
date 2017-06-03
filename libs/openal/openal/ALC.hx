package openal;

typedef Device  = hl.Abstract<"alc_device">;
typedef Context = hl.Abstract<"alc_context">;

@:hlNative("openal","alc_")
extern class ALC {
	// Context management
	public static function createContext      (device  : Device, attrlist : hl.Bytes) : Context;
	public static function makeContextCurrent (context : Context) : Bool;
	public static function processContext     (context : Context) : Void;
	public static function suspendContext     (context : Context) : Void;
	public static function destroyContext     (context : Context) : Void;
	public static function getCurrentContext  () : Context;
	public static function getContextsDevice  (context : Context) : Device;

	// Device management
	public static function openDevice  (devicename : hl.Bytes) : Device;
	public static function closeDevice (device : Device) : Bool;

	// Error support
	public static function getError (device : Device) : Int;

	// Extension support
	public static function isExtensionPresent (device : Device, extname : hl.Bytes)  : Bool;
	public static function getEnumValue       (device : Device, enumname : hl.Bytes) : Int;
	// public static function alcGetProcAddress(device : Device, const ALCchar *funcname);

	// Query function
	public static function getString   (device : Device, param : Int) : hl.Bytes;
	public static function getIntegerv (device : Device, param : Int, size : Int, values : hl.Bytes) : Void;

	// Capture function
	public static function captureOpenDevice  (devicename : hl.Bytes, frequency : Int, format : Int, buffersize : Int) : Device;
	public static function captureCloseDevice (device : Device) : Bool;
	public static function captureStart       (device : Device) : Void;
	public static function captureStop        (device : Device) : Void;
	public static function captureSamples     (device : Device, buffer : hl.Bytes, samples : Int) : Void;

	// ------------------------------------------------------------------------
	// Constants
	// ------------------------------------------------------------------------

	public static inline var ALC_FALSE                            = 0;
	public static inline var ALC_TRUE                             = 1;

	// Context attributes
	public static inline var ALC_FREQUENCY                        = 0x1007;
	public static inline var ALC_REFRESH                          = 0x1008;
	public static inline var ALC_SYNC                             = 0x1009;
	public static inline var ALC_MONO_SOURCES                     = 0x1010;
	public static inline var ALC_STEREO_SOURCES                   = 0x1011;

	// Errors
	public static inline var ALC_NO_ERROR                         = 0;
	public static inline var ALC_INVALID_DEVICE                   = 0xA001;
	public static inline var ALC_INVALID_CONTEXT                  = 0xA002;
	public static inline var ALC_INVALID_ENUM                     = 0xA003;
	public static inline var ALC_INVALID_VALUE                    = 0xA004;
	public static inline var ALC_OUT_OF_MEMORY                    = 0xA005;

	// Runtime ALC version
	public static inline var ALC_MAJOR_VERSION                    = 0x1000;
	public static inline var ALC_MINOR_VERSION                    = 0x1001;

	// Context attribute list properties
	public static inline var ALC_ATTRIBUTES_SIZE                  = 0x1002;
	public static inline var ALC_ALL_ATTRIBUTES                   = 0x1003;

	// Device strings
	public static inline var ALC_DEFAULT_DEVICE_SPECIFIER         = 0x1004;
	public static inline var ALC_DEVICE_SPECIFIER                 = 0x1005;
	public static inline var ALC_EXTENSIONS                       = 0x1006;

	// Capture extension
	public static inline var ALC_EXT_CAPTURE                      = 1;
	public static inline var ALC_CAPTURE_DEVICE_SPECIFIER         = 0x310;
	public static inline var ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER = 0x311;
	public static inline var ALC_CAPTURE_SAMPLES                  = 0x312;

	// Enumerate All extension
	public static inline var ALC_ENUMERATE_ALL_EXT                = 1;
	public static inline var ALC_DEFAULT_ALL_DEVICES_SPECIFIER    = 0x1012;
	public static inline var ALC_ALL_DEVICES_SPECIFIER            = 0x1013;
}
