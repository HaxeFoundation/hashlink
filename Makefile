
LBITS := $(shell getconf LONG_BIT)
MARCH ?= $(LBITS)
PREFIX ?= /usr/local
INSTALL_DIR ?= $(PREFIX)
INSTALL_BIN_DIR ?= $(PREFIX)/bin
INSTALL_LIB_DIR ?= $(PREFIX)/lib
INSTALL_INCLUDE_DIR ?= $(PREFIX)/include

LIBS = $(addsuffix .hdll,fmt sdl ssl openal ui uv mysql sqlite heaps)
ARCH ?= $(shell uname -m)

CFLAGS = -Wall -O3 -std=c11 -fvisibility=hidden
CPPFLAGS = -I src
LIBHL_LDFLAGS =
LIBHL_LDLIBS = -lm -lpthread
USE_LIBHL_LDFLAGS =
hl_LDLIBS = -ldl -lm
LIBEXT = so

PCRE_CPPFLAGS = -D PCRE2_CODE_UNIT_WIDTH=16
ifdef WITH_SYSTEM_PCRE2
LIBHL_LDLIBS += -lpcre2-16
else
PCRE_CPPFLAGS += -I include/pcre -D HAVE_CONFIG_H -D PCRE2_STATIC
PCRE = include/pcre/pcre2_auto_possess.o include/pcre/pcre2_chartables.o include/pcre/pcre2_compile.o \
	include/pcre/pcre2_config.o include/pcre/pcre2_context.o include/pcre/pcre2_convert.o \
	include/pcre/pcre2_dfa_match.o include/pcre/pcre2_error.o include/pcre/pcre2_extuni.o \
	include/pcre/pcre2_find_bracket.o include/pcre/pcre2_jit_compile.o include/pcre/pcre2_maketables.o \
	include/pcre/pcre2_match_data.o include/pcre/pcre2_match.o include/pcre/pcre2_newline.o \
	include/pcre/pcre2_ord2utf.o include/pcre/pcre2_pattern_info.o include/pcre/pcre2_script_run.o \
	include/pcre/pcre2_serialize.o include/pcre/pcre2_string_utils.o include/pcre/pcre2_study.o \
	include/pcre/pcre2_substitute.o include/pcre/pcre2_substring.o include/pcre/pcre2_tables.o \
	include/pcre/pcre2_ucd.o include/pcre/pcre2_valid_utf.o include/pcre/pcre2_xclass.o
endif

RUNTIME = src/gc.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o src/std/debug.o \
	src/std/file.o src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/random.o src/std/regexp.o \
	src/std/socket.o src/std/string.o src/std/sys.o src/std/types.o src/std/ucs2.o src/std/thread.o src/std/process.o \
	src/std/track.o

HL_OBJ = src/code.o src/jit.o src/main.o src/module.o src/debugger.o src/profile.o

FMT_CPPFLAGS = -I include/mikktspace -I include/minimp3

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
SSL_CPPFLAGS = -I libs/ssl -I include/mbedtls/include -D MBEDTLS_USER_CONFIG_FILE=\"mbedtls_user_config.h\"
else
ssl_LDLIBS = -lmbedtls -lmbedx509 -lmbedcrypto
endif

UV = libs/uv/uv.o

UI = libs/ui/ui_stub.o

MYSQL = libs/mysql/socket.o libs/mysql/sha1.o libs/mysql/my_proto.o libs/mysql/my_api.o libs/mysql/mysql.o

SQLITE = libs/sqlite/sqlite.o

HEAPS = libs/heaps/mikkt.o libs/heaps/meshoptimizer.o libs/heaps/vhacd.o libs/heaps/renderdoc.o
HEAPS += include/mikktspace/mikktspace.o
HEAPS += include/meshoptimizer/allocator.o include/meshoptimizer/overdrawoptimizer.o \
	include/meshoptimizer/vcacheoptimizer.o include/meshoptimizer/clusterizer.o \
	include/meshoptimizer/quantization.o include/meshoptimizer/vertexcodec.o \
	include/meshoptimizer/indexcodec.o include/meshoptimizer/simplifier.o \
	include/meshoptimizer/vertexfilter.o include/meshoptimizer/indexgenerator.o \
	include/meshoptimizer/spatialorder.o include/meshoptimizer/vfetchanalyzer.o \
	include/meshoptimizer/stripifier.o include/meshoptimizer/vfetchoptimizer.o \
	include/meshoptimizer/overdrawanalyzer.o include/meshoptimizer/vcacheanalyzer.o
HEAPS_CPPFLAGS = -I include/mikktspace -I include/meshoptimizer -I include/vhacd -I include/renderdoc

LIB = ${PCRE} ${RUNTIME} ${STD}

BOOT = src/_main.o

UNAME := $(shell uname)

# Cygwin / mingw
ifeq ($(OS),Windows_NT)

EXE_SUFFIX = .exe
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
VS_DX_LIBRARY ?= include/dx/bin/x86/dxcompiler.dll include/dx/bin/x86/dxil.dll
else
BUILD_DIR = x64/Release
VS_SDL_LIBRARY ?= include/sdl/lib/x64/SDL2.dll
VS_OPENAL_LIBRARY ?= include/openal/bin/Win64/soft_oal.dll
VS_DX_LIBRARY ?= include/dx/bin/x64/dxcompiler.dll include/dx/bin/x64/dxil.dll
endif

ifneq (, $(findstring MINGW64, $(UNAME)))
CFLAGS += -municode
LIBHL_LDLIBS += -lws2_32 -lwsock32
hl_LDLIBS = -lm
hlc_LDLIBS = -ldbghelp
ssl_LDLIBS += -lcrypt32 -lbcrypt -lws2_32
mysql_LDLIBS += -lws2_32 -lwsock32
endif

else ifeq ($(UNAME),Darwin)

# Mac
LIBEXT=dylib

BREW_PREFIX := $(shell brew --prefix)
# prefixes for keg-only packages
BREW_OPENAL_PREFIX := $(shell brew --prefix openal-soft)
BREW_SDL_PREFIX := $(shell brew --prefix sdl2)

CFLAGS += -m$(MARCH)
CPPFLAGS += -I include -I $(BREW_PREFIX)/include

SDL_CPPFLAGS = -I $(BREW_SDL_PREFIX)/include/SDL2 -DGL_SILENCE_DEPRECATION
OPENAL_CPPFLAGS = -I $(BREW_OPENAL_PREFIX)/include -Dopenal_soft

ifdef OSX_SDK
ISYSROOT = $(shell xcrun --sdk macosx$(OSX_SDK) --show-sdk-path)
CFLAGS += -isysroot $(ISYSROOT)
endif

LDFLAGS += -L$(BREW_PREFIX)/lib
sdl_LDFLAGS = -L$(BREW_SDL_PREFIX)/lib
sdl_LDLIBS = -lSDL2 -framework OpenGL
openal_LDFLAGS = -L$(BREW_OPENAL_PREFIX)/lib
openal_LDLIBS = -lopenal
ssl_LDLIBS += -framework Security -framework CoreFoundation
RELEASE_NAME = osx

# Mac native debug
ifneq ($(ARCH),arm64)
HL_DEBUG = include/mdbg/mdbg.o include/mdbg/mach_excServer.o include/mdbg/mach_excUser.o
LIB += ${HL_DEBUG}
endif

CFLAGS += -arch $(ARCH)

LIBHL_LDFLAGS += -install_name @rpath/libhl.dylib
USE_LIBHL_LDFLAGS = -rpath @executable_path -rpath $(INSTALL_LIB_DIR)
else

# Linux
CFLAGS += -m$(MARCH) -fPIC -pthread -fno-omit-frame-pointer
LDFLAGS += -Wl,--no-undefined
USE_LIBHL_LDFLAGS = -Wl,-rpath,.:'$$ORIGIN':$(INSTALL_LIB_DIR)

ifeq ($(MARCH),32)
CFLAGS += -msse2 -mfpmath=sse
CPPFLAGS += -I /usr/include/i386-linux-gnu
fmt_LDFLAGS = -L/opt/libjpeg-turbo/lib
else
fmt_LDFLAGS = -L/opt/libjpeg-turbo/lib64
endif

SDL_CPPFLAGS = $(shell pkg-config --cflags sdl2)
sdl_LDFLAGS = $(shell pkg-config --libs-only-L --libs-only-other sdl2)
sdl_LDLIBS = $(shell pkg-config --libs-only-l sdl2) -lGL
openal_LDLIBS = -lopenal
RELEASE_NAME = linux

endif


ifdef MESA
LIBS += mesa
endif

ifdef DEBUG
CFLAGS += -g
endif

LIBHL = libhl.$(LIBEXT)
HL = hl$(EXE_SUFFIX)
HLC = hlc$(EXE_SUFFIX)

all: $(LIBHL) libs
ifeq ($(ARCH),arm64)
	$(warning HashLink vm is not supported on arm64, skipping)
else
all: $(HL)
endif

install:
	$(UNAME)==Darwin && ${MAKE} uninstall
ifneq ($(ARCH),arm64)
	mkdir -p $(INSTALL_BIN_DIR)
	cp $(HL) $(INSTALL_BIN_DIR)
endif
	mkdir -p $(INSTALL_LIB_DIR)
	cp *.hdll $(INSTALL_LIB_DIR)
	cp $(LIBHL) $(INSTALL_LIB_DIR)
	mkdir -p $(INSTALL_INCLUDE_DIR)
	cp src/hl.h src/hlc.h src/hlc_main.c $(INSTALL_INCLUDE_DIR)

uninstall:
	rm -f $(INSTALL_BIN_DIR)/$(HL) $(INSTALL_LIB_DIR)/$(LIBHL) $(INSTALL_LIB_DIR)/*.hdll
	rm -f $(INSTALL_INCLUDE_DIR)/hl.h $(INSTALL_INCLUDE_DIR)/hlc.h $(INSTALL_INCLUDE_DIR)/hlc_main.c

libs: $(LIBS)

src/std/regexp.o $(PCRE): CPPFLAGS += $(PCRE_CPPFLAGS)
$(LIB): CPPFLAGS += -D LIBHL_EXPORTS
$(LIBHL): $(LIB)
	$(LINK.c) $(LIBHL_LDFLAGS) -shared $^ $(LIBHL_LDLIBS) -o $@

$(HL): $(HL_OBJ) $(LIBHL)
$(HLC): $(BOOT) $(LIBHL)
$(HL) $(HLC):
	$(LINK.c) $(USE_LIBHL_LDFLAGS) $^ $($@_LDLIBS) -o $@

%.hdll: HDLL_LINK = $(LINK.c)
%.hdll:
	$(HDLL_LINK) $(USE_LIBHL_LDFLAGS) $($*_LDFLAGS) -shared $^ $($*_LDLIBS) -o $@

$(FMT): CPPFLAGS += $(FMT_CPPFLAGS)
fmt_LDLIBS = -lpng -lturbojpeg -lvorbisfile -lz -lm
fmt.hdll: $(FMT) $(LIBHL)

$(SDL): CPPFLAGS += $(SDL_CPPFLAGS)
sdl.hdll: $(SDL) $(LIBHL)

$(OPENAL): CPPFLAGS += $(OPENAL_CPPFLAGS)
openal.hdll: $(OPENAL) $(LIBHL)

$(SSL): CPPFLAGS += $(SSL_CPPFLAGS)
# force rebuild ssl.o in case we mix SSL_STATIC with normal build
.PHONY: libs/ssl/ssl.o
libs/ssl/ssl.o: libs/ssl/ssl.c
	$(COMPILE.c) -o $@ $<
ssl.hdll: $(SSL) $(LIBHL)

ui.hdll: $(UI) $(LIBHL)

uv_LDLIBS = -luv
uv.hdll: $(UV) $(LIBHL)

mysql.hdll: $(MYSQL) $(LIBHL)

sqlite_LDLIBS = -lsqlite3
sqlite.hdll: $(SQLITE) $(LIBHL)

CXXFLAGS:=$(filter-out -std=c11,$(CFLAGS)) -std=c++11

$(HEAPS): CPPFLAGS += $(HEAPS_CPPFLAGS)
heaps.hdll: HDLL_LINK = $(LINK.cc)
heaps.hdll: $(HEAPS) $(LIBHL)

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
PACKAGE_NAME = $(eval PACKAGE_NAME := hashlink-$(shell $(BUILD_DIR)/$(HL) --version)-$(RELEASE_NAME))$(PACKAGE_NAME)

release_prepare:
	rm -rf $(PACKAGE_NAME)
	mkdir $(PACKAGE_NAME)
	mkdir $(PACKAGE_NAME)/include
	cp src/hl.h src/hlc.h src/hlc_main.c $(PACKAGE_NAME)/include

release_win:
	cp $(BUILD_DIR)/{$(HL),libhl.dll,*.hdll,*.lib} $(PACKAGE_NAME)
	cp $(VS_RUNTIME_LIBRARY) $(PACKAGE_NAME)
	cp $(VS_SDL_LIBRARY) $(PACKAGE_NAME)
	cp $(VS_OPENAL_LIBRARY) $(PACKAGE_NAME)/OpenAL32.dll
	cp $(VS_DX_LIBRARY) $(PACKAGE_NAME)
	# 7z switches: https://sevenzip.osdn.jp/chm/cmdline/switches/
	7z a -spf -y -mx9 -bt $(PACKAGE_NAME).zip $(PACKAGE_NAME)
	rm -rf $(PACKAGE_NAME)

release_linux release_osx:
ifeq ($(ARCH),arm64)
	cp $(LIBHL) *.hdll $(PACKAGE_NAME)
else
	cp $(HL) $(LIBHL) *.hdll $(PACKAGE_NAME)
endif
	tar -cvzf $(PACKAGE_NAME).tar.gz $(PACKAGE_NAME)
	rm -rf $(PACKAGE_NAME)

codesign_osx:
	sudo security delete-identity -c hl-cert || echo
	echo "[req]\ndistinguished_name=codesign_dn\n[codesign_dn]\ncommonName=hl-cert\n[v3_req]\nkeyUsage=critical,digitalSignature\nextendedKeyUsage=critical,codeSigning" > openssl.cnf
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -nodes -days 365 -subj '/CN=hl-cert' -outform der -out cert.cer -extensions v3_req -config openssl.cnf
	sudo security add-trusted-cert -d -k /Library/Keychains/System.keychain cert.cer
	sudo security import key.pem -k /Library/Keychains/System.keychain -A
	codesign --entitlements other/osx/entitlements.xml -fs hl-cert $(HL)
	rm key.pem cert.cer openssl.cnf

# restrict built in rules to only handle cpp, c and o files
.SUFFIXES:
.SUFFIXES: .cpp .c .o

clean_o:
	rm -f ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL_OBJ} ${FMT} ${SDL} ${SSL} ${OPENAL} ${UI} ${UV} ${MYSQL} ${SQLITE} ${HEAPS} ${HL_DEBUG}

clean: clean_o
	rm -f $(HL) $(HLC) $(LIBHL) *.hdll

.PHONY: libs release
