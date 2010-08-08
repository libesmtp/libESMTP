dnl @synopsis ACX_PTHREAD([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl
dnl This macro figures out how to build C programs using POSIX
dnl threads.  It sets the PTHREAD_LIBS output variable to the threads
dnl library and linker flags, and the PTHREAD_CFLAGS output variable
dnl to any special C compiler flags that are needed.  (The user can also
dnl force certain compiler flags/libs to be tested by setting these
dnl environment variables.)
dnl
dnl Also sets PTHREAD_CC to any special C compiler that is needed for
dnl multi-threaded programs (defaults to the value of CC otherwise).
dnl (This is necessary on AIX to use the special cc_r compiler alias.)
dnl
dnl If you are only building threads programs, you may wish to
dnl use these variables in your default LIBS, CFLAGS, and CC:
dnl
dnl        LIBS="$PTHREAD_LIBS $LIBS"
dnl        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
dnl        LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"
dnl        CC="$PTHREAD_CC"
dnl
dnl In addition, if the PTHREAD_CREATE_JOINABLE thread-attribute
dnl constant has a nonstandard name, defines PTHREAD_CREATE_JOINABLE
dnl to that name (e.g. PTHREAD_CREATE_UNDETACHED on AIX).
dnl
dnl ACTION-IF-FOUND is a list of shell commands to run if a threads
dnl library is found, and ACTION-IF-NOT-FOUND is a list of commands
dnl to run it if it is not found.  If ACTION-IF-FOUND is not specified,
dnl the default action will define HAVE_PTHREAD.
dnl
dnl Please let the authors know if this macro fails on any platform,
dnl or if you have any other suggestions or comments.  This macro was
dnl based on work by SGJ on autoconf scripts for FFTW (www.fftw.org)
dnl (with help from M. Frigo), as well as ac_pthread and hb_pthread
dnl macros posted by AFC to the autoconf macro repository.  We are also
dnl grateful for the helpful feedback of numerous users.
dnl
dnl @version $Id: acinclude.m4,v 1.6 2001/10/17 07:19:14 brian Exp $
dnl @author Steven G. Johnson <stevenj@alum.mit.edu> and Alejandro Forero Cuervo <bachue@bachue.com>

AC_DEFUN([ACX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
acx_pthread_ok=no

# First, check if the POSIX threads header, pthread.h, is available.
# If it isn't, don't bother looking for the threads libraries.
AC_CHECK_HEADER(pthread.h, , acx_pthread_ok=noheader)

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS$PTHREAD_LDFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS with LDFLAGS=$PTHREAD_LDFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, acx_pthread_ok=yes)
        AC_MSG_RESULT($acx_pthread_ok)
        if test x"$acx_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
                PTHREAD_LDFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        LDFLAGS="$save_LDFLAGS"
fi

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all.

acx_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
# pthread: Linux, etcetera
# --thread-safe: KAI C++

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthread or
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        acx_pthread_flags="-pthread -pthreads pthread -mt $acx_pthread_flags"
        ;;
esac

if test x"$acx_pthread_ok" = xno; then
for flag in $acx_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                PTHREAD_LDFLAGS="$flag"
                ;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>],
                    [pthread_t th; pthread_join(th, 0);
                     pthread_attr_init(0); pthread_cleanup_push(0, 0);
                     pthread_create(0,0,0,0); pthread_cleanup_pop(0); ],
                    [acx_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        LDFLAGS="$save_LDFLAGS"

        AC_MSG_RESULT($acx_pthread_ok)
        if test "x$acx_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
        PTHREAD_LDFLAGS=""
done
fi

# Various other checks:
if test "x$acx_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LDFLAGS="$LDFLAGS"
        LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"

        # Detect AIX lossage: threads are created detached by default
        # and the JOINABLE attribute has a nonstandard name (UNDETACHED).
        AC_MSG_CHECKING([for joinable pthread attribute])
        AC_TRY_LINK([#include <pthread.h>],
                    [int attr=PTHREAD_CREATE_JOINABLE;],
                    ok=PTHREAD_CREATE_JOINABLE, ok=unknown)
        if test x"$ok" = xunknown; then
                AC_TRY_LINK([#include <pthread.h>],
                            [int attr=PTHREAD_CREATE_UNDETACHED;],
                            ok=PTHREAD_CREATE_UNDETACHED, ok=unknown)
        fi
        if test x"$ok" != xPTHREAD_CREATE_JOINABLE; then
                AC_DEFINE(PTHREAD_CREATE_JOINABLE, $ok,
                          [Define to the necessary symbol if this constant
                           uses a non-standard name on your system.])
        fi
        AC_MSG_RESULT(${ok})
        if test x"$ok" = xunknown; then
                AC_MSG_WARN([we do not know how to create joinable pthreads])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
                *-aix* | *-freebsd*)     flag="-D_THREAD_SAFE";;
                *solaris* | alpha*-osf*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
                PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
        LDFLAGS="$save_LDFLAGS"

        # More AIX lossage: must compile with cc_r
        AC_CHECK_PROG(PTHREAD_CC, cc_r, cc_r, ${CC})
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LDFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$acx_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        acx_pthread_ok=no
        $2
fi

])dnl ACX_PTHREAD
dnl @synopsis ACX_WHICH_GETHOSTBYNAME_R
dnl
dnl Provides a test to determine the correct way to call gethostbyname_r
dnl
dnl defines HAVE_GETHOSTBYNAME_R to the number of arguments required
dnl
dnl e.g. 6 arguments (linux)
dnl e.g. 5 arguments (solaris)
dnl e.g. 3 arguments (osf/1)
dnl
dnl @version $Id: acinclude.m4,v 1.6 2001/10/17 07:19:14 brian Exp $
dnl @author Brian Stafford <brian@stafford.uklinux.net>
dnl
dnl based on version by Caolan McNamara <caolan@skynet.ie>
dnl based on David Arnold's autoconf suggestion in the threads faq
dnl
AC_DEFUN([ACX_WHICH_GETHOSTBYNAME_R],
[AC_CACHE_CHECK(number of arguments to gethostbyname_r,
                acx_cv_which_gethostbyname_r, [
	AC_TRY_COMPILE([
#		include <netdb.h> 
  	], 	[

        char *name;
        struct hostent *he;
        struct hostent_data data;
        (void) gethostbyname_r(name, he, &data);

		],acx_which_gethostbyname_r=3, 
			[
dnl			acx_which_gethostbyname_r=0
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
	char *name;
	struct hostent *he, *res;
	char *buffer = NULL;
	int buflen = 2048;
	int h_errnop;
	(void) gethostbyname_r(name, he, buffer, buflen, &res, &h_errnop)
  ],acx_which_gethostbyname_r=6,
  
  [
dnl  acx_which_gethostbyname_r=0
  AC_TRY_COMPILE([
#   include <netdb.h>
  ], [
			char *name;
			struct hostent *he;
			char *buffer = NULL;
			int buflen = 2048;
			int h_errnop;
			(void) gethostbyname_r(name, he, buffer, buflen, &h_errnop)
  ],acx_which_gethostbyname_r=5,acx_which_gethostbyname_r=0)

  ]
  
  )
			]
		)
	])

if test $acx_which_gethostbyname_r -gt 0 ; then
    AC_DEFINE_UNQUOTED([HAVE_GETHOSTBYNAME_R], $acx_which_gethostbyname_r,
    		       [Number of parameters to gethostbyname_r or 0 if not available])
fi

])
dnl @synopsis ACX_DEFINE_DIR(VARNAME, DIR [, DESCRIPTION])
dnl
dnl This macro _AC_DEFINEs VARNAME to the expansion of the DIR
dnl variable, taking care of fixing up ${prefix} and such.
dnl
dnl Note that the 3 argument form is only supported with autoconf 2.13 and
dnl later (i.e. only where _AC_DEFINE supports 3 arguments).
dnl
dnl Examples:
dnl
dnl    ACX_DEFINE_DIR(DATADIR, datadir)
dnl    ACX_DEFINE_DIR(PROG_PATH, bindir, [Location of installed binaries])
dnl
dnl @version $Id: acinclude.m4,v 1.6 2001/10/17 07:19:14 brian Exp $
dnl @author Guido Draheim <guidod@gmx.de>, original by Alexandre Oliva
dnl      Modified by Brian Stafford <brian@stafford.uklinux.net> to
dnl      change prefix AC_ to ACX_

AC_DEFUN([ACX_DEFINE_DIR], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  acx_define_dir=`eval echo $2`
  acx_define_dir=`eval echo [$]acx_define_dir`
  ifelse($3, ,
    AC_DEFINE_UNQUOTED($1, "$acx_define_dir"),
    AC_DEFINE_UNQUOTED($1, "$acx_define_dir", $3))
])

dnl @synopsis ACX_HELP_STRING(OPTION,DESCRIPTION)
AC_DEFUN([ACX_HELP_STRING],
	 [  $1 builtin([substr],[                       ],len($1))[$2]])

dnl @synopsis ACX_FEATURE(ENABLE_OR_WITH,NAME[,VALUE])
AC_DEFUN([ACX_FEATURE],
	 [echo "builtin([substr],[                                  ],len(--$1-$2))--$1-$2: ifelse($3,,[$]translit($1-$2,-,_),$3)"])

dnl @synopsis ACX_SNPRINTF(ACTION-IF-CORRECT,ACTION-IF-BROKEN)
dnl
dnl Provides a test to determine if snprintf correctly truncates strings
dnl too long for the buffer.  If snprintf works correctly execute 
dnl ACTION-IF-CORRECT else execute ACTION-IF-BROKEN.  If ACTION-IF-CORRECT
dnl is not supplied, the default is to define HAVE_WORKING_SNPRINTF
dnl
dnl @version $Id$
dnl @author Brian Stafford <brian@stafford.uklinux.net>
dnl
AC_DEFUN([ACX_SNPRINTF], [
    AC_CACHE_CHECK([for working snprintf], [acx_cv_working_snprintf], [
	AC_TRY_RUN([
#include <stdio.h> 

main ()
{
  char buf[16];

  snprintf (buf, 4, "abcd");
  exit ((buf[3] == '\0') ? 0 : 1);
}

], acx_cv_working_snprintf=yes, acx_cv_working_snprintf=no, acx_cv_working_snprintf=yes)
    ])
    if test x$acx_cv_working_snprintf = xyes ; then
        ifelse([$1],,AC_DEFINE([HAVE_WORKING_SNPRINTF], 1,
		  [snprintf correctly terminates the buffer on overflow]),[$1])
        ifelse([$2],,,[else $2])
    fi
])
	
