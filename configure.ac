dnl Process this file with autoconf to produce a configure script.

dnl #########################################################################
dnl 
dnl This file is part of libESMTP, a library for submission of RFC 822
dnl formatted electronic mail messages using the SMTP protocol described
dnl in RFC 821.
dnl 
dnl Copyright (C) 2001  Brian Stafford  <brian@stafford.uklinux.net>
dnl 
dnl This library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Lesser General Public
dnl License as published by the Free Software Foundation; either
dnl version 2.1 of the License, or (at your option) any later version.
dnl 
dnl This library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Lesser General Public License for more details.
dnl 
dnl You should have received a copy of the GNU Lesser General Public
dnl License along with this library; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl 
dnl #########################################################################

AC_INIT([libesmtp],[0.8.2],[brian@stafford.uklinux.net])
AC_PREREQ(2.50)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR(smtp-api.c)
AC_CONFIG_FILES(libesmtp-config libesmtp.spec
                Makefile login/Makefile plain/Makefile crammd5/Makefile)

EXTRA_CFLAGS=""

dnl #########################################################################
dnl Extra stuff in config.h
dnl #########################################################################

AH_TOP([
/* Turn off the GCC specific __attribute__ keyword */
#if !defined (__GNUC__) || __GNUC__ < 2
# define __attribute__(x)
#endif
])

AC_DEFINE(_ISOC9X_SOURCE,1,[Code is supposed to be ISO compliant])
AC_DEFINE(_POSIX_C_SOURCE,199506L,[Code is supposed to be POSIX compliant])
AC_DEFINE(_XOPEN_SOURCE,500,[Single Unix conformance])

dnl #########################################################################
dnl Library versioning
dnl #########################################################################

LIB_CURRENT=5
LIB_REVISION=3
LIB_AGE=0
AC_SUBST(LIB_CURRENT)
AC_SUBST(LIB_REVISION)
AC_SUBST(LIB_AGE)

LIBESMTP_VERSION="$LIB_CURRENT:$LIB_REVISION:$LIB_AGE"
AC_SUBST(LIBESMTP_VERSION)

dnl #########################################################################
dnl Checks for programs.
dnl #########################################################################
AC_PROG_CC
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl #########################################################################
dnl Checks for libraries and options.
dnl #########################################################################

AC_ARG_ENABLE([all],AC_HELP_STRING([--enable-all], [convenience: enable features which default to off]),
              ,
              enable_all=no)

AC_ARG_ENABLE([ltdl-install],AC_HELP_STRING([--enable-ltdl-install],[install libltdl]))
AC_LIBLTDL_INSTALLABLE
dnl When the installable (or installed) libltdl is used, programs linked
dnl with -lesmtp also need -lltdl
LIBADD_LTDL=-lltdl

AC_LTDL_DLLIB
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_CONFIG_SUBDIRS(libltdl)

dnl #########################################################################
dnl Check if using Posix Threads
dnl #########################################################################

AC_ARG_ENABLE([pthreads],
              AC_HELP_STRING([--enable-pthreads], [build with support for Posix threads  (default=auto)]),
  	      ,
  	      enable_pthreads=auto)
 
dnl if test x"$enable_pthreads" != xno ; then
dnl 	ACX_PTHREAD(LIBS="$PTHREAD_LIBS $LIBS"
dnl 		    CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
dnl 		    CC="$PTHREAD_CC"
dnl 		    AC_DEFINE(USE_PTHREADS,1,[Build with support for Posix threading]),
dnl 		    if test x"$enable_pthreads" = xyes ; then
dnl 				AC_MSG_ERROR([Posix Threads build requested but not available.])
dnl 		    fi
dnl 	)
dnl fi

if test x"$enable_pthreads" != xno ; then
	AC_SEARCH_LIBS(pthread_create, pthread pthreads c_r cr,
		       enable_pthreads=yes,
		       if test x"$enable_pthreads" = xyes ; then
		       		AC_MSG_ERROR([Cannot find the pthread library.])
		       else
		       		enable_pthreads=no
		       fi
	)
fi
AC_MSG_CHECKING(whether to use Posix Threads)
if test x"$enable_pthreads" != xno ; then
	AC_DEFINE(USE_PTHREADS,1,[Build with support for Posix threading])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Check if using Posix getaddrinfo()
dnl #########################################################################

AC_ARG_ENABLE([gethostbyname],
              AC_HELP_STRING([--enable-gethostbyname],
                             [enable use of gethostbyname (deprecated, default=no)]),
  	      ,
  	      enable_gethostbyname=no)
 
dnl Check if getaddrinfo is available.  If not found and gethostbyname is
dnl not enabled, print an error message.

AC_SEARCH_LIBS(getaddrinfo, resolv bind nsl c_r cr,
               AC_DEFINE(HAVE_GETADDRINFO, 1,
	                  [Build with support for RFC 2553/Posix getaddrinfo]),
	       if test x$enable_gethostbyname != xyes ; then
		    AC_MSG_ERROR([getaddrinfo not found: try using --enable-gethostbyname])
	       fi
)

dnl #########################################################################
dnl Check if using SMTP AUTH using SASL
dnl #########################################################################

dnl Force use of SMTP AUTH for now.  Eventually it is hoped that
dnl the SASL API will move into a library of its own.  The USE_SASL
dnl option will then be needed to select whether SMTP AUTH is to be
dnl built since libESMTP will depend on the SASL library. */

AC_DEFINE(USE_SASL,1,[Build with support for SMTP AUTH using SASL])
SASL_PLUGINS="login plain"

dnl Set up the authentication plugin dircetory

AC_ARG_WITH(auth-plugin-dir,
	    AC_HELP_STRING([--with-auth-plugin-dir[=DIR]],
			   [directory for SASL plugins (default=LIBDIR/esmtp)]),
	    plugindir=$withval)
if test x$plugindir = x ; then
	plugindir=$libdir/esmtp
fi
AC_SUBST(plugindir)
ACX_DEFINE_DIR([AUTHPLUGINDIR], $plugindir, [location of authentication plugins.])

dnl Check for md5 functions in OpenSSL.  If these are present, build the
dnl CRAM-MD5 SASL plugin.

AC_CHECK_LIB(crypto, MD5_Init, [
		AC_DEFINE(HAVE_LIBCRYPTO,1,[Set when -lcrypto from OpenSSL is available])
		SASL_PLUGINS="$SASL_PLUGINS crammd5"
		CRAMMD5_LIBS="-lcrypto"
		AC_SUBST(CRAMMD5_LIBS)
])

dnl #########################################################################
dnl Check if enabling STARTTLS
dnl #########################################################################

AC_ARG_ENABLE([starttls],
              AC_HELP_STRING([--enable-starttls],[experimental support for STARTTLS  (default=no)]),
  	      ,
  	      enable_starttls=$enable_all)
 
AC_MSG_CHECKING(whether to enable STARTTLS)
if test x"$enable_starttls" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_CHECK_LIB(ssl, SSL_library_init,
		     [
			AC_DEFINE(USE_TLS,1,[Build with support for SMTP STARTTLS extension])
			LIBS="-lssl -lcrypto $LIBS"
		     ],
		     AC_MSG_ERROR([Cannot find the ssl library.]), -lcrypto)
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable ETRN (experimental)
dnl #########################################################################

AC_ARG_ENABLE([etrn],
              AC_HELP_STRING([--enable-etrn],[experimental support for ETRN  (default=no)]),
  	      ,
  	      enable_etrn=$enable_all)
 
AC_MSG_CHECKING(whether to enable ETRN)
if test x"$enable_etrn" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(USE_ETRN,1,[Enable experimental support for SMTP STARTTLS extension])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Provide standard callbacks for reading messages (experimental)
dnl #########################################################################

AC_ARG_ENABLE([message-callbacks],
              AC_HELP_STRING([--enable-message-callbacks],[add standard message callbacks (experimental) (default=no)]),
  	      ,
  	      enable_message_callbacks=$enable_all)
 
if test x"$enable_message_callbacks" != xno ; then
  AC_DEFINE(USE_MESSAGE_CALLBACKS,1,
            [Add standard callback functions to the library])
fi

dnl #########################################################################
dnl Require all recipients hack (not reccommended)
dnl #########################################################################

AC_ARG_ENABLE([require-all-recipients],
              AC_HELP_STRING([--enable-require-all-recipients],[add extra API call, needed for Balsa (default=no)]),
  	      ,
  	      enable_require_all_recipients=$enable_all)
 
if test x"$enable_require_all_recipients" != xno ; then
  AC_DEFINE(USE_REQUIRE_ALL_RECIPIENTS,1,
            [Build smtp_option_require_all_recipients() API])
fi

dnl #########################################################################
dnl Enable debugging code, e.g. assert()
dnl #########################################################################

AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug],
                             [enable use of debugging code (default=no)]),
  	      ,
  	      enable_debug=no)
 
if test x"$enable_debug" != xno ; then
  AC_DEFINE(DEBUG,1, [Enable additional debugging code])
else
  AC_DEFINE(NDEBUG,1, [Disable assertions])
fi

dnl #########################################################################
dnl Checks for header files.
dnl #########################################################################

AC_HEADER_STDC
AC_CHECK_HEADERS(sys/ioctl.h unistd.h)

dnl #########################################################################
dnl Checks for typedefs, structures, and compiler characteristics.
dnl #########################################################################

AC_C_CONST
AC_STRUCT_TM
AC_HEADER_TIME
AC_STRUCT_TIMEZONE

dnl #########################################################################
dnl Check for types
dnl #########################################################################

dnl #########################################################################
dnl Checks for library functions.
dnl #########################################################################

AC_CHECK_FUNCS(snprintf socket strdup strndup strtol gethostname)

dnl Conditional check for functions needed in threaded code

if test x"$enable_pthreads" != xno ; then
	AC_SEARCH_LIBS(localtime_r, c_r cr,
		       AC_DEFINE(HAVE_LOCALTIME_R,1,[Set when gmtime_r is available]),
		       AC_MSG_ERROR([Cannot find gmtime_r.]))
	AC_SEARCH_LIBS(gmtime_r, c_r cr,
		       AC_DEFINE(HAVE_GMTIME_R,1,[Set when localtime_r is available]),
		       AC_MSG_ERROR([Cannot find localtime_r.]))
	AC_FUNC_STRERROR_R
else
	AC_CHECK_FUNCS(localtime gmtime strerror)
fi

dnl Conditional check for resolver functions needed in threaded code.
dnl Only done if the Posix getaddrinfo() function is not available.
dnl Check if IPv6 code is explicitly needed if using gethostbyname.
dnl This is not necessray if using getaddrinfo since the new interface
dnl isolates the code from protocol specifics.

if test x"$enable_gethostbyname" != xno ; then
	AC_LIBOBJ(gethostbyname)
	if test x"$enable_pthreads" != xno ; then
		AC_SEARCH_LIBS(getipnodebyname, resolv bind nsl c_r cr,
		       AC_DEFINE(HAVE_GETIPNODEBYNAME,1,[Set when getipnodebyname is available]),
		       [AC_SEARCH_LIBS(gethostbyname_r, resolv bind nsl c_r cr,
				       [ACX_WHICH_GETHOSTBYNAME_R],
				       AC_MSG_ERROR([Cannot find gethostbyname_r]))
		       ])
	else
		AC_SEARCH_LIBS(gethostbyname, resolv bind nsl, ,
			       AC_MSG_ERROR([Cannot find gethostbyname]))
	fi

	AC_CACHE_CHECK([for IPv6 support], acx_cv_sys_use_ipv6, [
		AC_TRY_COMPILE([
#                       include <netinet/in.h>
		], [
			struct sockaddr_in6 sin6;
			void *p;

			sin6.sin6_family = AF_INET6;
			sin6.sin6_port = 25;
			p = &sin6.sin6_addr;
		], [acx_cv_sys_use_ipv6=yes], [acx_cv_sys_use_ipv6=no])
	])
	if test x"$acx_cv_sys_use_ipv6" != xno ; then
		AC_DEFINE(USE_IPV6,1,[Enable IPv6 support])
	fi
fi

dnl #########################################################################
dnl turn warnings into errors to enforce clean code
dnl #########################################################################

AC_ARG_ENABLE([more-warnings],
              AC_HELP_STRING([--enable-more-warnings=no/yes/picky],[additional compiler warnings (default=yes)]),
	      ,
	      enable_more_warnings=yes)

if test "$GCC" = "yes" -a "$enable_more_warnings" != "no"; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS \
        -Wall -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wstrict-prototypes -Wnested-externs -Wpointer-arith \
        -Wbad-function-cast -Wcast-align -Wsign-promo"
fi
if test "$GCC" = "yes" -a "$enable_more_warnings" = "picky"; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS -fno-builtin -W -Werror -Wwrite-strings \
        		-Wcast-qual"
fi

AC_ARG_ENABLE([isoc],
              AC_HELP_STRING([--disable-isoc],[check ISO C compliance (default=yes)]),
	      ,
	      enable_isoc=yes)
if test "$GCC" = "yes" -a "$enable_isoc" != "no"; then
        EXTRA_CFLAGS="-ansi -pedantic $EXTRA_CFLAGS"
fi

dnl #########################################################################
dnl Make substitutions
dnl #########################################################################

AC_SUBST(SASL_PLUGINS)

AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(INCLTDL)
AC_SUBST(LIBLTDL)
AC_SUBST(LIBADD_LTDL)
AC_SUBST(LIBS)
AC_SUBST(CFLAGS)
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(CC)
AC_SUBST(RANLIB)
AC_SUBST(subdirs)

AC_OUTPUT

dnl #########################################################################
dnl Feature synopsis
dnl #########################################################################

AC_DEFUN([ACX_FEATURE],
[echo "m4_format([   %-34s:], [--$1-$2])" m4_builtin([ifelse],$3,,[$]m4_translit($1-$2,-,_),$3)])

echo
echo '*************************'
echo '*** libESMTP features ***'
echo '*************************'
d=`eval echo $plugindir`
ACX_FEATURE([with],[auth-plugin-dir],[`eval echo $d`])
ACX_FEATURE([enable],[pthreads])
ACX_FEATURE([enable],[starttls])
ACX_FEATURE([enable],[etrn])
ACX_FEATURE([enable],[message-callbacks])
ACX_FEATURE([enable],[require-all-recipients])
ACX_FEATURE([enable],[debug])

