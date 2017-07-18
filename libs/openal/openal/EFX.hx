package openal;
import openal.AL;

abstract Effect(Int) {
	public inline function toInt() : Int { return this; }
	public static inline function ofInt( v : Int ) : Effect { return cast v; }
}

abstract Filter(Int) {
	public inline function toInt() : Int { return this; }
	public static inline function ofInt( v : Int ) : Filter { return cast v; }
}

abstract EffectSlot(Int) {
	public inline function toInt() : Int { return this; }
	public static inline function ofInt( v : Int ) : EffectSlot { return cast v; }
}

// ------------------------------------------------------------------------
// ALC_EXT_EFX
// ------------------------------------------------------------------------

@:hlNative("openal","al_")
extern class EFX {
	// Device attributes
	public static inline var EFX_MAJOR_VERSION                     = 0x20001;
	public static inline var EFX_MINOR_VERSION                     = 0x20002;
	public static inline var MAX_AUXILIARY_SENDS                   = 0x20003;

	// Listener properties.
	public static inline var METERS_PER_UNIT                       = 0x20004;

	// Source properties.
	public static inline var DIRECT_FILTER                         = 0x20005;
	public static inline var AUXILIARY_SEND_FILTER                 = 0x20006;
	public static inline var AIR_ABSORPTION_FACTOR                 = 0x20007;
	public static inline var ROOM_ROLLOFF_FACTOR                   = 0x20008;
	public static inline var CONE_OUTER_GAINHF                     = 0x20009;
	public static inline var DIRECT_FILTER_GAINHF_AUTO             = 0x2000A;
	public static inline var AUXILIARY_SEND_FILTER_GAIN_AUTO       = 0x2000B;
	public static inline var AUXILIARY_SEND_FILTER_GAINHF_AUTO     = 0x2000C;


	// Effect properties.

	// Reverb effect parameters
	public static inline var REVERB_DENSITY                        = 0x0001;
	public static inline var REVERB_DIFFUSION                      = 0x0002;
	public static inline var REVERB_GAIN                           = 0x0003;
	public static inline var REVERB_GAINHF                         = 0x0004;
	public static inline var REVERB_DECAY_TIME                     = 0x0005;
	public static inline var REVERB_DECAY_HFRATIO                  = 0x0006;
	public static inline var REVERB_REFLECTIONS_GAIN               = 0x0007;
	public static inline var REVERB_REFLECTIONS_DELAY              = 0x0008;
	public static inline var REVERB_LATE_REVERB_GAIN               = 0x0009;
	public static inline var REVERB_LATE_REVERB_DELAY              = 0x000A;
	public static inline var REVERB_AIR_ABSORPTION_GAINHF          = 0x000B;
	public static inline var REVERB_ROOM_ROLLOFF_FACTOR            = 0x000C;
	public static inline var REVERB_DECAY_HFLIMIT                  = 0x000D;

	// EAX Reverb effect parameters
	public static inline var EAXREVERB_DENSITY                     = 0x0001;
	public static inline var EAXREVERB_DIFFUSION                   = 0x0002;
	public static inline var EAXREVERB_GAIN                        = 0x0003;
	public static inline var EAXREVERB_GAINHF                      = 0x0004;
	public static inline var EAXREVERB_GAINLF                      = 0x0005;
	public static inline var EAXREVERB_DECAY_TIME                  = 0x0006;
	public static inline var EAXREVERB_DECAY_HFRATIO               = 0x0007;
	public static inline var EAXREVERB_DECAY_LFRATIO               = 0x0008;
	public static inline var EAXREVERB_REFLECTIONS_GAIN            = 0x0009;
	public static inline var EAXREVERB_REFLECTIONS_DELAY           = 0x000A;
	public static inline var EAXREVERB_REFLECTIONS_PAN             = 0x000B;
	public static inline var EAXREVERB_LATE_REVERB_GAIN            = 0x000C;
	public static inline var EAXREVERB_LATE_REVERB_DELAY           = 0x000D;
	public static inline var EAXREVERB_LATE_REVERB_PAN             = 0x000E;
	public static inline var EAXREVERB_ECHO_TIME                   = 0x000F;
	public static inline var EAXREVERB_ECHO_DEPTH                  = 0x0010;
	public static inline var EAXREVERB_MODULATION_TIME             = 0x0011;
	public static inline var EAXREVERB_MODULATION_DEPTH            = 0x0012;
	public static inline var EAXREVERB_AIR_ABSORPTION_GAINHF       = 0x0013;
	public static inline var EAXREVERB_HFREFERENCE                 = 0x0014;
	public static inline var EAXREVERB_LFREFERENCE                 = 0x0015;
	public static inline var EAXREVERB_ROOM_ROLLOFF_FACTOR         = 0x0016;
	public static inline var EAXREVERB_DECAY_HFLIMIT               = 0x0017;

	// Chorus effect parameters
	public static inline var CHORUS_WAVEFORM                       = 0x0001;
	public static inline var CHORUS_PHASE                          = 0x0002;
	public static inline var CHORUS_RATE                           = 0x0003;
	public static inline var CHORUS_DEPTH                          = 0x0004;
	public static inline var CHORUS_FEEDBACK                       = 0x0005;
	public static inline var CHORUS_DELAY                          = 0x0006;

	// Distortion effect parameters
	public static inline var DISTORTION_EDGE                       = 0x0001;
	public static inline var DISTORTION_GAIN                       = 0x0002;
	public static inline var DISTORTION_LOWPASS_CUTOFF             = 0x0003;
	public static inline var DISTORTION_EQCENTER                   = 0x0004;
	public static inline var DISTORTION_EQBANDWIDTH                = 0x0005;

	// Echo effect parameters
	public static inline var ECHO_DELAY                            = 0x0001;
	public static inline var ECHO_LRDELAY                          = 0x0002;
	public static inline var ECHO_DAMPING                          = 0x0003;
	public static inline var ECHO_FEEDBACK                         = 0x0004;
	public static inline var ECHO_SPREAD                           = 0x0005;

	// Flanger effect parameters
	public static inline var FLANGER_WAVEFORM                      = 0x0001;
	public static inline var FLANGER_PHASE                         = 0x0002;
	public static inline var FLANGER_RATE                          = 0x0003;
	public static inline var FLANGER_DEPTH                         = 0x0004;
	public static inline var FLANGER_FEEDBACK                      = 0x0005;
	public static inline var FLANGER_DELAY                         = 0x0006;

	// Frequency shifter effect parameters
	public static inline var FREQUENCY_SHIFTER_FREQUENCY           = 0x0001;
	public static inline var FREQUENCY_SHIFTER_LEFT_DIRECTION      = 0x0002;
	public static inline var FREQUENCY_SHIFTER_RIGHT_DIRECTION     = 0x0003;

	// Vocal morpher effect parameters
	public static inline var VOCAL_MORPHER_PHONEMEA                = 0x0001;
	public static inline var VOCAL_MORPHER_PHONEMEA_COARSE_TUNING  = 0x0002;
	public static inline var VOCAL_MORPHER_PHONEMEB                = 0x0003;
	public static inline var VOCAL_MORPHER_PHONEMEB_COARSE_TUNING  = 0x0004;
	public static inline var VOCAL_MORPHER_WAVEFORM                = 0x0005;
	public static inline var VOCAL_MORPHER_RATE                    = 0x0006;

	// Pitchshifter effect parameters
	public static inline var PITCH_SHIFTER_COARSE_TUNE             = 0x0001;
	public static inline var PITCH_SHIFTER_FINE_TUNE               = 0x0002;

	// Ringmodulator effect parameters
	public static inline var RING_MODULATOR_FREQUENCY              = 0x0001;
	public static inline var RING_MODULATOR_HIGHPASS_CUTOFF        = 0x0002;
	public static inline var RING_MODULATOR_WAVEFORM               = 0x0003;

	// Autowah effect parameters
	public static inline var AUTOWAH_ATTACK_TIME                   = 0x0001;
	public static inline var AUTOWAH_RELEASE_TIME                  = 0x0002;
	public static inline var AUTOWAH_RESONANCE                     = 0x0003;
	public static inline var AUTOWAH_PEAK_GAIN                     = 0x0004;

	// Compressor effect parameters
	public static inline var COMPRESSOR_ONOFF                      = 0x0001;

	// Equalizer effect parameters
	public static inline var EQUALIZER_LOW_GAIN                    = 0x0001;
	public static inline var EQUALIZER_LOW_CUTOFF                  = 0x0002;
	public static inline var EQUALIZER_MID1_GAIN                   = 0x0003;
	public static inline var EQUALIZER_MID1_CENTER                 = 0x0004;
	public static inline var EQUALIZER_MID1_WIDTH                  = 0x0005;
	public static inline var EQUALIZER_MID2_GAIN                   = 0x0006;
	public static inline var EQUALIZER_MID2_CENTER                 = 0x0007;
	public static inline var EQUALIZER_MID2_WIDTH                  = 0x0008;
	public static inline var EQUALIZER_HIGH_GAIN                   = 0x0009;
	public static inline var EQUALIZER_HIGH_CUTOFF                 = 0x000A;

	// Effect type
	public static inline var EFFECT_FIRST_PARAMETER                = 0x0000;
	public static inline var EFFECT_LAST_PARAMETER                 = 0x8000;
	public static inline var EFFECT_TYPE                           = 0x8001;

	// Effect types, used with the AL_EFFECT_TYPE property
	public static inline var EFFECT_NULL                           = 0x0000;
	public static inline var EFFECT_REVERB                         = 0x0001;
	public static inline var EFFECT_CHORUS                         = 0x0002;
	public static inline var EFFECT_DISTORTION                     = 0x0003;
	public static inline var EFFECT_ECHO                           = 0x0004;
	public static inline var EFFECT_FLANGER                        = 0x0005;
	public static inline var EFFECT_FREQUENCY_SHIFTER              = 0x0006;
	public static inline var EFFECT_VOCAL_MORPHER                  = 0x0007;
	public static inline var EFFECT_PITCH_SHIFTER                  = 0x0008;
	public static inline var EFFECT_RING_MODULATOR                 = 0x0009;
	public static inline var EFFECT_AUTOWAH                        = 0x000A;
	public static inline var EFFECT_COMPRESSOR                     = 0x000B;
	public static inline var EFFECT_EQUALIZER                      = 0x000C;
	public static inline var EFFECT_EAXREVERB                      = 0x8000;

	// Auxiliary Effect Slot properties.
	public static inline var EFFECTSLOT_EFFECT                     = 0x0001;
	public static inline var EFFECTSLOT_GAIN                       = 0x0002;
	public static inline var EFFECTSLOT_AUXILIARY_SEND_AUTO        = 0x0003;

	// NULL Auxiliary Slot ID to disable a source send.
	public static inline var EFFECTSLOT_NULL                       = 0x0000;


	// Filter properties.

	// Lowpass filter parameters
	public static inline var LOWPASS_GAIN                          = 0x0001;
	public static inline var LOWPASS_GAINHF                        = 0x0002;

	// Highpass filter parameters
	public static inline var HIGHPASS_GAIN                         = 0x0001;
	public static inline var HIGHPASS_GAINLF                       = 0x0002;

	// Bandpass filter parameters
	public static inline var BANDPASS_GAIN                         = 0x0001;
	public static inline var BANDPASS_GAINLF                       = 0x0002;
	public static inline var BANDPASS_GAINHF                       = 0x0003;

	// Filter type
	public static inline var FILTER_FIRST_PARAMETER                = 0x0000;
	public static inline var FILTER_LAST_PARAMETER                 = 0x8000;
	public static inline var FILTER_TYPE                           = 0x8001;

	// Filter types, used with the AL_FILTER_TYPE property
	public static inline var FILTER_NULL                           = 0x0000;
	public static inline var FILTER_LOWPASS                        = 0x0001;
	public static inline var FILTER_HIGHPASS                       = 0x0002;
	public static inline var FILTER_BANDPASS                       = 0x0003;

	public static function genEffects    (n : Int, effects : hl.Bytes) : Void;
	public static function deleteEffects (n : Int, effects : hl.Bytes) : Void;
	public static function isEffect      (effect : Effect) : Bool;
	public static function effecti       (effect : Effect, param : Int, iValue    : Int)      : Void;
	public static function effectiv      (effect : Effect, param : Int, piValues  : hl.Bytes) : Void;
	public static function effectf       (effect : Effect, param : Int, flValue   : hl.F32)   : Void;
	public static function effectfv      (effect : Effect, param : Int, pflValues : hl.Bytes) : Void;
	public static function getEffecti    (effect : Effect, param : Int)                       : Int;
	public static function getEffectiv   (effect : Effect, param : Int, piValues  : hl.Bytes) : Void;
	public static function getEffectf    (effect : Effect, param : Int)                       : hl.F32;
	public static function getEffectfv   (effect : Effect, param : Int, pflValues : hl.Bytes) : Void;

	public static function genFilters    (n : Int, filters : hl.Bytes) : Void;
	public static function deleteFilters (n : Int, filters : hl.Bytes) : Void;
	public static function isFilter      (filter : Filter) : Bool;
	public static function filteri       (filter : Filter, param : Int, iValue    : Int)      : Void;
	public static function filteriv      (filter : Filter, param : Int, piValues  : hl.Bytes) : Void;
	public static function filterf       (filter : Filter, param : Int, flValue   : hl.F32)   : Void;
	public static function filterfv      (filter : Filter, param : Int, pflValues : hl.Bytes) : Void;
	public static function getFilteri    (filter : Filter, param : Int)                       : Int;
	public static function getFilteriv   (filter : Filter, param : Int, piValues  : hl.Bytes) : Void;
	public static function getFilterf    (filter : Filter, param : Int)                       : hl.F32;
	public static function getFilterfv   (filter : Filter, param : Int, pflValues : hl.Bytes) : Void;

	public static function genAuxiliaryEffectSlots    (n : Int, effectslots : hl.Bytes) : Void;
	public static function deleteAuxiliaryEffectSlots (n : Int, effectslots : hl.Bytes) : Void;
	public static function isAuxiliaryEffectSlot      (effectslot : EffectSlot) : Bool;
	public static function auxiliaryEffectSloti       (effectslot : EffectSlot, param : Int, iValue    : Int)      : Void;
	public static function auxiliaryEffectSlotiv      (effectslot : EffectSlot, param : Int, piValues  : hl.Bytes) : Void;
	public static function auxiliaryEffectSlotf       (effectslot : EffectSlot, param : Int, flValue   : hl.F32)   : Void;
	public static function auxiliaryEffectSlotfv      (effectslot : EffectSlot, param : Int, pflValues : hl.Bytes) : Void;
	public static function getAuxiliaryEffectSloti    (effectslot : EffectSlot, param : Int) : Int;
	public static function getAuxiliaryEffectSlotiv   (effectslot : EffectSlot, param : Int, piValues  : hl.Bytes)  : Void;
	public static function getAuxiliaryEffectSlotf    (effectslot : EffectSlot, param : Int) : hl.F32;
	public static function getAuxiliaryEffectSlotfv   (effectslot : EffectSlot, param : Int, pflValues : hl.Bytes)  : Void;
}