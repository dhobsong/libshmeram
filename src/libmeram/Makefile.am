## Process this file with automake to produce Makefile.in

INCLUDES = $(INCLTDL) \
           -I$(top_builddir) \
           -I$(top_srcdir)/include

EXTRA_DIST = Version_script.in

AM_CFLAGS =

# Libraries to build
lib_LTLIBRARIES = libmeram.la

libmeram_la_SOURCES = \
	meram.c

libmeram_la_CFLAGS = $(UIOMUX_CFLAGS)
libmeram_la_LDFLAGS = -version-info @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
libmeram_la_LIBADD = $(UIOMUX_LIBS)
