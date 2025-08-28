package openal;
import openal.ALC;

@:hlNative("?openal","alc_")
extern class ExtALC {
	// ----------------------------------------------------------------------------
	// ALC_LOKI_audio_channel
	// ----------------------------------------------------------------------------

	public static inline var CHAN_MAIN_LOKI                   = 0x500001;
	public static inline var CHAN_PCM_LOKI                    = 0x500002;
	public static inline var CHAN_CD_LOKI                     = 0x500003;

	// ----------------------------------------------------------------------------
	// ALC_EXT_disconnect
	// ----------------------------------------------------------------------------

	public static inline var CONNECTED                        = 0x313;

	// ----------------------------------------------------------------------------
	// ALC_EXT_thread_local_context
	// ----------------------------------------------------------------------------

	public static function setThreadContext(context : Context) : Bool;
	public static function getThreadContext() : Context;

	// ----------------------------------------------------------------------------
	// ALC_SOFT_loopback
	// ----------------------------------------------------------------------------

	public static inline var FORMAT_CHANNELS_SOFT             = 0x1990;
	public static inline var FORMAT_TYPE_SOFT                 = 0x1991;

	// Sample types
	public static inline var BYTE_SOFT                        = 0x1400;
	public static inline var UNSIGNED_BYTE_SOFT               = 0x1401;
	public static inline var SHORT_SOFT                       = 0x1402;
	public static inline var UNSIGNED_SHORT_SOFT              = 0x1403;
	public static inline var INT_SOFT                         = 0x1404;
	public static inline var UNSIGNED_INT_SOFT                = 0x1405;
	public static inline var FLOAT_SOFT                       = 0x1406;

	// Channel configurations
	public static inline var MONO_SOFT                        = 0x1500;
	public static inline var STEREO_SOFT                      = 0x1501;
	public static inline var QUAD_SOFT                        = 0x1503;
	public static inline var _5POINT1_SOFT                    = 0x1504;
	public static inline var _6POINT1_SOFT                    = 0x1505;
	public static inline var _7POINT1_SOFT                    = 0x1506;


	public static function loopbackOpenDeviceSoft      (devicename : hl.Bytes) : Device;
	public static function isRenderFormatSupportedSoft (device : Device, freq : Int, channels : Int, type : Int) : Bool;
	public static function renderSamplesSoft           (device : Device, buffer : hl.Bytes, samples : Int) : Void;

	// ----------------------------------------------------------------------------
	// ALC_EXT_DEFAULT_FILTER_ORDER
	// ----------------------------------------------------------------------------

	public static inline var DEFAULT_FILTER_ORDER             = 0x1100;

	// ----------------------------------------------------------------------------
	// ALC_SOFT_pause_device
	// ----------------------------------------------------------------------------

	public static function devicePauseSoft  (device : Device) : Void;
	public static function deviceResumeSoft (device : Device) : Void;

	// ----------------------------------------------------------------------------
	// ALC_SOFT_HRTF
	// ----------------------------------------------------------------------------

	public static inline var HRTF_SOFT                        = 0x1992;
	public static inline var DONT_CARE_SOFT                   = 0x0002;
	public static inline var HRTF_STATUS_SOFT                 = 0x1993;
	public static inline var HRTF_DISABLED_SOFT               = 0x0000;
	public static inline var HRTF_ENABLED_SOFT                = 0x0001;
	public static inline var HRTF_DENIED_SOFT                 = 0x0002;
	public static inline var HRTF_REQUIRED_SOFT               = 0x0003;
	public static inline var HRTF_HEADPHONES_DETECTED_SOFT    = 0x0004;
	public static inline var HRTF_UNSUPPORTED_FORMAT_SOFT     = 0x0005;
	public static inline var NUM_HRTF_SPECIFIERS_SOFT         = 0x1994;
	public static inline var HRTF_SPECIFIER_SOFT              = 0x1995;
	public static inline var HRTF_ID_SOFT                     = 0x1996;

	public static function getStringiSoft  (device : Device, param : Int, index : Int) : hl.Bytes;
	public static function resetDeviceSoft (device : Device, attribs : hl.Bytes) : Bool;

	// ----------------------------------------------------------------------------
	// ALC_SOFT_reopen_device
	// ----------------------------------------------------------------------------

	public static inline var SOFT_reopen_device = "ALC_SOFT_reopen_device";
	public static function reopenDeviceSoft (device : Device, deviceName : hl.Bytes, attribs : hl.Bytes) : Bool;
}
