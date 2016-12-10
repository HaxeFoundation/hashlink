#package openal;

@:hlNative("openal","alc_")
class ALC {
	private typedef ALCdevice  = hl.Abstract<"alc_device">;
	private typedef ALCcontext = hl.Abstract<"alc_context">;

	// Context management
	public static function alcCreateContext      (device  : ALCdevice, attrlist : hl.Bytes) : ALContext { return null; }
	public static function alcMakeContextCurrent (context : ALCcontext) : Bool { return false; }
	public static function alcProcessContext     (context : ALCcontext) {}
	public static function alcSuspendContext     (context : ALCcontext) {}
	public static function alcDestroyContext     (context : ALCcontext) {}
	public static function alcGetCurrentContext  () : ALCcontext  { return null; }
	public static function alcGetContextsDevice  (context : ALCcontext) : ALCDevice { return null; }

	// Device management
	public static function alcOpenDevice  (devicename : hl.Bytes) : ALCDevice { return null; }
	public static function alcCloseDevice (device : ALCDevice) : Bool { return false; }

	// Error support
	public static function alcGetError (device : ALCDevice) : Int { return 0; }

	// Extension support
	public static function alcIsExtensionPresent (device : ALCDevice, extname : hl.Bytes)  : Bool { return false; }
	public static function alcGetEnumValue       (device : ALCDevice, enumname : hl.Bytes) : Int  { return 0; }
	// public static function alcGetProcAddress(device : ALCDevice, const ALCchar *funcname);

	// Query function
	public static function alcGetString   (device : ALCDevice, param : Int) : hl.Bytes { return null; }
	public static function alcGetIntegerv (device : ALCDevice, param : Int, size : Int, values : hl.Bytes) {}

	// Capture function
	public static function alcCaptureOpenDevice  (devicename : hl.Bytes, frequency : Int, format : Int, buffersize : Int) : ALCDevice { return null; }
	public static function alcCaptureCloseDevice (device : ALCDevice) : Bool { return false; }
	public static function alcCaptureStart       (device : ALCDevice) {}
	public static function alcCaptureStop        (device : ALCDevice) {}
	public static function alcCaptureSamples     (device : ALCDevice, buffer : hl.Bytes, samples : Int) {}

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
