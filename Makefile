CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse
LFLAGS =

SRC = src/alloc.c src/callback.c src/code.c src/jit.c src/main.c src/module.c src/other.c src/std/misc.c

# Cygwin
ifeq ($(OS),Windows_NT)

CFLAGS += -Wl,--export-all-symbols

ifeq ($(ARCH),32)
CC=i686-pc-cygwin-gcc 
endif

else

# Linux
CFLAGS += -m$(ARCH)
LFLAGS += -Wl,--export-dynamic

endif

all: hl32 hl64

hl32:
	make ARCH=32 build
	
hl64:
	make ARCH=64 build

bench:
	(cd bench && haxe --interp -main Bench)
	
use: all
	cp hl32.exe Release/hl.exe
	cp hl64.exe x64/Release/hl.exe

build: $(SRC)
	${CC} ${CFLAGS} ${LFLAGS} -o hl$(ARCH) ${SRC}
	
.SUFFIXES : .c .o

.c.o :
	${CC} ${CFLAGS} -o $@ -c $<
	
.PHONY: hl32 hl64 bench