if COND_SINGLE_COMPILATION_UNIT
GCJ_SOURCES = single-compilation-unit.cpp
else
GCJ_SOURCES = gcj-module.cpp gcj-util.cpp
endif

lib_LTLIBRARIES = gcj.la
gcj_la_SOURCES = $(GCJ_SOURCES)
gcj_la_LDFLAGS = -module -avoid-version ${GCJ_LIBS}

INCLUDES = -I$(top_srcdir)/include $(GCJ_INCLUDES)

AM_CPPFLAGS=$(QORE_CPPFLAGS)

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/gcj.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/gcj-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/gcj.la
