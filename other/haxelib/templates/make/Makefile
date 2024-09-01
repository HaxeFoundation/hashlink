CFLAGS=-O3 -std=c11 -DHL_MAKE -I$$makePath(::relDir::)
LDFLAGS=-lhl -lm -luv::foreach libraries:: -l:::name::.hdll::end::

SRC_FILES=::foreach cfiles:: \
	::relDir::::path::::end::

HEADER_FILES=::foreach hfiles:: \
	::relDir::::path::::end::

::name::: $(SRC_FILES) $(HEADER_FILES)
	$(CC) $(CFLAGS) $(SRC_FILES) -o ::name:: $(LDFLAGS)

clean:
	rm ::name::
