## Process this file with automake to produce Makefile.in

libdir = @plugindir@

INCLUDES = -I@srcdir@

lib_LTLIBRARIES = sasl-cram-md5.la

sasl_cram_md5_la_SOURCES = client-crammd5.c hmacmd5.c hmacmd5.h
sasl_cram_md5_la_LDFLAGS = -module -avoid-version @CRAMMD5_LIBS@
