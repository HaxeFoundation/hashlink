#define HL_NAME(n) os_##n
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

HL_PRIM vbyte * HL_NAME(get_clipboard_image_data)() {
    clip::image img;
    if(clip::get_image(img)) {
        int size = img.spec().width * img.spec().height * (img.spec().bits_per_pixel / 8) * sizeof(vbyte);
        vbyte * data = (vbyte *)hl_gc_alloc_raw(size);
        memcpy(data, img.data(), size);
        return (vbyte *) data;
    }

    return NULL;
}   

HL_PRIM bool HL_NAME(set_clipboard_image)(vbyte * data, int w, int h, int bpp, int bpr, int rmask, int gmask, int bmask, int amask, int rshift, int gshift, int bshift, int ashift) {
    clip::image_spec imgS;
    imgS.width = w;
    imgS.height = h;
    imgS.bits_per_pixel = bpp;
    imgS.bytes_per_row = bpr;
    imgS.red_mask = rmask;
    imgS.green_mask = gmask;
    imgS.blue_mask = bmask;
    imgS.alpha_mask = amask;
    imgS.red_shift = rshift;
    imgS.green_shift = gshift;
    imgS.blue_shift = bshift;
    imgS.alpha_shift = ashift;

    clip::image img(data, imgS);
    return clip::set_image(img);
}

HL_PRIM bool HL_NAME(get_clipboard_image_spec)(int * w, int * h, int * bpp, int * bpr, int * rmask, int * gmask, int * bmask, int * amask, int * rshift, int * gshift, int * bshift, int * ashift) {
    clip::image_spec spec;
    if(clip::get_image_spec(spec)) {
        *w = (int) spec.width;
        *h = (int) spec.height;
        *bpp = (int) spec.bits_per_pixel;
        *bpr = (int) spec.bytes_per_row;
        *rmask = (int) spec.red_mask;
        *gmask = (int) spec.green_mask;
        *bmask = (int) spec.blue_mask;
        *amask = (int) spec.alpha_mask;
        *rshift = (int) spec.red_shift;
        *gshift = (int) spec.green_shift;
        *bshift = (int) spec.blue_shift;
        *ashift = (int) spec.alpha_shift;

        return true;
    }

    return false;
}

#define _IMAGE_SPEC _ABSTRACT(clip::image_spec)
DEFINE_PRIM(_BYTES, get_clipboard_text, _NO_ARG);
DEFINE_PRIM(_BOOL, set_clipboard_text, _BYTES);
DEFINE_PRIM(_BYTES, get_clipboard_image_data, _NO_ARG);
DEFINE_PRIM(_BOOL, set_clipboard_image, _BYTES _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, get_clipboard_image_spec, _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32));

