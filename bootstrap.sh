#!/bin/sh
case `uname` in
	Darwin*)
		LIBTOOLIZE=glibtoolize ;;
	*)
		LIBTOOLIZE=libtoolize ;;
esac
$LIBTOOLIZE --copy --install && \
	aclocal -I m4 && \
	autoheader && \
	automake --add-missing --copy && \
	autoconf
