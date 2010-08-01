AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/gnu-java-module.h src/gnu-java-util.h

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/gnu-java-test.q \
	docs/gnu-java-module-doc.html \
	qore-gnu-java-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
