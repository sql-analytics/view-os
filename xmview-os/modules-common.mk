DEBUGLEVEL = 3

moddir = $(pkglibdir)/modules


CBASEFLAGS =
CDEBUGFLAGS = -g3 -ggdb3

CPPBASEFLAGS = -I../include -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CPPDEBUGFLAGS = -DGDEBUG_ENABLED -DGDEBUG_LEVEL=$(DEBUGLEVEL)

if ENABLE_DEBUG
CFULLFLAGS = $(CBASEFLAGS) $(CDEBUGFLAGS)
CPPFULLFLAGS = $(CPPBASEFLAGS) $(CPPDEBUGFLAGS)

release: clean
	$(MAKE) CFULLFLAGS='$(CBASEFLAGS)' CPPFULLFLAGS='$(CPPBASEFLAGS)'

debug: all
else
CFULLFLAGS = $(CBASEFLAGS)
CPPFULLFLAGS = $(CPPBASEFLAGS)

release: all

debug: clean
	$(MAKE) CFULLFLAGS='$(CBASEFLAGS) $(CDEBUGFLAGS)' CPPFULLFLAGS='$(CPPBASEFLAGS) $(CPPDEBUGFLAGS)'
endif

AM_CFLAGS = $(CFULLFLAGS)
AM_CPPFLAGS = $(CPPFULLFLAGS)
AM_LDFLAGS = -module -avoid-version -export-dynamic
AM_LIBTOOLFLAGS = --tag=disable-static
LIBS = ../um_lib/libumlib.la

install-data-hook:
	cd "$(DESTDIR)/$(moddir)" && rm -f $(mod_LTLIBRARIES)

debug release: all


