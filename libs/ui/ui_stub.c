#define HL_NAME(n) ui_##n
#include <hl.h>
#ifndef HL_WIN
#include <unistd.h>
#endif

#define wref void
#define vsentinel void

HL_PRIM void HL_NAME(ui_init)() {
}

HL_PRIM int HL_NAME(ui_dialog)( const uchar *title, const uchar *message, int flags ) {
	uprintf(USTR("[DIALOG] [%s]"), title);
	uprintf(USTR(" %s\n"), title);
	return 0;
}

HL_PRIM wref *HL_NAME(ui_winlog_new)( const uchar *title, int width, int height ) {
	return NULL;
}

HL_PRIM wref *HL_NAME(ui_button_new)( wref *w, const uchar *txt, vclosure *callb ) {
	return NULL;
}

HL_PRIM void HL_NAME(ui_winlog_set_text)( wref *w, const uchar *txt, bool autoScroll ) {
}

HL_PRIM void HL_NAME(ui_win_set_text)( wref *w, const uchar *txt ) {
}

HL_PRIM void HL_NAME(ui_win_set_enable)( wref *w, bool enable ) {
}

HL_PRIM void HL_NAME(ui_win_destroy)( wref *w ) {
}

HL_PRIM int HL_NAME(ui_loop)( bool blocking ) {
	return 1;
}

HL_PRIM void HL_NAME(ui_stop_loop)() {
	// TODO ?
}

HL_PRIM vsentinel *HL_NAME(ui_start_sentinel)( double timeout, vclosure *c ) {
	return NULL;
}

HL_PRIM void HL_NAME(ui_sentinel_tick)( vsentinel *s ) {
}

HL_PRIM void HL_NAME(ui_sentinel_pause)( vsentinel *s, bool pause ) {
}

HL_PRIM bool HL_NAME(ui_sentinel_is_paused)( vsentinel *s ) {
	return false;
}

HL_PRIM void HL_NAME(ui_close_console)() {
}

HL_PRIM vbyte *HL_NAME(ui_choose_file)( bool forSave, vdynamic *options ) {
	return NULL;
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
