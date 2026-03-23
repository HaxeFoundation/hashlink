# Auto-generated from hashlink's Makefile template; do not edit

UNAME := $(shell uname)
CFLAGS = -std=c11 $(EXTRA_CFLAGS)
CPPFLAGS = -DHL_MAKE -I$$makePath(::relDir::)
LDFLAGS = $(EXTRA_LDFLAGS)
ifeq ($(UNAME),Darwin)
LDLIBS=-lhl -lm::foreach libraries::::if (name == "uv"):: -luv::end:: /usr/local/lib/::name::.hdll::end::
else
LDLIBS=-lhl -lm::foreach libraries::::if (name == "uv"):: -luv::end:: -l:::name::.hdll::end::
endif

ifdef DEBUG
CFLAGS += -g
else
CFLAGS += -O3
endif

ifdef HASHLINK
CPPFLAGS += -I$(HASHLINK)/include -I$(HASHLINK)/src
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
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c ::relDir::$*.c -o $@

clean:
	find . \( -name '*.o' -o -name '*.d' \) -delete
	rm ::name::$(EXE_SUFFIX)

.SUFFIXES:
