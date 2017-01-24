
LBITS := $(shell getconf LONG_BIT)

ifndef ARCH
	ARCH = $(LBITS)
endif

CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse -std=c11 -I include/pcre -I include/mbedtls/include -D LIBHL_EXPORTS
LFLAGS = -L. -lhl
LIBFLAGS =
HLFLAGS = -ldl
LIBEXT = so
LIBTURBOJPEG = -lturbojpeg

PCRE = include/pcre/pcre_chartables.o include/pcre/pcre_compile.o include/pcre/pcre_dfa_exec.o \
	include/pcre/pcre_exec.o include/pcre/pcre_fullinfo.o include/pcre/pcre_globals.o \
	include/pcre/pcre_newline.o include/pcre/pcre_string_utils.o include/pcre/pcre_tables.o include/pcre/pcre_xclass.o

RUNTIME = src/alloc.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o \
	src/std/file.o src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/random.o src/std/regexp.o \
	src/std/socket.o src/std/string.o src/std/sys.o src/std/types.o src/std/ucs2.o src/std/thread.o src/std/process.o

HL = src/callback.o src/code.o src/jit.o src/main.o src/module.o src/debugger.o

FMT = libs/fmt/fmt.o

SDL = libs/sdl/sdl.o libs/sdl/gl.o libs/sdl/openal.o

SSL = libs/ssl/ssl.o

MBEDTLS = include/mbedtls/library/ssl_srv.o include/mbedtls/library/sha512.o include/mbedtls/library/sha256.o \
	include/mbedtls/library/pk_wrap.o include/mbedtls/library/pem.o include/mbedtls/library/pkwrite.o \
	include/mbedtls/library/xtea.o include/mbedtls/library/gcm.o include/mbedtls/library/des.o \
	include/mbedtls/library/ssl_cli.o include/mbedtls/library/aes.o include/mbedtls/library/ecp_curves.o \
	include/mbedtls/library/aesni.o include/mbedtls/library/blowfish.o include/mbedtls/library/ssl_cookie.o \
	include/mbedtls/library/ecp.o include/mbedtls/library/ecdh.o include/mbedtls/library/x509_create.o \
	include/mbedtls/library/timing.o include/mbedtls/library/pk.o include/mbedtls/library/md.o \
	include/mbedtls/library/pkcs5.o include/mbedtls/library/oid.o include/mbedtls/library/pkcs11.o \
	include/mbedtls/library/error.o include/mbedtls/library/ccm.o include/mbedtls/library/ssl_ticket.o \
	include/mbedtls/library/asn1write.o include/mbedtls/library/certs.o include/mbedtls/library/threading.o \
	include/mbedtls/library/padlock.o include/mbedtls/library/x509_crl.o include/mbedtls/library/debug.o \
	include/mbedtls/library/platform.o include/mbedtls/library/dhm.o include/mbedtls/library/pkcs12.o \
	include/mbedtls/library/ssl_ciphersuites.o include/mbedtls/library/cipher_wrap.o \
	include/mbedtls/library/base64.o include/mbedtls/library/x509write_csr.o include/mbedtls/library/ripemd160.o \
	include/mbedtls/library/rsa.o include/mbedtls/library/entropy_poll.o include/mbedtls/library/x509write_crt.o \
	include/mbedtls/library/pkparse.o include/mbedtls/library/ssl_cache.o include/mbedtls/library/x509_crt.o \
	include/mbedtls/library/ecdsa.o include/mbedtls/library/md_wrap.o include/mbedtls/library/md5.o \
	include/mbedtls/library/version.o include/mbedtls/library/arc4.o include/mbedtls/library/ctr_drbg.o \
	include/mbedtls/library/ecjpake.o include/mbedtls/library/entropy.o include/mbedtls/library/sha1.o \
	include/mbedtls/library/x509.o include/mbedtls/library/camellia.o include/mbedtls/library/cipher.o \
	include/mbedtls/library/memory_buffer_alloc.o include/mbedtls/library/hmac_drbg.o include/mbedtls/library/ssl_tls.o \
	include/mbedtls/library/havege.o include/mbedtls/library/version_features.o include/mbedtls/library/asn1parse.o \
	include/mbedtls/library/bignum.o include/mbedtls/library/x509_csr.o

LIB = ${PCRE} ${RUNTIME} ${STD}

BOOT = src/hlc_main.o src/_main.o

UNAME := $(shell uname)

# Cygwin
ifeq ($(OS),Windows_NT)

LIBFLAGS += -Wl,--export-all-symbols
LIBEXT = dll

ifeq ($(ARCH),32)
CC=i686-pc-cygwin-gcc
endif

else ifeq ($(UNAME),Darwin)

# Mac
LIBEXT=dylib
CFLAGS += -m$(ARCH) -I /opt/libjpeg-turbo/include -I /usr/local/opt/jpeg-turbo/include -I /usr/local/include -I /usr/local/opt/libvorbis/include -I /usr/local/opt/openal-soft/include
LFLAGS += -Wl,-export_dynamic -L/usr/local/lib
LIBFLAGS += -L/opt/libjpeg-turbo/lib -L/usr/local/opt/jpeg-turbo/lib -L/usr/local/lib -L/usr/local/opt/libvorbis/lib -L/usr/local/opt/openal-soft/lib
LIBOPENGL = -framework OpenGL
LIBSSL = -framework Security -framework CoreFoundation


else

# Linux
CFLAGS += -m$(ARCH) -fPIC
LFLAGS += -lm -Wl,--export-dynamic -Wl,--no-undefined

# otherwise ld will link to the .a and complain about missing -fPIC (Ubuntu 14)
LIBTURBOJPEG = -l:libturbojpeg.so.0

ifeq ($(ARCH),32)
CFLAGS += -I /usr/include/i386-linux-gnu
endif

endif

all: libhl hl libs

install_lib:
	cp libhl.${LIBEXT} /usr/local/lib

libs: fmt sdl ssl

libhl: ${LIB}
	${CC} -o libhl.$(LIBEXT) -m${ARCH} ${LIBFLAGS} -shared ${LIB} -lpthread -lm

hlc: ${BOOT}
	${CC} ${CFLAGS} -o hlc ${BOOT} ${LFLAGS}

hl: ${HL}
	echo $(ARCH)
	${CC} ${CFLAGS} -o hl ${HL} ${LFLAGS} ${HLFLAGS}

fmt: ${FMT}
	${CC} ${CFLAGS} -shared -o fmt.hdll ${FMT} ${LIBFLAGS} -lhl -lpng $(LIBTURBOJPEG) -lz -lvorbisfile

sdl: ${SDL}
	${CC} ${CFLAGS} -shared -o sdl.hdll ${SDL} ${LIBFLAGS} -lhl -lSDL2 -lopenal $(LIBOPENGL)

ssl: ${MBEDTLS} ${SSL}
	${CC} ${CFLAGS} -shared -o ssl.hdll ${SSL} ${MBEDTLS} ${LIBFLAGS} -lhl $(LIBSSL)

.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<

clean_o:
	rm -f ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL} ${FMT} ${SDL} ${MBEDTLS} ${SSL}

clean: clean_o
	rm -f hl hl.exe libhl.$(LIBEXT) *.hdll

.PHONY: libhl hl hlc fmt sdl libs
