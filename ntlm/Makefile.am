## Process this file with automake to produce Makefile.in

libdir = @plugindir@

INCLUDES = -I@srcdir@
AM_CFLAGS = @CFLAGS@ @EXTRA_CFLAGS@

lib_LTLIBRARIES = sasl-ntlm.la

sasl_ntlm_la_SOURCES = client-ntlm.c ntlmdes.c ntlmstruct.c ntlm.h
sasl_ntlm_la_LDFLAGS = -module -avoid-version @NTLM_LIBS@
