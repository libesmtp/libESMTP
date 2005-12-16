## Process this file with automake to produce Makefile.in

libdir = @plugindir@

INCLUDES = -I@srcdir@
AM_CFLAGS = @CFLAGS@ @EXTRA_CFLAGS@

lib_LTLIBRARIES = sasl-cram-md5.la

sasl_cram_md5_la_SOURCES = client-crammd5.c hmacmd5.c hmacmd5.h
sasl_cram_md5_la_LDFLAGS = -module -avoid-version @CRAMMD5_LIBS@
sasl_cram_md5_la_DEPENDENCIES = @CRAMMD5_LTLIBOBJS@
sasl_cram_md5_la_LIBADD = @CRAMMD5_LTLIBOBJS@
EXTRA_DIST = md5.h md5.c
