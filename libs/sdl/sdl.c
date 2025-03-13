#define HL_NAME(n) sdl_##n

#include <hl.h>
#include <locale.h>

#if defined(_WIN32) || defined(__ANDROID__) || defined(HL_IOS) || defined(HL_TVOS)
#	include <SDL.h>
#	include <SDL_vulkan.h>
#	include <SDL_syswm.h>
#else
#	include <SDL2/SDL.h>
#endif

#if defined (HL_IOS) || defined(HL_TVOS)
#	include <OpenGLES/ES3/gl.h>
#	include <OpenGLES/ES3/glext.h>
#endif

#ifndef SDL_MAJOR_VERSION
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

#define TWIN _ABSTRACT(sdl_window)
#define TGL _ABSTRACT(sdl_gl)

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
	GControllerAxis,
	TouchDown = 200,
	TouchUp,
	TouchMove,
	JoystickAxisMotion = 300,
	JoystickBallMotion,
	JoystickHatMotion,
	JoystickButtonDown,
	JoystickButtonUp,
	JoystickAdded,
	JoystickRemoved,
	DropStart = 400,
	DropFile,
	DropText,
	DropEnd,
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
	int mouseXRel;
	int mouseYRel;
	int button;
	int wheelDelta;
	ws_change state;
	int keyCode;
	int scanCode;
	bool keyRepeat;
	int reference;
	int value;
	int __unused;
	int window;
	vbyte* dropFile;
} event_data;

static bool isGlOptionsSet = false;

HL_PRIM bool HL_NAME(init_once)() {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if( SDL_Init(SDL_INIT_EVERYTHING) != 0 ) {
		hl_error("SDL_Init failed: %s", hl_to_utf16(SDL_GetError()));
		return false;
	}
	setlocale(LC_ALL, "C");
#	ifdef _WIN32
	// Set the internal windows timer period to 1ms (will give accurate sleep for vsync)
	timeBeginPeriod(1);
#	endif
	// default GL parameters
	if (!isGlOptionsSet) {
#ifdef HL_MOBILE
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}

	return true;
}

HL_PRIM void HL_NAME(gl_options)( int major, int minor, int depth, int stencil, int flags, int samples ) {
	isGlOptionsSet = true;
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
	else {
#ifdef HL_MOBILE
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	}

	if (samples > 1) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	}
}

HL_PRIM bool HL_NAME(hint_value)( vbyte* name, vbyte* value) {
	return SDL_SetHint((char*)name, (char*)value) == SDL_TRUE;
}

HL_PRIM int HL_NAME(event_poll)( SDL_Event *e ) {
	return SDL_PollEvent(e);
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
			event->window = e.motion.windowID;
			event->mouseX = e.motion.x;
			event->mouseY = e.motion.y;
			event->mouseXRel = e.motion.xrel;
			event->mouseYRel = e.motion.yrel;
			break;
		case SDL_KEYDOWN:
			event->type = KeyDown;
			event->window = e.key.windowID;
			event->keyCode = e.key.keysym.sym;
			event->scanCode = e.key.keysym.scancode;
			event->keyRepeat = e.key.repeat != 0;
			break;
		case SDL_KEYUP:
			event->type = KeyUp;
			event->window = e.key.windowID;
			event->keyCode = e.key.keysym.sym;
			event->scanCode = e.key.keysym.scancode;
			break;
		case SDL_SYSWMEVENT:
			continue;
		case SDL_MOUSEBUTTONDOWN:
			event->type = MouseDown;
			event->window = e.button.windowID;
			event->button = e.button.button;
			event->mouseX = e.button.x;
			event->mouseY = e.motion.y;
			break;
		case SDL_MOUSEBUTTONUP:
			event->type = MouseUp;
			event->window = e.button.windowID;
			event->button = e.button.button;
			event->mouseX = e.button.x;
			event->mouseY = e.motion.y;
			break;
		case SDL_FINGERDOWN:
			event->type = TouchDown;
			event->mouseX = (int)(e.tfinger.x*10000);
			event->mouseY = (int)(e.tfinger.y*10000);
			event->reference = (int)e.tfinger.fingerId;
			break;
		case SDL_FINGERMOTION:
			event->type = TouchMove;
			event->mouseX = (int)(e.tfinger.x*10000);
			event->mouseY = (int)(e.tfinger.y*10000);
			event->reference = (int)e.tfinger.fingerId;
			break;
		case SDL_FINGERUP:
			event->type = TouchUp;
			event->mouseX = (int)(e.tfinger.x*10000);
			event->mouseY = (int)(e.tfinger.y*10000);
			event->reference = (int)e.tfinger.fingerId;
			break;
		case SDL_MOUSEWHEEL:
			event->type = MouseWheel;
			event->window = e.wheel.windowID;
			event->wheelDelta = e.wheel.y;
#						if SDL_VERSION_ATLEAST(2,0,4)
			if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) event->wheelDelta *= -1;
#						endif
			event->mouseX = e.wheel.x;
			event->mouseY = e.wheel.y;
			break;
		case SDL_WINDOWEVENT:
			event->type = WindowState;
			event->window = e.window.windowID;
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
			case SDL_WINDOWEVENT_SIZE_CHANGED:
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
			event->window = e.text.windowID;
			event->keyCode = *(int*)e.text.text;
			event->keyCode &= e.text.text[0] ? e.text.text[1] ? e.text.text[2] ? e.text.text[3] ? 0xFFFFFFFF : 0xFFFFFF : 0xFFFF : 0xFF : 0;
			break;
		case SDL_CONTROLLERDEVICEADDED:
			event->type = GControllerAdded;
			event->reference = e.jdevice.which;
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			event->type = GControllerRemoved;
			event->reference = e.jdevice.which;
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			event->type = GControllerDown;
			event->reference = e.cbutton.which;
			event->button = e.cbutton.button;
			break;
		case SDL_CONTROLLERBUTTONUP:
			event->type = GControllerUp;
			event->reference = e.cbutton.which;
			event->button = e.cbutton.button;
			break;
		case SDL_CONTROLLERAXISMOTION:
			event->type = GControllerAxis;
			event->reference = e.caxis.which;
			event->button = e.caxis.axis;
			event->value = e.caxis.value;
			break;
		case SDL_JOYAXISMOTION:
			event->type = JoystickAxisMotion;
			event->reference = e.jaxis.which;
			event->button = e.jaxis.axis;
			event->value = e.jaxis.value;
			break;
		case SDL_JOYBALLMOTION:
			event->type = JoystickBallMotion;
			event->reference = e.jball.which;
			event->button = e.jball.ball;
			event->mouseXRel = e.jball.xrel;
			event->mouseYRel = e.jball.yrel;
			break;
		case SDL_JOYHATMOTION:
			event->type = JoystickHatMotion;
			event->reference = e.jhat.which;
			event->button = e.jhat.hat;
			event->value = e.jhat.value;
			break;
		case SDL_JOYBUTTONDOWN:
			event->type = JoystickButtonDown;
			event->reference = e.jbutton.which;
			event->button = e.jbutton.button;
			break;
		case SDL_JOYBUTTONUP:
			event->type = JoystickButtonUp;
			event->reference = e.jbutton.which;
			event->button = e.jbutton.button;
			break;
		case SDL_JOYDEVICEADDED:
			event->type = JoystickAdded;
			event->reference = e.jdevice.which;
			break;
		case SDL_JOYDEVICEREMOVED:
			event->type = JoystickRemoved;
			event->reference = e.jdevice.which;
			break;
		case SDL_DROPBEGIN:
			event->type = DropStart;
			event->window = e.drop.windowID;
			break;
		case SDL_DROPFILE: case SDL_DROPTEXT: {
			vbyte* bytes = hl_copy_bytes(e.drop.file, (int)strlen(e.drop.file) + 1);
			SDL_free(e.drop.file);
			event->type = e.type == SDL_DROPFILE ? DropFile : DropText;
			event->dropFile = bytes;
			event->window = e.drop.windowID;
			break;
		}
		case SDL_DROPCOMPLETE:
			event->type = DropEnd;
			event->window = e.drop.windowID;
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
	hl_blocking(true);
	SDL_Delay(time);
	hl_blocking(false);
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

HL_PRIM int HL_NAME(get_screen_width_of_window)(SDL_Window* win) {
	SDL_DisplayMode e;
	SDL_GetCurrentDisplayMode(win != NULL ? SDL_GetWindowDisplayIndex(win) : 0, &e);
	return e.w;
}

HL_PRIM int HL_NAME(get_screen_height_of_window)(SDL_Window* win) {
	SDL_DisplayMode e;
	SDL_GetCurrentDisplayMode(win != NULL ? SDL_GetWindowDisplayIndex(win) : 0, &e);
	return e.h;
}

HL_PRIM int HL_NAME(get_framerate)(SDL_Window* win) {
	SDL_DisplayMode e;
	SDL_GetCurrentDisplayMode(win != NULL ? SDL_GetWindowDisplayIndex(win) : 0, &e);
	return e.refresh_rate;
}

HL_PRIM void HL_NAME(message_box)(vbyte *title, vbyte *text, bool error) {
	hl_blocking(true);
	SDL_ShowSimpleMessageBox(error ? SDL_MESSAGEBOX_ERROR : 0, (char*)title, (char*)text, NULL);
	hl_blocking(false);
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

HL_PRIM bool HL_NAME(get_relative_mouse_mode)() {
	return SDL_GetRelativeMouseMode();
}

HL_PRIM int HL_NAME(warp_mouse_global)(int x, int y) {
	return SDL_WarpMouseGlobal(x, y);
}

HL_PRIM void HL_NAME(warp_mouse_in_window)(SDL_Window* window, int x, int y) {
	SDL_WarpMouseInWindow(window, x, y);
}

HL_PRIM void HL_NAME(set_window_grab)(SDL_Window* window, bool grabbed) {
	SDL_SetWindowGrab(window, grabbed);
}

HL_PRIM bool HL_NAME(get_window_grab)(SDL_Window* window) {
	return SDL_GetWindowGrab(window);
}

HL_PRIM int HL_NAME(get_global_mouse_state)(int* x, int* y) {
	return SDL_GetGlobalMouseState(x, y);
}

HL_PRIM const char *HL_NAME(detect_keyboard_layout)() {
	char q = SDL_GetKeyFromScancode(SDL_SCANCODE_Q);
	char w = SDL_GetKeyFromScancode(SDL_SCANCODE_W);
	char y = SDL_GetKeyFromScancode(SDL_SCANCODE_Y);

	if (q == 'q' && w == 'w' && y == 'y') return "qwerty";
	if (q == 'a' && w == 'z' && y == 'y') return "azerty";
	if (q == 'q' && w == 'w' && y == 'z') return "qwertz";
	if (q == 'q' && w == 'z' && y == 'y') return "qzerty";
	return "unknown";
}

#define TWIN _ABSTRACT(sdl_window)
DEFINE_PRIM(_BOOL, init_once, _NO_ARG);
DEFINE_PRIM(_VOID, gl_options, _I32 _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, event_loop, _DYN );
DEFINE_PRIM(_I32, event_poll, _STRUCT );
DEFINE_PRIM(_VOID, quit, _NO_ARG);
DEFINE_PRIM(_VOID, delay, _I32);
DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_width_of_window, TWIN);
DEFINE_PRIM(_I32, get_screen_height_of_window, TWIN);
DEFINE_PRIM(_I32, get_framerate, TWIN);
DEFINE_PRIM(_VOID, message_box, _BYTES _BYTES _BOOL);
DEFINE_PRIM(_VOID, set_vsync, _BOOL);
DEFINE_PRIM(_BOOL, detect_win32, _NO_ARG);
DEFINE_PRIM(_VOID, text_input, _BOOL);
DEFINE_PRIM(_I32, set_relative_mouse_mode, _BOOL);
DEFINE_PRIM(_BOOL, get_relative_mouse_mode, _NO_ARG);
DEFINE_PRIM(_I32, warp_mouse_global, _I32 _I32);
DEFINE_PRIM(_VOID, warp_mouse_in_window, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, set_window_grab, TWIN _BOOL);
DEFINE_PRIM(_BOOL, get_window_grab, TWIN);
DEFINE_PRIM(_I32, get_global_mouse_state, _REF(_I32) _REF(_I32));
DEFINE_PRIM(_BYTES, detect_keyboard_layout, _NO_ARG);
DEFINE_PRIM(_BOOL, hint_value, _BYTES _BYTES);

// Window

HL_PRIM SDL_Window *HL_NAME(win_create_ex)(int x, int y, int width, int height, int sdlFlags) {
	// force window to match device resolution on mobile
	if ((sdlFlags & (
#ifdef HL_MAC
		SDL_WINDOW_METAL | 
#endif
		SDL_WINDOW_VULKAN )) == 0) {
		sdlFlags |= SDL_WINDOW_OPENGL;
	}

#ifdef	HL_MOBILE
	SDL_DisplayMode displayMode;
	SDL_GetDesktopDisplayMode(0, &displayMode);
	SDL_Window* win = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_BORDERLESS | sdlFlags);
#else
	SDL_Window* win = SDL_CreateWindow("", x, y, width, height, sdlFlags);
#endif
#	ifdef HL_WIN
	// force window to show even if the debugger force process windows to be hidden
	if( (SDL_GetWindowFlags(win) & SDL_WINDOW_INPUT_FOCUS) == 0 ) {
		SDL_HideWindow(win);
		SDL_ShowWindow(win);
	}
	SDL_RaiseWindow(win); // better first focus lost behavior
#	endif
	return win;
}

HL_PRIM SDL_Window *HL_NAME(win_create)(int width, int height) {
	return HL_NAME(win_create_ex)(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
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
		SDL_SetWindowSize(win, save->w, save->h);
		free(save);
		SDL_SetWindowData(win,"save",NULL);
		save = NULL;
	}
#	endif
	switch( mode ) {
	case 0: // WINDOWED
		return SDL_SetWindowFullscreen(win, 0) == 0;
	case 1: // FULLSCREEN
		return SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN) == 0;
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
		return SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
#	endif
	}
	return false;
}

HL_PRIM bool HL_NAME(win_set_display_mode)(SDL_Window *win, int width, int height, int framerate) {
	SDL_DisplayMode mode;
	int display_idx = SDL_GetWindowDisplayIndex(win);
	for (int i = 0; i < SDL_GetNumDisplayModes(display_idx); i++) {
		if (SDL_GetDisplayMode(display_idx, i, &mode) == 0) {
			if (mode.w == width && mode.h == height && mode.refresh_rate == framerate) {
				return SDL_SetWindowDisplayMode(win, &mode) >= 0;
			}
		}
	}
	return false;
 }

HL_PRIM int HL_NAME(win_display_handle)(SDL_Window *win) {
	return SDL_GetWindowDisplayIndex(win);
}

HL_PRIM void HL_NAME(win_set_title)(SDL_Window *win, vbyte *title) {
	SDL_SetWindowTitle(win, (char*)title);
}

HL_PRIM void HL_NAME(win_set_position)(SDL_Window *win, int x, int y) {
	SDL_SetWindowPosition(win, x, y);
}

HL_PRIM void HL_NAME(win_get_position)(SDL_Window *win, int *x, int *y) {
	SDL_GetWindowPosition(win, x, y);
}

HL_PRIM void HL_NAME(win_set_size)(SDL_Window *win, int width, int height) {
	SDL_SetWindowSize(win, width, height);
}

HL_PRIM void HL_NAME(win_set_min_size)(SDL_Window *win, int width, int height) {
	SDL_SetWindowMinimumSize(win, width, height);
}

HL_PRIM void HL_NAME(win_set_max_size)(SDL_Window *win, int width, int height) {
	SDL_SetWindowMaximumSize(win, width, height);
}

HL_PRIM void HL_NAME(win_get_size)(SDL_Window *win, int *width, int *height) {
	SDL_GetWindowSize(win, width, height);
}

HL_PRIM void HL_NAME(win_get_min_size)(SDL_Window *win, int *width, int *height) {
	SDL_GetWindowMinimumSize(win, width, height);
}

HL_PRIM void HL_NAME(win_get_max_size)(SDL_Window *win, int *width, int *height) {
	SDL_GetWindowMaximumSize(win, width, height);
}

HL_PRIM double HL_NAME(win_get_opacity)(SDL_Window *win) {
	float opacity = 1.0f;
	SDL_GetWindowOpacity(win, &opacity);
	return opacity;
}

HL_PRIM bool HL_NAME(win_set_opacity)(SDL_Window *win, double opacity) {
	return SDL_SetWindowOpacity(win, (float)opacity) == 0;
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
	case 3:
		SDL_ShowWindow(win);
		break;
	case 4:
		SDL_HideWindow(win);
		break;
	default:
		break;
	}
}


HL_PRIM void HL_NAME(win_swap_window)(SDL_Window *win) {
#if defined(HL_IOS) || defined(HL_TVOS)
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(win, &info);

	glBindFramebuffer(GL_FRAMEBUFFER, info.info.uikit.framebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER,info.info.uikit.colorbuffer);
#endif
	SDL_GL_SwapWindow(win);
}

HL_PRIM void HL_NAME(win_render_to)(SDL_Window *win, SDL_GLContext gl) {
	SDL_GL_MakeCurrent(win, gl);
}

HL_PRIM int HL_NAME(win_get_id)(SDL_Window *window) {
	return SDL_GetWindowID(window);
}

HL_PRIM void HL_NAME(win_destroy)(SDL_Window *win, SDL_GLContext gl) {
	SDL_DestroyWindow(win);
	SDL_GL_DeleteContext(gl);
}

#define TGL _ABSTRACT(sdl_gl)
DEFINE_PRIM(TWIN, win_create_ex, _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(TWIN, win_create, _I32 _I32);
DEFINE_PRIM(TGL, win_get_glcontext, TWIN);
DEFINE_PRIM(_BOOL, win_set_fullscreen, TWIN _I32);
DEFINE_PRIM(_BOOL, win_set_display_mode, TWIN _I32 _I32 _I32);
DEFINE_PRIM(_I32, win_display_handle, TWIN);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
DEFINE_PRIM(_VOID, win_set_title, TWIN _BYTES);
DEFINE_PRIM(_VOID, win_set_position, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_get_position, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_set_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_min_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_max_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_get_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_min_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_max_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_F64, win_get_opacity, TWIN);
DEFINE_PRIM(_BOOL, win_set_opacity, TWIN _F64);
DEFINE_PRIM(_VOID, win_swap_window, TWIN);
DEFINE_PRIM(_VOID, win_render_to, TWIN TGL);
DEFINE_PRIM(_VOID, win_destroy, TWIN TGL);
DEFINE_PRIM(_I32, win_get_id, TWIN);

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

// joystick

HL_PRIM int HL_NAME(joy_count)() {
	return SDL_NumJoysticks();
}

HL_PRIM SDL_Joystick *HL_NAME(joy_open)(int idx) {
	return SDL_JoystickOpen(idx);
}

HL_PRIM void HL_NAME(joy_close)(SDL_Joystick *joystick) {
	SDL_JoystickClose(joystick);
}

HL_PRIM int HL_NAME(joy_get_axis)(SDL_Joystick *joystick, int axisIdx ){
	return SDL_JoystickGetAxis(joystick, axisIdx);
}

HL_PRIM int HL_NAME(joy_get_hat)(SDL_Joystick *joystick, int hatIdx ){
	return SDL_JoystickGetHat(joystick, hatIdx);
}

HL_PRIM bool HL_NAME(joy_get_button)(SDL_Joystick *joystick, int btnIdx) {
	return SDL_JoystickGetButton(joystick, btnIdx) == 1;
}

HL_PRIM int HL_NAME(joy_get_id)(SDL_Joystick *joystick) {
	return SDL_JoystickInstanceID(joystick);
}

HL_PRIM vbyte *HL_NAME(joy_get_name)(SDL_Joystick *joystick) {
	return (vbyte*)SDL_JoystickName(joystick);
}

#define TJOY _ABSTRACT(sdl_joystick)
DEFINE_PRIM(_I32, joy_count, _NO_ARG);
DEFINE_PRIM(TJOY, joy_open, _I32);
DEFINE_PRIM(_VOID, joy_close, TJOY);
DEFINE_PRIM(_I32, joy_get_axis, TJOY _I32);
DEFINE_PRIM(_I32, joy_get_hat, TJOY _I32);
DEFINE_PRIM(_BOOL, joy_get_button, TJOY _I32);
DEFINE_PRIM(_I32, joy_get_id, TJOY);
DEFINE_PRIM(_BYTES, joy_get_name, TJOY);

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

HL_PRIM bool HL_NAME(is_cursor_visible)() {
	return SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE;
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

HL_PRIM bool HL_NAME(set_clipboard_text)(char* text) {
	return SDL_SetClipboardText(text) == 0;
}

HL_PRIM char* HL_NAME(get_clipboard_text)() {
	char* chr = SDL_GetClipboardText();
	if (chr == NULL)
		return NULL;
	vbyte* bytes = hl_copy_bytes(chr, (int) strlen(chr) + 1);
	SDL_free(chr);
	return bytes;
}

HL_PRIM void HL_NAME(set_drag_and_drop_enabled)( bool enabled ) {
	SDL_EventState(SDL_DROPFILE, enabled ? SDL_ENABLE : SDL_DISABLE);
}

HL_PRIM bool HL_NAME(get_drag_and_drop_enabled)() {
	return SDL_EventState(SDL_DROPFILE, SDL_QUERY);
}

HL_PRIM varray* HL_NAME(get_displays)() {
	int n = SDL_GetNumVideoDisplays();
	if (n < 0)
		return NULL;
	varray* arr = hl_alloc_array(&hlt_dynobj, n);
	for (int i = 0; i < n; i++) {
		vdynamic *obj = (vdynamic*) hl_alloc_dynobj();
		SDL_Rect rect;
		SDL_GetDisplayBounds(i, &rect);
		hl_dyn_seti(obj, hl_hash_utf8("right"), &hlt_i32, rect.x+rect.w);
		hl_dyn_seti(obj, hl_hash_utf8("bottom"), &hlt_i32, rect.y+rect.h);
		hl_dyn_seti(obj, hl_hash_utf8("left"), &hlt_i32, rect.x);
		hl_dyn_seti(obj, hl_hash_utf8("top"), &hlt_i32, rect.y);
		hl_dyn_seti(obj, hl_hash_utf8("handle"), &hlt_i32, i);
		const char *name = SDL_GetDisplayName(i);
		hl_dyn_setp(obj, hl_hash_utf8("name"), &hlt_bytes, hl_copy_bytes(name, (int) strlen(name)+1));
		hl_aptr(arr, vdynamic*)[i] = obj;
	}
	return arr;
}

HL_PRIM varray* HL_NAME(get_display_modes)(int display_id) {
	int n = SDL_GetNumDisplayModes(display_id);
	if (n < 0)
		return NULL;
	varray* arr = hl_alloc_array(&hlt_dynobj, n);
	for (int i = 0; i < n; i++) {
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(display_id, i, &mode);
		vdynamic *obj = (vdynamic*)hl_alloc_dynobj();
		hl_dyn_seti(obj, hl_hash_utf8("width"), &hlt_i32, mode.w);
		hl_dyn_seti(obj, hl_hash_utf8("height"), &hlt_i32, mode.h);
		hl_dyn_seti(obj, hl_hash_utf8("framerate"), &hlt_i32, mode.refresh_rate);
		hl_aptr(arr, vdynamic*)[i] = obj;
	}
	return arr;
}

HL_PRIM vdynobj* HL_NAME(get_current_display_mode)(int display_id, bool registry) {
	SDL_DisplayMode mode;
	int r;
	if(registry)
		r = SDL_GetDesktopDisplayMode(display_id, &mode);
	else
		r = SDL_GetCurrentDisplayMode(display_id, &mode);
	if (r < 0) {
		printf("can't find mode for %d : %d\n", display_id, r);
		return NULL;
	}
	vdynamic* obj = (vdynamic*)hl_alloc_dynobj();
	hl_dyn_seti(obj, hl_hash_utf8("width"), &hlt_i32, mode.w);
	hl_dyn_seti(obj, hl_hash_utf8("height"), &hlt_i32, mode.h);
	hl_dyn_seti(obj, hl_hash_utf8("framerate"), &hlt_i32, mode.refresh_rate);
	return (vdynobj*) obj;
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
DEFINE_PRIM(_BOOL, is_cursor_visible, _NO_ARG);
DEFINE_PRIM(_CURSOR, cursor_create, _SURF _I32 _I32);
DEFINE_PRIM(_CURSOR, cursor_create_system, _I32);
DEFINE_PRIM(_VOID, free_cursor, _CURSOR);
DEFINE_PRIM(_VOID, set_cursor, _CURSOR);
DEFINE_PRIM(_BOOL, set_clipboard_text, _BYTES);
DEFINE_PRIM(_BYTES, get_clipboard_text, _NO_ARG);
DEFINE_PRIM(_VOID, set_drag_and_drop_enabled, _BOOL);
DEFINE_PRIM(_BOOL, get_drag_and_drop_enabled, _NO_ARG);
DEFINE_PRIM(_ARR, get_displays, _NO_ARG);
DEFINE_PRIM(_ARR, get_display_modes, _I32);
DEFINE_PRIM(_DYN, get_current_display_mode, _I32 _BOOL);
DEFINE_PRIM(_ARR, get_devices, _NO_ARG);
