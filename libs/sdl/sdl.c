#define HL_NAME(n) sdl_##n

#include <hl.h>

#ifdef _WIN32
#	include <SDL.h>
#	include <SDL_syswm.h>
#else
#	include <SDL2/SDL.h>
#endif

#ifndef _SDL_H
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

typedef struct {
	int x;
	int y;
	int w;
	int h;
	int style;
} wsave_pos;

typedef enum {
	Quit,
	MouseMove,
	MouseLeave,
	MouseDown,
	MouseUp,
	MouseWheel,
	WindowState,
	KeyDown,
	KeyUp,
	TextInput,
	GControllerAdded = 100,
	GControllerRemoved,
	GControllerDown,
	GControllerUp,
	GControllerAxis
} event_type;

typedef enum {
	Show,
	Hide,
	Expose,
	Move,
	Resize,
	Minimize,
	Maximize,
	Restore,
	Enter,
	Leave,
	Focus,
	Blur,
	Close
} ws_change;

typedef struct {
	hl_type *t;
	event_type type;
	int mouseX;
	int mouseY;
	int button;
	int wheelDelta;
	ws_change state;
	int keyCode;
	bool keyRepeat;
	int controller;
	int value;
} event_data;

HL_PRIM bool HL_NAME(init_once)() {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		return false;
#	ifdef _WIN32
	// Set the internal windows timer period to 1ms (will give accurate sleep for vsync)
	timeBeginPeriod(1);
#	endif
	// default GL parameters
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	return true;
}

HL_PRIM void HL_NAME(gl_options)( int major, int minor, int depth, int stencil, int flags ) {
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencil);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, (flags&1));
	if( flags&2 )
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	else if( flags&4 )
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	else if( flags&8 )
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0); // auto
}

HL_PRIM bool HL_NAME(event_loop)( event_data *event ) {
	while (true) {
		SDL_Event e;
		if (SDL_PollEvent(&e) == 0) break;
		switch (e.type) {
		case SDL_QUIT:
			event->type = Quit;
			break;
		case SDL_MOUSEMOTION:
			event->type = MouseMove;
			event->mouseX = e.motion.x;
			event->mouseY = e.motion.y;
			break;
		case SDL_KEYDOWN:
			event->type = KeyDown;
			event->keyCode = e.key.keysym.sym;
			event->keyRepeat = e.key.repeat != 0;
			break;
		case SDL_KEYUP:
			event->type = KeyUp;
			event->keyCode = e.key.keysym.sym;
			break;
		case SDL_SYSWMEVENT:
			continue;
		case SDL_MOUSEBUTTONDOWN:
			event->type = MouseDown;
			event->button = e.button.button;
			event->mouseX = e.button.x;
			event->mouseY = e.motion.y;
			break;
		case SDL_MOUSEBUTTONUP:
			event->type = MouseUp;
			event->button = e.button.button;
			event->mouseX = e.button.x;
			event->mouseY = e.motion.y;
			break;
		case SDL_MOUSEWHEEL:
			event->type = MouseWheel;
			event->wheelDelta = e.wheel.y;
#						if SDL_VERSION_ATLEAST(2,0,4)
			if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) event->wheelDelta *= -1;
#						endif
			event->mouseX = e.wheel.x;
			event->mouseY = e.wheel.y;
			break;
		case SDL_WINDOWEVENT:
			event->type = WindowState;
			switch (e.window.event) {
			case SDL_WINDOWEVENT_SHOWN:
				event->state = Show;
				break;
			case SDL_WINDOWEVENT_HIDDEN:
				event->state = Hide;
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				event->state = Expose;
				break;
			case SDL_WINDOWEVENT_MOVED:
				event->state = Move;
				break;
			case SDL_WINDOWEVENT_RESIZED:
				event->state = Resize;
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				event->state = Minimize;
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				event->state = Maximize;
				break;
			case SDL_WINDOWEVENT_RESTORED:
				event->state = Restore;
				break;
			case SDL_WINDOWEVENT_ENTER:
				event->state = Enter;
				break;
			case SDL_WINDOWEVENT_LEAVE:
				event->state = Leave;
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				event->state = Focus;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				event->state = Blur;
				break;
			case SDL_WINDOWEVENT_CLOSE:
				event->state = Close;
				break;
			default:
				//printf("Unknown window state code %d\\n", e.window.event);
				continue;
			}
			break;
		case SDL_TEXTEDITING:
			// skip
			continue;
		case SDL_TEXTINPUT:
			event->type = TextInput;
			event->keyCode = *(int*)e.text.text;
			event->keyCode &= e.text.text[0] ? e.text.text[1] ? e.text.text[2] ? e.text.text[3] ? 0xFFFFFFFF : 0xFFFFFF : 0xFFFF : 0xFF : 0;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			event->type = GControllerAdded;
			event->controller = e.jdevice.which;
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			event->type = GControllerRemoved;
			event->controller = e.jdevice.which;
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			event->type = GControllerDown;
			event->controller = e.cbutton.which;
			event->button = e.cbutton.button;
			break;
		case SDL_CONTROLLERBUTTONUP:
			event->type = GControllerUp;
			event->controller = e.cbutton.which;
			event->button = e.cbutton.button;
			break;
		case SDL_CONTROLLERAXISMOTION:
			event->type = GControllerAxis;
			event->controller = e.caxis.which;
			event->button = e.caxis.axis;
			event->value = e.caxis.value;
			break;
		default:
			//printf("Unknown event type 0x%X\\n", e.type);
			continue;
		}
		return true;
	}
	return false;
}

HL_PRIM void HL_NAME(quit)() {
	SDL_Quit();
#	ifdef _WIN32
	timeEndPeriod(1);
#	endif
}

HL_PRIM void HL_NAME(delay)( int time ) {
	SDL_Delay(time);
}

HL_PRIM int HL_NAME(get_screen_width)() {
	SDL_DisplayMode e;
	SDL_GetCurrentDisplayMode(0, &e);
	return e.w;
}

HL_PRIM int HL_NAME(get_screen_height)() {
	SDL_DisplayMode e;
	SDL_GetCurrentDisplayMode(0, &e);
	return e.h;
}


HL_PRIM void HL_NAME(message_box)(vbyte *title, vbyte *text, bool error) {
	SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : 0, (char*)title, (char*)text, NULL);
}

HL_PRIM void HL_NAME(set_vsync)(bool v) {
	SDL_GL_SetSwapInterval(v ? 1 : 0);
}

HL_PRIM bool HL_NAME(detect_win32)() {
#	ifdef _WIN32
	return true;
#	else
	return false;
#	endif
}

HL_PRIM void HL_NAME(text_input)( bool enable ) {
	if( enable )
		SDL_StartTextInput();
	else
		SDL_StopTextInput();
}

HL_PRIM int HL_NAME(set_relative_mouse_mode)(bool enable) {
	return SDL_SetRelativeMouseMode(enable);
}

HL_PRIM vbyte *HL_NAME(detect_keyboard_layout)() {
	char q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q);
	char w = SDL_GetKeyFromScancode(SDL_SCANCODE_W);
	char y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y);

	if (q == 'q' && w == 'w' && y == 'y') return "qwerty";
	if (q == 'a' && w == 'z' && y == 'y') return "azerty";
	if (q == 'q' && w == 'w' && y == 'z') return "qwertz";
	if (q == 'q' && w == 'z' && y == 'y') return "qzerty";
	return "unknown";
}

DEFINE_PRIM(_BOOL, init_once, _NO_ARG);
DEFINE_PRIM(_VOID, gl_options, _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, event_loop, _OBJ(_I32 _I32 _I32 _I32 _I32 _I32 _I32 _BOOL _I32 _I32) );
DEFINE_PRIM(_VOID, quit, _NO_ARG);
DEFINE_PRIM(_VOID, delay, _I32);
DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);
DEFINE_PRIM(_VOID, message_box, _BYTES _BYTES _BOOL);
DEFINE_PRIM(_VOID, set_vsync, _BOOL);
DEFINE_PRIM(_BOOL, detect_win32, _NO_ARG);
DEFINE_PRIM(_VOID, text_input, _BOOL);
DEFINE_PRIM(_I32, set_relative_mouse_mode, _BOOL);
DEFINE_PRIM(_BYTES, detect_keyboard_layout, _NO_ARG);

// Window

HL_PRIM SDL_Window *HL_NAME(win_create)(int width, int height) {
	SDL_Window *w;
	w = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
#	ifdef HL_WIN
	// force window to show even if the debugger force process windows to be hidden
	if( (SDL_GetWindowFlags(w) & SDL_WINDOW_INPUT_FOCUS) == 0 ) {
		SDL_HideWindow(w);
		SDL_ShowWindow(w);
	}
#	endif
	return w;
}

HL_PRIM SDL_GLContext HL_NAME(win_get_glcontext)(SDL_Window *win) {
	return SDL_GL_CreateContext(win);
}

HL_PRIM bool HL_NAME(win_set_fullscreen)(SDL_Window *win, int mode) {
#	ifdef HL_WIN
	wsave_pos *save = SDL_GetWindowData(win,"save");
	SDL_SysWMinfo info;
	HWND wnd;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(win,&info);
	wnd = info.info.win.window;
	if( save && mode != 2 ) {
		// exit borderless
		SetWindowLong(wnd,GWL_STYLE,save->style);
		SetWindowPos(wnd,NULL,save->x,save->y,save->w,save->h,0);
		free(save);
		SDL_SetWindowData(win,"save",NULL);
		save = NULL;
	}
#	endif
	switch( mode ) {
	case 0: // WINDOWED
		return SDL_SetWindowFullscreen(win, 0) == 0;
	case 1: // FULLSCREEN
		return SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
	case 2: // BORDERLESS
#		ifdef _WIN32
		{
			HMONITOR hmon = MonitorFromWindow(wnd,MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			RECT r;
			if( !GetMonitorInfo(hmon, &mi) )
				return false;
			GetWindowRect(wnd,&r);
			save = (wsave_pos*)malloc(sizeof(wsave_pos));
			save->x = r.left;
			save->y = r.top;
			save->w = r.right - r.left;
			save->h = r.bottom - r.top;
			save->style = GetWindowLong(wnd,GWL_STYLE);
			SDL_SetWindowData(win,"save",save);
			SetWindowLong(wnd,GWL_STYLE, WS_POPUP | WS_VISIBLE);
			SetWindowPos(wnd,NULL,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right - mi.rcMonitor.left,mi.rcMonitor.bottom - mi.rcMonitor.top + 2 /* prevent opengl driver to use exclusive mode !*/,0);
			return true;
		}
#	else
		break;
#	endif
	case 3:
		return SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN) == 0;
	}
	return false;
}

HL_PRIM void HL_NAME(win_set_title)(SDL_Window *win, vbyte *title) {
	SDL_SetWindowTitle(win, title);
}

HL_PRIM void HL_NAME(win_set_size)(SDL_Window *win, int width, int height) {
	SDL_SetWindowSize(win, width, height);
}

HL_PRIM void HL_NAME(win_get_size)(SDL_Window *win, int *width, int *height) {
	SDL_GetWindowSize(win, width, height);
}

HL_PRIM void HL_NAME(win_resize)(SDL_Window *win, int mode) {
	switch( mode ) {
	case 0:
		SDL_MaximizeWindow(win);
		break;
	case 1:
		SDL_MinimizeWindow(win);
		break;
	case 2:
		SDL_RestoreWindow(win);
		break;
	default:
		break;
	}
}


HL_PRIM void HL_NAME(win_swap_window)(SDL_Window *win) {
	SDL_GL_SwapWindow(win);
}

HL_PRIM void HL_NAME(win_render_to)(SDL_Window *win, SDL_GLContext gl) {
	SDL_GL_MakeCurrent(win, gl);
}

HL_PRIM void HL_NAME(win_destroy)(SDL_Window *win, SDL_GLContext gl) {
	SDL_DestroyWindow(win);
	SDL_GL_DeleteContext(gl);
}

#define TWIN _ABSTRACT(sdl_window)
#define TGL _ABSTRACT(sdl_gl)
DEFINE_PRIM(TWIN, win_create, _I32 _I32);
DEFINE_PRIM(TGL, win_get_glcontext, TWIN);
DEFINE_PRIM(_BOOL, win_set_fullscreen, TWIN _I32);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
DEFINE_PRIM(_VOID, win_set_title, TWIN _BYTES);
DEFINE_PRIM(_VOID, win_set_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_get_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_swap_window, TWIN);
DEFINE_PRIM(_VOID, win_render_to, TWIN TGL);
DEFINE_PRIM(_VOID, win_destroy, TWIN TGL);

// game controller

HL_PRIM int HL_NAME(gctrl_count)() {
	return SDL_NumJoysticks();
}

HL_PRIM SDL_GameController *HL_NAME(gctrl_open)(int idx) {
	if (SDL_IsGameController(idx))
		return SDL_GameControllerOpen(idx);
	return NULL;
}

HL_PRIM void HL_NAME(gctrl_close)(SDL_GameController *controller) {
	SDL_GameControllerClose(controller);
}

HL_PRIM int HL_NAME(gctrl_get_axis)(SDL_GameController *controller, int axisIdx ){
	return SDL_GameControllerGetAxis(controller, axisIdx);
}

HL_PRIM bool HL_NAME(gctrl_get_button)(SDL_GameController *controller, int btnIdx) {
	return SDL_GameControllerGetButton(controller, btnIdx) == 1;
}

HL_PRIM int HL_NAME(gctrl_get_id)(SDL_GameController *controller) {
	return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

HL_PRIM vbyte *HL_NAME(gctrl_get_name)(SDL_GameController *controller) {
	return (vbyte*)SDL_GameControllerName(controller);
}

#define TGCTRL _ABSTRACT(sdl_gamecontroller)
DEFINE_PRIM(_I32, gctrl_count, _NO_ARG);
DEFINE_PRIM(TGCTRL, gctrl_open, _I32);
DEFINE_PRIM(_VOID, gctrl_close, TGCTRL);
DEFINE_PRIM(_I32, gctrl_get_axis, TGCTRL _I32);
DEFINE_PRIM(_BOOL, gctrl_get_button, TGCTRL _I32);
DEFINE_PRIM(_I32, gctrl_get_id, TGCTRL);
DEFINE_PRIM(_BYTES, gctrl_get_name, TGCTRL);

HL_PRIM SDL_Haptic *HL_NAME(haptic_open)(SDL_GameController *controller) {
	return SDL_HapticOpenFromJoystick(SDL_GameControllerGetJoystick(controller));
}

HL_PRIM void HL_NAME(haptic_close)(SDL_Haptic *haptic) {
	SDL_HapticClose(haptic);
}

HL_PRIM int HL_NAME(haptic_rumble_init)(SDL_Haptic *haptic) {
	return SDL_HapticRumbleInit(haptic);
}

HL_PRIM int HL_NAME(haptic_rumble_play)(SDL_Haptic *haptic, double strength, int length) {
	return SDL_HapticRumblePlay(haptic, (float)strength, length);
}
#define THAPTIC _ABSTRACT(sdl_haptic)
DEFINE_PRIM(THAPTIC, haptic_open, TGCTRL);
DEFINE_PRIM(_VOID, haptic_close, THAPTIC);
DEFINE_PRIM(_I32, haptic_rumble_init, THAPTIC);
DEFINE_PRIM(_I32, haptic_rumble_play, THAPTIC _F64 _I32);


// surface


HL_PRIM SDL_Surface *HL_NAME(surface_from)( vbyte *ptr, int width, int height, int depth, int pitch, int rmask, int gmask, int bmask, int amask ) {
	return SDL_CreateRGBSurfaceFrom(ptr,width,height,depth,pitch,rmask,gmask,bmask,amask);
}

HL_PRIM void HL_NAME(free_surface)( SDL_Surface *s ) {
	SDL_FreeSurface(s);
}

#define _SURF	_ABSTRACT(sdl_surface)
DEFINE_PRIM(_SURF, surface_from, _BYTES _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, free_surface, _SURF);

// cursor

HL_PRIM void HL_NAME(show_cursor)( bool b ) {
	SDL_ShowCursor(b?SDL_ENABLE:SDL_DISABLE);
}

HL_PRIM SDL_Cursor *HL_NAME(cursor_create)( SDL_Surface *s, int hotX, int hotY ) {
	return SDL_CreateColorCursor(s,hotX,hotY);
}

HL_PRIM SDL_Cursor *HL_NAME(cursor_create_system)( int kind ) {
	return SDL_CreateSystemCursor(kind);
}

HL_PRIM void HL_NAME(free_cursor)( SDL_Cursor *c ) {
	SDL_FreeCursor(c);
}

HL_PRIM void HL_NAME(set_cursor)( SDL_Cursor *c ) {
	SDL_SetCursor(c);
}

#define MAX_DEVICES 16
HL_PRIM varray *HL_NAME(get_devices)() {
	varray *a = hl_alloc_array(&hlt_bytes, MAX_DEVICES);
#	ifdef _WIN32
	int i=0;
	DISPLAY_DEVICE inf;
	inf.cb = sizeof(inf);
	while( i < MAX_DEVICES && EnumDisplayDevices(NULL,i,&inf,0) ) {
		hl_aptr(a,vbyte*)[i] = hl_copy_bytes((vbyte*)inf.DeviceString,((int)wcslen(inf.DeviceString) + 1)*2);
		i++;
	}
#	endif
	return a;
}

#define _CURSOR _ABSTRACT(sdl_cursor)
DEFINE_PRIM(_VOID, show_cursor, _BOOL);
DEFINE_PRIM(_CURSOR, cursor_create, _SURF _I32 _I32);
DEFINE_PRIM(_CURSOR, cursor_create_system, _I32);
DEFINE_PRIM(_VOID, free_cursor, _CURSOR);
DEFINE_PRIM(_VOID, set_cursor, _CURSOR);
DEFINE_PRIM(_ARR, get_devices, _NO_ARG);