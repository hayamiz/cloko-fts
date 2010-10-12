lib_LTLIBRARIES = libinv_index.la

AM_CFLAGS = $(GLIB_CFLAGS)

LDFLAGS = -no-undefined

libinv_index_la_SOURCES = inv-index.c

bin_PROGRAMS = fts-node

fts_node_SOURCES = main.c

fts_node_LDADD =				\
	libinv_index.la				\
	$(GLIB_LIBS)
