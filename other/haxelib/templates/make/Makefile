# Auto-generated from hashlink's Makefile template; do not edit

CFLAGS=-O3 -std=c11 -DHL_MAKE -I$$makePath(::relDir::)
LDLIBS=-lhl -lm -luv::foreach libraries:: -l:::name::.hdll::end::

ifeq ($(OS),Windows_NT)
CFLAGS+=-I$(HASHLINK)/include -I$(HASHLINK)/src
LDFLAGS=-L$(HASHLINK) -L$(HASHLINK)/lib -municode
LDLIBS+=C:/Windows/System32/dbghelp.dll
EXE_SUFFIX=.exe
endif

SRC_FILES=::foreach cfiles:: \
	::relDir::::path::::end::

OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))

DEP_FILES=$(patsubst %.c,%.d,$(SRC_FILES))

::name::$(EXE_SUFFIX): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	find . \( -name '*.o' -o -name '*.d' \) -delete
	rm ::name::$(EXE_SUFFIX)
