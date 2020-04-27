main: main.c ../../src/gc.h bench/mandelbrot.c
	gcc -Wall -Werror -pedantic \
		-Wno-unused-function -Wno-unused-variable -Wno-gnu-zero-variadic-macro-arguments \
		-O3 -std=c11 -o main -I ../../src main.c bench/mandelbrot.c ../../libhl.a

#-O3 -std=c11 -o main -I ../../src main.c bench/mandelbrot.c -lhl
