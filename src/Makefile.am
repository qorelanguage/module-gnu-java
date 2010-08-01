if COND_SINGLE_COMPILATION_UNIT
GNU_JAVA_SOURCES = single-compilation-unit.cpp
else
GNU_JAVA_SOURCES = gnu-java-module.cpp gnu-java-util.cpp
endif

lib_LTLIBRARIES = gnu-java.la
gnu_java_la_SOURCES = $(GNU_JAVA_SOURCES)
gnu_java_la_LDFLAGS = -module -avoid-version ${GNU_JAVA_LIBS}

INCLUDES = -I$(top_srcdir)/include $(GNU_JAVA_INCLUDES)

AM_CPPFLAGS=$(QORE_CPPFLAGS)

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/gnu-java.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/gnu-java-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/gnu-java.la
