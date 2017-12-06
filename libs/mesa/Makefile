MESA_CFLAGS=-fPIC -O3

MESA_CONFIG= \
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
	gcc -shared -fPIC -D HL_MESA -I ../sdl -I $(MESA)/include -o mesa.hdll ../sdl/gl.c mesa.c $(MESA_LIBS) -lm -lstdc++ -L $(LLVM_LIBDIR) -lLLVM -ldl -lpthread -lz -lhl

.PHONY: libmesa mesa.hdll
