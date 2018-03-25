
LBITS := $(shell getconf LONG_BIT)

ifndef ARCH
	ARCH = $(LBITS)
endif

LIBS=fmt sdl ssl openal ui uv

CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse -std=c11 -I include/pcre -D LIBHL_EXPORTS
LFLAGS = -L. -lhl
LIBFLAGS =
HLFLAGS = -ldl
LIBEXT = so
LIBTURBOJPEG = -lturbojpeg

PCRE = include/pcre/pcre_chartables.o include/pcre/pcre_compile.o include/pcre/pcre_dfa_exec.o \
	include/pcre/pcre_exec.o include/pcre/pcre_fullinfo.o include/pcre/pcre_globals.o \
	include/pcre/pcre_newline.o include/pcre/pcre_string_utils.o include/pcre/pcre_tables.o include/pcre/pcre_xclass.o

RUNTIME = src/alloc.o src/track.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o \
	src/std/file.o src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/random.o src/std/regexp.o \
	src/std/socket.o src/std/string.o src/std/sys.o src/std/types.o src/std/ucs2.o src/std/thread.o src/std/process.o

HL = src/code.o src/jit.o src/main.o src/module.o src/debugger.o

FMT = libs/fmt/fmt.o libs/fmt/sha1.o

SDL = libs/sdl/sdl.o libs/sdl/gl.o

OPENAL = libs/openal/openal.o

SSL = libs/ssl/ssl.o

UV = libs/uv/uv.o

UI = libs/ui/ui_stub.o

LIB = ${PCRE} ${RUNTIME} ${STD}

BOOT = src/_main.o

UNAME := $(shell uname)

# Cygwin
ifeq ($(OS),Windows_NT)

LIBFLAGS += -Wl,--export-all-symbols
LIBEXT = dll
RELEASE_NAME=win

ifeq ($(ARCH),32)
CC=i686-pc-cygwin-gcc
endif

else ifeq ($(UNAME),Darwin)

# Mac
LIBEXT=dylib
CFLAGS += -m$(ARCH) -I /opt/libjpeg-turbo/include -I /usr/local/opt/jpeg-turbo/include -I /usr/local/include -I /usr/local/opt/libvorbis/include -I /usr/local/opt/openal-soft/include -Dopenal_soft
LFLAGS += -Wl,-export_dynamic -L/usr/local/lib
LIBFLAGS += -L/opt/libjpeg-turbo/lib -L/usr/local/opt/jpeg-turbo/lib -L/usr/local/lib -L/usr/local/opt/libvorbis/lib -L/usr/local/opt/openal-soft/lib
LIBOPENGL = -framework OpenGL
LIBOPENAL = -lopenal
LIBSSL = -framework Security -framework CoreFoundation
RELEASE_NAME = osx

else

# Linux
CFLAGS += -m$(ARCH) -fPIC -pthread
LFLAGS += -lm -Wl,--export-dynamic -Wl,--no-undefined

ifeq ($(ARCH),32)
CFLAGS += -I /usr/include/i386-linux-gnu
LIBFLAGS += -L/opt/libjpeg-turbo/lib
else
LIBFLAGS += -L/opt/libjpeg-turbo/lib64
endif

LIBOPENAL = -lopenal
RELEASE_NAME = linux

endif

ifndef INSTALL_DIR
INSTALL_DIR=/usr/local
endif

ifdef MESA
LIBS += mesa
endif

all: libhl hl libs

install:
	mkdir -p $(INSTALL_DIR)
	mkdir -p $(INSTALL_DIR)/bin
	mkdir -p $(INSTALL_DIR)/lib
	mkdir -p $(INSTALL_DIR)/include
	cp hl $(INSTALL_DIR)/bin
	cp libhl.${LIBEXT} $(INSTALL_DIR)/lib
	cp *.hdll $(INSTALL_DIR)/lib
	cp src/hl.h src/hlc.h src/hlc_main.c $(INSTALL_DIR)/include

uninstall:
	rm -f $(INSTALL_DIR)/bin/hl $(INSTALL_DIR)/lib/libhl.${LIBEXT} $(INSTALL_DIR)/lib/*.hdll
	rm -f $(INSTALL_DIR)/include/hl.h $(INSTALL_DIR)/include/hlc.h $(INSTALL_DIR)/include/hlc_main.c

libs: $(LIBS)

libhl: ${LIB}
	${CC} -o libhl.$(LIBEXT) -m${ARCH} ${LIBFLAGS} -shared ${LIB} -lpthread -lm

hlc: ${BOOT}
	${CC} ${CFLAGS} -o hlc ${BOOT} ${LFLAGS}

hl: ${HL} libhl
	${CC} ${CFLAGS} -o hl ${HL} ${LFLAGS} ${HLFLAGS}

fmt: ${FMT} libhl
	${CC} ${CFLAGS} -shared -o fmt.hdll ${FMT} ${LIBFLAGS} -L. -lhl -lpng $(LIBTURBOJPEG) -lz -lvorbisfile

sdl: ${SDL} libhl
	${CC} ${CFLAGS} -shared -o sdl.hdll ${SDL} ${LIBFLAGS} -L. -lhl -lSDL2 $(LIBOPENGL)

openal: ${OPENAL} libhl
	${CC} ${CFLAGS} -shared -o openal.hdll ${OPENAL} ${LIBFLAGS} -L. -lhl $(LIBOPENAL)

ssl: ${SSL} libhl
	${CC} ${CFLAGS} -shared -o ssl.hdll ${SSL} ${LIBFLAGS} -L. -lhl -lmbedtls -lmbedx509 -lmbedcrypto $(LIBSSL)

ui: ${UI} libhl
	${CC} ${CFLAGS} -shared -o ui.hdll ${UI} ${LIBFLAGS} -L. -lhl

uv: ${UV} libhl
	${CC} ${CFLAGS} -shared -o uv.hdll ${UV} ${LIBFLAGS} -L. -lhl -luv
	
mesa:
	(cd libs/mesa && make)

release: release_version release_$(RELEASE_NAME)

release_version:
	$(eval HL_VER := `(hl --version)`-$(RELEASE_NAME))	
	rm -rf hl-$(HL_VER)
	mkdir hl-$(HL_VER)
	mkdir hl-$(HL_VER)/include
	cp src/hl.h src/hlc* hl-$(HL_VER)/include

release_haxelib:
	make HLIB=directx release_haxelib_package
	make HLIB=sdl release_haxelib_package
	make HLIB=openal release_haxelib_package

ifeq ($(HLIB),directx)
HLPACK=dx
else
HLPACK=$(HLIB)
endif
	
release_haxelib_package:
	rm -rf $(HLIB)_release
	mkdir $(HLIB)_release
	(cd libs/$(HLIB) && cp -R $(HLPACK) *.h *.c* haxelib.json ../../$(HLIB)_release | true)
	zip -r $(HLIB).zip $(HLIB)_release
	haxelib submit $(HLIB).zip
	rm -rf $(HLIB)_release	
	
release_win:
	(cd ReleaseVS2013 && cp hl.exe libhl.dll *.hdll *.lib ../hl-$(HL_VER))
	cp c:/windows/syswow64/msvcr120.dll hl-$(HL_VER)
	cp `which SDL2.dll` hl-$(HL_VER)
	cp `which OpenAL32.dll` hl-$(HL_VER)
	zip -r hl-$(HL_VER).zip hl-$(HL_VER)
	rm -rf hl-$(HL_VER)

release_linux:
	cp hl libhl.so *.hdll hl-$(HL_VER)
	tar -czf hl-$(HL_VER).tgz hl-$(HL_VER)
	rm -rf hl-$(HL_VER)

release_osx:
	cp hl libhl.dylib *.hdll hl-$(HL_VER)
	tar -czf hl-$(HL_VER).tgz hl-$(HL_VER)
	rm -rf hl-$(HL_VER)

.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<

clean_o:
	rm -f ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL} ${FMT} ${SDL} ${SSL} ${OPENAL} ${UI} ${UV}

clean: clean_o
	rm -f hl hl.exe libhl.$(LIBEXT) *.hdll

.PHONY: libhl hl hlc fmt sdl libs release
