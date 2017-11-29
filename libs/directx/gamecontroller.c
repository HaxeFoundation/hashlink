#define HL_NAME(n) directx_##n
#include <hl.h>
#include <xinput.h>
#include <InitGuid.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <dbt.h>

#define HAT_CENTERED    0x00
#define HAT_UP          0x01
#define HAT_RIGHT       0x02
#define HAT_DOWN        0x04
#define HAT_LEFT        0x08

typedef struct {
	hl_type *t;
	int num;
	int mask; // for hat only
	int axis;
} dinput_mapping_btn;

typedef struct {
	hl_type *t;
	unsigned int guid;
	vbyte *name;
	varray *button; // dinput_mapping_btn
} dinput_mapping;

typedef struct _dx_gctrl_device dx_gctrl_device;
struct _dx_gctrl_device {
	char *name;
	bool isNew;
	int xUID;
	GUID dGuid;
	LPDIRECTINPUTDEVICE8 dDevice;
	dinput_mapping *dMapping;
	dx_gctrl_device *next;
};

typedef struct {
	hl_type *t;
	dx_gctrl_device *device;
	int index;
	vstring *name;
	int buttons;
	vbyte *axes;
	vdynamic *rumbleEnd;
} dx_gctrl_data;

static dx_gctrl_device *dx_gctrl_devices = NULL;
static dx_gctrl_device *dx_gctrl_removed = NULL;
static CRITICAL_SECTION dx_gctrl_cs;
static bool dx_gctrl_syncNeeded = FALSE;

// DirectInput specific
static LPDIRECTINPUT8 gctrl_dinput = NULL;
static varray *gctrl_dinput_mappings = NULL;
static HWND gctrl_dinput_win = NULL;
static bool gctrl_dinput_changed = FALSE;

// XInput
static void gctrl_xinput_add(int uid, dx_gctrl_device **current) {
	dx_gctrl_device *device = *current;
	dx_gctrl_device *prev = NULL;
	while (device) {
		if (device->xUID == uid) {
			if (device == *current) *current = device->next;
			else if (prev) prev->next = device->next;

			device->next = dx_gctrl_devices;
			dx_gctrl_devices = device;
			return;
		}

		prev = device;
		device = device->next;
	}

	device = (dx_gctrl_device*)malloc(sizeof(dx_gctrl_device));
	ZeroMemory(device, sizeof(dx_gctrl_device));
	device->name = "XInput controller";
	device->xUID = uid;
	device->isNew = TRUE;
	device->next = dx_gctrl_devices;
	dx_gctrl_devices = device;
	dx_gctrl_syncNeeded = TRUE;
}

static void gctrl_xinput_update(dx_gctrl_data *data) {
	XINPUT_STATE state;
	HRESULT r = XInputGetState(data->device->xUID, &state);
	if (FAILED(r))
		return;

	data->buttons = (state.Gamepad.wButtons & 0x0FFF) | (state.Gamepad.wButtons & 0xF000) >> 2;
	((double*)data->axes)[0] = (double)(state.Gamepad.sThumbLX < -32767 ? -1. : (state.Gamepad.sThumbLX / 32767.));
	((double*)data->axes)[1] = (double)(state.Gamepad.sThumbLY < -32767 ? -1. : (state.Gamepad.sThumbLY / 32767.));
	((double*)data->axes)[2] = (double)(state.Gamepad.sThumbRX < -32767 ? -1. : (state.Gamepad.sThumbRX / 32767.));
	((double*)data->axes)[3] = (double)(state.Gamepad.sThumbRY < -32767 ? -1. : (state.Gamepad.sThumbRY / 32767.));
	((double*)data->axes)[4] = (double)(state.Gamepad.bLeftTrigger / 255.);
	((double*)data->axes)[5] = (double)(state.Gamepad.bRightTrigger / 255.);
}

// DirectInput

static LRESULT CALLBACK gctrl_WndProc(HWND wnd, UINT umsg, WPARAM wparam, LPARAM lparam) {
	switch (umsg) {
	case WM_DEVICECHANGE:
		switch (wparam) {
		case DBT_DEVICEARRIVAL:
		case DBT_DEVICEREMOVECOMPLETE:
			if (((DEV_BROADCAST_HDR*)lparam)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
				gctrl_dinput_changed = TRUE;
			}
			break;
		}
	}
	return DefWindowProc(wnd, umsg, wparam, lparam);
}

static void gctrl_dinput_init( varray *mappings ) {
	if( !gctrl_dinput) {
		DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, &IID_IDirectInput8, &gctrl_dinput, NULL);
		if (gctrl_dinput) {
			WNDCLASSEX wc;
			memset(&wc, 0, sizeof(wc));
			wc.lpfnWndProc = gctrl_WndProc;
			wc.hInstance = GetModuleHandle(NULL);
			wc.lpszClassName = USTR("HL_GCTRL");
			wc.cbSize = sizeof(WNDCLASSEX);
			RegisterClassEx(&wc);

			gctrl_dinput_win = CreateWindowEx(0, USTR("HL_GCTRL"), NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

			GUID guid = { 0x4D1E55B2L, 0xF16F, 0x11CF,{ 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
			DEV_BROADCAST_DEVICEINTERFACE dbh;
			memset(&dbh, 0, sizeof(dbh));
			dbh.dbcc_size = sizeof(dbh);
			dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
			dbh.dbcc_classguid = guid;
			RegisterDeviceNotification(gctrl_dinput_win, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE);
			gctrl_dinput_changed = TRUE;
		}
	}

	if( mappings ) {
		if( gctrl_dinput_mappings )
			hl_remove_root(&gctrl_dinput_mappings);
		gctrl_dinput_mappings = mappings;
		hl_add_root(&gctrl_dinput_mappings);
	}
}

static BOOL CALLBACK gctrl_dinput_deviceCb(const DIDEVICEINSTANCE *instance, void *context) {
	dx_gctrl_device **current = (dx_gctrl_device**)context;
	dx_gctrl_device *device = *current;
	dx_gctrl_device *prev = NULL;
	LPDIRECTINPUTDEVICE8 di_device;
	HRESULT result;
	dinput_mapping *mapping = NULL;

	while( device ) {
		if(device->xUID < 0 && !memcmp(&device->dGuid, &instance->guidInstance, sizeof(device->dGuid)) ) {
			if( device == *current ) *current = device->next;
			else if( prev ) prev->next = device->next;

			device->next = dx_gctrl_devices;
			dx_gctrl_devices = device;
			return DIENUM_CONTINUE;
		}

		prev = device;
		device = device->next;
	}

	// Find mapping
	for( int i=0; i<gctrl_dinput_mappings->size; i++ ){
		dinput_mapping *m = hl_aptr(gctrl_dinput_mappings, dinput_mapping*)[i];
		if( instance->guidProduct.Data1 == m->guid ) {
			mapping = m;
			break;
		}
	}
	if (!mapping ) return DIENUM_CONTINUE;

	result = IDirectInput8_CreateDevice(gctrl_dinput, &instance->guidInstance, &di_device, NULL);
	if( FAILED(result) ) return DIENUM_CONTINUE;
	
	device = (dx_gctrl_device*)malloc(sizeof(dx_gctrl_device));
	ZeroMemory(device, sizeof(dx_gctrl_device));

	result = IDirectInputDevice8_QueryInterface(di_device, &IID_IDirectInputDevice8, (LPVOID *)&device->dDevice);
	IDirectInputDevice8_Release(di_device);
	if( FAILED(result) ) {
		free(device);
		return DIENUM_CONTINUE;
	}

	result = IDirectInputDevice8_SetDataFormat(device->dDevice, &c_dfDIJoystick2);
	if( FAILED(result) ) {
		free(device);
		return DIENUM_CONTINUE;
	}

	device->name = mapping->name;
	device->xUID = -1;
	device->dMapping = mapping;
	memcpy(&device->dGuid, &instance->guidInstance, sizeof(device->dGuid));
	device->isNew = TRUE;
	device->next = dx_gctrl_devices;
	dx_gctrl_devices = device;
	dx_gctrl_syncNeeded = TRUE;

	return DIENUM_CONTINUE;
}

static void gctrl_dinput_remove(dx_gctrl_device *device) {
	IDirectInputDevice8_Unacquire(device->dDevice);
	IDirectInputDevice8_Release(device->dDevice);
}

// Based on SDL ( https://hg.libsdl.org/SDL/file/007dfe83abf8/src/joystick/windows/SDL_dinputjoystick.c )
static int gctrl_dinput_translatePOV(DWORD value) {
	const int HAT_VALS[] = {
		HAT_UP,
		HAT_UP | HAT_RIGHT,
		HAT_RIGHT,
		HAT_DOWN | HAT_RIGHT,
		HAT_DOWN,
		HAT_DOWN | HAT_LEFT,
		HAT_LEFT,
		HAT_UP | HAT_LEFT
	};

	if( LOWORD(value) == 0xFFFF ) return HAT_CENTERED;

	value += 4500 / 2;
	value %= 36000;
	value /= 4500;

	if( value < 0 || value >= 8 ) return HAT_CENTERED;

	return HAT_VALS[value];
}

static void gctrl_dinput_update(dx_gctrl_data *data) {
	DIJOYSTATE2 state;
	long *p = (long*)&state;
	dinput_mapping *mapping = data->device->dMapping;
	LPDIRECTINPUTDEVICE8 dDevice = data->device->dDevice;

	HRESULT result = IDirectInputDevice8_GetDeviceState(dDevice, sizeof(DIJOYSTATE2), &state);
	if( result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED ) {
		IDirectInputDevice8_Acquire(dDevice);
		result = IDirectInputDevice8_GetDeviceState(dDevice, sizeof(DIJOYSTATE2), &state);
	}
	if( result != DI_OK )
		return;
	
	data->buttons = 0;
	for( int i = 0; i < mapping->button->size; i++ ) {
		dinput_mapping_btn *bmap = hl_aptr(mapping->button, dinput_mapping_btn*)[i];
		if (!bmap) continue;

		int val;
		if( bmap->axis < 0 && bmap->mask == 0 ) {
			val = state.rgbButtons[bmap->num];
		} else if( bmap->axis < 0 ){
			val = gctrl_dinput_translatePOV(state.rgdwPOV[bmap->num])&bmap->mask;
		} else {
			val = *(p + bmap->axis);
		}

		switch (i) {
			case 0: 
			case 2:
				((double*)data->axes)[i] = (double)(val / 65535.) * 2 - 1.; 
				break;
			case 1:
			case 3:
				((double*)data->axes)[i] = (double)(val / 65535.) * - 2 + 1.;
				break;
			case 4:
			case 5:
				((double*)data->axes)[i] = bmap->axis < 0 ? (val > 0 ? 1 : 0) : (double)(val / 65535.);
				break;
			default: 
				data->buttons |= (val > 0 ? 1 : 0) << (i - 6);
				break;
		}
	}
}

// 

void gctrl_detect_thread(void *p) {
	while (true) {
		EnterCriticalSection(&dx_gctrl_cs);

		dx_gctrl_device *current = dx_gctrl_devices;
		dx_gctrl_devices = NULL;

		// XInput
		for (int uid = XUSER_MAX_COUNT - 1; uid >= 0; uid--) {
			XINPUT_CAPABILITIES capabilities;
			if (XInputGetCapabilities(uid, XINPUT_FLAG_GAMEPAD, &capabilities) == ERROR_SUCCESS)
				gctrl_xinput_add(uid, &current);
		}

		// DInput
		if (gctrl_dinput ) {
			if (gctrl_dinput_changed) {
				IDirectInput8_EnumDevices(gctrl_dinput, DI8DEVCLASS_GAMECTRL, gctrl_dinput_deviceCb, &current, DIEDFL_ATTACHEDONLY);
				gctrl_dinput_changed = FALSE;
			} else {
				while (current) {
					dx_gctrl_device *next = current->next;
					if (current->xUID < 0) {
						current->next = dx_gctrl_devices;
						dx_gctrl_devices = current;
					}
					current = next;
				}
			}
		}

		while (current) {
			dx_gctrl_device *next = current->next;
			current->next = dx_gctrl_removed;
			dx_gctrl_removed = current;
			dx_gctrl_syncNeeded = TRUE;
			current = next;
		}

		LeaveCriticalSection(&dx_gctrl_cs);
		Sleep(300);
	}
}

HL_PRIM void HL_NAME(gctrl_init)( varray *mappings ) {
	gctrl_dinput_init( mappings );
	InitializeCriticalSection(&dx_gctrl_cs);
	hl_thread_start(gctrl_detect_thread, NULL, FALSE);
}


static void gctrl_onDetect( vclosure *onDetect, dx_gctrl_device *device, const char *name) {
	if( !onDetect ) return;
	if( onDetect->hasValue )
		((void(*)(void *, dx_gctrl_device*, vbyte*))onDetect->fun)(onDetect->value, device, (vbyte*)name);
	else
		((void(*)(dx_gctrl_device*, vbyte*))onDetect->fun)(device, (vbyte*)name);
}

HL_PRIM void HL_NAME(gctrl_detect)( vclosure *onDetect ) {
	if (dx_gctrl_syncNeeded) {
		EnterCriticalSection(&dx_gctrl_cs);

		dx_gctrl_device *device = dx_gctrl_devices;
		while (device) {
			if (device->isNew) {
				gctrl_onDetect(onDetect, device, device->name);
				device->isNew = FALSE;
			}
			device = device->next;
		}

		while (dx_gctrl_removed) {
			device = dx_gctrl_removed;
			dx_gctrl_removed = device->next;

			gctrl_onDetect(onDetect, device, NULL);
			if (device->dDevice)
				gctrl_dinput_remove(device);
			free(device);
		}

		dx_gctrl_syncNeeded = FALSE;
		LeaveCriticalSection(&dx_gctrl_cs);
	}
}

HL_PRIM void HL_NAME(gctrl_update)(dx_gctrl_data *data) {
	if( data->device->xUID >= 0 )
		gctrl_xinput_update(data);
	else if( data->device->dDevice && data->device->dMapping )
		gctrl_dinput_update(data);
}

HL_PRIM void HL_NAME(gctrl_set_vibration)(dx_gctrl_device *device, double strength) {
	if( device->xUID >= 0 ){
		XINPUT_VIBRATION vibration;
		vibration.wLeftMotorSpeed = vibration.wRightMotorSpeed = (WORD)(strength * 65535);
		XInputSetState(device->xUID, &vibration);
	}
}

#define TGAMECTRL _ABSTRACT(dx_gctrl_device)
DEFINE_PRIM(_VOID, gctrl_init, _ARR);
DEFINE_PRIM(_VOID, gctrl_detect, _FUN(_VOID, TGAMECTRL _BYTES));
DEFINE_PRIM(_VOID, gctrl_update, _OBJ(TGAMECTRL _I32 _STRING _I32 _BYTES _NULL(_F64)));
DEFINE_PRIM(_VOID, gctrl_set_vibration, TGAMECTRL _F64);