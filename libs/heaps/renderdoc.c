#define HL_NAME(n) heaps_##n
#include <hl.h>

#if !defined(HL_NO_RENDERDOC) && !defined(HL_WIN) && !defined(HL_LINUX) && !defined(HL_MAC)
#define HL_NO_RENDERDOC
#endif

#ifdef HL_NO_RENDERDOC
typedef void RENDERDOC_API_1_0_0;
#else
#include <renderdoc_app.h>
#ifdef HL_WIN
#	undef _GUID
#	include <windows.h>
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#	define dlopen(l,p)		(void*)( (l) ? LoadLibraryA(l) : (HMODULE)&__ImageBase)
#	define dlsym(h,n)		GetProcAddress((HANDLE)h,n)
#else
#	include <dlfcn.h>
#endif
#endif

RENDERDOC_API_1_0_0 *rdoc_api;

HL_PRIM bool HL_NAME(rdoc_init)() {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	pRENDERDOC_GetAPI RENDERDOC_GetAPI = NULL;
#ifdef HL_WIN
	void *mod = dlopen("renderdoc.dll", RTLD_NOW | RTLD_NOLOAD);
#elif defined(HL_LINUX)
	void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
#elif defined(HL_MAC)
	void *mod = dlopen("librenderdoc.dylib", RTLD_NOW | RTLD_NOLOAD);
#endif
	if( mod )
		RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
	if( RENDERDOC_GetAPI )
	{
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_0_0, (void **)&rdoc_api);
		if( ret == 1 )
			return true;
	}
	rdoc_api = NULL;
	return false;
#endif
}

HL_PRIM bool HL_NAME(rdoc_set_capture_keys)(vbyte *keys, int num) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	rdoc_api->SetCaptureKeys((RENDERDOC_InputButton*)keys, num);
	return true;
#endif
}

HL_PRIM bool HL_NAME(rdoc_set_capture_file_path_template)(vbyte *pathtemplate) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	rdoc_api->SetCaptureFilePathTemplate((char*)pathtemplate);
	return true;
#endif
}

HL_PRIM vbyte *HL_NAME(rdoc_get_capture_file_path_template)() {
#ifdef HL_NO_RENDERDOC
	return NULL;
#else
	if( rdoc_api == NULL )
		return NULL;
	return (vbyte *)rdoc_api->GetCaptureFilePathTemplate();
#endif
}

HL_PRIM int HL_NAME(rdoc_get_num_captures)() {
#ifdef HL_NO_RENDERDOC
	return -1;
#else
	if( rdoc_api == NULL )
		return -1;
	return rdoc_api->GetNumCaptures();
#endif
}

HL_PRIM bool HL_NAME(rdoc_get_capture)(int index, vbyte *filename, int *pathlength, int64 *timestamp) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	int ret = rdoc_api->GetCapture(index, (char*)filename, (uint32_t*)pathlength, (uint64_t*)timestamp);
	return ret == 1 ? true : false;
#endif
}

HL_PRIM bool HL_NAME(rdoc_trigger_capture)() {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	rdoc_api->TriggerCapture();
	return true;
#endif
}

HL_PRIM bool HL_NAME(rdoc_is_target_control_connected)() {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	int ret = rdoc_api->IsTargetControlConnected();
	return ret == 1 ? true : false;
#endif
}

HL_PRIM bool HL_NAME(rdoc_launch_replay_ui)(int connectTargetControl, vbyte *cmdline) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	int ret = rdoc_api->LaunchReplayUI(connectTargetControl, (char*)cmdline);
	return ret == 1 ? true : false;
#endif
}

HL_PRIM bool HL_NAME(rdoc_start_frame_capture)(void *device, void *wndHandle) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	rdoc_api->StartFrameCapture(device, wndHandle);
	return true;
#endif
}

HL_PRIM bool HL_NAME(rdoc_is_frame_capturing)() {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	int ret = rdoc_api->IsFrameCapturing();
	return ret == 1 ? true : false;
#endif
}

HL_PRIM bool HL_NAME(rdoc_end_frame_capture)(void *device, void *wndHandle) {
#ifdef HL_NO_RENDERDOC
	return false;
#else
	if( rdoc_api == NULL )
		return false;
	int ret = rdoc_api->EndFrameCapture(device, wndHandle);
	return ret == 1 ? true : false;
#endif
}

DEFINE_PRIM(_BOOL, rdoc_init, _NO_ARG);
DEFINE_PRIM(_BOOL, rdoc_set_capture_keys, _BYTES _I32);
DEFINE_PRIM(_BOOL, rdoc_set_capture_file_path_template, _BYTES);
DEFINE_PRIM(_BYTES, rdoc_get_capture_file_path_template, _NO_ARG);
DEFINE_PRIM(_I32, rdoc_get_num_captures, _NO_ARG);
DEFINE_PRIM(_BOOL, rdoc_get_capture, _I32 _BYTES _REF(_I32) _REF(_I64));
DEFINE_PRIM(_BOOL, rdoc_trigger_capture, _NO_ARG);
DEFINE_PRIM(_BOOL, rdoc_is_target_control_connected, _NO_ARG);
DEFINE_PRIM(_BOOL, rdoc_launch_replay_ui, _I32 _BYTES);
DEFINE_PRIM(_BOOL, rdoc_start_frame_capture, _DYN _DYN);
DEFINE_PRIM(_BOOL, rdoc_is_frame_capturing, _NO_ARG);
DEFINE_PRIM(_BOOL, rdoc_end_frame_capture, _DYN _DYN);
