
LBITS := $(shell getconf LONG_BIT)
MARCH ?= $(LBITS)
PREFIX ?= /usr/local
INSTALL_DIR ?= $(PREFIX)
INSTALL_BIN_DIR ?= $(PREFIX)/bin
INSTALL_LIB_DIR ?= $(PREFIX)/lib
INSTALL_INCLUDE_DIR ?= $(PREFIX)/include

LIBS=fmt sdl ssl openal ui uv mysql sqlite heaps
ARCH ?= $(shell uname -m)

CFLAGS = -Wall -O3 -I src -std=c11 -D LIBHL_EXPORTS
LFLAGS = -L. -lhl
EXTRA_LFLAGS ?=
LIBFLAGS =
HLFLAGS = -ldl
LIBEXT = so
LIBTURBOJPEG = -lturbojpeg

LHL_LINK_FLAGS =

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

ifdef SSL_STATIC
SSL += include/mbedtls/library/aes.o include/mbedtls/library/aesce.o include/mbedtls/library/aesni.o \
	include/mbedtls/library/aria.o include/mbedtls/library/asn1parse.o include/mbedtls/library/asn1write.o \
	include/mbedtls/library/base64.o include/mbedtls/library/bignum.o include/mbedtls/library/bignum_core.o \
	include/mbedtls/library/bignum_mod.o include/mbedtls/library/bignum_mod_raw.o \
	include/mbedtls/library/block_cipher.o include/mbedtls/library/camellia.o include/mbedtls/library/ccm.o \
	include/mbedtls/library/chacha20.o include/mbedtls/library/chachapoly.o include/mbedtls/library/cipher.o \
	include/mbedtls/library/cipher_wrap.o include/mbedtls/library/cmac.o include/mbedtls/library/constant_time.o \
	include/mbedtls/library/ctr_drbg.o include/mbedtls/library/debug.o include/mbedtls/library/des.o \
	include/mbedtls/library/dhm.o include/mbedtls/library/ecdh.o include/mbedtls/library/ecdsa.o \
	include/mbedtls/library/ecjpake.o include/mbedtls/library/ecp.o include/mbedtls/library/ecp_curves.o \
	include/mbedtls/library/ecp_curves_new.o include/mbedtls/library/entropy.o include/mbedtls/library/entropy_poll.o \
	include/mbedtls/library/error.o include/mbedtls/library/gcm.o include/mbedtls/library/hkdf.o \
	include/mbedtls/library/hmac_drbg.o include/mbedtls/library/lmots.o include/mbedtls/library/lms.o \
	include/mbedtls/library/md.o include/mbedtls/library/md5.o include/mbedtls/library/memory_buffer_alloc.o \
	include/mbedtls/library/mps_reader.o include/mbedtls/library/mps_trace.o include/mbedtls/library/net_sockets.o \
	include/mbedtls/library/nist_kw.o include/mbedtls/library/oid.o include/mbedtls/library/padlock.o \
	include/mbedtls/library/pem.o include/mbedtls/library/pk.o include/mbedtls/library/pk_ecc.o \
	include/mbedtls/library/pk_wrap.o include/mbedtls/library/pkcs12.o include/mbedtls/library/pkcs5.o \
	include/mbedtls/library/pkcs7.o include/mbedtls/library/pkparse.o include/mbedtls/library/pkwrite.o \
	include/mbedtls/library/platform.o include/mbedtls/library/platform_util.o include/mbedtls/library/poly1305.o \
	include/mbedtls/library/psa_crypto.o include/mbedtls/library/psa_crypto_aead.o include/mbedtls/library/psa_crypto_cipher.o \
	include/mbedtls/library/psa_crypto_client.o include/mbedtls/library/psa_crypto_driver_wrappers_no_static.o \
	include/mbedtls/library/psa_crypto_ecp.o include/mbedtls/library/psa_crypto_ffdh.o \
	include/mbedtls/library/psa_crypto_hash.o include/mbedtls/library/psa_crypto_mac.o \
	include/mbedtls/library/psa_crypto_pake.o include/mbedtls/library/psa_crypto_rsa.o \
	include/mbedtls/library/psa_crypto_se.o include/mbedtls/library/psa_crypto_slot_management.o \
	include/mbedtls/library/psa_crypto_storage.o include/mbedtls/library/psa_its_file.o \
	include/mbedtls/library/psa_util.o include/mbedtls/library/ripemd160.o include/mbedtls/library/rsa.o \
	include/mbedtls/library/rsa_alt_helpers.o include/mbedtls/library/sha1.o include/mbedtls/library/sha256.o \
	include/mbedtls/library/sha3.o include/mbedtls/library/sha512.o include/mbedtls/library/ssl_cache.o \
	include/mbedtls/library/ssl_ciphersuites.o include/mbedtls/library/ssl_client.o include/mbedtls/library/ssl_cookie.o \
	include/mbedtls/library/ssl_debug_helpers_generated.o include/mbedtls/library/ssl_msg.o \
	include/mbedtls/library/ssl_ticket.o include/mbedtls/library/ssl_tls.o include/mbedtls/library/ssl_tls12_client.o \
	include/mbedtls/library/ssl_tls12_server.o include/mbedtls/library/ssl_tls13_client.o \
	include/mbedtls/library/ssl_tls13_generic.o include/mbedtls/library/ssl_tls13_keys.o \
	include/mbedtls/library/ssl_tls13_server.o include/mbedtls/library/threading.o include/mbedtls/library/timing.o \
	include/mbedtls/library/version.o include/mbedtls/library/version_features.o include/mbedtls/library/x509.o \
	include/mbedtls/library/x509_create.o include/mbedtls/library/x509_crl.o include/mbedtls/library/x509_crt.o \
	include/mbedtls/library/x509_csr.o include/mbedtls/library/x509write.o include/mbedtls/library/x509write_crt.o \
	include/mbedtls/library/x509write_csr.o
SSL_CFLAGS = -fvisibility=hidden -I libs/ssl -I include/mbedtls/include -D MBEDTLS_USER_CONFIG_FILE=\"mbedtls_user_config.h\"
else
SSL_LDLIBS = -lmbedtls -lmbedx509 -lmbedcrypto
endif

UV = libs/uv/uv.o

UI = libs/ui/ui_stub.o

MYSQL = libs/mysql/socket.o libs/mysql/sha1.o libs/mysql/my_proto.o libs/mysql/my_api.o libs/mysql/mysql.o

SQLITE = libs/sqlite/sqlite.o

HEAPS = libs/heaps/mikkt.o libs/heaps/meshoptimizer.o libs/heaps/vhacd.o
HEAPS += include/mikktspace/mikktspace.o
HEAPS += include/meshoptimizer/allocator.o include/meshoptimizer/overdrawoptimizer.o \
	include/meshoptimizer/vcacheoptimizer.o include/meshoptimizer/clusterizer.o \
	include/meshoptimizer/quantization.o include/meshoptimizer/vertexcodec.o \
	include/meshoptimizer/indexcodec.o include/meshoptimizer/simplifier.o \
	include/meshoptimizer/vertexfilter.o include/meshoptimizer/indexgenerator.o \
	include/meshoptimizer/spatialorder.o include/meshoptimizer/vfetchanalyzer.o \
	include/meshoptimizer/stripifier.o include/meshoptimizer/vfetchoptimizer.o \
	include/meshoptimizer/overdrawanalyzer.o include/meshoptimizer/vcacheanalyzer.o
HEAPS_CFLAGS = -fvisibility=hidden -I include/mikktspace -I include/meshoptimizer -I include/vhacd

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

BREW_PREFIX := $(shell brew --prefix)
# prefixes for keg-only packages
BREW_OPENAL_PREFIX := $(shell brew --prefix openal-soft)

CFLAGS += -m$(MARCH) -I include -I $(BREW_PREFIX)/include -I $(BREW_OPENAL_PREFIX)/include -Dopenal_soft -DGL_SILENCE_DEPRECATION
LFLAGS += -Wl,-export_dynamic

ifdef OSX_SDK
ISYSROOT = $(shell xcrun --sdk macosx$(OSX_SDK) --show-sdk-path)
CFLAGS += -isysroot $(ISYSROOT)
LFLAGS += -isysroot $(ISYSROOT)
endif

LIBFLAGS += -L$(BREW_PREFIX)/lib -L$(BREW_OPENAL_PREFIX)/lib
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

LFLAGS += -rpath @executable_path -rpath $(INSTALL_LIB_DIR)
LIBFLAGS += -rpath @executable_path -rpath $(INSTALL_LIB_DIR)
LHL_LINK_FLAGS += -install_name @rpath/libhl.dylib
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

all: libhl libs
ifeq ($(ARCH),arm64)
	$(warning HashLink vm is not supported on arm64, skipping)
else
all: hl
endif

install:
	$(UNAME)==Darwin && ${MAKE} uninstall
ifneq ($(ARCH),arm64)
	mkdir -p $(INSTALL_BIN_DIR)
	cp hl $(INSTALL_BIN_DIR)
endif
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
	${CC} ${CFLAGS} -o libhl.$(LIBEXT) -m${MARCH} ${LIBFLAGS} ${LHL_LINK_FLAGS} -shared ${LIB} -lpthread -lm

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

./include/mbedtls/%.o: ./include/mbedtls/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${SSL_CFLAGS}

# force rebuild ssl.o in case we mix SSL_STATIC with normal build
.PHONY: libs/ssl/ssl.o
libs/ssl/ssl.o: libs/ssl/ssl.c
	${CC} ${CFLAGS} -o $@ -c $< ${SSL_CFLAGS}

ssl: ${SSL} libhl
	${CC} ${CFLAGS} ${SSL_CFLAGS} -shared -o ssl.hdll ${SSL} ${LIBFLAGS} -L. -lhl ${SSL_LDLIBS} $(LIBSSL)

ui: ${UI} libhl
	${CC} ${CFLAGS} -shared -o ui.hdll ${UI} ${LIBFLAGS} -L. -lhl

uv: ${UV} libhl
	${CC} ${CFLAGS} -shared -o uv.hdll ${UV} ${LIBFLAGS} -L. -lhl -luv

mysql: ${MYSQL} libhl
	${CC} ${CFLAGS} -shared -o mysql.hdll ${MYSQL} ${LIBFLAGS} -L. -lhl

sqlite: ${SQLITE} libhl
	${CC} ${CFLAGS} -shared -o sqlite.hdll ${SQLITE} ${LIBFLAGS} -L. -lhl -lsqlite3

CXXFLAGS:=$(filter-out -std=c11,$(CFLAGS)) -std=c++11

./include/mikktspace/%.o: ./include/mikktspace/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${HEAPS_CFLAGS}

./include/meshoptimizer/%.o: ./include/meshoptimizer/%.cpp
	${CC} ${CXXFLAGS} -o $@ -c $< ${HEAPS_CFLAGS}

./libs/heaps/%.o: ./libs/heaps/%.c
	${CC} ${CFLAGS} -o $@ -c $< ${HEAPS_CFLAGS}

./libs/heaps/%.o: ./libs/heaps/%.cpp
	${CC} ${CXXFLAGS} -o $@ -c $< ${HEAPS_CFLAGS}

heaps: ${HEAPS} libhl
	${CXX} ${CFLAGS} ${HEAPS_CFLAGS} -shared -o heaps.hdll ${HEAPS} ${LIBFLAGS} -L. -lhl

mesa:
	(cd libs/mesa && ${MAKE})

release: release_prepare release_$(RELEASE_NAME)

release_haxelib:
	${MAKE} HLIB=hashlink release_haxelib_package
	${MAKE} HLIB=directx release_haxelib_package
	${MAKE} HLIB=sdl release_haxelib_package
	${MAKE} HLIB=openal release_haxelib_package

ifeq ($(HLIB),hashlink)
HLDIR=other/haxelib
HLPACK=templates hlmem memory.hxml Run.hx
else
HLDIR=libs/$(HLIB)
ifeq ($(HLIB),directx)
HLPACK=dx *.h *.c *.cpp
else
HLPACK=$(HLIB) *.h *.c
endif
endif

release_haxelib_package:
	rm -rf $(HLIB)_release
	mkdir $(HLIB)_release
	(cd $(HLDIR) && cp -R $(HLPACK) haxelib.json $(CURDIR)/$(HLIB)_release | true)
	zip -r $(HLIB).zip $(HLIB)_release
	haxelib submit $(HLIB).zip
	rm -rf $(HLIB)_release

BUILD_DIR ?= .
PACKAGE_NAME = $(eval PACKAGE_NAME := hashlink-$(shell $(BUILD_DIR)/hl --version)-$(RELEASE_NAME))$(PACKAGE_NAME)

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
ifeq ($(ARCH),arm64)
	cp libhl.$(LIBEXT) *.hdll $(PACKAGE_NAME)
else
	cp hl libhl.$(LIBEXT) *.hdll $(PACKAGE_NAME)
endif
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
	rm -f ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL} ${FMT} ${SDL} ${SSL} ${OPENAL} ${UI} ${UV} ${MYSQL} ${SQLITE} ${HEAPS} ${HL_DEBUG}

clean: clean_o
	rm -f hl hl.exe libhl.$(LIBEXT) *.hdll

.PHONY: libhl hl hlc fmt sdl libs release
