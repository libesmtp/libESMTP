## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = gnu dist-bzip2

INCLUDES = -I$(srcdir) $(VERSION_FLAGS) $(INCLTDL)
SUBDIRS = @subdirs@ @SASL_PLUGINS@
CFLAGS = @CFLAGS@ @EXTRA_CFLAGS@

lib_LTLIBRARIES = libesmtp.la

libesmtp_la_SOURCES = auth-client.c base64.c concatenate.c errors.c \
		      headers.c htable.c message-source.c protocol.c \
		      rfc822date.c siobuf.c smtp-api.c smtp-auth.c \
		      smtp-tls.c smtp-etrn.c tokens.c \
		      api.h base64.h message-callbacks.c \
		      concatenate.h config.h headers.h htable.h \
		      libesmtp-private.h message-source.h protocol-states.h \
		      protocol.h rfc822date.h siobuf.h tokens.h \
		      gethostbyname.h
libesmtp_la_LIBADD = @LIBOBJS@

libesmtp_la_LDFLAGS = $(LIBLTDL) -export-dynamic \
		      -version-info $(LIBESMTP_VERSION)

include_HEADERS = libesmtp.h auth-client.h auth-plugin.h 

bin_SCRIPTS = libesmtp-config

EXTRA_DIST = libesmtp.spec.in libesmtp.spec COPYING.GPL Notes \
	     examples/Makefile examples/mail-file.c examples/test-mail
