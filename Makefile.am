AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/gcj-module.h src/gcj-util.h

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/gcj-test.q \
	docs/gcj-module-doc.html \
	qore-gcj-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
