## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = gnu dist-bzip2

INCLUDES = -I$(srcdir) $(VERSION_FLAGS)
SUBDIRS = @subdirs@ @SASL_PLUGINS@
DIST_SUBDIRS = @subdirs@ @DIST_PLUGINS@
AM_CFLAGS = @CFLAGS@ @EXTRA_CFLAGS@
ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libesmtp.la

libesmtp_la_SOURCES = auth-client.c base64.c concatenate.c errors.c \
		      headers.c htable.c message-source.c protocol.c \
		      rfc2822date.c siobuf.c smtp-api.c smtp-auth.c \
		      smtp-bdat.c smtp-tls.c smtp-etrn.c tokens.c \
		      api.h base64.h message-callbacks.c \
		      concatenate.h headers.h htable.h \
		      libesmtp-private.h message-source.h protocol-states.h \
		      protocol.h rfc2822date.h siobuf.h tokens.h \
		      getaddrinfo.h gethostbyname.h missing.h
libesmtp_la_LIBADD = @LTLIBOBJS@

libesmtp_la_LDFLAGS = -export-dynamic \
		      -version-info $(LIBESMTP_VERSION)

include_HEADERS = libesmtp.h auth-client.h auth-plugin.h 

bin_SCRIPTS = libesmtp-config

EXTRA_DIST = libesmtp.spec libesmtp.spec.in Notes doc/api.xml \
	     examples/Makefile examples/mail-file.c examples/test-mail
