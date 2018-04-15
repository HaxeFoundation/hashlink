#define HL_NAME(n) ui_##n
#include <windows.h>
#include <richedit.h>
#include <hl.h>

#define CLASS_NAME		USTR("HLUIWindow")
#define PTEXT			USTR("_text")
#define PREF			USTR("_ref")

typedef struct _wref wref;

struct _wref {
	void (*finalize)( wref * );
	HWND h;
	vclosure *callb;
	int width;
	int height;
};

static void finalize_wref( wref *w ) {
	SetProp(w->h,PREF,NULL);
}

static wref *alloc_ref( HWND h ) {
	wref *ref = hl_gc_alloc_finalizer(sizeof(wref));
	memset(ref,0,sizeof(wref));
	ref->h = h;
	ref->finalize = finalize_wref;
	SetProp(h,PREF,ref);
	return ref;
}

static LRESULT CALLBACK WindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch( msg ) {
	case WM_CLOSE:
		return 0;
	case WM_COMMAND:
		if( wparam == BN_CLICKED ) {
			wref *r = (wref*)GetProp((HWND)lparam,PREF);
			if( r && r->callb ) hl_dyn_call(r->callb,NULL,0);
		}
		break;
	}
	return DefWindowProc(hwnd,msg,wparam,lparam);
}

HL_PRIM void HL_NAME(ui_init)() {
	WNDCLASSEX wcl;
	HINSTANCE hinst = GetModuleHandle(NULL);
	memset(&wcl,0,sizeof(wcl));
	wcl.cbSize			= sizeof(WNDCLASSEX);
	wcl.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcl.lpfnWndProc		= WindowProc;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hInstance		= hinst;
	wcl.hIcon			= NULL;
	wcl.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wcl.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcl.lpszMenuName	= USTR("");
	wcl.lpszClassName	= CLASS_NAME;
	wcl.hIconSm			= 0;
	RegisterClassEx(&wcl);
	LoadLibrary(USTR("RICHED32.DLL"));
}

HL_PRIM int HL_NAME(ui_dialog)( const uchar *title, const uchar *message, int flags ) {
	int ret;
	hl_blocking(true);
	ret = MessageBoxW(NULL,message,title,((flags & 1)?MB_YESNO:MB_OK) | ((flags & 2)?MB_ICONERROR:MB_ICONINFORMATION) ) == IDYES;
	hl_blocking(false);
	return ret;
}

static HFONT font = NULL;

HL_PRIM wref *HL_NAME(ui_winlog_new)( const uchar *title, int width, int height ) {
	HWND wnd, text;
	RECT rc;
	RECT dtop;
	DWORD style = WS_SYSMENU | WS_OVERLAPPED | WS_CAPTION;
	DWORD exstyle = 0;
	wref *ref;
	// SIZE
	rc.left = 0;
	rc.right = width;
	rc.top = 0;
	rc.bottom = height;
	AdjustWindowRectEx(&rc, style, FALSE, exstyle);
	GetWindowRect(GetDesktopWindow(),&dtop);
	// WINDOW
	wnd = CreateWindowEx(
		exstyle,
		CLASS_NAME,
		title,
		style,
		(dtop.right - rc.right) / 2,
		(dtop.bottom - rc.bottom) / 2,
		rc.right - rc.left,
		rc.bottom - rc.top,
		GetActiveWindow(),
		NULL,
		GetModuleHandle(NULL),
		NULL
	);
	// FONT
	if( font == NULL ) {
		LOGFONT f;
		f.lfHeight = -8;
		f.lfWidth = 0;
		f.lfEscapement = 0;
		f.lfOrientation = 0;
		f.lfWeight = FW_NORMAL;
		f.lfItalic = FALSE;
		f.lfUnderline = FALSE;
		f.lfStrikeOut = FALSE;
		f.lfCharSet = DEFAULT_CHARSET;
		f.lfOutPrecision = OUT_DEFAULT_PRECIS;
		f.lfClipPrecision = 0;
		f.lfQuality = DEFAULT_QUALITY;
		f.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wcscpy(f.lfFaceName,USTR("MS Sans Serif"));
		font = CreateFontIndirect(&f);
	}
	// TEXT
	text = CreateWindowEx(WS_EX_CLIENTEDGE,USTR("RICHEDIT20A"),USTR(""),ES_MULTILINE | ES_DISABLENOSCROLL | ES_READONLY | WS_VSCROLL | WS_VISIBLE | WS_CHILD,5,5,width - 10,height - 50,wnd,NULL,NULL,NULL);
	SendMessage(text,WM_SETFONT,(WPARAM)font,TRUE);
	SetProp(wnd,PTEXT,text);
	SetTimer(wnd,0,1000,NULL); // prevent lock in ui_loop
	ShowWindow(wnd,SW_SHOW);
	ref = alloc_ref(wnd);
	ref->width = width;
	ref->height = height;
	return ref;
}

HL_PRIM wref *HL_NAME(ui_button_new)( wref *w, const uchar *txt, vclosure *callb ) {
	HWND but = CreateWindowEx(0,USTR("BUTTON"),USTR(""),WS_VISIBLE | WS_CHILD,w->width - 80,w->height - 30,75,25,w->h,NULL,NULL,NULL);
	wref *ref = alloc_ref(but);
	w->width -= 80;
	ref->callb = callb;
	SendMessage(but,WM_SETFONT,(WPARAM)font,TRUE);
	SetWindowText(but,txt);
	return ref;
}

HL_PRIM void HL_NAME(ui_winlog_set_text)( wref *w, const uchar *txt, bool autoScroll ) {
	HWND text = (HWND)GetProp(w->h,PTEXT);
	DWORD a,b;
	SCROLLINFO sinf;
	POINT pt;
	sinf.cbSize = sizeof(sinf);
	sinf.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	GetScrollInfo(text,SB_VERT,&sinf);
	SendMessage(text,EM_GETSCROLLPOS,0,(LPARAM)&pt);
	SendMessage(text,EM_GETSEL,(WPARAM)&a,(LPARAM)&b);
	SetWindowText(text,txt);
	SendMessage(text,EM_SETSEL,a,b);
	if( autoScroll ) {
		if( sinf.nPos + sinf.nPage == sinf.nMax || sinf.nMax == 1 ) {
			GetScrollInfo(text,SB_VERT,&sinf);
			pt.y = sinf.nMax - sinf.nPage;
		}
		SendMessage(text,EM_SETSCROLLPOS,0,(LPARAM)&pt);
	}
}

HL_PRIM void HL_NAME(ui_win_set_text)( wref *w, const uchar *txt ) {
	SetWindowText(w->h,txt);
}

HL_PRIM void HL_NAME(ui_win_set_enable)( wref *w, bool enable ) {
	EnableWindow(w->h,enable);
}

HL_PRIM void HL_NAME(ui_win_destroy)( wref *w ) {
	DestroyWindow(w->h);
}

HL_PRIM int HL_NAME(ui_loop)( bool blocking ) {
	MSG msg;
	if( blocking )
		GetMessage(&msg,NULL,0,0);
	else if( !PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
		return 0;
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	if( msg.message == WM_QUIT )
		return 2;
	return 1;
}

HL_PRIM void HL_NAME(ui_stop_loop)() {
	PostQuitMessage(0);
}

typedef struct {
	hl_thread *thread;
	DWORD original;
	void *callback;
	double timeout;
	int ticks;
	bool pause;
} vsentinel;

static void sentinel_loop( vsentinel *s ) {
	int time_ms = (int)((s->timeout * 1000.) / 16.);
	HANDLE h = OpenThread(THREAD_ALL_ACCESS,FALSE,s->original);
	CONTEXT regs;
	regs.ContextFlags = CONTEXT_FULL;
	while( true ) {
		int k = 0;
		int tick = s->ticks;
		while( true ) {
			Sleep(time_ms);
			if( tick != s->ticks || s->pause ) break;
			if( hl_is_blocking() ) continue;
			k++;
			if( k == 16 ) {
				if( hl_detect_debugger() ) {
					k = 0;
					continue;
				}
				// pause
				SuspendThread(h);
				GetThreadContext(h,&regs);
				// simulate a call
#				ifdef HL_64
				*--(int_val*)regs.Rsp = regs.Rip;
				*--(int_val*)regs.Rsp = regs.Rsp;
				regs.Rip = (int_val)s->callback;
#				else
				*--(int_val*)regs.Esp = regs.Eip;
				*--(int_val*)regs.Esp = regs.Esp;
				regs.Eip = (int_val)s->callback;
#				endif
				// resume
				SetThreadContext(h,&regs);
				ResumeThread(h);
				break;
			}
		}
	}
}

HL_PRIM vsentinel *HL_NAME(ui_start_sentinel)( double timeout, vclosure *c ) {
	vsentinel *s = (vsentinel*)malloc(sizeof(vsentinel));
	if( c->hasValue ) hl_error("Cannot set sentinel on closure callback");
#	ifdef HL_DEBUG
	timeout *= 2;
#	endif
	s->timeout = timeout;
	s->ticks = 0;
	s->pause = false;
	s->original = GetCurrentThreadId();
	s->callback = c->fun;
#	ifdef HL_THREADS
	s->thread = hl_thread_start(sentinel_loop,s,false);
#	endif
	return s;
}

HL_PRIM void HL_NAME(ui_sentinel_tick)( vsentinel *s ) {
	s->ticks++;
}

HL_PRIM void HL_NAME(ui_sentinel_pause)( vsentinel *s, bool pause ) {
	s->pause = pause;
}

HL_PRIM bool HL_NAME(ui_sentinel_is_paused)( vsentinel *s ) {
	return s->pause;
}

HL_PRIM void HL_NAME(ui_close_console)() {
	FreeConsole();
}


HL_PRIM vbyte *HL_NAME(ui_choose_file)( bool forSave, vdynamic *options ) {
	wref *win = (wref*)hl_dyn_getp(options,hl_hash_utf8("window"), &hlt_abstract);
	varray *filters = (varray*)hl_dyn_getp(options,hl_hash_utf8("filters"),&hlt_array);
	wchar_t *fileName = (wchar_t*)hl_dyn_getp(options,hl_hash_utf8("fileName"),&hlt_bytes);
	OPENFILENAME op;
	wchar_t filterStr[1024];
	wchar_t outputFile[1024] = {0};
	ZeroMemory(&op, sizeof(op));
	op.lStructSize = sizeof(op);
	op.hwndOwner = win ? win->h : NULL;
	if( filters && filters->size > 0 ) {
		int i, pos = 0;
		for(i=0;i<filters->size;i++) {
			wchar_t *str = hl_aptr(filters,wchar_t*)[i];
			int len = (int)wcslen(str);
			if( pos + len > 1024 ) return false;
			memcpy(filterStr + pos, str, (len + 1) << 1);
			pos += len + 1;
		}
		filterStr[pos] = 0;
		op.lpstrFilter = filterStr;
		op.nFilterIndex = hl_dyn_geti(options,hl_hash_utf8("filterIndex"),&hlt_i32) + 1; // 1 based
	}
	if( fileName )
		memcpy(outputFile, fileName, (wcslen(fileName)+1) * 2 );
	op.lpstrFile = outputFile;
	op.nMaxFile = 1024;
	op.lpstrInitialDir = hl_dyn_getp(options,hl_hash_utf8("directory"),&hlt_bytes);
	op.lpstrTitle = hl_dyn_getp(options,hl_hash_utf8("title"),&hlt_bytes);
	op.Flags |= OFN_NOCHANGEDIR;
	if( forSave ) {
		op.Flags |= OFN_OVERWRITEPROMPT;
		if( !GetSaveFileName(&op) )
			return NULL;
	} else {
		op.Flags |= OFN_CREATEPROMPT;
		if( !GetOpenFileName(&op) )
			return NULL;
	}
	return hl_copy_bytes((vbyte*)outputFile, (int)(wcslen(outputFile)+1)*2);
}


#define _WIN _ABSTRACT(ui_window)
#define _SENTINEL _ABSTRACT(ui_sentinel)

DEFINE_PRIM(_VOID, ui_init, _NO_ARG);
DEFINE_PRIM(_I32, ui_dialog, _BYTES _BYTES _I32);
DEFINE_PRIM(_WIN, ui_winlog_new, _BYTES _I32 _I32);
DEFINE_PRIM(_WIN, ui_button_new, _WIN _BYTES _FUN(_VOID,_NO_ARG));
DEFINE_PRIM(_VOID, ui_winlog_set_text, _WIN _BYTES _BOOL);
DEFINE_PRIM(_VOID, ui_win_set_text, _WIN _BYTES);
DEFINE_PRIM(_VOID, ui_win_set_enable, _WIN _BOOL);
DEFINE_PRIM(_VOID, ui_win_destroy, _WIN);
DEFINE_PRIM(_I32, ui_loop, _BOOL);
DEFINE_PRIM(_VOID, ui_stop_loop, _NO_ARG);
DEFINE_PRIM(_VOID, ui_close_console, _NO_ARG);

DEFINE_PRIM(_SENTINEL, ui_start_sentinel, _F64 _FUN(_VOID,_NO_ARG));
DEFINE_PRIM(_VOID, ui_sentinel_tick, _SENTINEL);
DEFINE_PRIM(_VOID, ui_sentinel_pause, _SENTINEL _BOOL);
DEFINE_PRIM(_BOOL, ui_sentinel_is_paused, _SENTINEL);

DEFINE_PRIM(_BYTES, ui_choose_file, _BOOL _DYN);
