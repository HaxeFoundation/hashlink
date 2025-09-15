# Auto-generated from hashlink's Makefile template; do not edit

UNAME := $(shell uname)
CFLAGS=-O3 -std=c11 -DHL_MAKE -I$$makePath(::relDir::)
ifeq ($(UNAME),Darwin)
LDLIBS=-lhl -lm::foreach libraries::::if (name == "uv"):: -luv::end:: /usr/local/lib/::name::.hdll::end::
else
LDLIBS=-lhl -lm::foreach libraries::::if (name == "uv"):: -luv::end:: -l:::name::.hdll::end::
endif

ifdef HASHLINK
CFLAGS+=-I$(HASHLINK)/include -I$(HASHLINK)/src
LDFLAGS+=-L$(HASHLINK)/lib -L$(HASHLINK)
endif

ifeq ($(OS),Windows_NT)
LDFLAGS+=-municode
LDLIBS+=-ldbghelp
EXE_SUFFIX=.exe
endif

SRC_FILES=::foreach cfiles:: \
	::path::::end::

OBJ_FILES=$(patsubst %.c,%.o,$(SRC_FILES))

DEP_FILES=$(patsubst %.c,%.d,$(SRC_FILES))

::name::$(EXE_SUFFIX): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(LDFLAGS) $(LDLIBS)

-include $(DEP_FILES)

%.o:
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c ::relDir::$*.c -o $@

clean:
	find . \( -name '*.o' -o -name '*.d' \) -delete
	rm ::name::$(EXE_SUFFIX)

.SUFFIXES:
