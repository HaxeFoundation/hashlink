MESA_CFLAGS=-fPIC -O3
CFLAGS = -fPIC -D HL_MESA -I ../sdl -I $(MESA)/include -I ../../src
LFLAGS = -lm -lstdc++ -L $(LLVM_LIBDIR) -lLLVM -ldl -lpthread -lz -L../.. -lhl

MESA_CONFIG= --enable-autotools \
	--enable-opengl --disable-gles1 --disable-gles2 --disable-egl \
	--disable-dri --disable-gbm --disable-glx \
	--disable-osmesa --enable-gallium-osmesa \
	--enable-llvm --with-gallium-drivers=swrast \
	--enable-static --disable-shared

MESA_LIBS= \
	$(MESA)/src/gallium/targets/osmesa/.libs/libOSMesa.a \
	$(MESA)/src/mesa/.libs/libmesagallium.a \
	$(MESA)/src/mapi/shared-glapi/.libs/libglapi.a

LLVM_LIBDIR=`llvm-config --libdir`
	
all: libmesa mesa.hdll

libmesa:
	(cd $(MESA) && ./configure CFLAGS="$(MESA_CFLAGS)" CXXFLAGS="$(MESA_CFLAGS)" $(MESA_CONFIG) && make)

mesa.hdll:
	gcc -shared $(CFLAGS) -o ../../mesa.hdll ../sdl/gl.c mesa.c $(MESA_LIBS) $(LFLAGS)

.PHONY: libmesa mesa.hdll
