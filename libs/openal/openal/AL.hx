package openal;

abstract Buffer(Int) {
	public inline function toInt() : Int { return this; }
	public static inline function ofInt( v : Int ) : Buffer { return cast v; }
}

abstract Source(Int) {
	public inline function toInt() : Int { return this; }
	public static inline function ofInt( v : Int ) : Source { return cast v; }
}

@:hlNative("openal","al_")
extern class AL {
	public static function dopplerFactor   (value : hl.F32) : Void;
	public static function dopplerVelocity (value : hl.F32) : Void;
	public static function speedOfSound    (value : hl.F32) : Void;
	public static function distanceModel   (distanceModel : Int) : Void;

	// Renderer State management
	public static function enable    (capability : Int) : Void;
	public static function disable   (capability : Int) : Void;
	public static function isEnabled (capability : Int) : Bool;

	// State retrieval
	public static function getBooleanv (param : Int, values : hl.Bytes) : Void;
	public static function getIntegerv (param : Int, values : hl.Bytes) : Void;
	public static function getFloatv   (param : Int, values : hl.Bytes) : Void;
	public static function getDoublev  (param : Int, values : hl.Bytes) : Void;

	public static function getString   (param : Int) : hl.Bytes;
	public static function getBoolean  (param : Int) : Bool;
	public static function getInteger  (param : Int) : Int;
	public static function getFloat    (param : Int) : hl.F32;
	public static function getDouble   (param : Int) : hl.F64;

	// Error retrieval
	public static function getError() : Int;

	// Extension support
	public static function loadExtensions     () : Void;
	public static function isExtensionPresent (extname : hl.Bytes) : Bool;
	public static function getEnumValue       (ename   : hl.Bytes) : Int;

	// Set Listener parameters
	public static function listenerf  (param : Int, value  : hl.F32) : Void;
	public static function listener3f (param : Int, value1 : hl.F32, value2 : hl.F32, value3 : hl.F32) : Void;
	public static function listenerfv (param : Int, values : hl.Bytes) : Void;
	public static function listeneri  (param : Int, value  : Int) : Void;
	public static function listener3i (param : Int, value1 : Int, value2 : Int, value3 : Int) : Void;
	public static function listeneriv (param : Int, values : hl.Bytes) : Void;

	// Get Listener parameters
	public static function getListenerf  (param : Int) : hl.F32;
	public static function getListener3f (param : Int, value1 : hl.Ref<hl.F32>, value2 : hl.Ref<hl.F32>, value3 : hl.Ref<hl.F32>) : Void;
	public static function getListenerfv (param : Int, values : hl.Bytes) : Void;
	public static function getListeneri  (param : Int) : Int;
	public static function getListener3i (param : Int, value1 : hl.Ref<Int>, value2 : hl.Ref<Int>, value3 : hl.Ref<Int>) : Void;
	public static function getListeneriv (param : Int, values : hl.Bytes) : Void;

	// Source management
	public static function genSources    (n : Int, sources : hl.Bytes) : Void;
	public static function deleteSources (n : Int, sources : hl.Bytes) : Void;
	public static function isSource      (source : Source) : Bool;

	// Set Source parameters
	public static function sourcef  (source : Source, param : Int, value  : hl.F32) : Void;
	public static function source3f (source : Source, param : Int, value1 : hl.F32, value2 : hl.F32, value3 : hl.F32) : Void;
	public static function sourcefv (source : Source, param : Int, values : hl.Bytes) : Void;
	public static function sourcei  (source : Source, param : Int, value  : Int) : Void;
	public static function source3i (source : Source, param : Int, value1 : Int, value2 : Int, value3 : Int) : Void;
	public static function sourceiv (source : Source, param : Int, values : hl.Bytes) : Void;

	// Get Source parameters
	public static function getSourcef  (source : Source, param : Int) : hl.F32;
	public static function getSource3f (source : Source, param : Int, value1 : hl.Ref<hl.F32>, value2 : hl.Ref<hl.F32>, value3 : hl.Ref<hl.F32>) : Void;
	public static function getSourcefv (source : Source, param : Int, values : hl.Bytes) : Void;
	public static function getSourcei  (source : Source, param : Int) : Int;
	public static function getSource3i (source : Source, param : Int, value1 : hl.Ref<Int>, value2 : hl.Ref<Int>, value3 : hl.Ref<Int>) : Void;
	public static function getSourceiv (source : Source, param : Int, values : hl.Bytes) : Void;

	// Source controls
	public static function sourcePlayv   (n : Int, sources : hl.Bytes) : Void;
	public static function sourceStopv   (n : Int, sources : hl.Bytes) : Void;
	public static function sourceRewindv (n : Int, sources : hl.Bytes) : Void;
	public static function sourcePausev  (n : Int, sources : hl.Bytes) : Void;

	public static function sourcePlay   (source : Source) : Void;
	public static function sourceStop   (source : Source) : Void;
	public static function sourceRewind (source : Source) : Void;
	public static function sourcePause  (source : Source) : Void;

	// Queue buffers onto a source
	public static function sourceQueueBuffers   (source : Source, nb : Int, buffers : hl.Bytes) : Void;
	public static function sourceUnqueueBuffers (source : Source, nb : Int, buffers : hl.Bytes) : Void;

	// Buffer management
	public static function genBuffers    (n : Int, buffers : hl.Bytes) : Void;
	public static function deleteBuffers (n : Int, buffers : hl.Bytes) : Void;
	public static function isBuffer      (buffer : Buffer) : Bool;
	public static function bufferData    (buffer : Buffer, format : Int, data : hl.Bytes, size : Int, freq : Int) : Void;

	// Set Buffer parameters
	public static function bufferf  (buffer : Buffer, param : Int, value  : hl.F32) : Void;
	public static function buffer3f (buffer : Buffer, param : Int, value1 : hl.F32, value2 : hl.F32, value3 : hl.F32) : Void;
	public static function bufferfv (buffer : Buffer, param : Int, values : hl.Bytes) : Void;
	public static function bufferi  (buffer : Buffer, param : Int, value  : Int) : Void;
	public static function buffer3i (buffer : Buffer, param : Int, value1 : Int, value2 : Int, value3 : Int) : Void;
	public static function bufferiv (buffer : Buffer, param : Int, values : hl.Bytes) : Void;

	// Get Buffer parameters
	public static function getBufferf  (buffer : Buffer, param : Int) : hl.F32;
	public static function getBuffer3f (buffer : Buffer, param : Int, value1 : hl.Ref<hl.F32>, value2 : hl.Ref<hl.F32>, value3 : hl.Ref<hl.F32>) : Void;
	public static function getBufferfv (buffer : Buffer, param : Int, values : hl.Bytes) : Void;
	public static function getBufferi  (buffer : Buffer, param : Int) : Int;
	public static function getBuffer3i (buffer : Buffer, param : Int, value1 : hl.Ref<Int>, value2 : hl.Ref<Int>, value3 : hl.Ref<Int>) : Void;
	public static function getBufferiv (buffer : Buffer, param : Int, values : hl.Bytes) : Void;

	// ------------------------------------------------------------------------
	// Constants
	// ------------------------------------------------------------------------

	public static inline var NONE                                  = 0;
	public static inline var FALSE                                 = 0;
	public static inline var TRUE                                  = 1;

	public static inline var SOURCE_RELATIVE                       = 0x202;
	public static inline var CONE_INNER_ANGLE                      = 0x1001;
	public static inline var CONE_OUTER_ANGLE                      = 0x1002;
	public static inline var PITCH                                 = 0x1003;

	public static inline var POSITION                              = 0x1004;
	public static inline var DIRECTION                             = 0x1005;

	public static inline var VELOCITY                              = 0x1006;
	public static inline var LOOPING                               = 0x1007;
	public static inline var BUFFER                                = 0x1009;

	public static inline var GAIN                                  = 0x100A;
	public static inline var MIN_GAIN                              = 0x100D;
	public static inline var MAX_GAIN                              = 0x100E;
	public static inline var ORIENTATION                           = 0x100F;
	public static inline var SOURCE_STATE                          = 0x1010;

	// Source state values
	public static inline var INITIAL                               = 0x1011;
	public static inline var PLAYING                               = 0x1012;
	public static inline var PAUSED                                = 0x1013;
	public static inline var STOPPED                               = 0x1014;

	public static inline var BUFFERS_QUEUED                        = 0x1015;
	public static inline var BUFFERS_PROCESSED                     = 0x1016;

	public static inline var REFERENCE_DISTANCE                    = 0x1020;
	public static inline var ROLLOFF_FACTOR                        = 0x1021;
	public static inline var CONE_OUTER_GAIN                       = 0x1022;
	public static inline var MAX_DISTANCE                          = 0x1023;

	public static inline var SEC_OFFSET                            = 0x1024;
	public static inline var SAMPLE_OFFSET                         = 0x1025;
	public static inline var BYTE_OFFSET                           = 0x1026;
	public static inline var SOURCE_TYPE                           = 0x1027;

	// Source type value
	public static inline var STATIC                                = 0x1028;
	public static inline var STREAMING                             = 0x1029;
	public static inline var UNDETERMINED                          = 0x1030;

	// Buffer format specifier
	public static inline var FORMAT_MONO8                          = 0x1100;
	public static inline var FORMAT_MONO16                         = 0x1101;
	public static inline var FORMAT_STEREO8                        = 0x1102;
	public static inline var FORMAT_STEREO16                       = 0x1103;

	// Buffer query
	public static inline var FREQUENCY                             = 0x2001;
	public static inline var BITS                                  = 0x2002;
	public static inline var CHANNELS                              = 0x2003;
	public static inline var SIZE                                  = 0x2004;

	// Buffer state (private)
	public static inline var UNUSED                                = 0x2010;
	public static inline var PENDING                               = 0x2011;
	public static inline var PROCESSED                             = 0x2012;

	// Errors
	public static inline var NO_ERROR                              = 0;
	public static inline var INVALID_NAME                          = 0xA001;
	public static inline var INVALID_ENUM                          = 0xA002;
	public static inline var INVALID_VALUE                         = 0xA003;
	public static inline var INVALID_OPERATION                     = 0xA004;
	public static inline var OUT_OF_MEMORY                         = 0xA005;

	// Context strings
	public static inline var VENDOR                                = 0xB001;
	public static inline var VERSION                               = 0xB002;
	public static inline var RENDERER                              = 0xB003;
	public static inline var EXTENSIONS                            = 0xB004;

	// Context values
	public static inline var DOPPLER_FACTOR                        = 0xC000;
	public static inline var DOPPLER_VELOCITY                      = 0xC001;
	public static inline var SPEED_OF_SOUND                        = 0xC003;
	public static inline var DISTANCE_MODEL                        = 0xD000;

	// Distance model values
	public static inline var INVERSE_DISTANCE                      = 0xD001;
	public static inline var INVERSE_DISTANCE_CLAMPED              = 0xD002;
	public static inline var LINEAR_DISTANCE                       = 0xD003;
	public static inline var LINEAR_DISTANCE_CLAMPED               = 0xD004;
	public static inline var EXPONENT_DISTANCE                     = 0xD005;
	public static inline var EXPONENT_DISTANCE_CLAMPED             = 0xD006;
}