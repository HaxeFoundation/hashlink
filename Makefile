
LBITS := $(shell getconf LONG_BIT)
MARCH ?= $(LBITS)
PREFIX ?= /usr/local
INSTALL_DIR ?= $(PREFIX)
INSTALL_BIN_DIR ?= $(PREFIX)/bin
INSTALL_LIB_DIR ?= $(PREFIX)/lib
INSTALL_INCLUDE_DIR ?= $(PREFIX)/include

LIBS=fmt sdl ssl openal ui uv mysql sqlite

CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse -std=c11 -D LIBHL_EXPORTS
LFLAGS = -L. -lhl
EXTRA_LFLAGS ?=
LIBFLAGS =
HLFLAGS = -ldl
LIBEXT = so
LIBTURBOJPEG = -lturbojpeg

PCRE_INCLUDE = -I include/pcre

PCRE = include/pcre/pcre_chartables.o include/pcre/pcre_compile.o include/pcre/pcre_dfa_exec.o \
	include/pcre/pcre_exec.o include/pcre/pcre_fullinfo.o include/pcre/pcre_globals.o \
	include/pcre/pcre_newline.o include/pcre/pcre_string_utils.o include/pcre/pcre_tables.o include/pcre/pcre_xclass.o \
	include/pcre/pcre16_ord2utf16.o include/pcre/pcre16_valid_utf16.o include/pcre/pcre_ucd.o

RUNTIME = src/gc.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o src/std/debug.o \
	src/std/file.o src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/random.o src/std/regexp.o \
	src/std/socket.o src/std/string.o src/std/sys.o src/std/types.o src/std/ucs2.o src/std/thread.o src/std/process.o \
	src/std/track.o

HL = src/code.o src/jit.o src/main.o src/module.o src/debugger.o src/profile.o

FMT_INCLUDE = -I include/mikktspace -I include/minimp3

FMT = libs/fmt/fmt.o libs/fmt/sha1.o include/mikktspace/mikktspace.o libs/fmt/mikkt.o libs/fmt/dxt.o

SDL = libs/sdl/sdl.o libs/sdl/gl.o

OPENAL = libs/openal/openal.o

SSL = libs/ssl/ssl.o

UV = libs/uv/uv.o

UI = libs/ui/ui_stub.o

MYSQL = libs/mysql/socket.o libs/mysql/sha1.o libs/mysql/my_proto.o libs/mysql/my_api.o libs/mysql/mysql.o

SQLITE = libs/sqlite/sqlite.o

LIB = ${PCRE} ${RUNTIME} ${STD}

BOOT = src/_main.o

UNAME := $(shell uname)

# Cygwin
ifeq ($(OS),Windows_NT)

LIBFLAGS += -Wl,--export-all-symbols
LIBEXT = dll
RELEASE_NAME=win

ifeq ($(MARCH),32)
CC=i686-pc-cygwin-gcc
endif

else ifeq ($(UNAME),Darwin)

# Mac
LIBEXT=dylib
CFLAGS += -m$(MARCH) -I include -I /usr/local/include -I /usr/local/opt/libjpeg-turbo/include \
	-I /usr/local/opt/jpeg-turbo/include -I /usr/local/opt/sdl2/include/SDL2 -I /usr/local/opt/libvorbis/include \
	-I /usr/local/opt/openal-soft/include -Dopenal_soft  -DGL_SILENCE_DEPRECATION
LFLAGS += -Wl,-export_dynamic -L/usr/local/lib

ifdef OSX_SDK
ISYSROOT = $(shell xcrun --sdk macosx$(OSX_SDK) --show-sdk-path)
CFLAGS += -isysroot $(ISYSROOT)
LFLAGS += -isysroot $(ISYSROOT)
endif

LIBFLAGS += -L/usr/local/opt/libjpeg-turbo/lib -L/usr/local/opt/jpeg-turbo/lib -L/usr/local/lib -L/usr/local/opt/libvorbis/lib -L/usr/local/opt/openal-soft/lib
LIBOPENGL = -framework OpenGL
LIBOPENAL = -lopenal
LIBSSL = -framework Security -framework CoreFoundation
RELEASE_NAME = osx

# Mac native debug
HL_DEBUG = include/mdbg/mdbg.o include/mdbg/mach_excServer.o include/mdbg/mach_excUser.o
LIB += ${HL_DEBUG}

else

# Linux
CFLAGS += -m$(MARCH) -fPIC -pthread -fno-omit-frame-pointer
LFLAGS += -lm -Wl,-rpath,.:'$$ORIGIN':$(INSTALL_LIB_DIR) -Wl,--export-dynamic -Wl,--no-undefined

ifeq ($(MARCH),32)
CFLAGS += -I /usr/include/i386-linux-gnu
LIBFLAGS += -L/opt/libjpeg-turbo/lib
else
LIBFLAGS += -L/opt/libjpeg-turbo/lib64
endif

LIBOPENAL = -lopenal
RELEASE_NAME = linux

endif


ifdef MESA
LIBS += mesa
endif

ifdef DEBUG
CFLAGS += -g
endif

all: libhl hl libs

install:
	$(UNAME)==Darwin && make uninstall
	mkdir -p $(INSTALL_BIN_DIR)
	cp hl $(INSTALL_BIN_DIR)
	mkdir -p $(INSTALL_LIB_DIR)
	cp *.hdll $(INSTALL_LIB_DIR)
	cp libhl.${LIBEXT} $(INSTALL_LIB_DIR)
	mkdir -p $(INSTALL_INCLUDE_DIR)
	cp src/hl.h src/hlc.h src/hlc_main.c $(INSTALL_INCLUDE_DIR)

uninstall:
	rm -f $(INSTALL_BIN_DIR)/hl $(INSTALL_LIB_DIR)/libhl.${LIBEXT} $(INSTALL_LIB_DIR)/*.hdll
	rm -f $(INSTALL_INCLUDE_DIR)/hl.h $(INSTALL_INCLUDE_DIR)/hlc.h $(INSTALL_INCLUDE_DIR)/hlc_main.c

libs: $(LIBS)

./include/pcre/%.o: include/pcre/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${PCRE_FLAGS}

src/std/regexp.o: src/std/regexp.c
	${CC} ${CFLAGS} -o $@ -c $< ${PCRE_FLAGS}

libhl: ${LIB}
	${CC} -o libhl.$(LIBEXT) -m${MARCH} ${LIBFLAGS} -shared ${LIB} -lpthread -lm

hlc: ${BOOT}
	${CC} ${CFLAGS} -o hlc ${BOOT} ${LFLAGS} ${EXTRA_LFLAGS}

hl: ${HL} libhl
	${CC} ${CFLAGS} -o hl ${HL} ${LFLAGS} ${EXTRA_LFLAGS} ${HLFLAGS}

libs/fmt/%.o: libs/fmt/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${FMT_INCLUDE}

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

mysql: ${MYSQL} libhl
	${CC} ${CFLAGS} -shared -o mysql.hdll ${MYSQL} ${LIBFLAGS} -L. -lhl

sqlite: ${SQLITE} libhl
	${CC} ${CFLAGS} -shared -o sqlite.hdll ${SQLITE} ${LIBFLAGS} -L. -lhl -lsqlite3

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
	(cd x64/ReleaseVS2015 && cp hl.exe libhl.dll *.hdll *.lib ../../hl-$(HL_VER))
	cp c:/windows/system32/vcruntime140.dll hl-$(HL_VER)
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

codesign_osx:
	sudo security delete-identity -c hl-cert || echo
	echo "[req]\ndistinguished_name=codesign_dn\n[codesign_dn]\ncommonName=hl-cert\n[v3_req]\nkeyUsage=critical,digitalSignature\nextendedKeyUsage=critical,codeSigning" > openssl.cnf
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -nodes -days 365 -subj '/CN=hl-cert' -outform der -out cert.cer -extensions v3_req -config openssl.cnf
	sudo security add-trusted-cert -d -k /Library/Keychains/System.keychain cert.cer
	sudo security import key.pem -k /Library/Keychains/System.keychain -A
	codesign --entitlements other/osx/entitlements.xml -fs hl-cert hl
	rm key.pem cert.cer openssl.cnf

.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<

clean_o:
	rm -f ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL} ${FMT} ${SDL} ${SSL} ${OPENAL} ${UI} ${UV} ${MYSQL} ${SQLITE} ${HL_DEBUG}

clean: clean_o
	rm -f hl hl.exe libhl.$(LIBEXT) *.hdll

.PHONY: libhl hl hlc fmt sdl libs release
