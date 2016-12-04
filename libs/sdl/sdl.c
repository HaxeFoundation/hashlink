#define HL_NAME(n) sdl_##n

#include <hl.h>

#ifdef _WIN32
#	include <SDL.h>
#else
#	include <SDL2/SDL.h>
#endif

#ifndef _SDL_H
#	error "SDL2 SDK not found in hl/include/sdl/"
#endif

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
	GControllerAdded,
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
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		return false;
#	ifdef _WIN32
	// Set the internal windows timer period to 1ms (will give accurate sleep for vsync)
	timeBeginPeriod(1);
#	endif
	return true;
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
		case SDL_TEXTINPUT:
			// skip
			continue;
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

DEFINE_PRIM(_BOOL, init_once, _NO_ARG);
DEFINE_PRIM(_BOOL, event_loop, _OBJ(_I32 _I32 _I32 _I32 _I32 _I32 _I32 _BOOL _I32 _I32) );
DEFINE_PRIM(_VOID, quit, _NO_ARG);
DEFINE_PRIM(_VOID, delay, _I32);
DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);
DEFINE_PRIM(_VOID, message_box, _BYTES _BYTES _BOOL);
DEFINE_PRIM(_VOID, set_vsync, _BOOL);
DEFINE_PRIM(_BOOL, detect_win32, _NO_ARG);

// Window

HL_PRIM SDL_Window *HL_NAME(win_create)(vbyte *title, int width, int height) {
	SDL_Window *w;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	w = SDL_CreateWindow((char*)title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	// force window to show even if the debugger force process windows to be hidden
	SDL_HideWindow(w);
	SDL_ShowWindow(w);
	return w;
}

HL_PRIM SDL_GLContext HL_NAME(win_get_glcontext)(SDL_Window *win) {
	return SDL_GL_CreateContext(win);
}

HL_PRIM bool HL_NAME(win_set_fullscreen)(SDL_Window *win, bool b) {
	return SDL_SetWindowFullscreen(win, b ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0;
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
DEFINE_PRIM(TWIN, win_create, _BYTES _I32 _I32);
DEFINE_PRIM(TGL, win_get_glcontext, TWIN);
DEFINE_PRIM(_BOOL, win_set_fullscreen, TWIN _BOOL);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
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

#define _CURSOR _ABSTRACT(sdl_cursor)
DEFINE_PRIM(_VOID, show_cursor, _BOOL);
DEFINE_PRIM(_CURSOR, cursor_create, _SURF _I32 _I32);
DEFINE_PRIM(_CURSOR, cursor_create_system, _I32);
DEFINE_PRIM(_VOID, free_cursor, _CURSOR);
DEFINE_PRIM(_VOID, set_cursor, _CURSOR);
