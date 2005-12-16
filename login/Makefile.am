## Process this file with automake to produce Makefile.in

libdir = @plugindir@

INCLUDES = -I@srcdir@

lib_LTLIBRARIES = sasl-login.la
AM_CFLAGS = @CFLAGS@ @EXTRA_CFLAGS@

sasl_login_la_SOURCES = client-login.c
sasl_login_la_LDFLAGS = -module -avoid-version
