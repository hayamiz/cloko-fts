
noinst_LTLIBRARIES = test_inv_index.la

AM_CFLAGS = $(GLIB_CFLAGS)

LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_inv_index_la_SOURCES = test-inv-index.c

INCLUDES = $(CUTTER_CFLAGS) -I$(top_srcdir)/src
LIBS = $(CUTTER_LIBS) $(top_builddir)/src/libinv_index.la

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

echo-cutter:
	echo $(CUTTER)