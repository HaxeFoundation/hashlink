#define HL_NAME(n) openal_##n
#include <hl.h>

#if defined(__APPLE__) && !defined(openal_soft)
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif

// ----------------------------------------------------------------------------
// ALC
// ----------------------------------------------------------------------------

#define ALC_IMPORT(fun, t) t fun
#include "ALCImports.h"
#undef ALC_IMPORT

// Context management

HL_PRIM ALCcontext* HL_NAME(alc_create_context)(ALCdevice *device, vbyte *attrlist) {
	return alcCreateContext(device, (ALCint*)attrlist);
}

HL_PRIM bool HL_NAME(alc_make_context_current)(ALCcontext *context) {
	return alcMakeContextCurrent(context) == ALC_TRUE;
}

HL_PRIM void HL_NAME(alc_process_context)(ALCcontext *context) {
	alcProcessContext(context);
}

HL_PRIM void HL_NAME(alc_suspend_context)(ALCcontext *context) {
	alcSuspendContext(context);
}

HL_PRIM void HL_NAME(alc_destroy_context)(ALCcontext *context) {
	alcDestroyContext(context);
}

HL_PRIM ALCcontext* HL_NAME(alc_get_current_context)() {
	return alcGetCurrentContext();
}

HL_PRIM ALCdevice* HL_NAME(alc_get_contexts_device)(ALCcontext *context) {
	return alcGetContextsDevice(context);
}

// Device management

HL_PRIM ALCdevice* HL_NAME(alc_open_device)(vbyte *devicename) {
	return alcOpenDevice((char*)devicename);
}

HL_PRIM bool HL_NAME(alc_close_device)(ALCdevice *device) {
	return alcCloseDevice(device) == ALC_TRUE;
}

// Error support

HL_PRIM int HL_NAME(alc_get_error)(ALCdevice *device) {
	return alcGetError(device);
}

// Extension support

#define ALC_IMPORT(fun,t) fun = (t)alcGetProcAddress(device,#fun)
HL_PRIM void HL_NAME(alc_load_extensions)(ALCdevice *device) {
#	include "ALCImports.h"
}

HL_PRIM bool HL_NAME(alc_is_extension_present)(ALCdevice *device, vbyte *extname) {
	return alcIsExtensionPresent(device, (char*)extname) == ALC_TRUE;
}

HL_PRIM int HL_NAME(alc_get_enum_value)(ALCdevice *device, vbyte *enumname) {
	return alcGetEnumValue(device, (char*)enumname);
}

// Query function

HL_PRIM vbyte* HL_NAME(alc_get_string)(ALCdevice *device, int param) {
	return (vbyte*)alcGetString(device, param);
}

HL_PRIM void HL_NAME(alc_get_integerv)(ALCdevice *device, int param, int size, vbyte *values) {
	alcGetIntegerv(device, param, size, (ALCint*)values);
}

// Capture function

HL_PRIM ALCdevice* HL_NAME(alc_capture_open_device)(vbyte *devicename, int frequency, int format, int buffersize) {
	return alcCaptureOpenDevice((char*)devicename, (ALCuint)frequency, format, buffersize);
}

HL_PRIM bool HL_NAME(alc_capture_close_device)(ALCdevice *device) {
	return alcCaptureCloseDevice(device) == ALC_TRUE;
}

HL_PRIM void HL_NAME(alc_capture_start)(ALCdevice *device) {
	alcCaptureStart(device);
}

HL_PRIM void HL_NAME(alc_capture_stop)(ALCdevice *device) {
	alcCaptureStop(device);
}

HL_PRIM void HL_NAME(alc_capture_samples)(ALCdevice *device, vbyte *buffer, int samples) {
	alcCaptureSamples(device, buffer, samples);
}

#define TDEVICE  _ABSTRACT(alc_device)
#define TCONTEXT _ABSTRACT(alc_context)

DEFINE_PRIM(TCONTEXT, alc_create_context,       TDEVICE _BYTES);
DEFINE_PRIM(_BOOL,    alc_make_context_current, TCONTEXT);
DEFINE_PRIM(_VOID,    alc_process_context,      TCONTEXT);
DEFINE_PRIM(_VOID,    alc_suspend_context,      TCONTEXT);
DEFINE_PRIM(_VOID,    alc_destroy_context,      TCONTEXT);
DEFINE_PRIM(TCONTEXT, alc_get_current_context,  _NO_ARG);
DEFINE_PRIM(TDEVICE,  alc_get_contexts_device,  TCONTEXT);

DEFINE_PRIM(TDEVICE,  alc_open_device,  _BYTES); 
DEFINE_PRIM(_BOOL,    alc_close_device, TDEVICE);

DEFINE_PRIM(_I32,     alc_get_error, TDEVICE);

DEFINE_PRIM(_VOID,    alc_load_extensions,      TDEVICE);
DEFINE_PRIM(_BOOL,    alc_is_extension_present, TDEVICE _BYTES);
DEFINE_PRIM(_I32,     alc_get_enum_value,       TDEVICE _BYTES); 

DEFINE_PRIM(_BYTES,   alc_get_string,   TDEVICE _I32);
DEFINE_PRIM(_VOID,    alc_get_integerv, TDEVICE _I32 _I32 _BYTES);

DEFINE_PRIM(TDEVICE,  alc_capture_open_device,  _BYTES _I32 _I32 _I32);
DEFINE_PRIM(_BOOL,    alc_capture_close_device, TDEVICE);
DEFINE_PRIM(_VOID,    alc_capture_start,        TDEVICE);
DEFINE_PRIM(_VOID,    alc_capture_stop,         TDEVICE);
DEFINE_PRIM(_VOID,    alc_capture_samples,      TDEVICE _BYTES _I32);

// ----------------------------------------------------------------------------
// AL
// ----------------------------------------------------------------------------

#define AL_IMPORT(fun, t) t fun
#include "ALImports.h"
#undef AL_IMPORT

HL_PRIM void HL_NAME(al_doppler_factor)(float value) {
	alDopplerFactor(value);
}

HL_PRIM void HL_NAME(al_doppler_velocity)(float value) {
	alDopplerVelocity(value);
}

HL_PRIM void HL_NAME(al_speed_of_sound)(float value) {
	alSpeedOfSound(value);
}

HL_PRIM void HL_NAME(al_distance_model)(int value) {
	alDistanceModel(value);
}

// Renderer State management

HL_PRIM void HL_NAME(al_enable)(int capability) {
	alEnable(capability);
}

HL_PRIM void HL_NAME(al_disable)(int capability) {
	alDisable(capability);
}

HL_PRIM bool HL_NAME(al_is_enabled)(int capability) {
	return alIsEnabled(capability) == AL_TRUE;
}

// State retrieval

HL_PRIM void HL_NAME(al_get_booleanv)(int param, vbyte *values) {
	alGetBooleanv(param, (ALboolean*)values);
}

HL_PRIM void HL_NAME(al_get_integerv)(int param, vbyte *values) {
	alGetIntegerv(param, (ALint*)values);
}

HL_PRIM void HL_NAME(al_get_floatv)(int param, vbyte *values) {
	alGetFloatv(param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_get_doublev)(int param, vbyte *values) {
	alGetDoublev(param, (ALdouble*)values);
}

HL_PRIM vbyte *HL_NAME(al_get_string)(int param) {
	return (vbyte*)alGetString(param);
}

HL_PRIM bool HL_NAME(al_get_boolean)(int param) {
	return alGetBoolean(param) == AL_TRUE;
}

HL_PRIM int HL_NAME(al_get_integer)(int param) {
	return alGetInteger(param);
}

HL_PRIM float HL_NAME(al_get_float)(int param) {
	return alGetFloat(param);
}

HL_PRIM double HL_NAME(al_get_double)(int param) {
	return alGetDouble(param);
}

// Error retrieval

HL_PRIM int HL_NAME(al_get_error)() {
	return alGetError();
}

// Extension support

#define AL_IMPORT(fun,t) fun = (t)alGetProcAddress(#fun)
HL_PRIM void HL_NAME(al_load_extensions)() {
#	include "ALImports.h"
}


HL_PRIM bool HL_NAME(al_is_extension_present)(vbyte *extname) {
	return alIsExtensionPresent((char*)extname) == AL_TRUE;
}

HL_PRIM int HL_NAME(al_get_enum_value)(vbyte *ename) {
	return alGetEnumValue((char*)ename);
}

// Set Listener parameters

HL_PRIM void HL_NAME(al_listenerf)(int param, float value) {
	alListenerf(param, value);
}

HL_PRIM void HL_NAME(al_listener3f)(int param, float value1, float value2, float value3) {
	alListener3f(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_listenerfv)(int param, vbyte *values) {
	alListenerfv(param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_listeneri)(int param, int value) {
	alListeneri(param, value);
}

HL_PRIM void HL_NAME(al_listener3i)(int param, int value1, int value2, int value3) {
	alListener3i(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_listeneriv)(int param, vbyte *values) {
	alListeneriv(param, (ALint*)values);
}

// Get Listener parameters

HL_PRIM float HL_NAME(al_get_listenerf)(int param) {
	float value;
	alGetListenerf(param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_listener3f)(int param, float *value1, float *value2, float *value3) {
	alGetListener3f(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_listenerfv)(int param, vbyte *values) {
	alGetListenerfv(param, (ALfloat*)values);
}

HL_PRIM int HL_NAME(al_get_listeneri)(int param) {
	int value;
	alGetListeneri(param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_listener3i)(int param, int *value1, int *value2, int *value3) {
	alGetListener3i(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_listeneriv)(int param, vbyte *values) {
	alGetListeneriv(param, (ALint*)values);
}

// Source management

HL_PRIM void HL_NAME(al_gen_sources)(int n, vbyte *sources) {
	alGenSources(n, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_delete_sources)(int n, vbyte *sources) {
	alDeleteSources(n, (ALuint*)sources);
}

HL_PRIM bool HL_NAME(al_is_source)(unsigned source) {
	return alIsSource(source) == AL_TRUE;
}

// Set Source parameters

HL_PRIM void HL_NAME(al_sourcef)(unsigned source, int param, float value) {
	alSourcef(source, param, value);
}

HL_PRIM void HL_NAME(al_source3f)(unsigned source, int param, float value1, float value2, float value3) {
	alSource3f(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_sourcefv)(unsigned source, int param, vbyte *values) {
	alSourcefv(source, param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_sourcei)(unsigned source, int param, int value) {
	alSourcei(source, param, value);
}

HL_PRIM void HL_NAME(al_source3i)(unsigned source, int param, int value1, int value2, int value3) {
	alSource3i(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_sourceiv)(unsigned source, int param, vbyte *values) {
	alSourceiv(source, param, (ALint*)values);
}

// Get Source parameters

HL_PRIM float HL_NAME(al_get_sourcef)(unsigned source, int param) {
	float value;
	alGetSourcef(source, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_source3f)(unsigned source, int param, float *value1, float *value2, float *value3) {
	alGetSource3f(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_sourcefv)(unsigned source, int param, vbyte *values) {
	alGetSourcefv(source, param, (ALfloat*)values);
}

HL_PRIM int HL_NAME(al_get_sourcei)(unsigned source, int param) {
	int value;
	alGetSourcei(source, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_source3i)(unsigned source, int param, int *value1, int *value2, int *value3) {
	alGetSource3i(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_sourceiv)(unsigned source, int param, vbyte *values) {
	alGetSourceiv(source, param, (ALint*)values);
}

// Source controls

HL_PRIM void HL_NAME(al_source_playv)(int n, vbyte *sources) {
	alSourcePlayv(n, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_source_stopv)(int n, vbyte *sources) {
	alSourceStopv(n, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_source_rewindv)(int n, vbyte *sources) {
	alSourceRewindv(n, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_source_pausev)(int n, vbyte *sources) {
	alSourcePausev(n, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_source_play)(unsigned source) {
	alSourcePlay(source);
}

HL_PRIM void HL_NAME(al_source_stop)(unsigned source) {
	alSourceStop(source);
}

HL_PRIM void HL_NAME(al_source_rewind)(unsigned source) {
	alSourceRewind(source);
}

HL_PRIM void HL_NAME(al_source_pause)(unsigned source) {
	alSourcePause(source);
}

// Queue buffers onto a source

HL_PRIM void HL_NAME(al_source_queue_buffers)(unsigned source, int nb, vbyte *buffers) {
	alSourceQueueBuffers(source, nb, (ALuint*)buffers);
}

HL_PRIM void HL_NAME(al_source_unqueue_buffers)(unsigned source, int nb, vbyte *buffers) {
	alSourceUnqueueBuffers(source, nb, (ALuint*)buffers);
}

// Buffer management

HL_PRIM void HL_NAME(al_gen_buffers)(int n, vbyte *buffers) {
	alGenBuffers(n, (ALuint*)buffers);
}

HL_PRIM void HL_NAME(al_delete_buffers)(int n, vbyte *buffers) {
	alDeleteBuffers(n, (ALuint*)buffers);
}

HL_PRIM bool HL_NAME(al_is_buffer)(unsigned buffer) {
	return alIsBuffer(buffer) == AL_TRUE;
}

HL_PRIM void HL_NAME(al_buffer_data)(unsigned buffer, int format, vbyte *data, int size, int freq) {
	alBufferData(buffer, format, data, size, freq);
}

// Set Buffer parameters

HL_PRIM void HL_NAME(al_bufferf)(unsigned buffer, int param, float value) {
	alBufferf(buffer, param, value);
}

HL_PRIM void HL_NAME(al_buffer3f)(unsigned buffer, int param, float value1, float value2, float value3) {
	alBuffer3f(buffer, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_bufferfv)(unsigned buffer, int param, vbyte *values) {
	alBufferfv(buffer, param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_bufferi)(unsigned buffer, int param, int value) {
	alBufferi(buffer, param, value);
}

HL_PRIM void HL_NAME(al_buffer3i)(unsigned buffer, int param, int value1, int value2, int value3) {
	alBuffer3i(buffer, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_bufferiv)(unsigned buffer, int param, vbyte *values) {
	alBufferiv(buffer, param, (ALint*)values);
}

// Get Buffer parameters

HL_PRIM float HL_NAME(al_get_bufferf)(unsigned buffer, int param) {
	float value;
	alGetBufferf(buffer, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_buffer3f)(unsigned buffer, int param, float *value1, float *value2, float *value3) {
	alGetBuffer3f(buffer, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_bufferfv)(unsigned buffer, int param, vbyte *values) {
	alGetBufferfv(buffer, param, (ALfloat*)values);
}

HL_PRIM int HL_NAME(al_get_bufferi)(unsigned buffer, int param) {
	int value;
	alGetBufferi(buffer, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_buffer3i)(unsigned buffer, int param, int *value1, int *value2, int *value3) {
	alGetBuffer3i(buffer, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_bufferiv)(unsigned buffer, int param, vbyte *values) {
	alGetBufferiv(buffer, param, (ALint*)values);
}

DEFINE_PRIM(_VOID, al_doppler_factor  , _F32);
DEFINE_PRIM(_VOID, al_doppler_velocity, _F32);
DEFINE_PRIM(_VOID, al_speed_of_sound  , _F32);
DEFINE_PRIM(_VOID, al_distance_model  , _I32);

DEFINE_PRIM(_VOID, al_enable,    _I32);
DEFINE_PRIM(_VOID, al_disable,   _I32);
DEFINE_PRIM(_BOOL, al_is_enabled, _I32);

DEFINE_PRIM(_VOID, al_get_booleanv, _I32 _BYTES);
DEFINE_PRIM(_VOID, al_get_integerv, _I32 _BYTES);
DEFINE_PRIM(_VOID, al_get_floatv,   _I32 _BYTES);
DEFINE_PRIM(_VOID, al_get_doublev,  _I32 _BYTES);

DEFINE_PRIM(_BYTES, al_get_string,  _I32);
DEFINE_PRIM(_BOOL,  al_get_boolean, _I32);
DEFINE_PRIM(_I32,   al_get_integer, _I32);
DEFINE_PRIM(_F32,   al_get_float,   _I32);
DEFINE_PRIM(_F64,   al_get_double,  _I32);
DEFINE_PRIM(_I32,   al_get_error,   _NO_ARG);

DEFINE_PRIM(_VOID, al_load_extensions,      _NO_ARG);
DEFINE_PRIM(_BOOL, al_is_extension_present, _BYTES);
DEFINE_PRIM(_I32,  al_get_enum_value,       _BYTES);

DEFINE_PRIM(_VOID, al_listenerf,  _I32 _F32);
DEFINE_PRIM(_VOID, al_listener3f, _I32 _F32 _F32 _F32);
DEFINE_PRIM(_VOID, al_listenerfv, _I32 _BYTES);
DEFINE_PRIM(_VOID, al_listeneri,  _I32 _I32);
DEFINE_PRIM(_VOID, al_listener3i, _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_listeneriv, _I32 _BYTES);

DEFINE_PRIM(_F32,  al_get_listenerf,  _I32);
DEFINE_PRIM(_VOID, al_get_listener3f, _I32 _REF(_F32) _REF(_F32) _REF(_F32));
DEFINE_PRIM(_VOID, al_get_listenerfv, _I32 _BYTES);
DEFINE_PRIM(_I32,  al_get_listeneri,  _I32);
DEFINE_PRIM(_VOID, al_get_listener3i, _I32 _REF(_I32) _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, al_get_listeneriv, _I32 _BYTES);

DEFINE_PRIM(_VOID, al_gen_sources,    _I32 _BYTES);
DEFINE_PRIM(_VOID, al_delete_sources, _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_source,      _I32);

DEFINE_PRIM(_VOID, al_sourcef,  _I32 _I32 _F32);
DEFINE_PRIM(_VOID, al_source3f, _I32 _I32 _F32 _F32 _F32);
DEFINE_PRIM(_VOID, al_sourcefv, _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_sourcei,  _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_source3i, _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_sourceiv, _I32 _I32 _BYTES);

DEFINE_PRIM(_F32,  al_get_sourcef,  _I32 _I32);
DEFINE_PRIM(_VOID, al_get_source3f, _I32 _I32 _REF(_F32) _REF(_F32) _REF(_F32));
DEFINE_PRIM(_VOID, al_get_sourcefv, _I32 _I32 _BYTES);
DEFINE_PRIM(_I32,  al_get_sourcei,  _I32 _I32);
DEFINE_PRIM(_VOID, al_get_source3i, _I32 _I32 _REF(_I32) _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, al_get_sourceiv, _I32 _I32 _BYTES);

DEFINE_PRIM(_VOID, al_source_playv,   _I32 _BYTES);
DEFINE_PRIM(_VOID, al_source_stopv,   _I32 _BYTES);
DEFINE_PRIM(_VOID, al_source_rewindv, _I32 _BYTES);
DEFINE_PRIM(_VOID, al_source_pausev,  _I32 _BYTES);
DEFINE_PRIM(_VOID, al_source_play,    _I32);
DEFINE_PRIM(_VOID, al_source_stop,    _I32);
DEFINE_PRIM(_VOID, al_source_rewind,  _I32);
DEFINE_PRIM(_VOID, al_source_pause,   _I32);

DEFINE_PRIM(_VOID, al_source_queue_buffers,   _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_source_unqueue_buffers, _I32 _I32 _BYTES);

DEFINE_PRIM(_VOID, al_gen_buffers,    _I32 _BYTES);
DEFINE_PRIM(_VOID, al_delete_buffers, _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_buffer,      _I32);
DEFINE_PRIM(_VOID, al_buffer_data,    _I32 _I32 _BYTES _I32 _I32);

DEFINE_PRIM(_VOID, al_bufferf,  _I32 _I32 _F32);
DEFINE_PRIM(_VOID, al_buffer3f, _I32 _I32 _F32 _F32 _F32);
DEFINE_PRIM(_VOID, al_bufferfv, _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_bufferi,  _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_buffer3i, _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_bufferiv, _I32 _I32 _BYTES);

DEFINE_PRIM(_F32,  al_get_bufferf,  _I32 _I32);
DEFINE_PRIM(_VOID, al_get_buffer3f, _I32 _I32 _REF(_F32) _REF(_F32) _REF(_F32));
DEFINE_PRIM(_VOID, al_get_bufferfv, _I32 _I32 _BYTES);
DEFINE_PRIM(_I32,  al_get_bufferi,  _I32 _I32);
DEFINE_PRIM(_VOID, al_get_buffer3i, _I32 _I32 _REF(_I32) _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, al_get_bufferiv, _I32 _I32 _BYTES);

// ----------------------------------------------------------------------------
// EXTENSIONS
// ----------------------------------------------------------------------------

#define CHECK_EXT(fun) if(fun == NULL) hl_error("Unsupported extension function")

// ----------------------------------------------------------------------------
#ifdef EXT_thread_local_context
// ----------------------------------------------------------------------------

HL_PRIM bool HL_NAME(alc_set_thread_context)(ALCcontext *context) {
	CHECK_EXT(alcSetThreadContext);
	return alcSetThreadContext(context) == ALC_TRUE;
}

HL_PRIM ALCcontext* HL_NAME(alc_get_thread_context)() {
	CHECK_EXT(alcGetThreadContext);
	return alcGetThreadContext();
}

DEFINE_PRIM(_BOOL,    alc_set_thread_context, TCONTEXT);
DEFINE_PRIM(TCONTEXT, alc_get_thread_context, _NO_ARG);
#endif

// ----------------------------------------------------------------------------
#ifdef ALC_SOFT_loopback
// ----------------------------------------------------------------------------

HL_PRIM ALCdevice* HL_NAME(alc_loopback_open_device_soft)(vbyte *devicename) {
	CHECK_EXT(alcLoopbackOpenDeviceSOFT);
	return alcLoopbackOpenDeviceSOFT((char*)devicename);
}

HL_PRIM bool HL_NAME(alc_is_render_format_supported_soft)(ALCdevice *device, int freq, int channels, int type) {
	CHECK_EXT(alcIsRenderFormatSupportedSOFT);
	return alcIsRenderFormatSupportedSOFT(device, freq, channels, type) == ALC_TRUE;
}

HL_PRIM void HL_NAME(alc_render_samples_soft)(ALCdevice *device, vbyte *buffer, int samples) {
	CHECK_EXT(alcRenderSamplesSOFT);
	alcRenderSamplesSOFT(device, buffer, samples);
}

DEFINE_PRIM(TDEVICE, alc_loopback_open_device_soft,       _BYTES);
DEFINE_PRIM(_BOOL,   alc_is_render_format_supported_soft, TDEVICE _I32 _I32 _I32);
DEFINE_PRIM(_VOID,   alc_render_samples_soft,             TDEVICE _BYTES _I32);
#endif

// ----------------------------------------------------------------------------
#ifdef ALC_SOFT_pause_device
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(alc_device_pause_soft)(ALCdevice *device) {
	CHECK_EXT(alcDevicePauseSOFT);
	alcDevicePauseSOFT(device);
}

HL_PRIM void HL_NAME(alc_device_resume_soft)(ALCdevice *device) {
	CHECK_EXT(alcDeviceResumeSOFT);
	alcDeviceResumeSOFT(device);
}

DEFINE_PRIM(_VOID, alc_device_pause_soft,  TDEVICE);
DEFINE_PRIM(_VOID, alc_device_resume_soft, TDEVICE);
#endif

// ----------------------------------------------------------------------------
#ifdef SOFT_HRTF
// ----------------------------------------------------------------------------

HL_PRIM vbyte* HL_NAME(alc_get_stringi_soft)(ALCdevice *device, int param, int index) {
	CHECK_EXT(alcGetStringiSOFT);
	return (vbyte*)alcGetStringiSOFT(device, param, index);
}

HL_PRIM bool HL_NAME(alc_reset_device_soft)(ALCdevice *device, vbyte *attribs) {
	CHECK_EXT(alcResetDeviceSOFT);
	return alcResetDeviceSOFT(device, (ALCint*)attribs) == ALC_TRUE;
}

DEFINE_PRIM(_BYTES, alc_get_stringi_soft,  TDEVICE _I32 _I32);
DEFINE_PRIM(_BOOL,  alc_reset_device_soft, TDEVICE _BYTES);
#endif

// ----------------------------------------------------------------------------
#ifdef AL_EXT_STATIC_BUFFER
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_buffer_data_static)(unsigned buffer, int format, vbyte *data, int len, int freq) {
	CHECK_EXT(alBufferDataStatic);
	alBufferDataStatic(buffer, format, data, len, freq);
}

DEFINE_PRIM(_VOID, al_buffer_data_static, _I32 _I32 _BYTES _I32 _I32);
#endif

// ----------------------------------------------------------------------------
#ifdef AL_SOFT_buffer_sub_data
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_buffer_sub_data_soft)(unsigned buffer, int format, vbyte *data, int offset, int length) {
	CHECK_EXT(alBufferSubDataSOFT);
	alBufferSubDataSOFT(buffer, format, data, offset, length);
}

DEFINE_PRIM(_VOID, al_buffer_sub_data_soft, _I32 _I32 _BYTES _I32 _I32);
#endif

// ----------------------------------------------------------------------------
#ifdef AL_EXT_FOLDBACK
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_request_foldback_start)(int mode, int count, int length, vbyte *mem, vclosure *callback) {
	CHECK_EXT(alRequestFoldbackStart);
	if (callback->hasValue) if( callback->hasValue ) hl_error("Cannot set foldback on closure callback");
	alRequestFoldbackStart(mode, count, length, (ALfloat*)mem, (LPALFOLDBACKCALLBACK)callback->fun);
}

HL_PRIM void HL_NAME(al_request_foldback_stop)() {
	CHECK_EXT(alRequestFoldbackStop);
	alRequestFoldbackStop();
}

DEFINE_PRIM(_VOID, al_request_foldback_start, _I32 _I32 _I32 _BYTES _FUN(_VOID, _I32 _I32));
DEFINE_PRIM(_VOID, al_request_foldback_stop,  _NO_ARG);
#endif


// ----------------------------------------------------------------------------
#ifdef AL_SOFT_buffer_samples
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_buffer_samples_soft)(unsigned buffer, int samplerate, int internatlformat, int samples, int channels, int type, vbyte *data) {
	CHECK_EXT(alBufferSamplesSOFT);
	alBufferSamplesSOFT(buffer, samplerate, internatlformat, samples, channels, type, data);
}

HL_PRIM void HL_NAME(al_buffer_sub_samples_soft)(unsigned buffer, int offset, int samples, int channels, int type, vbyte *data) {
	CHECK_EXT(alBufferSubSamplesSOFT);
	alBufferSubSamplesSOFT(buffer, offset, samples, channels, type, data);
}

HL_PRIM void HL_NAME(al_get_buffer_samples_soft)(unsigned buffer, int offset, int samples, int channels, int type, vbyte *data) {
	CHECK_EXT(alGetBufferSamplesSOFT);
	alGetBufferSamplesSOFT(buffer, offset, samples, channels, type, data);
}

HL_PRIM bool HL_NAME(al_is_buffer_format_supported_soft)(int format) {
	CHECK_EXT(alIsBufferFormatSupportedSOFT);
	return alIsBufferFormatSupportedSOFT(format) == AL_TRUE;
}

DEFINE_PRIM(_VOID, al_buffer_samples_soft,              _I32 _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_buffer_sub_samples_soft,          _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_get_buffer_samples_soft,          _I32 _I32 _I32 _I32 _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_buffer_format_supported_soft,  _I32);
#endif

// ----------------------------------------------------------------------------
#ifdef AL_SOFT_source_latency
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_sourced_soft)(unsigned source, int param, double value) {
	CHECK_EXT(alSourcedSOFT);
	alSourcedSOFT(source, param, value);
}

HL_PRIM void HL_NAME(al_source3d_soft)(unsigned source, int param, double value1, double value2, double value3) {
	CHECK_EXT(alSource3dSOFT);
	alSource3dSOFT(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_sourcedv_soft)(unsigned source, int param, vbyte *values) {
	CHECK_EXT(alSourcedvSOFT);
	alSourcedvSOFT(source, param, (ALdouble*)values);
}

HL_PRIM double HL_NAME(al_get_sourced_soft)(unsigned source, int param) {
	double value;
	CHECK_EXT(alGetSourcedSOFT);
	alGetSourcedSOFT(source, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_source3d_soft)(unsigned source, int param, double *value1, double *value2, double *value3) {
	CHECK_EXT(alGetSource3dSOFT);
	alGetSource3dSOFT(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_sourcedv_soft)(unsigned source, int param, vbyte *values) {
	CHECK_EXT(alGetSourcedvSOFT);
	alGetSourcedvSOFT(source, param, (ALdouble*)values);
}

#define I64_COMBINE(hi, lo) (((ALint64SOFT)hi) << 32) | lo
#define I64_HI(v) v >> 32
#define I64_LO(v) v & 0xFFFFFFFF;

HL_PRIM void HL_NAME(al_sourcei64_soft)(unsigned source, int param, int valueHi, int valueLo) {
	CHECK_EXT(alSourcei64SOFT);
	alSourcei64SOFT(source, param, I64_COMBINE(valueHi, valueLo));
}

HL_PRIM void HL_NAME(al_source3i64_soft)(unsigned source, int param, 
		int value1Hi, int value1Lo, int value2Hi, int value2Lo, int value3Hi, int value3Lo) {
	CHECK_EXT(alSource3i64SOFT);
	alSource3i64SOFT(source, param, 
		I64_COMBINE(value1Hi, value1Lo), 
		I64_COMBINE(value2Hi, value2Lo), 
		I64_COMBINE(value3Hi, value3Lo));
}

HL_PRIM void HL_NAME(al_sourcei64v_soft)(unsigned source, int param, vbyte *values) {
	CHECK_EXT(alSourcei64vSOFT);
	alSourcei64vSOFT(source, param, (ALint64SOFT*)values);
}

HL_PRIM void HL_NAME(al_get_sourcei64_soft)(unsigned source, int param, int *valueHi, int *valueLo) {
	ALint64SOFT value;
	CHECK_EXT(alGetSourcei64SOFT);
	alGetSourcei64SOFT(source, param, &value);
	*valueHi = I64_HI(value);
	*valueLo = I64_LO(value);
}

HL_PRIM void HL_NAME(al_get_source3i64_soft)(unsigned source, int param, 
		int *value1Hi, int *value1Lo, int *value2Hi, int *value2Lo, int *value3Hi, int *value3Lo) {
	ALint64SOFT value1, value2, value3;
	CHECK_EXT(alGetSource3i64SOFT);
	alGetSource3i64SOFT(source, param, &value1, &value2, &value3);
	*value1Hi = I64_HI(value1);
	*value1Lo = I64_LO(value1);
	*value2Hi = I64_HI(value2);
	*value2Lo = I64_LO(value2);
	*value3Hi = I64_HI(value3);
	*value3Lo = I64_LO(value3);
}

HL_PRIM void HL_NAME(al_get_sourcei64v_soft)(unsigned source, int param, vbyte *values) {
	CHECK_EXT(alGetSourcei64vSOFT);
	alGetSourcei64vSOFT(source, param, (ALint64SOFT*)values);
}

DEFINE_PRIM(_VOID, al_sourced_soft,  _I32 _I32 _F64);
DEFINE_PRIM(_VOID, al_source3d_soft, _I32 _I32 _F64 _F64 _F64);
DEFINE_PRIM(_VOID, al_sourcedv_soft, _I32 _I32 _BYTES);

DEFINE_PRIM(_F64,  al_get_sourced_soft,  _I32 _I32);
DEFINE_PRIM(_VOID, al_get_source3d_soft, _I32 _I32 _REF(_F64) _REF(_F64) _REF(_F64));
DEFINE_PRIM(_VOID, al_get_sourcedv_soft, _I32 _I32 _BYTES);

DEFINE_PRIM(_VOID, al_sourcei64_soft,  _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_source3i64_soft, _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_sourcei64v_soft, _I32 _I32 _BYTES);

DEFINE_PRIM(_VOID, al_get_sourcei64_soft,  _I32 _I32 _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, al_get_source3i64_soft, _I32 _I32 _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, al_get_sourcei64v_soft, _I32 _I32 _BYTES);
#endif

// ----------------------------------------------------------------------------
#ifdef AL_SOFT_deferred_updates
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_defer_updates_soft)() {
	CHECK_EXT(alDeferUpdatesSOFT);
	alDeferUpdatesSOFT();
}

HL_PRIM void HL_NAME(al_process_updates_soft)() {
	CHECK_EXT(alProcessUpdatesSOFT);
	alProcessUpdatesSOFT();
}

DEFINE_PRIM(_VOID, al_defer_updates_soft,   _NO_ARG);
DEFINE_PRIM(_VOID, al_process_updates_soft, _NO_ARG);
#endif

// ----------------------------------------------------------------------------
#ifdef ALC_EXT_EFX
// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_gen_effects)(int n, vbyte *effects) {
	alGenEffects(n, (ALuint*)effects);
}

HL_PRIM void HL_NAME(al_delete_effects)(int n, vbyte *effects) {
	alDeleteEffects(n, (ALuint*)effects);
}

HL_PRIM bool HL_NAME(al_is_effect)(unsigned effect) {
	return alIsEffect(effect) == AL_TRUE;
}

HL_PRIM void HL_NAME(al_effecti)(unsigned effect, int param, int iValue) {
	alEffecti(effect, param, iValue);
}

HL_PRIM void HL_NAME(al_effectiv)(unsigned effect, int param, vbyte *piValues) {
	alEffectiv(effect, param, (ALint*)piValues);
}

HL_PRIM void HL_NAME(al_effectf)(unsigned effect, int param, float flValue) {
	alEffectf(effect, param, flValue);
}

HL_PRIM void HL_NAME(al_effectfv)(unsigned effect, int param, vbyte *pflValues) {
	alEffectfv(effect, param, (ALfloat*)pflValues);
}

HL_PRIM int HL_NAME(al_get_effecti)(unsigned effect, int param) {
	int value;
	alGetEffecti(effect, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_effectiv)(unsigned effect, int param, vbyte *piValues) {
	alGetEffectiv(effect, param, (ALint*)piValues);
}

HL_PRIM float HL_NAME(al_get_effectf)(unsigned effect, int param) {
	float value;
	alGetEffectf(effect, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_effectfv)(unsigned effect, int param, vbyte *pflValues) {
	alGetEffectfv(effect, param, (ALfloat*)pflValues);
}

DEFINE_PRIM(_VOID, al_gen_effects,    _I32 _BYTES);
DEFINE_PRIM(_VOID, al_delete_effects, _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_effect,      _I32);

DEFINE_PRIM(_VOID, al_effecti,        _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_effectiv,       _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_effectf,        _I32 _I32 _F32);
DEFINE_PRIM(_VOID, al_effectfv,       _I32 _I32 _BYTES);

DEFINE_PRIM(_I32,  al_get_effecti,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_effectiv,   _I32 _I32 _BYTES);
DEFINE_PRIM(_F32,  al_get_effectf,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_effectfv,   _I32 _I32 _BYTES);

// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_gen_filters)(int n, vbyte *filters) {
	alGenFilters(n, (ALuint*)filters);
}

HL_PRIM void HL_NAME(al_delete_filters)(int n, vbyte *filters) {
	alDeleteFilters(n, (ALuint*)filters);
}

HL_PRIM bool HL_NAME(al_is_filter)(unsigned filter) {
	return alIsFilter(filter) == AL_TRUE;
}

HL_PRIM void HL_NAME(al_filteri)(unsigned filter, int param, int iValue) {
	alFilteri(filter, param, iValue);
}

HL_PRIM void HL_NAME(al_filteriv)(unsigned filter, int param, vbyte *piValues) {
	alFilteriv(filter, param, (ALint*)piValues);
}

HL_PRIM void HL_NAME(al_filterf)(unsigned filter, int param, float flValue) {
	alFilterf(filter, param, flValue);
}

HL_PRIM void HL_NAME(al_filterfv)(unsigned filter, int param, vbyte *pflValues) {
	alFilterfv(filter, param, (ALfloat*)pflValues);
}

HL_PRIM int HL_NAME(al_get_filteri)(unsigned filter, int param) {
	int value;
	alGetFilteri(filter, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_filteriv)(unsigned filter, int param, vbyte *piValues) {
	alGetFilteriv(filter, param, (ALint*)piValues);
}

HL_PRIM float HL_NAME(al_get_filterf)(unsigned filter, int param) {
	float value;
	alGetFilterf(filter, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_filterfv)(unsigned filter, int param, vbyte *pflValues) {
	alGetFilterfv(filter, param, (ALfloat*)pflValues);
}

DEFINE_PRIM(_VOID, al_gen_filters,    _I32 _BYTES);
DEFINE_PRIM(_VOID, al_delete_filters, _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_filter,      _I32);

DEFINE_PRIM(_VOID, al_filteri,        _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_filteriv,       _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_filterf,        _I32 _I32 _F32);
DEFINE_PRIM(_VOID, al_filterfv,       _I32 _I32 _BYTES);

DEFINE_PRIM(_I32,  al_get_filteri,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_filteriv,   _I32 _I32 _BYTES);
DEFINE_PRIM(_F32,  al_get_filterf,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_filterfv,   _I32 _I32 _BYTES);

// ----------------------------------------------------------------------------

HL_PRIM void HL_NAME(al_gen_auxiliary_effect_slots)(int n, vbyte *effectslots) {
	alGenAuxiliaryEffectSlots(n, (ALuint*)effectslots);
}

HL_PRIM void HL_NAME(al_delete_auxiliary_effect_slots)(int n, vbyte *effectslots) {
	alDeleteAuxiliaryEffectSlots(n, (ALuint*)effectslots);
}

HL_PRIM bool HL_NAME(al_is_auxiliary_effect_slot)(unsigned effectslot) {
	return alIsAuxiliaryEffectSlot(effectslot) == AL_TRUE;
}

HL_PRIM void HL_NAME(al_auxiliary_effect_sloti)(unsigned effectslot, int param, int iValue) {
	alAuxiliaryEffectSloti(effectslot, param, iValue);
}

HL_PRIM void HL_NAME(al_auxiliary_effect_slotiv)(unsigned effectslot, int param, vbyte *piValues) {
	alAuxiliaryEffectSlotiv(effectslot, param, (ALint*)piValues);
}

HL_PRIM void HL_NAME(al_auxiliary_effect_slotf)(unsigned effectslot, int param, float flValue) {
	alAuxiliaryEffectSlotf(effectslot, param, flValue);
}

HL_PRIM void HL_NAME(al_auxiliary_effect_slotfv)(unsigned effectslot, int param, vbyte *pflValues) {
	alAuxiliaryEffectSlotfv(effectslot, param, (ALfloat*)pflValues);
}

HL_PRIM int HL_NAME(al_get_auxiliary_effect_sloti)(unsigned effectslot, int param) {
	int value;
	alGetAuxiliaryEffectSloti(effectslot, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_auxiliary_effect_slotiv)(unsigned effectslot, int param, vbyte *piValues) {
	alGetAuxiliaryEffectSlotiv(effectslot, param, (ALint*)piValues);
}

HL_PRIM float HL_NAME(al_get_auxiliary_effect_slotf)(unsigned effectslot, int param) {
	float value;
	alGetAuxiliaryEffectSlotf(effectslot, param, &value);
	return value;
}

HL_PRIM void HL_NAME(al_get_auxiliary_effect_slotfv)(unsigned effectslot, int param, vbyte *pflValues) {
	alGetAuxiliaryEffectSlotfv(effectslot, param, (ALfloat*)pflValues);
}

DEFINE_PRIM(_VOID, al_gen_auxiliary_effect_slots,    _I32 _BYTES);
DEFINE_PRIM(_VOID, al_delete_auxiliary_effect_slots, _I32 _BYTES);
DEFINE_PRIM(_BOOL, al_is_auxiliary_effect_slot,      _I32);

DEFINE_PRIM(_VOID, al_auxiliary_effect_sloti,        _I32 _I32 _I32);
DEFINE_PRIM(_VOID, al_auxiliary_effect_slotiv,       _I32 _I32 _BYTES);
DEFINE_PRIM(_VOID, al_auxiliary_effect_slotf,        _I32 _I32 _F32);
DEFINE_PRIM(_VOID, al_auxiliary_effect_slotfv,       _I32 _I32 _BYTES);

DEFINE_PRIM(_I32,  al_get_auxiliary_effect_sloti,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_auxiliary_effect_slotiv,   _I32 _I32 _BYTES);
DEFINE_PRIM(_F32,  al_get_auxiliary_effect_slotf,    _I32 _I32);
DEFINE_PRIM(_VOID, al_get_auxiliary_effect_slotfv,   _I32 _I32 _BYTES);
#endif