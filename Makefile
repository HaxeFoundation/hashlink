CFLAGS = -g -Wall -O3 -I src -msse2 -mfpmath=sse -std=c11 -I include/pcre -D HLDLL_EXPORTS
LFLAGS = -L. -lhl -ldl
LIBFLAGS =
LIBEXT = so

ifndef ARCH
ARCH=32
endif

PCRE = include/pcre/pcre_chartables.o include/pcre/pcre_compile.o include/pcre/pcre_dfa_exec.o \
	include/pcre/pcre_exec.o include/pcre/pcre_fullinfo.o include/pcre/pcre_globals.o \
	include/pcre/pcre_newline.o include/pcre/pcre_string_utils.o include/pcre/pcre_tables.o include/pcre/pcre_xclass.o 

RUNTIME = src/alloc.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o \
	src/std/file.o src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/random.o src/std/regexp.o \
	src/std/socket.o src/std/string.o src/std/sys.o src/std/types.o src/std/ucs2.o

HL = src/callback.o src/code.o src/jit.o src/main.o src/module.o
	
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
CFLAGS += -m$(ARCH)
LFLAGS += -Wl,-export_dynamic

else

# Linux
CFLAGS += -m$(ARCH)
LFLAGS += -lm -Wl,--export-dynamic

endif

all: libhl hl

install_lib:
	cp libhl.${LIBEXT) /usr/local/lib

libs: fmt ui sdl 

libhl: ${LIB}
	${CC} -o libhl.$(LIBEXT) -m${ARCH} ${LIBFLAGS} -shared ${LIB}

hlc: ${BOOT}
	${CC} ${CFLAGS} -o hlc ${BOOT} ${LFLAGS}
	
hl: ${HL}
	${CC} ${CFLAGS} -o hl ${HL} ${LFLAGS}
	
.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<
	
clean_o:
	rm -rf ${STD} ${BOOT} ${RUNTIME} ${PCRE} ${HL}
	
clean: clean_o 

.PHONY: libhl hl hlc


