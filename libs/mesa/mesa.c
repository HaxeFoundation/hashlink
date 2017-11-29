#define HL_NAME(n) mesa__##n
#include <hl.h>
#include <GL/osmesa.h>

HL_PRIM void *HL_NAME(create_context)( int *attribs, OSMesaContext shared ) {
	OSMesaContext ctx = OSMesaCreateContextAttribs(attribs, shared);
	return ctx;
}

HL_PRIM void HL_NAME(destroy_context)( OSMesaContext ctx ) {
	OSMesaDestroyContext(ctx);
}

HL_PRIM bool HL_NAME(make_current)( OSMesaContext ctx, void *buffer, int type, int width, int height ) {
	return OSMesaMakeCurrent(ctx,buffer,type,width,height);
}

#define _CTX _ABSTRACT(mesa_ctx)
DEFINE_PRIM(_CTX, create_context, _BYTES _CTX);
DEFINE_PRIM(_VOID, destroy_context, _CTX);
DEFINE_PRIM(_BOOL, make_current, _CTX _BYTES _I32 _I32 _I32);
