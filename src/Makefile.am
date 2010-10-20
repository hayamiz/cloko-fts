# lib_LTLIBRARIES = libinv_index.la libbloom_filter.la
noinst_LTLIBRARIES = libinv_index.la libbloom_filter.la

AM_CFLAGS = $(GLIB_CFLAGS)

LDFLAGS = -no-undefined -Wl,--eh-frame-hdr

libinv_index_la_SOURCES =			\
	inv-index.c				\
	posting-list.c				\
	document-set.c				\
	document.c				\
	tokenizer.c				\
	fixed-posting-list.c			\
	fixed-index.c				\
	phrase.c				\
	thread-pool.c				\
	frame.c

libbloom_filter_la_SOURCES =			\
	bloom-filter.c

libbloom_filter_la_LIBADD = -lm

bin_PROGRAMS = fts-node fts-search termdump

fts_node_SOURCES =				\
	main.c

fts_search_SOURCES =				\
	fts-search.c

termdump_SOURCES =				\
	termdump.c

fts_node_LDADD =				\
	libinv_index.la				\
	libbloom_filter.la			\
	$(GLIB_LIBS)

fts_search_LDADD =				\
	libinv_index.la				\
	$(GLIB_LIBS)

termdump_LDADD =				\
	libinv_index.la				\
	libbloom_filter.la			\
	$(GLIB_LIBS)
