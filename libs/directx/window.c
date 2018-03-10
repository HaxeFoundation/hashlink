#define HL_NAME(n) directx_##n
#include <hl.h>

#define MAX_EVENTS 1024

typedef enum {
	Quit		= 0,
	MouseMove	= 1,
	MouseLeave	= 2,
	MouseDown	= 3,
	MouseUp		= 4,
	MouseWheel	= 5,
	WindowState	= 6,
	KeyDown		= 7,
	KeyUp		= 8,
	TextInput	= 9
} EventType;

typedef enum {
	Show	= 0,
	Hide	= 1,
	Expose	= 2,
	Move	= 3,
	Resize	= 4,
	Minimize= 5,
	Maximize= 6,
	Restore	= 7,
	Enter	= 8,
	Leave	= 9,
	Focus	= 10,
	Blur	= 11,
	Close 	= 12
} WindowStateChange;

typedef struct {
	hl_type *t;
	EventType type;
	int mouseX;
	int mouseY;
	int button;
	int wheelDelta;
	WindowStateChange state;
	int keyCode;
	bool keyRepeat;
	int controller;
	int value;
} dx_event;

typedef struct {
	dx_event events[MAX_EVENTS];
	int event_count;
	int next_event;
} dx_events;

typedef struct HWND__ dx_window;

static dx_window *cur_clip_cursor_window = NULL;

static void updateClipCursor(HWND wnd) {
	if (cur_clip_cursor_window == wnd) {
		RECT rect;

		GetClientRect(wnd, &rect);
		ClientToScreen(wnd, (LPPOINT)& rect.left);
		ClientToScreen(wnd, (LPPOINT)& rect.right);

		ClipCursor(&rect);
	}
}

static dx_event *addEvent( HWND wnd, EventType type ) {
	dx_events *buf = (dx_events*)GetWindowLongPtr(wnd,GWLP_USERDATA);
	dx_event *e;
	if( buf->event_count == MAX_EVENTS )
		e = &buf->events[MAX_EVENTS-1];
	else
		e = &buf->events[buf->event_count++];
	e->type = type;
	return e;
}

#define addMouse(etype,but) { \
	e = addEvent(wnd,etype); \
	e->button = but; \
	e->mouseX = (short)LOWORD(lparam); \
	e->mouseY = (short)HIWORD(lparam); \
}

#define addState(wstate) { e = addEvent(wnd,WindowState); e->state = wstate; }

static bool shift_downs[] = { false, false };

static LRESULT CALLBACK WndProc( HWND wnd, UINT umsg, WPARAM wparam, LPARAM lparam ) {
	dx_event *e = NULL;
	switch(umsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		SetWindowLongPtr(wnd,GWLP_USERDATA,(LONG_PTR)((CREATESTRUCT*)lparam)->lpCreateParams);
		break;
	case WM_SIZE:
		{
			dx_events *buf = (dx_events*)GetWindowLongPtr(wnd,GWLP_USERDATA);
			if( buf->event_count > buf->next_event && buf->events[buf->event_count-1].type == WindowState && buf->events[buf->event_count-1].state == Resize )
				buf->event_count--;
		}
		addState(Resize);
		break;
	case WM_LBUTTONDOWN: addMouse(MouseDown,1); break;
	case WM_LBUTTONUP: addMouse(MouseUp,1); break;
	case WM_MBUTTONDOWN: addMouse(MouseDown,2); break;
	case WM_MBUTTONUP: addMouse(MouseUp,2); break;
	case WM_RBUTTONDOWN: addMouse(MouseDown,3); break;
	case WM_RBUTTONUP: addMouse(MouseUp,3); break;
	case WM_XBUTTONUP: addMouse(MouseDown,3 + HIWORD(wparam)); break;
	case WM_XBUTTONDOWN: addMouse(MouseUp,3 + HIWORD(wparam)); break;
	case WM_MOUSEMOVE: addMouse(MouseMove,0); break;
	case WM_MOUSEWHEEL: addMouse(MouseWheel,0); e->wheelDelta = ((int)(short)HIWORD(wparam)) / 120; break;
	case WM_SYSCOMMAND:
		if( wparam == SC_KEYMENU && (lparam>>16) <= 0 )
			return 0;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
		// right alt has triggered a control !
		if( wparam == VK_MENU && lparam & (1<<24) ) {
			dx_events *buf = (dx_events*)GetWindowLongPtr(wnd,GWLP_USERDATA);
			if( buf->event_count > buf->next_event && buf->events[buf->event_count-1].type == (umsg == WM_KEYUP ? KeyUp : KeyDown) && buf->events[buf->event_count-1].keyCode == (VK_CONTROL|256) ) {
				buf->event_count--;
				//printf("CANCEL\n");
			}
		}
		bool repeat = (umsg == WM_KEYDOWN || umsg == WM_SYSKEYDOWN) && (lparam & 0x40000000) != 0;
		// see 
		e = addEvent(wnd,(umsg == WM_KEYUP || umsg == WM_SYSKEYUP) ? KeyUp : KeyDown);
		e->keyCode = (int)wparam;
		e->keyRepeat = repeat;
		// L/R location
		if( e->keyCode == VK_SHIFT ) {
			bool right = MapVirtualKey((lparam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT;
			e->keyCode |= right ? 512 : 256;			
			e->keyRepeat = false;
			shift_downs[right?1:0] = e->type == KeyDown;
		}
		if( e->keyCode == VK_SHIFT || e->keyCode == VK_CONTROL || e->keyCode == VK_MENU )
			e->keyCode |= (lparam & (1<<24)) ? 512 : 256;
		if( e->keyCode == 13 && (lparam & 0x1000000) )
			e->keyCode = 108; // numpad enter
		//printf("%.8X - %d[%s]%s%s\n",lparam,e->keyCode&255,e->type == KeyUp ? "UP":"DOWN",e->keyRepeat?" REPEAT":"",(e->keyCode&256) ? " LEFT" : (e->keyCode & 512) ? " RIGHT" : "");
		if( (e->keyCode & 0xFF) == VK_MENU )
			return 0;
		break;
	case WM_TIMER:
		// bugfix for shifts being considered as a single key (one single WM_KEYUP is received when both are down)
		if( shift_downs[0] && GetKeyState(VK_LSHIFT) >= 0 ) {
			//printf("LSHIFT RELEASED\n");
			shift_downs[0] = false;
			e = addEvent(wnd,KeyUp);
			e->keyCode = VK_SHIFT | 256;
		}			
		if( shift_downs[1] && GetKeyState(VK_RSHIFT) >= 0 ) {
			//printf("RSHIFT RELEASED\n");
			shift_downs[1] = false;
			e = addEvent(wnd,KeyUp);
			e->keyCode = VK_SHIFT | 512;
		}			
		break;
	case WM_CHAR:
		e = addEvent(wnd,TextInput);
		e->keyCode = (int)wparam;
		e->keyRepeat = (lparam & 0xFFFF) != 0;
		break;
	case WM_SETFOCUS:
		updateClipCursor(wnd);
		addState(Focus);
		break;
	case WM_KILLFOCUS:
		shift_downs[0] = false;
		shift_downs[1] = false;
		addState(Blur);
		break;
	case WM_WINDOWPOSCHANGED:
		updateClipCursor(wnd);
		break;
	case WM_CLOSE:
		addEvent(wnd, Quit);
		return 0;
	}
	return DefWindowProc(wnd, umsg, wparam, lparam);
}

HL_PRIM dx_window *HL_NAME(win_create)( int width, int height ) {
	static bool wnd_class_reg = false;
	HINSTANCE hinst = GetModuleHandle(NULL);
	if( !wnd_class_reg ) {
		WNDCLASSEX wc;
		wchar_t fileName[1024];
		GetModuleFileName(hinst,fileName,1024);
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hinst;
		wc.hIcon		 = ExtractIcon(hinst, fileName, 0);
		wc.hIconSm       = wc.hIcon;
		wc.hCursor       = NULL;
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = USTR("HL_WIN");
		wc.cbSize        = sizeof(WNDCLASSEX);
		RegisterClassEx(&wc);
	}

	RECT r;
	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	r.left = r.top = 0;
	r.right = width;
	r.bottom = height;
	AdjustWindowRect(&r,style,false);
	dx_events *event_buffer = (dx_events*)malloc(sizeof(dx_events));
	memset(event_buffer,0, sizeof(dx_events));
	dx_window *win = CreateWindowEx(WS_EX_APPWINDOW, USTR("HL_WIN"), USTR(""), style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, hinst, event_buffer);
	SetTimer(win,0,10,NULL);
	ShowWindow(win, SW_SHOW);
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	SetForegroundWindow(win);
	SetFocus(win);
	return win;
}

HL_PRIM void HL_NAME(win_set_title)(dx_window *win, vbyte *title) {
	SetWindowText(win,(LPCWSTR)title);
}

HL_PRIM void HL_NAME(win_set_size)(dx_window *win, int width, int height) {
	RECT r;
	GetWindowRect(win,&r);
	r.left = r.top = 0;
	r.right = width;
	r.bottom = height;
	AdjustWindowRectEx(&r,GetWindowLong(win,GWL_STYLE),GetMenu(win) != NULL,GetWindowLong(win,GWL_EXSTYLE));
	SetWindowPos(win,NULL,0,0,r.right - r.left,r.bottom - r.top,SWP_NOMOVE|SWP_NOOWNERZORDER);
}

HL_PRIM void HL_NAME(win_get_size)(dx_window *win, int *width, int *height) {
	RECT r;
	GetClientRect(win,&r);
	if( width ) *width = r.right;
	if( height ) *height = r.bottom;
}

HL_PRIM void HL_NAME(win_get_position)(dx_window *win, int *x, int *y) {
	RECT r;
	GetWindowRect(win,&r);
	if( x ) *x = r.left;
	if( y ) *y = r.top;
}

HL_PRIM void HL_NAME(win_set_position)(dx_window *win, int x, int y) {
	SetWindowPos(win,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

HL_PRIM void HL_NAME(win_resize)(dx_window *win, int mode) {
	switch( mode ) {
	case 0:
		ShowWindow(win, SW_MAXIMIZE);
		break;
	case 1:
		ShowWindow(win, SW_MINIMIZE);
		break;
	case 2:
		ShowWindow(win, SW_RESTORE);
		break;
	case 3:
		ShowWindow(win, SW_HIDE);
		break;
	case 4:
		ShowWindow(win, SW_SHOW);
		break;
	default:
		break;
	}
}

HL_PRIM void HL_NAME(win_set_fullscreen)(dx_window *win, bool fs) {
	if( fs ) {
		MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(win,MONITOR_DEFAULTTOPRIMARY), &mi);
		SetWindowLong(win,GWL_STYLE,WS_POPUP | WS_VISIBLE);
		SetWindowPos(win,NULL,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right - mi.rcMonitor.left,mi.rcMonitor.bottom - mi.rcMonitor.top,SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	} else {
		SetWindowLong(win,GWL_STYLE,WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
		SetWindowPos(win,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_FRAMECHANGED|SWP_SHOWWINDOW);
	}
}

HL_PRIM void HL_NAME(win_destroy)(dx_window *win) {
	if (cur_clip_cursor_window == win) {
		cur_clip_cursor_window = NULL;
		ClipCursor(NULL);
	}
	dx_events *buf = (dx_events*)GetWindowLongPtr(win,GWLP_USERDATA);
	free(buf);
	SetWindowLongPtr(win,GWLP_USERDATA,0);
	DestroyWindow(win);
}

HL_PRIM bool HL_NAME(win_get_next_event)( dx_window *win, dx_event *e ) {
	dx_events *buf = (dx_events*)GetWindowLongPtr(win,GWLP_USERDATA);
	hl_type *save;
	if( !buf ) {
		e->type = Quit;
		return true;
	}
	if( buf->next_event == buf->event_count ) {
		buf->next_event = buf->event_count = 0;
		return false;
	}
	save = e->t;
	memcpy(e,&buf->events[buf->next_event++],sizeof(dx_event));
	e->t = save;
	return true;
}

HL_PRIM void HL_NAME(win_clip_cursor)(dx_window *win) {
	cur_clip_cursor_window = win;
	if (win)
		updateClipCursor(win);
	else
		ClipCursor(NULL);
}


HL_PRIM int HL_NAME(get_screen_width)() {
	return GetSystemMetrics(SM_CXSCREEN);
}

HL_PRIM int HL_NAME(get_screen_height)() {
	return GetSystemMetrics(SM_CYSCREEN);
}

#define TWIN _ABSTRACT(dx_window)
DEFINE_PRIM(TWIN, win_create, _I32 _I32);
DEFINE_PRIM(_VOID, win_set_fullscreen, TWIN _BOOL);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
DEFINE_PRIM(_VOID, win_set_title, TWIN _BYTES);
DEFINE_PRIM(_VOID, win_set_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_set_position, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_get_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_get_position, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_destroy, TWIN);
DEFINE_PRIM(_BOOL, win_get_next_event, TWIN _DYN);
DEFINE_PRIM(_VOID, win_clip_cursor, TWIN);

DEFINE_PRIM(_I32, get_screen_width, _NO_ARG);
DEFINE_PRIM(_I32, get_screen_height, _NO_ARG);

typedef HCURSOR dx_cursor;

HL_PRIM dx_cursor HL_NAME(load_cursor)( int res ) {
	return LoadCursor(NULL,MAKEINTRESOURCE(res));
}

HL_PRIM dx_cursor HL_NAME(create_cursor)( int width, int height, vbyte *data, int hotX, int hotY ) {
	int pad = sizeof(void*) << 3;
    HICON hicon;
    HDC hdc = GetDC(NULL);
    BITMAPV4HEADER bmh;
    void *pixels;
    void *maskbits;
    int maskbitslen;
    ICONINFO ii;

    ZeroMemory(&bmh,sizeof(bmh));
    bmh.bV4Size = sizeof(bmh);
    bmh.bV4Width = width;
    bmh.bV4Height = -height;
    bmh.bV4Planes = 1;
    bmh.bV4BitCount = 32;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4AlphaMask = 0xFF000000;
    bmh.bV4RedMask   = 0x00FF0000;
    bmh.bV4GreenMask = 0x0000FF00;
    bmh.bV4BlueMask  = 0x000000FF;

    maskbitslen = ((width + (-width)%pad) >> 3) * height;
    maskbits = malloc(maskbitslen);
	if( maskbits == NULL )
		return NULL;
	memset(maskbits,0xFF,maskbitslen);

	memset(&ii,0,sizeof(ii));
    ii.fIcon = FALSE;
    ii.xHotspot = (DWORD)hotX;
    ii.yHotspot = (DWORD)hotY;
    ii.hbmColor = CreateDIBSection(hdc, (BITMAPINFO*)&bmh, DIB_RGB_COLORS, &pixels, NULL, 0);
    ii.hbmMask = CreateBitmap(width, height, 1, 1, maskbits);
    ReleaseDC(NULL, hdc);
    free(maskbits);

    memcpy(pixels, data, height * width * 4);
    hicon = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);
	return hicon;
}

HL_PRIM void HL_NAME(destroy_cursor)( dx_cursor c ) {
	DestroyIcon(c);
}

HL_PRIM void HL_NAME(set_cursor)( dx_cursor c ) {
	SetCursor(c);
}

HL_PRIM void HL_NAME(show_cursor)( bool visible ) {
	ShowCursor(visible);
}

#define TCURSOR _ABSTRACT(dx_cursor)
DEFINE_PRIM(TCURSOR, load_cursor, _I32);
DEFINE_PRIM(TCURSOR, create_cursor, _I32 _I32 _BYTES _I32 _I32);
DEFINE_PRIM(_VOID, destroy_cursor, TCURSOR);
DEFINE_PRIM(_VOID, set_cursor, TCURSOR);
DEFINE_PRIM(_VOID, show_cursor, _BOOL);

HL_PRIM vbyte *HL_NAME(detect_keyboard_layout)() {
	char q = MapVirtualKey(0x10, MAPVK_VSC_TO_VK);
	char w = MapVirtualKey(0x11, MAPVK_VSC_TO_VK);
	char y = MapVirtualKey(0x15, MAPVK_VSC_TO_VK);

	if (q == 'Q' && w == 'W' && y == 'Y') return "qwerty";
	if (q == 'A' && w == 'Z' && y == 'Y') return "azerty";
	if (q == 'Q' && w == 'W' && y == 'Z') return "qwertz";
	if (q == 'Q' && w == 'Z' && y == 'Y') return "qzerty";
	return "unknown";
}
DEFINE_PRIM(_BYTES, detect_keyboard_layout, _NO_ARG);
