package openal;
import openal.AL;

@:hlNative("?openal","al_")
extern class ExtAL {
	// ------------------------------------------------------------------------
	// AL_LOKI_IMA_ADPCM_format
	// ------------------------------------------------------------------------

	public static inline var FORMAT_IMA_ADPCM_MONO16_EXT           = 0x10000;
	public static inline var FORMAT_IMA_ADPCM_STEREO16_EXT         = 0x10001;

	// ------------------------------------------------------------------------
	// AL_LOKI_WAVE_format
	// ------------------------------------------------------------------------

	public static inline var FORMAT_WAVE_EXT                       = 0x10002;

	// ------------------------------------------------------------------------
	// AL_EXT_vorbis
	// ------------------------------------------------------------------------

	public static inline var FORMAT_VORBIS_EXT                     = 0x10003;

	// ------------------------------------------------------------------------
	// AL_LOKI_quadriphonic
	// ------------------------------------------------------------------------

	public static inline var FORMAT_QUAD8_LOKI                     = 0x10004;
	public static inline var FORMAT_QUAD16_LOKI                    = 0x10005;

	// ------------------------------------------------------------------------
	// AL_EXT_float32
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_FLOAT32                   = 0x10010;
	public static inline var FORMAT_STEREO_FLOAT32                 = 0x10011;

	// ------------------------------------------------------------------------
	// AL_EXT_double
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_DOUBLE_EXT                = 0x10012;
	public static inline var FORMAT_STEREO_DOUBLE_EXT              = 0x10013;

	// ------------------------------------------------------------------------
	// AL_EXT_MULAW
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_MULAW_EXT                 = 0x10014;
	public static inline var FORMAT_STEREO_MULAW_EXT               = 0x10015;

	// ------------------------------------------------------------------------
	// AL_EXT_ALAW
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_ALAW_EXT                  = 0x10016;
	public static inline var FORMAT_STEREO_ALAW_EXT                = 0x10017;

	// ------------------------------------------------------------------------
	// AL_EXT_MCFORMATS
	// ------------------------------------------------------------------------

	public static inline var FORMAT_QUAD8                          = 0x1204;
	public static inline var FORMAT_QUAD16                         = 0x1205;
	public static inline var FORMAT_QUAD32                         = 0x1206;
	public static inline var FORMAT_REAR8                          = 0x1207;
	public static inline var FORMAT_REAR16                         = 0x1208;
	public static inline var FORMAT_REAR32                         = 0x1209;
	public static inline var FORMAT_51CHN8                         = 0x120A;
	public static inline var FORMAT_51CHN16                        = 0x120B;
	public static inline var FORMAT_51CHN32                        = 0x120C;
	public static inline var FORMAT_61CHN8                         = 0x120D;
	public static inline var FORMAT_61CHN16                        = 0x120E;
	public static inline var FORMAT_61CHN32                        = 0x120F;
	public static inline var FORMAT_71CHN8                         = 0x1210;
	public static inline var FORMAT_71CHN16                        = 0x1211;
	public static inline var FORMAT_71CHN32                        = 0x1212;

	// ------------------------------------------------------------------------
	// AL_EXT_MULAW_MCFORMATS
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_MULAW                     = 0x10014;
	public static inline var FORMAT_STEREO_MULAW                   = 0x10015;
	public static inline var FORMAT_QUAD_MULAW                     = 0x10021;
	public static inline var FORMAT_REAR_MULAW                     = 0x10022;
	public static inline var FORMAT_51CHN_MULAW                    = 0x10023;
	public static inline var FORMAT_61CHN_MULAW                    = 0x10024;
	public static inline var FORMAT_71CHN_MULAW                    = 0x10025;

	// ------------------------------------------------------------------------
	// AL_EXT_IMA4
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_IMA4                      = 0x1300;
	public static inline var FORMAT_STEREO_IMA4                    = 0x1301;

	// ------------------------------------------------------------------------
	// AL_EXT_STATIC_BUFFER
	// ------------------------------------------------------------------------

	public static function bufferDataStatic(buffer : Buffer, format : Int, data : hl.Bytes, len : Int, freq : Int) : Void;

	// ------------------------------------------------------------------------
	// AL_EXT_source_distance_model
	// ------------------------------------------------------------------------

	public static inline var SOURCE_DISTANCE_MODEL                 = 0x200;

	// ------------------------------------------------------------------------
	// AL_SOFT_buffer_sub_data
	// ------------------------------------------------------------------------

	public static inline var BYTE_RW_OFFSETS_SOFT                  = 0x1031;
	public static inline var SAMPLE_RW_OFFSETS_SOFT                = 0x1032;

	public static function bufferSubDataSoft(buffer : Buffer, format : Int, data : hl.Bytes, offset : Int, length : Int) : Void;

	// ------------------------------------------------------------------------
	// AL_SOFT_loop_points
	// ------------------------------------------------------------------------

	public static inline var LOOP_POINTS_SOFT                      = 0x2015;

	// ------------------------------------------------------------------------
	// AL_EXT_FOLDBACK
	// ------------------------------------------------------------------------

	public static inline var FOLDBACK_EVENT_BLOCK                  = 0x4112;
	public static inline var FOLDBACK_EVENT_START                  = 0x4111;
	public static inline var FOLDBACK_EVENT_STOP                   = 0x4113;
	public static inline var FOLDBACK_MODE_MONO                    = 0x4101;
	public static inline var FOLDBACK_MODE_STEREO                  = 0x4102;

	public static function requestFoldbackStart(mode : Int, count : Int, length : Int, mem : hl.Bytes, callback : Int->Int->Void) : Void;
	public static function requestFoldbackStop() : Void;

	// ------------------------------------------------------------------------
	// ALC_EXT_DEDICATED
	// ------------------------------------------------------------------------

	public static inline var DEDICATED_GAIN                        = 0x0001;
	public static inline var EFFECT_DEDICATED_DIALOGUE             = 0x9001;
	public static inline var EFFECT_DEDICATED_LOW_FREQUENCY_EFFECT = 0x9000;

	// ------------------------------------------------------------------------
	// AL_SOFT_buffer_samples
	// ------------------------------------------------------------------------

	// Channel configurations
	public static inline var MONO_SOFT                             = 0x1500;
	public static inline var STEREO_SOFT                           = 0x1501;
	public static inline var REAR_SOFT                             = 0x1502;
	public static inline var QUAD_SOFT                             = 0x1503;
	public static inline var _5POINT1_SOFT                         = 0x1504;
	public static inline var _6POINT1_SOFT                         = 0x1505;
	public static inline var _7POINT1_SOFT                         = 0x1506;

	// Sample types
	public static inline var BYTE_SOFT                             = 0x1400;
	public static inline var UNSIGNED_BYTE_SOFT                    = 0x1401;
	public static inline var SHORT_SOFT                            = 0x1402;
	public static inline var UNSIGNED_SHORT_SOFT                   = 0x1403;
	public static inline var INT_SOFT                              = 0x1404;
	public static inline var UNSIGNED_INT_SOFT                     = 0x1405;
	public static inline var FLOAT_SOFT                            = 0x1406;
	public static inline var DOUBLE_SOFT                           = 0x1407;
	public static inline var BYTE3_SOFT                            = 0x1408;
	public static inline var UNSIGNED_BYTE3_SOFT                   = 0x1409;

	// Storage formats
	public static inline var MONO8_SOFT                            = 0x1100;
	public static inline var MONO16_SOFT                           = 0x1101;
	public static inline var MONO32F_SOFT                          = 0x10010;
	public static inline var STEREO8_SOFT                          = 0x1102;
	public static inline var STEREO16_SOFT                         = 0x1103;
	public static inline var STEREO32F_SOFT                        = 0x10011;
	public static inline var QUAD8_SOFT                            = 0x1204;
	public static inline var QUAD16_SOFT                           = 0x1205;
	public static inline var QUAD32F_SOFT                          = 0x1206;
	public static inline var REAR8_SOFT                            = 0x1207;
	public static inline var REAR16_SOFT                           = 0x1208;
	public static inline var REAR32F_SOFT                          = 0x1209;
	public static inline var _5POINT1_8_SOFT                       = 0x120A;
	public static inline var _5POINT1_16_SOFT                      = 0x120B;
	public static inline var _5POINT1_32F_SOFT                     = 0x120C;
	public static inline var _6POINT1_8_SOFT                       = 0x120D;
	public static inline var _6POINT1_16_SOFT                      = 0x120E;
	public static inline var _6POINT1_32F_SOFT                     = 0x120F;
	public static inline var _7POINT1_8_SOFT                       = 0x1210;
	public static inline var _7POINT1_16_SOFT                      = 0x1211;
	public static inline var _7POINT1_32F_SOFT                     = 0x1212;

	// Buffer attributes
	public static inline var INTERNAL_FORMAT_SOFT                  = 0x2008;
	public static inline var BYTE_LENGTH_SOFT                      = 0x2009;
	public static inline var SAMPLE_LENGTH_SOFT                    = 0x200A;
	public static inline var SEC_LENGTH_SOFT                       = 0x200B;

	public static function bufferSamplesSoft           (buffer : Buffer, samplerate : Int, internalformat : Int, samples : Int, channels : Int, type : Int, data : hl.Bytes) : Void;
	public static function bufferSubSamplesSoft        (buffer : Buffer, offset : Int, samples : Int, channels : Int, type : Int, data : hl.Bytes) : Void;
	public static function getBufferSamplesSoft        (buffer : Buffer, offset : Int, samples : Int, channels : Int, type : Int, data : hl.Bytes) : Void;
	public static function isBufferFormatSupportedSOFT (format : Int) : Bool;

	// ------------------------------------------------------------------------
	// AL_SOFT_direct_channels
	// ------------------------------------------------------------------------

	public static inline var DIRECT_CHANNELS_SOFT                  = 0x1033;

	// ------------------------------------------------------------------------
	// AL_EXT_STEREO_ANGLES
	// ------------------------------------------------------------------------

	public static inline var STEREO_ANGLES                         = 0x1030;

	// ------------------------------------------------------------------------
	// AL_EXT_SOURCE_RADIUS
	// ------------------------------------------------------------------------

	public static inline var SOURCE_RADIUS                         = 0x1031;

	// ------------------------------------------------------------------------
	// AL_SOFT_source_latency
	// ------------------------------------------------------------------------

	public static inline var SAMPLE_OFFSET_LATENCY_SOFT            = 0x1200;
	public static inline var SEC_OFFSET_LATENCY_SOFT               = 0x1201;

	public static function sourcedSoft       (source : Source, param : Int, value  : Float) : Void;
	public static function source3dSoft      (source : Source, param : Int, value1 : Float, value2 : Float, value3 : Float) : Void;
	public static function sourcedvSoft      (source : Source, param : Int, values : hl.Bytes) : Void;

	public static function getSourcedSoft    (source : Source, param : Int) : Float;
	public static function getSource3dSoft   (source : Source, param : Int, value1 : hl.Ref<Float>, value2 : hl.Ref<Float>, value3 : hl.Ref<Float>) : Void;
	public static function getSourcedvSoft   (source : Source, param : Int, values : hl.Bytes) : Void;

	public static function sourcei64Soft     (source : Source, param : Int, valueHi  : Int, valueLo : Int) : Void;
	public static function source3i64Soft    (source : Source, param : Int, value1Hi : Int, value1Lo : Int, value2Hi : Int, value2Lo : Int, value3Hi : Int, value3Lo : Int) : Void;
	public static function sourcei64vSoft    (source : Source, param : Int, values   : hl.Bytes) : Void;

	public static function getSourcei64Soft  (source : Source, param : Int, valueHi  : hl.Ref<Int>, valueLo : hl.Ref<Int>) : Void;
	public static function getSource3i64Soft (source : Source, param : Int, value1Hi : hl.Ref<Int>, value1Lo : hl.Ref<Int>, value2Hi : hl.Ref<Int>, value2Lo : hl.Ref<Int>, value3Hi : hl.Ref<Int>, value3Lo : hl.Ref<Int>) : Void;
	public static function getSourcei64vSoft (source : Source, param : Int, values   : hl.Bytes) : Void;

	// ------------------------------------------------------------------------
	// AL_SOFT_deferred_updates
	// ------------------------------------------------------------------------

	public static inline var DEFERRED_UPDATES_SOFT                 = 0xC002;

	public static function deferUpdatesSoft   () : Void;
	public static function processUpdatesSoft () : Void;

	// ------------------------------------------------------------------------
	// AL_SOFT_block_alignment
	// ------------------------------------------------------------------------

	public static inline var UNPACK_BLOCK_ALIGNMENT_SOFT           = 0x200C;
	public static inline var PACK_BLOCK_ALIGNMENT_SOFT             = 0x200D;

	// ------------------------------------------------------------------------
	// AL_SOFT_MSADPCM
	// ------------------------------------------------------------------------

	public static inline var FORMAT_MONO_MSADPCM_SOFT              = 0x1302;
	public static inline var FORMAT_STEREO_MSADPCM_SOFT            = 0x1303;

	// ------------------------------------------------------------------------
	// AL_EXT_BFORMAT
	// ------------------------------------------------------------------------

	public static inline var FORMAT_BFORMAT2D_8                    = 0x20021;
	public static inline var FORMAT_BFORMAT2D_16                   = 0x20022;
	public static inline var FORMAT_BFORMAT2D_FLOAT32              = 0x20023;
	public static inline var FORMAT_BFORMAT3D_8                    = 0x20031;
	public static inline var FORMAT_BFORMAT3D_16                   = 0x20032;
	public static inline var FORMAT_BFORMAT3D_FLOAT32              = 0x20033;

	// ------------------------------------------------------------------------
	// AL_EXT_MULAW_BFORMAT
	// ------------------------------------------------------------------------

	public static inline var FORMAT_BFORMAT2D_MULAW                = 0x10031;
	public static inline var FORMAT_BFORMAT3D_MULAW                = 0x10032;

	// ------------------------------------------------------------------------
	// AL_SOFT_gain_clamp_ex
	// ------------------------------------------------------------------------

	public static inline var GAIN_LIMIT_SOFT                       = 0x200E;

	// ------------------------------------------------------------------------
	// AL_SOFT_source_resampler
	// ------------------------------------------------------------------------

	public static inline var NUM_RESAMPLERS_SOFT                   = 0x1210;
	public static inline var DEFAULT_RESAMPLER_SOFT                = 0x1211;
	public static inline var SOURCE_RESAMPLER_SOFT                 = 0x1212;
	public static inline var RESAMPLER_NAME_SOFT                   = 0x1213;

	public static function getStringiSoft(param : Int, index : Int) : hl.Bytes;

	// ------------------------------------------------------------------------
	// AL_SOFT_source_spatialize
	// ------------------------------------------------------------------------

	public static inline var SOURCE_SPATIALIZE_SOFT                = 0x1214;
	public static inline var AUTO_SOFT                             = 0x0002;

	// ------------------------------------------------------------------------
	// AL_SOFT_direct_channels_remix
	// ------------------------------------------------------------------------

	public static inline var DROP_UNMATCHED_SOFT                   = 0x0001;
	public static inline var REMIX_UNMATCHED_SOFT                  = 0x0002;

	// ------------------------------------------------------------------------
	// AL_SOFT_bformat_ex
	// ------------------------------------------------------------------------

	public static inline var AMBISONIC_LAYOUT_SOFT                 = 0x1997;
	public static inline var AMBISONIC_SCALING_SOFT                = 0x1998;

	// Ambisonic layouts
	public static inline var FUMA_SOFT                             = 0x0000;
	public static inline var ACN_SOFT                              = 0x0001;

	// Ambisonic scalings (normalization)
	public static inline var SN3D_SOFT                             = 0x0001;
	public static inline var N3D_SOFT                              = 0x0002;

	// ------------------------------------------------------------------------
	// AL_SOFT_effect_target
	// ------------------------------------------------------------------------

	public static inline var EFFECTSLOT_TARGET_SOFT                = 0x199C;

	// ------------------------------------------------------------------------
	// AL_SOFT_events
	// ------------------------------------------------------------------------

	public static inline var EVENT_CALLBACK_FUNCTION_SOFT          = 0x19A2;
	public static inline var EVENT_CALLBACK_USER_PARAM_SOFT        = 0x19A3;
	public static inline var EVENT_TYPE_BUFFER_COMPLETED_SOFT      = 0x19A4;
	public static inline var EVENT_TYPE_SOURCE_STATE_CHANGED_SOFT  = 0x19A5;
	public static inline var EVENT_TYPE_DISCONNECTED_SOFT          = 0x19A6;

	public static function eventControlSoft(count : Int, types : hl.Ref<Int>, enable : Bool) : Void;
	public static function eventCallbackSoft(callback : (Int,Int,Int,Int,hl.Bytes,Dynamic)->Void, userParam : Dynamic) : Void;
	public static function getPointerSoft(param : Int) : Dynamic;
	public static function getPointervSoft(param : Int, values : hl.Ref<Dynamic>) : Void;

	// ------------------------------------------------------------------------
	// AL_SOFT_callback_buffer
	// ------------------------------------------------------------------------

	public static inline var BUFFER_CALLBACK_FUNCTION_SOFT         = 0x19A0;
	public static inline var BUFFER_CALLBACK_USER_PARAM_SOFT       = 0x19A1;

	public static function bufferCallbackSoft(buffer : Int, format : Int, freq : Int, callback : (Dynamic,hl.Bytes,Int)->Int, userptr : Dynamic) : Void;
	public static function getBufferPtrSoft(buffer : Int, param : Int, ptr : hl.Ref<Dynamic>) : Void;
	public static function getBuffer3PtrSoft(buffer : Int, param : Int, ptr0 : hl.Ref<Dynamic>, ptr1 : hl.Ref<Dynamic>, ptr2 : hl.Ref<Dynamic>) : Void;
	public static function getBufferPtrvSoft(buffer : Int, param : Int, ptr : hl.Ref<Dynamic>) : Void;

	// ------------------------------------------------------------------------
	// AL_SOFT_UHJ
	// ------------------------------------------------------------------------

	public static inline var FORMAT_UHJ2CHN8_SOFT                  = 0x19A2;
	public static inline var FORMAT_UHJ2CHN16_SOFT                 = 0x19A3;
	public static inline var FORMAT_UHJ2CHN_FLOAT32_SOFT           = 0x19A4;
	public static inline var FORMAT_UHJ3CHN8_SOFT                  = 0x19A5;
	public static inline var FORMAT_UHJ3CHN16_SOFT                 = 0x19A6;
	public static inline var FORMAT_UHJ3CHN_FLOAT32_SOFT           = 0x19A7;
	public static inline var FORMAT_UHJ4CHN8_SOFT                  = 0x19A8;
	public static inline var FORMAT_UHJ4CHN16_SOFT                 = 0x19A9;
	public static inline var FORMAT_UHJ4CHN_FLOAT32_SOFT           = 0x19AA;
	public static inline var STEREO_MODE_SOFT                      = 0x19B0;
	public static inline var NORMAL_SOFT                           = 0x0000;
	public static inline var SUPER_STEREO_SOFT                     = 0x0001;
	public static inline var SUPER_STEREO_WIDTH_SOFT               = 0x19B1;

	// ------------------------------------------------------------------------
	// AL_SOFT_UHJ_ex
	// ------------------------------------------------------------------------

	public static inline var FORMAT_UHJ2CHN_MULAW_SOFT             = 0x19B3;
	public static inline var FORMAT_UHJ2CHN_ALAW_SOFT              = 0x19B4;
	public static inline var FORMAT_UHJ2CHN_IMA4_SOFT              = 0x19B5;
	public static inline var FORMAT_UHJ2CHN_MSADPCM_SOFT           = 0x19B6;
	public static inline var FORMAT_UHJ3CHN_MULAW_SOFT             = 0x19B7;
	public static inline var FORMAT_UHJ3CHN_ALAW_SOFT              = 0x19B8;
	public static inline var FORMAT_UHJ4CHN_MULAW_SOFT             = 0x19B9;
	public static inline var FORMAT_UHJ4CHN_ALAW_SOFT              = 0x19BA;

	// ------------------------------------------------------------------------
	// AL_SOFT_source_start_delay
	// ------------------------------------------------------------------------

	public static function sourcePlayAtTimeSoft(source : Int, startTimeHi : Int, startTimeLo : Int) : Void;
	public static function sourcePlayAtTimevSoft(n : Int, sources : hl.Bytes, startTimeHi : Int, startTimeLo : Int) : Void;
}
