dnl Process this file with autoconf to produce a configure script.

dnl #########################################################################
dnl 
dnl This file is part of libESMTP, a library for submission of RFC 2822
dnl formatted electronic mail messages using the SMTP protocol described
dnl in RFC 2821.
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

AC_INIT(smtp-api.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([libesmtp],[1.0.6])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

dnl #########################################################################
dnl Library versioning
dnl #########################################################################

LIB_CURRENT=7
LIB_REVISION=6
LIB_AGE=1
AC_SUBST(LIB_CURRENT)
AC_SUBST(LIB_REVISION)
AC_SUBST(LIB_AGE)

LIBESMTP_VERSION="$LIB_CURRENT:$LIB_REVISION:$LIB_AGE"
AC_SUBST(LIBESMTP_VERSION)
subdirs=

dnl #########################################################################
dnl Checks for programs.
dnl #########################################################################
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_LIBTOOL

dnl #########################################################################
dnl Miscellaneous stuff
dnl #########################################################################

EXTRA_CFLAGS=""
case $host_vendor-$host_os in
sun*)
	AC_DEFINE(__EXTENSIONS__,1,[Sun's netdb.h needs this for getaddrinfo])
	;;
osf*)
	AC_DEFINE(_OSF_SOURCE,1,[OSF needs this for getaddrinfo])
	;;
*gnu*)
	AC_DEFINE(_GNU_SOURCE,1,[GNU needs this for strcasecmp etc])
	;;
esac

dnl #########################################################################
dnl turn warnings into errors to enforce clean code
dnl #########################################################################

AC_ARG_ENABLE([more-warnings],
              ACX_HELP_STRING([--enable-more-warnings=no/yes/picky],[additional compiler warnings (default=yes)]),
	      ,
	      enable_more_warnings=yes)

if test "$GCC" = "yes" -a "$enable_more_warnings" != "no"; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS \
        -Wall -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wstrict-prototypes -Wnested-externs -Wpointer-arith \
        -Wbad-function-cast -Wcast-align"
fi
if test "$GCC" = "yes" -a "$enable_more_warnings" = "picky"; then
        EXTRA_CFLAGS="$EXTRA_CFLAGS -fno-builtin -W -Werror -Wwrite-strings \
        		-Wcast-qual"
fi

AC_ARG_ENABLE([isoc],
              ACX_HELP_STRING([--disable-isoc],[check ISO C compliance (default=c99)]),
	      ,
	      enable_isoc=c99)
if test "$enable_isoc" != "no"; then
	if test "$GCC" = "yes"; then
		CFLAGS="-std=$enable_isoc -pedantic $CFLAGS"
	fi
	AC_DEFINE(_ISOC9X_SOURCE,1,[ISO compliant code])
	AC_DEFINE(_ISOC99_SOURCE,1,[ISO compliant code])
	AC_DEFINE(_POSIX_C_SOURCE,199506L,[POSIX compliant code])
	AC_DEFINE(_XOPEN_SOURCE,500,[Single Unix conformance])
fi

dnl #########################################################################
dnl Checks for libraries and options.
dnl #########################################################################

AC_ARG_ENABLE([all],ACX_HELP_STRING([--enable-all], [convenience: enable all experimental features]),
              ,
              enable_all=no)

dnl #########################################################################
dnl Check whether to use SUS functions or libltdl for loading plugins.
dnl #########################################################################

have_dl=no
AC_SEARCH_LIBS(dlsym, dl svdl, [
	AC_DEFINE([HAVE_DLSYM],1,[the dlsym() function is available])
	have_dl=yes
])
if test x$have_dl = xno ; then
dnl # WARNING: I don't know what libraries ltdl depends on.  I just assume
dnl #          that if the punter asks for it, then it must be properly
dnl #          installed and the dynamic linker will sort the dependencies
dnl #	       at link time.

	AC_SEARCH_LIBS(lt_dlsym, ltdl,
		have_dl=yes,
		AC_MSG_ERROR([libltdl not found (available from http://www.gnu.org/software/libtool/)])
	)
fi

dnl #########################################################################
dnl Check if using Posix Threads
dnl #########################################################################

AC_ARG_ENABLE([pthreads],
              ACX_HELP_STRING([--enable-pthreads], [build with support for Posix threads  (default=auto)]),
  	      ,
  	      enable_pthreads=auto)
 
if test x"$enable_pthreads" != xno ; then
	ACX_PTHREAD(enable_pthreads=yes,
		       if test x"$enable_pthreads" = xyes ; then
		       		AC_MSG_ERROR([Cannot find the pthread library.])
		       else
		       		enable_pthreads=no
		       fi
	)
fi

AC_MSG_CHECKING(whether to use Posix Threads)
if test x"$enable_pthreads" != xno ; then
	CFLAGS="$PTHREAD_CFLAGS $CFLAGS"
	LDFLAGS="$PTHREAD_LDFLAGS $LDFLAGS"
	LIBS="$PTHREAD_LIBS $LIBS"
	CC="$PTHREAD_CC"
	AC_DEFINE([USE_PTHREADS],1,[Build with support for Posix threading])
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Check if Posix getaddrinfo() is available.  It is also possible to use
dnl the version from the lwres library distributed with BIND.
dnl #########################################################################
AC_ARG_ENABLE([emulate-getaddrinfo],
              ACX_HELP_STRING([--enable-emulate-getaddrinfo],
                             [enable getaddrinfo emulation (default=no)]),
  	      ,
  	      enable_emulate_getaddrinfo=no)
AC_ARG_WITH(lwres,
	    ACX_HELP_STRING([--with-lwres=DIR],
			   [use lwres library for getaddrinfo (default=no)]),
	    ,
	    with_lwres=no)

dnl ## enable force to test getaddrinfo.c
if test x$enable_emulate_getaddrinfo = xforce ; then
	enable_emulate_getaddrinfo=yes
	have_getaddrinfo=no
else

have_getaddrinfo=no
if test x$with_lwres != xno ; then
	if test "$with_lwres" != yes ; then
                   CPPFLAGS="-I${with_lwres}/include $CPPFLAGS"
                   LDFLAGS="-L${with_lwres}/lib $LDFLAGS"
	fi
        AC_CHECK_HEADERS(lwres/netdb.h, ,
			[AC_MSG_ERROR([cannot find <lwres/netdb.h>])])
	AC_CHECK_LIB(lwres, lwres_getaddrinfo, ,
	             [AC_MSG_ERROR([cannot find the lwres library])],
	             -lnsl -lpthread)
	have_getaddrinfo=yes
fi

if test x$have_getaddrinfo != xyes ; then
	AC_SEARCH_LIBS(getaddrinfo, socket resolv bind nsl c_r cr, have_getaddrinfo=yes)
fi

dnl # Special nonsense for systems that actually have getaddrinfo but
dnl # redefine the name to something else, e.g. OSF
if test x$have_getaddrinfo != xyes ; then
	AC_MSG_CHECKING(if getaddrinfo is redefined in netdb.h)
	AC_TRY_LINK([
#		include <netdb.h>
	], [
  		struct addrinfo hints, *res;
		int err;

  		err = getaddrinfo ("host", "service", &hints, &res);
	], [
		have_getaddrinfo=yes
		AC_MSG_RESULT(yes)
	], [AC_MSG_RESULT(no)])
fi

fi

if test x$have_getaddrinfo != xno ; then
	if test x$enable_emulate_getaddrinfo != xno ; then
		AC_MSG_ERROR([getaddrinfo found but emulate-getaddrinfo was enabled])
	fi
	AC_DEFINE(HAVE_GETADDRINFO, 1,
		  [Does system provide RFC 2553/Posix getaddrinfo?])
else
	if test x$enable_emulate_getaddrinfo != xyes ; then
		AC_MSG_ERROR([getaddrinfo not found: try --with-lwres or --enable-emulate-getaddrinfo])
	fi
	AC_LIBOBJ(getaddrinfo)
fi

if test x"$enable_emulate_getaddrinfo" != xno ; then
    have_resolver=no

  dnl  Try for getipnodebyname
    AC_SEARCH_LIBS(getipnodebyname, resolv bind nsl c_r cr, have_resolver=yes)
    if test x"$have_resolver" != xno ; then
	 AC_DEFINE(HAVE_GETIPNODEBYNAME, 1,
		   [Set when getipnodebyname is available])
    fi

  dnl  Try for gethostbyname_r
    if test x"$have_resolver" = xno ; then
	AC_SEARCH_LIBS(gethostbyname_r, resolv bind nsl c_r cr,
		       [have_resolver=yes
			ACX_WHICH_GETHOSTBYNAME_R])
    fi

  dnl  Try for gethostbyname
    if test x"$have_resolver" = xno ; then
	if test x"$enable_pthreads" != xno ; then
	    AC_MSG_WARN([using threads but cannot find gethostbyname_r or getipnodebyname])
	fi
	AC_SEARCH_LIBS(gethostbyname, resolv bind nsl, ,
		       [AC_MSG_ERROR([cannot find gethostbyname])])
    fi
    AC_LIBOBJ(gethostbyname)

    AC_CACHE_CHECK([for IPv6 support], acx_cv_sys_use_ipv6, [
	    AC_TRY_COMPILE([
#		include <netinet/in.h>
	    ], [
		struct sockaddr_in6 sin6;
		void *p;

		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = 587;
		p = &sin6.sin6_addr;
	    ], [acx_cv_sys_use_ipv6=yes], [acx_cv_sys_use_ipv6=no])
    ])
    if test x"$acx_cv_sys_use_ipv6" != xno ; then
	    AC_DEFINE(USE_IPV6,1,[Enable IPv6 support])
    fi
fi

dnl #########################################################################
dnl Check if using OpenSSL
dnl #########################################################################
AC_ARG_WITH(openssl,
	    ACX_HELP_STRING([--with-openssl=DIR],
			   [build features depending on OpenSSL (default=auto)]),
	    ,
	    with_openssl=auto)
if test x$with_openssl != xno ; then
	if test "$with_openssl" != yes -a "$with_openssl" != auto ; then
                   CPPFLAGS="-I${with_openssl}/include $CPPFLAGS"
                   LDFLAGS="-L${with_openssl}/lib $LDFLAGS"
                   with_openssl=yes
	fi
	dnl *** with_openssl is either "yes" or "auto"
	AC_CHECK_HEADER(openssl/ssl.h,,
		        if test x"$with_openssl" = xyes ; then
	                	AC_MSG_ERROR([cannot find the ssl headers])
	                else
		       		with_openssl=no
	                fi
	)
fi
if test x$with_openssl != xno ; then
	AC_CHECK_LIB(ssl, SSL_library_init, [
				with_openssl=yes
				LIBS="-lssl -lcrypto $LIBS"
		     ], [
		     if test x"$with_openssl" = xyes ; then
				AC_MSG_ERROR([cannot find the ssl library])
		     else
				with_openssl=no
		     fi],
		     -lcrypto
	)
fi

dnl #########################################################################
dnl Set OpenSSL dependencies for RPM - this sucks, there must be a better way.
dnl #########################################################################

RPM_REQUIRES=""
RPM_BUILDREQUIRES=""
RPM_OPENSSL=""
RPM_OPENSSLDEVEL=""
if test x$with_openssl != xno ; then
	RPM_REQUIRES="Requires:"
	RPM_BUILDREQUIRES="BuildRequires:"
	RPM_OPENSSL="openssl >= %{openssl}"
	RPM_OPENSSLDEVEL="openssl-devel >= %{openssl}"
fi
AC_SUBST(RPM_REQUIRES)
AC_SUBST(RPM_BUILDREQUIRES)
AC_SUBST(RPM_OPENSSL)
AC_SUBST(RPM_OPENSSLDEVEL)

dnl #########################################################################
dnl Check if using SMTP AUTH using SASL
dnl #########################################################################

dnl Force use of SMTP AUTH for now.  Eventually it is hoped that
dnl the SASL API will move into a library of its own.

AC_DEFINE(USE_SASL,1,[Build with support for SMTP AUTH using SASL])
SASL_PLUGINS="login plain crammd5"
DIST_PLUGINS="login plain crammd5"

dnl Set up the authentication plugin directory

AC_ARG_WITH(auth-plugin-dir,
	    ACX_HELP_STRING([--with-auth-plugin-dir=DIR],
			   [directory for SASL plugins (default=LIBDIR/esmtp-plugins)]),
	    plugindir=$withval)
if test x$plugindir = x ; then
	plugindir=$libdir/esmtp-plugins
fi
AC_SUBST(plugindir)
ACX_DEFINE_DIR([AUTHPLUGINDIR], $plugindir, [location of authentication plugins.])

have_libcrypto=no

dnl Check for md5 functions in OpenSSL.  If these are present, use them
dnl in preference to the supplied md5.[ch] when building the CRAM-MD5
dnl SASL plugin.  The assumption is that the OpenSSL functions are optimised
dnl and actively maintained.

use_local_md5_c=yes
CRAMMD5_LIBOBJS=""
if test x$with_openssl != xno ; then
	AC_CHECK_HEADER(openssl/md5.h, [
		AC_CHECK_LIB(crypto, MD5_Init, [
			     have_libcrypto=yes
			     use_local_md5_c=no
			     CRAMMD5_LIBS="-lcrypto"
			     AC_SUBST(CRAMMD5_LIBS)
		])
	])
fi
if test x$use_local_md5_c != xno ; then
	CRAMMD5_LIBOBJS="$CRAMMD5_LIBOBJS md5.o"
fi
CRAMMD5_LTLIBOBJS=`echo "$CRAMMD5_LIBOBJS" | sed ['s/\.[^.]* /.lo /g;s/\.[^.]*$/.lo/']`
AC_SUBST(CRAMMD5_LTLIBOBJS)

dnl Check if enabling experimental NTLM authentication.  This requires that
dnl OpenSSL is available and that it is explicitly requested.

AC_ARG_ENABLE([ntlm],
              ACX_HELP_STRING([--enable-ntlm],[experimental support for NTLM authentication  (default=no)]),
  	      ,
  	      enable_ntlm=$enable_all)
DIST_PLUGINS="$DIST_PLUGINS ntlm"
 
AC_MSG_CHECKING(whether to enable NTLM authentication)
if test x$with_openssl != xno -a x"$enable_ntlm" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_CHECK_HEADER(openssl/md4.h, [
		AC_CHECK_LIB(crypto, MD4_Init, [
			     have_libcrypto=yes
			     SASL_PLUGINS="$SASL_PLUGINS ntlm"
			     NTLM_LIBS="-lcrypto"
			     AC_SUBST(NTLM_LIBS)
		])
	])
else
	AC_MSG_RESULT([no])
fi

if test x$have_libcrypto != xno ; then
   AC_DEFINE(HAVE_LIBCRYPTO,1,[Set when -lcrypto from OpenSSL is available])
fi

dnl #########################################################################
dnl Check if enabling STARTTLS
dnl #########################################################################

AC_MSG_CHECKING(whether to enable STARTTLS)
if test x"$with_openssl" != xno ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(USE_TLS,1,[Build with support for SMTP STARTTLS extension])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable CHUNKING
dnl #########################################################################

AC_ARG_ENABLE([chunking],
              ACX_HELP_STRING([--disable-chunking],[support for CHUNKING  (default=yes)]),
  	      ,
  	      enable_chunking=yes)
 
AC_MSG_CHECKING(whether to enable CHUNKING)
if test x"$enable_chunking" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(USE_CHUNKING,1,[Enable experimental support for SMTP CHUNKING extension])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable ETRN (experimental)
dnl #########################################################################

AC_ARG_ENABLE([etrn],
              ACX_HELP_STRING([--enable-etrn],[experimental support for ETRN  (default=no)]),
  	      ,
  	      enable_etrn=$enable_all)
 
AC_MSG_CHECKING(whether to enable ETRN)
if test x"$enable_etrn" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(USE_ETRN,1,[Enable experimental support for SMTP ETRN extension])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable XUSR
dnl #########################################################################

AC_ARG_ENABLE([xusr],
              ACX_HELP_STRING([--disable-xusr],
                             [support for sendmail XUSR extension (default=yes)]),
  	      ,
  	      enable_xusr=yes)
 
AC_MSG_CHECKING(whether to enable sendmail XUSR)
if test x"$enable_xusr" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(USE_XUSR,1,[Enable support for sendmail XUSR extension])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable non-standard AUTH= syntax in EHLO response
dnl #########################################################################

AC_ARG_ENABLE([nsauth],
              ACX_HELP_STRING([--disable-nsauth],
                              [support non-standard EHLO AUTH= response (default=yes)]),
  	      ,
  	      enable_nsauth=yes)
 
AC_MSG_CHECKING([whether to enable non-standard EHLO AUTH= response])
if test x"$enable_nsauth" = xyes ; then
	AC_MSG_RESULT([yes])
	AC_DEFINE(AUTH_ID_HACK,1,[support non-standard EHLO AUTH= response])
else
	AC_MSG_RESULT([no])
fi

dnl #########################################################################
dnl Enable debugging code, e.g. assert()
dnl #########################################################################

AC_ARG_ENABLE([debug],
              ACX_HELP_STRING([--enable-debug],
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
AC_C_INLINE
AC_C_BIGENDIAN
AC_STRUCT_TM
AC_HEADER_TIME
AC_STRUCT_TIMEZONE

dnl #########################################################################
dnl Check for types
dnl #########################################################################
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(unsigned long)

dnl #########################################################################
dnl Checks for library functions.
dnl #########################################################################

AC_REPLACE_FUNCS(strdup strcasecmp strncasecmp memrchr)
AC_CHECK_FUNCS(strtol uname gethostname gettimeofday)
AC_SEARCH_LIBS(socket, socket)

dnl Conditional check for functions needed in threaded code

if test x"$enable_pthreads" != xno ; then
	AC_SEARCH_LIBS(localtime_r, c_r cr,
		       AC_DEFINE(HAVE_LOCALTIME_R,1,[Set when localtime_r is available]),
		       AC_MSG_ERROR([Cannot find localtime_r.]))
	AC_CHECK_FUNCS(strerror_r,[
	    AC_CACHE_CHECK([for working strerror_r],
	                   acx_cv_sys_working_strerror_r, [
		AC_TRY_COMPILE([
#			define _SVID_SOURCE	1
#			include <string.h>
		], [
		    	char *s;

		    	s = strerror_r (0, (char *) 0, 0);
		],
		[acx_cv_sys_working_strerror_r=yes],
		[acx_cv_working_strerror_r=no])
	    ])
	    if test x$acx_cv_sys_working_strerror_r = xyes ; then
	    	AC_DEFINE(HAVE_WORKING_STRERROR_R,1,[strerror_r works!])
	    fi
	])
else
	AC_CHECK_FUNCS(localtime strerror)
fi

dnl #########################################################################
dnl Check that snprintf works correctly.
dnl #########################################################################
ACX_SNPRINTF(,[AC_LIBOBJ(snprintf)])

dnl #########################################################################
dnl Make substitutions
dnl #########################################################################

AC_SUBST(SASL_PLUGINS)
AC_SUBST(DIST_PLUGINS)
AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(LIBS)
AC_SUBST(CFLAGS)
AC_SUBST(EXTRA_CFLAGS)
AC_SUBST(CC)
AC_SUBST(RANLIB)
AC_SUBST(subdirs)

dnl ## LTLIBOBJS=`echo "$LIBOBJS" | sed ['s/\.[^.]* /.lo /g;s/\.[^.]*$/.lo/']`
AC_SUBST(LTLIBOBJS)
LTALLOCA=`echo "$ALLOCA" | sed ['s/\.[^.]* /.lo /g;s/\.[^.]*$/.lo/']`
AC_SUBST(LTALLOCA)

AC_OUTPUT([libesmtp-config libesmtp.spec
	   Makefile login/Makefile plain/Makefile crammd5/Makefile ntlm/Makefile])

dnl #########################################################################
dnl Feature synopsis
dnl #########################################################################

echo
echo '*************************'
echo '*** libESMTP features ***'
echo '*************************'
d=`eval echo $plugindir`
ACX_FEATURE([with],[auth-plugin-dir],[`eval echo $d`])
ACX_FEATURE([with],[lwres])
ACX_FEATURE([with],[openssl])
ACX_FEATURE([enable],[pthreads])
ACX_FEATURE([enable],[etrn])
ACX_FEATURE([enable],[ntlm])
ACX_FEATURE([enable],[chunking])
ACX_FEATURE([enable],[xusr])
ACX_FEATURE([enable],[nsauth])
ACX_FEATURE([enable],[debug])

