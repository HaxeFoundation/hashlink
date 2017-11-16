SOURCES := haxe -lib webidl -main bullet.Generator --interp

all:

generate:
	haxe -lib webidl --macro "bullet.Generator.buildHL()"

genjs:
	haxe -lib webidl --macro "bullet.Generator.buildJS()"

.SUFFIXES : .cpp .bc

.cpp.bc:
	emcc -o $@ -c $<
