CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse -std=c11 -I include/pcre
LFLAGS =

PCRE = include/pcre/pcre_chartables.o include/pcre/pcre_compile.o include/pcre/pcre_dfa_exec.o \
	include/pcre/pcre_exec.o include/pcre/pcre_fullinfo.o include/pcre/pcre_globals.o \
	include/pcre/pcre_newline.o include/pcre/pcre_string_utils.o include/pcre/pcre_tables.o include/pcre/pcre_xclass.o 

RUNTIME = src/alloc.o

STD = src/std/array.o src/std/buffer.o src/std/bytes.o src/std/cast.o src/std/date.o src/std/error.o \
	src/std/fun.o src/std/maps.o src/std/math.o src/std/obj.o src/std/regexp.o src/std/string.o src/std/sys.o \
	src/std/types.o src/std/ucs2.o src/std/random.o
	
LIB = ${PCRE} ${RUNTIME} ${STD}

BOOT = src/hlc_main.o src/_main.o

UNAME := $(shell uname)

# Cygwin
ifeq ($(OS),Windows_NT)

CFLAGS += -Wl,--export-all-symbols

ifeq ($(ARCH),32)
CC=i686-pc-cygwin-gcc 
endif

else ifeq ($(UNAME),Darwin)

# Mac
CFLAGS += -m$(ARCH)
LFLAGS += -Wl,-export_dynamic

else

# Linux
CFLAGS += -m$(ARCH)
LFLAGS += -Wl,--export-dynamic

endif

all: libs hlc32 clean hlc64

libs: hl32lib hl64lib clean

hlc32:
	make ARCH=32 clean_o hlc

hlc64:
	make ARCH=64 clean_o hlc
	
hl32:
	make ARCH=32 build
	
hl64:
	make ARCH=64 build
	
hl32lib:
	make ARCH=32 clean_o lib
	
hl64lib:
	make ARCH=64 clean_o lib

lib: ${LIB}
	${AR} rcs hl${ARCH}lib.a ${LIB}

hlc: ${BOOT}
	${CC} ${CFLAGS} ${LFLAGS} -o hlc${ARCH} ${BOOT} hl${ARCH}lib.a
	
build: ${SRC}
	${CC} ${CFLAGS} ${LFLAGS} -o hl${ARCH} ${SRC}
	
.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<
	
clean_o:
	rm -rf ${STD} ${BOOT} ${RUNTIME} ${PCRE}
	
clean: clean_o
	
.PHONY: hl32 hl64 hlc32 hlc64 hlc
