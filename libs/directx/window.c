#define HL_NAME(n) directx_##n
#include <hl.h>

typedef struct HWND__ dx_window;

static LRESULT CALLBACK WndProc( HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam ) {
	switch(umsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
}

HL_PRIM dx_window *HL_NAME(win_create)( int width, int height ) {
	static bool wnd_class_reg = false;
	if( !wnd_class_reg ) {
		WNDCLASSEX wc;
		wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc   = WndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.hIcon		 = LoadIcon(NULL, IDI_WINLOGO);
		wc.hIconSm       = wc.hIcon;
		wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
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
	dx_window *win = CreateWindowEx(WS_EX_APPWINDOW, USTR("HL_WIN"), USTR(""), style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(win, SW_SHOW);
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
	*width = r.right;
	*height = r.bottom;
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
	default:
		break;
	}
}


HL_PRIM bool HL_NAME(win_set_fullscreen)(dx_window *win, int mode) {
	switch( mode ) {
	case 0: // WINDOWED
	case 1: // FULLSCREEN
	case 2: // BORDERLESS
		;
	}
	return false;
}

HL_PRIM void HL_NAME(win_swap_window)(dx_window *win) {
}

HL_PRIM void HL_NAME(win_destroy)(dx_window *win) {
	DestroyWindow(win);
}

HL_PRIM void HL_NAME(win_set_vsync)(dx_window *win, bool vsync) {
}

#define TWIN _ABSTRACT(dx_window)
DEFINE_PRIM(TWIN, win_create, _I32 _I32);
DEFINE_PRIM(_BOOL, win_set_fullscreen, TWIN _I32);
DEFINE_PRIM(_VOID, win_resize, TWIN _I32);
DEFINE_PRIM(_VOID, win_set_title, TWIN _BYTES);
DEFINE_PRIM(_VOID, win_set_size, TWIN _I32 _I32);
DEFINE_PRIM(_VOID, win_get_size, TWIN _REF(_I32) _REF(_I32));
DEFINE_PRIM(_VOID, win_swap_window, TWIN);
DEFINE_PRIM(_VOID, win_destroy, TWIN);
DEFINE_PRIM(_VOID, win_set_vsync, TWIN _BOOL);

