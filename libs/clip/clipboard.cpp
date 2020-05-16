#define HL_NAME(n) clip_##n
#include <hl.h>

#include <clip.h>

HL_PRIM const char * HL_NAME(get_clipboard_text)() {
    std::string text;
    clip::get_text(text);
    return text.c_str();
}

HL_PRIM bool HL_NAME(set_clipboard_text)(const char * text) {
    return clip::set_text(text);    
}

DEFINE_PRIM(_BYTES, get_clipboard_text, _NO_ARG);
DEFINE_PRIM(_BOOL, set_clipboard_text, _BYTES);