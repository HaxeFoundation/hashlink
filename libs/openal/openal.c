#define HL_NAME(n) al_##n
#include <hl.h>

#include <AL/al.h>

HL_PRIM void HL_NAME(al_doppler_factor)(float value) {
    alDopplerFactor(value);
}

HL_PRIM void HL_NAME(al_doppler_velocity)(float value) {
    alDopplerVelocity(value);
}

HL_PRIM void HL_NAME(al_speed_of_sound)(float value) {
    alSpeedOfSound(value);
}

HL_PRIM void HL_NAME(al_distance_model)(float value) {
    alDistanceModel(value);
}

/* Renderer State management */

HL_PRIM void HL_NAME(al_enable)(int capability) {
    alEnable(capability);
}

HL_PRIM void HL_NAME(al_disable)(int capability) {
    alDisable(capability);
}

HL_PRIM bool HL_NAME(al_is_enabled)(int capability) {
    return alIsEnabled(capability);
}

/* State retrieval */

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
    return alGetBoolean(param);
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

/* Error retrieval */

HL_PRIM int HL_NAME(al_get_error)() {
    return alGetError();
}

/* Extension support */

HL_PRIM bool HL_NAME(al_is_extension_present)(vbyte *extname) {
    return alGetError(extname);
}

HL_PRIM int HL_NAME(al_get_enum_value)(vbyte *ename) {
    return alGetEnumValue(ename);
}

/* Set Listener parameters */

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

/* Get Listener parameters */

HL_PRIM void HL_NAME(al_get_listenerf)(int param, float *value) {
    alGetListenerf(param, value);
}

HL_PRIM void HL_NAME(al_get_listener3f)(int param, float *value1, float *value2, float *value3) {
    alGetListener3f(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_listenerfv)(int param, vbyte *values) {
    alGetListenerfv(param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_get_listeneri)(int param, int *value) {
    alGetListeneri(param, value);
}

HL_PRIM void HL_NAME(al_get_listener3i)(int param, int *value1, int *value2, int *value3) {
    alGetListener3i(param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_listeneriv)(int param, vbyte *values) {
    alGetListeneriv(param, (ALint*)values);
}

/* Source management */

HL_PRIM void HL_NAME(al_gen_sources)(int n, vbyte *sources) {
    alGenSources(param, (ALuint*)sources);
}

HL_PRIM void HL_NAME(al_delete_sources)(int n, vbyte *sources) {
    alDeleteSources(param, (ALuint*)sources);
}

HL_PRIM bool HL_NAME(al_is_source)(unsigned source) {
    return alIsSource(source);
}

/* Set Source parameters */

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

/* Get Source parameters */

HL_PRIM void HL_NAME(al_get_sourcef)(unsigned source, int param, float *value) {
    alGetSourcef(source, param, value);
}

HL_PRIM void HL_NAME(al_get_source3f)(unsigned source, int param, float *value1, float *value2, float *value3) {
    alGetSource3f(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_sourcefv)(unsigned source, int param, vbyte *values) {
    alGetSourcefv(source, param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_get_sourcei)(unsigned source, int param, int *value) {
    alGetSourcei(source, param, value);
}

HL_PRIM void HL_NAME(al_get_source3i)(unsigned source, int param, int *value1, int *value2, int *value3) {
    alGetSource3i(source, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_sourceiv)(unsigned source, int param, vbyte *values) {
    alGetSourceiv(source, param, (ALint*)values);
}

/* Source controls */

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

/* Queue buffers onto a source */

HL_PRIM void HL_NAME(al_source_queue_buffers)(unsigned source, int nb, vbyte *buffers) {
    alSourceQueueBuffers(source, nb, (ALuint*)buffers);
}

HL_PRIM void HL_NAME(al_source_unqueue_buffers)(unsigned source, int nb, vbyte *buffers) {
    sourceUnqueueBuffers(source, nb, (ALuint*)buffers);
}

/* Buffer management */

HL_PRIM void HL_NAME(al_gen_buffers)(int n, vbyte *buffers) {
    alGenBuffers(n, (ALuint*)buffers);
}

HL_PRIM void HL_NAME(al_delete_buffers)(int n, vbyte *buffers) {
    alDeleteBuffers(n, (ALuint*)buffers);
}

HL_PRIM bool HL_NAME(al_is_buffer)(unsigned buffer) {
    return alIsBuffer(buffer);
}

HL_PRIM void HL_NAME(al_buffer_data)(unsigned buffer, int format, vbyte *data, int size, int freq) {
    alBufferData(buffer, format, data, size, freq);
}

/* Set Buffer parameters */

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

/* Get Buffer parameters */

HL_PRIM void HL_NAME(al_get_bufferf)(unsigned buffer, int param, float *value) {
    alGetBufferf(buffer, param, value);
}

HL_PRIM void HL_NAME(al_get_buffer3f)(unsigned buffer, int param, float *value1, float *value2, float *value3) {
    alGetBuffer3f(buffer, param, value1, value2, value3);
}

HL_PRIM void HL_NAME(al_get_bufferfv)(unsigned buffer, int param, vbyte *values) {
    alGetBufferfv(buffer, param, (ALfloat*)values);
}

HL_PRIM void HL_NAME(al_get_bufferi)(unsigned buffer, int param, int *value) {
    alGetBufferi(buffer, param, value);
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
DEFINE_PRIM(_BOOL, al_isEnabled, _I32);

DEFINE_PRIM(_VOID, al_get_booleanv, _I32, _BYTES);
DEFINE_PRIM(_VOID, al_get_integerv, _I32, _BYTES);
DEFINE_PRIM(_VOID, al_get_floatv,   _I32, _BYTES);
DEFINE_PRIM(_VOID, al_get_doublev,  _I32, _BYTES);

DEFINE_PRIM(_BYTES, al_get_string,  _I32);
DEFINE_PRIM(_BOOL,  al_get_boolean, _I32);
DEFINE_PRIM(_I32,   al_get_integer, _I32);
DEFINE_PRIM(_F32,   al_get_float,   _I32);
DEFINE_PRIM(_F64,   al_get_double,  _I32);
DEFINE_PRIM(_I32,   al_get_error,  _NO_ARG);

DEFINE_PRIM(_BOOL, al_is_extension_present, _BYTES);
DEFINE_PRIM(_I32,  al_get_enum_value,       _BYTES);

DEFINE_PRIM(_VOID, al_listenerf,  _I32, _F32);
DEFINE_PRIM(_VOID, al_listener3f, _I32, _F32, _F32, F32);
DEFINE_PRIM(_VOID, al_listenerfv, _I32, _BYTES);
DEFINE_PRIM(_VOID, al_listeneri,  _I32, _I32);
DEFINE_PRIM(_VOID, al_listener3i, _I32, _I32, _I32, _I32);
DEFINE_PRIM(_VOID, al_listeneriv, _I32, _BYTES);

DEFINE_PRIM(_VOID, al_get_listenerf,   _I32, _REF(_F32));
DEFINE_PRIM(_VOID, al_get_llistener3f, _I32, _REF(_F32), _REF(_F32), _REF(F32));
DEFINE_PRIM(_VOID, al_get_llistenerfv, _I32, _BYTES);
DEFINE_PRIM(_VOID, al_get_llisteneri,  _I32, _REF(_I32));
DEFINE_PRIM(_VOID, al_get_llistener3i, _I32, _REF(_I32), _REF(_I32), _REF(_I32));
DEFINE_PRIM(_VOID, al_get_llisteneriv, _I32, _BYTES);
