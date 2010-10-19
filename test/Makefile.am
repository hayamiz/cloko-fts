
noinst_LTLIBRARIES =				\
	test_inv_index.la			\
	test_document_set.la			\
	test_doc_tokenizer.la			\
	test_fixed_posting_list.la		\
	test_fixed_index.la			\
	test_posting_list.la			\
	test_bloom_filter.la			\
	test_search.la

AM_CFLAGS = $(GLIB_CFLAGS)

LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

test_inv_index_la_SOURCES = test-inv-index.c
test_posting_list_la_SOURCES = test-posting-list.c 
test_document_set_la_SOURCES = test-document-set.c
test_doc_tokenizer_la_SOURCES = test-doc-tokenizer.c
test_fixed_posting_list_la_SOURCES = test-fixed-posting-list.c
test_fixed_index_la_SOURCES = test-fixed-index.c
test_bloom_filter_la_SOURCES = test-bloom-filter.c
test_search_la_SOURCES = test-search.c

INCLUDES = $(CUTTER_CFLAGS) -I$(top_srcdir)/src
LIBS =						\
	$(CUTTER_LIBS)				\
	$(top_builddir)/src/libinv_index.la	\
	$(top_builddir)/src/libbloom_filter.la

TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

echo-cutter:
	echo $(CUTTER)