CFLAGS = -Wall -O3 -I src -msse2 -mfpmath=sse

SRC=src/alloc.c src/callback.c src/code.c src/jit.c src/main.c src/module.c

all: hl32 hl64

hl32:
	make CC=i686-pc-cygwin-gcc ARCH=32 build
	
hl64:
	make CC=gcc ARCH=64 build

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