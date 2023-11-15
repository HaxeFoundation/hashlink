
LBITS := $(shell getconf LONG_BIT)
MARCH ?= $(LBITS)
PREFIX ?= /usr/local
INSTALL_DIR ?= $(PREFIX)
INSTALL_BIN_DIR ?= $(PREFIX)/bin
INSTALL_LIB_DIR ?= $(PREFIX)/lib
INSTALL_INCLUDE_DIR ?= $(PREFIX)/include

LIBS=fmt sdl ssl openal ui uv mysql sqlite
ARCH ?= $(shell uname -m)

CFLAGS = -Wall -O3 -I src -std=c11 -D LIBHL_EXPORTS
LFLAGS = -L. -lhl
EXTRA_LFLAGS ?=
LIBFLAGS =
HLFLAGS = -ldl
LIBEXT = so
LIBTURBOJPEG = -lturbojpeg

PCRE_FLAGS = -I include/pcre -D HAVE_CONFIG_H -D PCRE2_CODE_UNIT_WIDTH=16

PCRE = include/pcre/pcre2_auto_possess.o include/pcre/pcre2_chartables.o include/pcre/pcre2_compile.o \
	include/pcre/pcre2_config.o include/pcre/pcre2_context.o include/pcre/pcre2_convert.o \
	include/pcre/pcre2_dfa_match.o include/pcre/pcre2_error.o include/pcre/pcre2_extuni.o \
	include/pcre/pcre2_find_bracket.o include/pcre/pcre2_jit_compile.o include/pcre/pcre2_maketables.o \
	include/pcre/pcre2_match_data.o include/pcre/pcre2_match.o include/pcre/pcre2_newline.o \
	include/pcre/pcre2_ord2utf.o include/pcre/pcre2_pattern_info.o include/pcre/pcre2_script_run.o \
	include/pcre/pcre2_serialize.o include/pcre/pcre2_string_utils.o include/pcre/pcre2_study.o \
	include/pcre/pcre2_substitute.o include/pcre/pcre2_substring.o include/pcre/pcre2_tables.o \
	include/pcre/pcre2_ucd.o include/pcre/pcre2_valid_utf.o include/pcre/pcre2_xclass.o

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
# VS variables are for packaging Visual Studio builds
VS_RUNTIME_LIBRARY ?= c:/windows/system32/vcruntime140.dll

ifeq ($(MARCH),32)
CFLAGS += -msse2 -mfpmath=sse
CC=i686-pc-cygwin-gcc
BUILD_DIR = Release
VS_SDL_LIBRARY ?= include/sdl/lib/x86/SDL2.dll
VS_OPENAL_LIBRARY ?= include/openal/bin/Win32/soft_oal.dll
else
BUILD_DIR = x64/Release
VS_SDL_LIBRARY ?= include/sdl/lib/x64/SDL2.dll
VS_OPENAL_LIBRARY ?= include/openal/bin/Win64/soft_oal.dll
endif

else ifeq ($(UNAME),Darwin)

# Mac
LIBEXT=dylib

BPREFIX := $(shell brew --prefix)

BREW_LIBJPEG := $(shell brew --prefix libjpeg-turbo)
BREW_SDL2 := $(shell brew --prefix sdl2)
BREW_JPEGTURBO := $(shell brew --prefix jpeg-turbo)
BREW_VORBIS := $(shell brew --prefix libvorbis)
BREW_OPENAL := $(shell brew --prefix openal-soft)
BREW_MBEDTLS := $(shell brew --prefix mbedtls@2)
BREW_LIBPNG := $(shell brew --prefix libpng)
BREW_LIBOGG := $(shell brew --prefix libogg)
BREW_LIBUV := $(shell brew --prefix libuv)

CFLAGS += -m$(MARCH) -I include -I $(BREW_LIBJPEG)/include \
	-I $(BREW_JPEGTURBO)/include -I $(BREW_SDL2)/include -I $(BREW_VORBIS)/include \
	-I $(BREW_MBEDTLS)/include -I $(BREW_LIBPNG)/include -I $(BREW_LIBOGG)/include \
	-I $(BREW_LIBUV)/include \
	-I $(BREW_OPENAL)/include -Dopenal_soft  -DGL_SILENCE_DEPRECATION
LFLAGS += -Wl,-export_dynamic 

CFLAGS += -m$(MARCH) -I include -I /usr/local/include -I /usr/local/opt/libjpeg-turbo/include \
	-I /usr/local/opt/jpeg-turbo/include -I /usr/local/opt/sdl2/include -I /usr/local/opt/libvorbis/include \
	-I /usr/local/opt/openal-soft/include -Dopenal_soft  -DGL_SILENCE_DEPRECATION
LFLAGS += -Wl,-export_dynamic -L/usr/local/lib

ifdef OSX_SDK
ISYSROOT = $(shell xcrun --sdk macosx$(OSX_SDK) --show-sdk-path)
CFLAGS += -isysroot $(ISYSROOT)
LFLAGS += -isysroot $(ISYSROOT)
endif

LIBFLAGS += -L$(BREW_LIBJPEG)/lib -L$(BREW_SDL2)/lib -L$(BREW_JPEGTURBO)/lib \
			-L$(BREW_VORBIS)/lib -L$(BREW_OPENAL)/lib -L$(BREW_MBEDTLS)/lib \
			-L$(BREW_LIBPNG)/lib -L$(BREW_LIBOGG)/lib -L$(BREW_LIBUV)/lib
LIBOPENGL = -framework OpenGL
LIBOPENAL = -lopenal
LIBSSL = -framework Security -framework CoreFoundation
RELEASE_NAME = osx

# Mac native debug
ifneq ($(ARCH),arm64)
HL_DEBUG = include/mdbg/mdbg.o include/mdbg/mach_excServer.o include/mdbg/mach_excUser.o
LIB += ${HL_DEBUG}
endif

CFLAGS += -arch $(ARCH)
LFLAGS += -arch $(ARCH)

else

# Linux
CFLAGS += -m$(MARCH) -fPIC -pthread -fno-omit-frame-pointer
LFLAGS += -lm -Wl,-rpath,.:'$$ORIGIN':$(INSTALL_LIB_DIR) -Wl,--export-dynamic -Wl,--no-undefined

ifeq ($(MARCH),32)
CFLAGS += -I /usr/include/i386-linux-gnu -msse2 -mfpmath=sse
LIBFLAGS += -L/opt/libjpeg-turbo/lib
else
LIBFLAGS += -L/opt/libjpeg-turbo/lib64
endif

LIBOPENAL = -lopenal
LIBOPENGL = -lGL
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
	$(UNAME)==Darwin && ${MAKE} uninstall
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
	${CC} ${CFLAGS} -o libhl.$(LIBEXT) -m${MARCH} ${LIBFLAGS} -shared ${LIB} -lpthread -lm

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
	(cd libs/mesa && ${MAKE})

release: release_prepare release_$(RELEASE_NAME)

release_haxelib:
	${MAKE} HLIB=directx release_haxelib_package
	${MAKE} HLIB=sdl release_haxelib_package
	${MAKE} HLIB=openal release_haxelib_package

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

BUILD_DIR ?= .
PACKAGE_NAME := hashlink-$(shell $(BUILD_DIR)/hl --version)-$(RELEASE_NAME)

release_prepare:
	rm -rf $(PACKAGE_NAME)
	mkdir $(PACKAGE_NAME)
	mkdir $(PACKAGE_NAME)/include
	cp src/hl.h src/hlc.h src/hlc_main.c $(PACKAGE_NAME)/include

release_win:
	cp $(BUILD_DIR)/{hl.exe,libhl.dll,*.hdll,*.lib} $(PACKAGE_NAME)
	cp $(VS_RUNTIME_LIBRARY) $(PACKAGE_NAME)
	cp $(VS_SDL_LIBRARY) $(PACKAGE_NAME)
	cp $(VS_OPENAL_LIBRARY) $(PACKAGE_NAME)/OpenAL32.dll
	# 7z switches: https://sevenzip.osdn.jp/chm/cmdline/switches/
	7z a -spf -y -mx9 -bt $(PACKAGE_NAME).zip $(PACKAGE_NAME)
	rm -rf $(PACKAGE_NAME)

release_linux release_osx:
	cp hl libhl.$(LIBEXT) *.hdll $(PACKAGE_NAME)
	tar -cvzf $(PACKAGE_NAME).tar.gz $(PACKAGE_NAME)
	rm -rf $(PACKAGE_NAME)

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
