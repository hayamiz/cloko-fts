# lib_LTLIBRARIES = libinv_index.la
noinst_LTLIBRARIES = libinv_index.la

AM_CFLAGS = $(GLIB_CFLAGS)

LDFLAGS = -no-undefined

libinv_index_la_SOURCES =			\
	inv-index.c				\
	posting-list.c				\
	document-set.c				\
	document.c				\
	tokenizer.c				\
	fixed-posting-list.c			\
	fixed-index.c				\
	phrase.c				\
	doc-id-list.c

bin_PROGRAMS = fts-node

fts_node_SOURCES =				\
	main.c

fts_node_LDADD =				\
	libinv_index.la				\
	$(GLIB_LIBS)
