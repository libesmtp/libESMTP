# ChangeLog (unmaintained)

This is the original libESMTP Changelog reformatted as markdown.
This file is no longer maintained. Please refer to the git log or GitHub
issues for up to date information.

-----
### 2010-08-10 Stable Version 1.0.6 released

  - 2010-08-10

      - `smtp-tls.c`

        Each component matched by `match_domain()` is either a single '\*' which
        matches anything or a case-insensitive comparison with a string of
        alphanumeric characters or a '-'.  This is more restrictive than
        [RFC 2818]() appears to allow and replaces the previous match which was
        supposed to allow multiple wildcards but which just didn't work.
        Revised `check_acceptable_security()` to check subjectAltName falling
        back to commonName only if subjectAltName is not available.

-----
### 1.0.5 never released

  - 2006-10-31

      - `protocol.c`

        The Gmail server reports enhanced status codes but then fails to
        provide them in some cases.  The parser is now tolerant of this but
        warns the application using a new event flag`SMTP_EV_SYNTAXWARNING`
        that it is progressing despite the syntax error.

      - `headers.c`

        Fixed bug where `To`, `Cc`, `Bcc` etc. accepted only single values instead
        of a list.
        The comparison on the return value of `gettimeofday()` was reversed.

-----
### 2005-12-16 Stable Version 1.0.4 released

  - 2005-12-16

      - `headers.c`

        Replaced static counter used when generating the default Message-Id
        header with `getpid()` to minimise the risk of 2 processes generating
        the same Message-Id.  If the platform provides `gettimeofday()` this is
        used to further reduce the possibility of collision.
        Thanks to *Dmitry Maksyoma* <dmaks@esphion.com> for spotting this and
        suggesting the fix.

      - `Makefile.am */Makefile.am`

        Replace CFLAGS with `AM_CFLAGS` to silence warning from automake.

      - `Makefile.am COPYING COPYING.LIB`

        Fixed the names of the files with the GPL and LGPL.  It seems
        the LGPL version of COPYING got zapped by autoconf at some time
        in the past.

  - 2005-08-29

      - `acinclude.m4`

        Fix underquoted definition of `ACX_WHICH_GETHOSTBYNAME_R`.  Thanks
        to *Matthias Andree* <matthias.andree@gmx.de>.

  - 2005-07-25

      - `errors.c`

        Added `#ifdef`s for some of the `EAI_` constants used by `getaddrinfo()`
        which are not defined by OSX.
        Thanks to *Thomas Deselaers* <deselaers@gmail.com>

  - 2005-07-21

      - `acinclude.m4`

        Fix cross compiling issue when detecting snprintf as suggested by
        *Chris Richards* <Chris.Richards@red-m.com>

  - 2005-07-02

      - `smtp-api.c`

        Plug memory leaks in `smtp_destroy_session()` and `smtp_set_server()`.
        Thanks to *Bas ten Berge* <sam.ten.berge@hccnet.nl> for report and patch.
        Also reported by *Heikki Lindholm* <heikki.lindholm@ipnetworks.fi>

  - 2005-02-03

      - `protocol.c`

        exts was set with the wrong flag (DSN) when checking if CHUNKING
        is a required extension.

  - 2004-07-16

      - `smtp-tls.c`

        Applied OpenSSL patch from Pawel Salek when checking subjectAltName.

-----
### 2004-04-20 Stable Version 1.0.3 released

  - 2004-04-20

      - `memrchr.c configure.in`

        Added `memrchr()` implementation for systems that don't have one.

      - `smtp-tls.c`

        Applied patches from Pawel Salek to check subjectAltName for
        wildcarded domain name when validating server certificate.

-----
### 2004-01-06 Stable Version 1.0.2 released

  - 2003-12-01

      - `smtp-tls.c examples/mail-file.c`

        Applied patch from Pawel Salek.

      - `smtp-tls.c`

        Fixed typo in `check_file()` which prevented it from doing quite
        the right thing.
        The domain name check for the server certificate is now implemented
        using the wildcard match described in [RFC 2818]().
        `Check_file()` and `check_directory()` return different values for
        unusable vs absent files.

-----
### 2003-09-12 Stable Version 1.0.1 released

  - 2003-09-11

      - `protocol.c smtp-auth.c smtp-bdat.c smtp-etrn.c smtp-tls.c`

        More thoroughly check return value from `read_smtp_response()`.

      - `libesmtp.h errors.c`

        Added new `Client error` error code.  This is just a cop-out,
        used when an API called by libesmtp fails.

      - `base64.c`

        Make conversions immune to `NULL` source data,

      - `examples/mail-file.c`

        Cleaned up some compiler warnings

  - 2003-09-02

      - `siobuf.[hc]`

        Added a few extra `sio_` calls.  Not actually used in libESMTP though.

  - 2003-07-29

      - `concatenate.c errors.c getaddrinfo.c headers.c htable.c`

      - `protocol.c siobuf.c`

        Don't perform zero length operations using the `memxxx()` functions.
        This may avoid segfaults on some platforms or libraries.

      - `siobuf.c`

        Improved handling of flushes in `sio_write()` particularly in the
        case where data would exactly fill remaining space in the buffer.

  - 2003-07-27

      - `rfc2822date.c`

        Correct leap year compensation for January and February in
        `libesmtp_mktime()`.

  - 2003-07-27

      - `examples/Makefile`

        Changed compiler flags from `-ansi` to `-std=c99` and added `-W`

  - 2003-03-04

      - `headers.c`

        Eliminated bug where `find_header()` could pass -1 to the length
        argument of `memchr()` causing a core dump on some architectures.

  - 2003-02-26

      - `libesmtp-private.h protocol.c smtp-bdat.c`

        M$ Exchange does not accept a chunk size of 0 in BDAT 0 LAST as
        explicitly permitted by [RFC 3030](), *sigh*.  Hackish workaround
        implemented.

  - 2003-01-27

      - `configure.in Makefile.am`

        Added `DIST_SUBDIRS` macro to make sure tarball gets built properly.
        This one slipped past 'make distcheck' last time for some reason
        but then autoconf & friends are totally inscruitable.

      - `ntlm/ntlmdes.c`

        OpenSSL 0.9.7 changes some typedefs.  Changed to suit, should
        still be compatible with previous OpenSSL versions.

-----
### 2002-11-09 Stable Version 1.0 released

  - 2002-11-09

      - `configure.in`

        All version 1.0 features enabled by default.
        `--enable-isoc` now sets `-std=c99` instead of `-ansi`

      - `headers.c`

        Added missing check for `NULL` pointer in `destroy_header_table`.
        Reversed order of freeing header structures and hash table to
        avoid referencing freed memory. (Wally Yau)

  - 2002-06-24

      - `smtp-etrn.c`

        Compilation fails with
	    `./configure --enable-more-warnings=picky --disable-etrn'`.
	    Added missing `__attribute__ ((unused))` markers
        to offending function arguments to avoid this.

-----
### 2002-06-24 Version 1.0rc1 released

  - 2002-06-24

      - `configure.in Makefile.am protocol.c protocol-states.h`

      - `smtp-api.c smtp-bdat.c libesmtp.h libesmtp-private.h`

        Added experimental support for the SMTP CHUNKING extension.

      - `configure.in`

        Enable non-standard AUTH= response by default.

  - 2002-05-31

      - `protocol.c smtp-api.c libesmtp.h libesmtp-private.h`

        Added API call to permit protocol timeouts to be set.

      - `ntlm/ntlmstruct.c`

        Replaced use of byteswap.h and bswap\_{16,32} with locally defined
        functions.

-----
### 2002-04-24 Version 0.8.12 released

  - 2002-03-14

      - `headers.c`

        Setting `Hdr_PROHIBIT` did not work properly.  Thanks to Ronald
        F. Guilmette for pointing this out.

  - 2002-03-13

      - `protocol.c smtp-api.c libesmtp.h configure.in`

        Revoked deprecated status from `smtp_option_require_all_recipients`
        and remove the corresponding `--enable-require-all-recipients`
        parameter to configure.

  - 2002-03-07

      - `libesmtp.h libesmtp-private.h protocol.c smtp-tls.c`

        [RFC 2487]() is obsoleted by [RFC 3207]().  Updated references.

      - `protocol.c`

        The check for required STARTTLS was omitted when processing the
        HELO command.  If a server did not implement EHLO the session
        would proceed instead of quitting.  Check added and the event
        callback added to report the missing extension to the
        application.

-----
### 2002-03-06 Version 0.8.11 released

  - 2002-03-04

      - `protocol.c`

        Fix buffer overflow problem in `read_smtp_response`.  This
        overflow could be exploited by a malicious SMTP server to
        overwrite the stack and hence a carefully crafted response could
        cause arbitrary code to be executed.  Also took the opportunity
        to add a related check for a potential DoS attack which makes
        use of excessively long SMTP responses.  Thanks to Colin Phipps
        for detecting this.

      - `concatenate.[ch]`

        New function `cat_shrink` to shrink-wrap the allocated buffer.

      - `libesmtp.h errors.c`

        New unterminated response error code and description.

      - `ntlm/ntlmstruct.c configure.in crammd5/md5.h`

        stdint.h does not yet seem to be widely available causing
        compilation to fail on some platforms.  Changed `uint{16,32}_t` to
        `unsigned{16,32}_t`, detect correct sizes with autoconf and added
        typedefs in ntlmstruct.c.  Changed detection types from int to
        `unsigned int` in configure.in and made corresponding changes in
        crammd5/md5.h.  Thanks to Ronald F. Guilmette for spotting this.

  - 2002-02-12

      - `strcasecmp.c strncasecmp.c`

        These now return the correct sign of result for differing strings.

-----
### 2002-01-30 Version 0.8.10p1 released

  - 2002-01-29

      - `ntlm/Makefile.am`

        Added ntlm.h to list of sources.  This omission stopped 0.8.10
        form building.

-----
### 2002-01-29 Version 0.8.10 released

  - 2002-01-26

      - `various files`

        Copyright messages now show the correct year.
        Minor tweaks to kill warnings when compiling with
        `--enable-more-warnings=picky.`  In a few cases this meant adding
        a few casts which superficially look unnecessary.  In other
        cases this meant adding a number of `#undefs` to get a vanilla
        ISOC environment.

      - `missing.h`

        Added missing.h which has declarations for Posix/SUS functions
        which may be missing from system libraries on some platforms.

      - `snprintf.c configure.in`

        Detect broken or missing `snprintf()` implementations and replace
        if necessary.  N.B. the replacement snprintf.c is taken from the
        libmutt distro and it too, is broken.  However, it *does*
        correctly truncate and \\0 terminate output which is too long to
        fit in the buffer and that is the behaviour I rely on.

      - `strdup.c`

        Added `strdup()` for systems which don't have it.

      - `examples/mail-file.c`

        Check for errors when `smtp_start_session` returns.  Fixed
        authinteract so that responses are not accidentally overwritten.

  - 2002-01-24

      - `htable.c configure.in strndup.c`

        Altered code to avoid the use of strndup.  strndup.c is removed
        from the distribution.

  - 2002-01-16

      - `ntlm/* configure.in`

        Added NTLM auhentication module.

  - 2002-01-07

      - `concatenate.c errors.c siobuf.c`

        Check return value from snprintf.

-----
### 2002-01-03 Version 0.8.9 released

  - 2002-01-02

      - `configure.in`

        Added `-lsocket` to list of libraries searched for `getaddrinfo()`.

  - 2001-12-29

      - `protocol.c configure.in`

        Added hack for stupid SMTP servers that advertise AUTH using
        non-standard syntax from an internet draft that never made it
        into [RFC 2554]().  Because this feature is non-standard, it must
        be explicitly enabled when configuring.
        rsp\_{helo,ehlo}() now reset the auth mechanism list before
        processing the result.  Previously this was done only when AUTH
        was advertised.

      - `smtp-auth.c`

        `set_auth_mechanisms` no longer resets the mechanism list
        before processing.  Added a test to avoid duplicates in the
        mechanism list.  `select_auth_mechanism` now guarantees to select
        the *first* usable mechanism.  The net effect of these changes
        is that multiple calls to `set_auth_mechanisms` accumulate.

      - `auth-client.c`

        Rearranged code in `auth_set_mechanisms` and `load_client_plugin`
        avoiding the need to repeat the test for plugin acceptability.

  - 2001-12-24

      - `configure.in`

        Compiling with picky warnings turned on was broken.  Also,
        recent glibc versions seem to have decided that strcasecmp and
        a few other functions are GNU extensions causing compiles to
        fail because of missing declarations.  Naturally, autoconf does
        not detect this.  Added a \_GNU\_SOURCE define to fix this on
        potentially affected systems.  No, I don't like it either.

      - `strcasecmp.c strncasecmp.c strndup.c`

        Added these functions in case some systems don't provide them.

  - 2001-12-21

      - `htable.[ch]`

        `h_insert` now returns a void pointer to the data instead of a
        struct `h_node` eliminating the need for the `h_dptr` macro and
        for code using hash tables to maintain two pointers instead
        of one.

      - `headers.c`

        Updated to use the simpler hash table interface.

  - 2001-12-10

      - `auth-client.c`

        Use dlsym and friends directly on platforms that have it.

      - `configure.in Makefile.am`

        Detect dlsym, fall back to using libltdl for other platforms.
        libltdl is no longer distributed significantly reducing tarball
        size.

  - 2001-12-10

      - `configure.in`

        A missing comma caused the test for getipnodebyname to fail on
        systems which provide it.

-----
### 2001-12-06 Version 0.8.8 released

  - 2001-11-30

      - `crammd5/md5.h`

        The len parameter of `md5_update` differed in type between
        prototype and definition, preventing compilation if `size_t`
        is not an `unsigned int`.

  - 2001-11-29

      - `crammd5/*.[ch]`

        Moved include of config.h from hmacmd5.h to hmacmd5.c.  Make
        sure sys/types.h is included since `size_t` is used.

      - `configure.in`

        Added some extra nonsense for systems which redefine
        getaddrinfo to something else in netdb.h

  - 2001-11-27

      - `configure.in errors.c`

        Add test for broken `strerror_r` on OSF-1.

  - 2001-11-12

      - `configure.in`

        Updated the tests for pthreads.  Should now supply the correct
        compiler flags on more systems.

-----
### 2001-11-07 Version 0.8.7 released

  - 2001-11-05

      - `errors.h libesmtp.h`

        Improve handling of error codes from getaddrinfo.  Delay mapping
        of codes to make debugging easier.  libesmtp.h defines new error
        codes for the relevant `EAI_XXX` codes from getaddrinfo.
        `smtp_strerror` will use `gai_strerror` if appropriate.

  - 2001-10-31

      - `configure.in`

        Added test for sun platforms and define `EXTENSIONS` so that
        sun's netdb.h will declare the getaddrinfo stuff. (James McPherson)

      - `crammd5/md5.[ch]`

        Type sanity: change `u_intXX_t` to `uintXX_t`.  Also changed the
        argument for the buffer and length to `void *` and `size_t`
        respectively in `md5_update`.  Buffer for `md5_final` is now
        `unsigned char`.

  - 2001-10-17

      - `headers.c`

        Fixed a core dump bug which strikes when existing headers in a
        message are substituted.

      - `Makefile.am`

        Reinstated libesmtp.spec into tarballs.

-----
### 2001-08-17 Version 0.8.6 released

  - 2001-10-17

      - `libesmtp-config.in`

        Corrected output for `--cflags.`  Added `--numeric-version` to make it
        simpler for configure scripts to compare version numbers.

      - `configure.in libesmtp-spec.in`

        Merged changes from Cristophe Lambin.  Spec file now creates
        libesmtp and libesmtp-devel packages.  If OpenSSL is used
        spec file will have openssl dependencies added.

      - `Makefile.am`

        Make sure libesmtp.spec and config.h do not make their way into
        tarballs.  These confused the build on some platforms.

  - 2001-10-16

      - `configure.in`

        Added `--with-openssl[=DIR]` option, removed `--enable-starttls`.
        OpenSSL dependent features are now enabled or disabled en masse
        using `--with-openssl`.

      - `crammd5/md5.[ch] crammd5/Makefile.am`

        Added public domain MD5 implementation to crammd5 module.  This
        enables the CRAM-MD5 mechanism to be built, even if OpenSSL is
        not available.

      - `smtp-tls.c`

        Applied patch from James McPherson correcting \_\_attribute to
        [attribute]

-----
### 2001-08-05 Version 0.8.5 released

  - 2001-10-05

      - `libesmtp.spec.in`

        Make sure libesmtp-config gets installed\!

      - `configure.in`

        Removed STARTTLS's experimental status.  The code works and just
        needs debugging.  Certificate management is basic but usable.
        Set defines for strict iso/posix/xopen in headers only when
        \--enable-isoc is in force.  This helps avoid disabling the
        `tm_gmtoff` member in the BSD struct tm unnecessarily.

      - `siobuf.[ch]`

        `sio_read`/write use void buffers rather than char.

  - 2001-09-28

      - `smtp-tls.c`

        Use the event callback to report STARTTLS in use if the security
        level was OK.

      - `rfc2822date.c`

        Provide a function to portably calculate the timezone offset
        when struct tm does not provide `tm_gmtoff`.

      - `configure.in`

        Don't bother to check for gmtime\[\_r\] since it isn't used any more.

  - 2001-09-26

      - `headers.c`

        Make sure `set_to` accepts `NULL` for the mailbox value.  Added `set_cc`
        which is same as `set_to` except it fails with a `NULL` mailbox.

      - `Most files.`

        Changed references to RFC 821/822 to RFC 2821/2822 respectively.

  - 2001-09-24

      - `headers.c`

        [RFC 2822]() requires only the originator and date headers to
        be present in a message.  In particular, the presence of the `To:`
        header is no longer required.  [RFC 2822]()'s restriction that
        headers may not appear multiple times in a messge is enforced
        respecting certain special exceptions.

      - `examples/mail-file.c`

        Added API call to make sure a `To:` header is generated if not
        in the message.

  - 2001-09-14

      - `configure.in acinclude.m4 acconfig.h`

        Reverted to configure.in, reinstated acconfig.h and added
        some compatibility stuff to acinclude.m4.  All this to try
        and be compatible with autoconf 2.5 *and* 2.13.  I really
        hate autoconf.

  - 2001-09-05

      - `getaddrinfo.c`

        Check if `NO_ADDRESS` is defined and different to `NO_DATA`

  - 2001-08-27

      - `protocol.c smtp-tls.c`

        Move some detail of selecting STARTTLS to smtp-tls.c

      - `smtp-tls.c`

        Changed STARTTLS policy for `Starttls_ENABLED`.  If a server offers
        STARTTLS, then it must be used and all security requirements must
        be met.  If STARTTLS is not offered the session continues in
        cleartext.  Previously, `Starttls_ENABLED` permitted a session to
        continue with possibly compromised security.

  - 2001-08-22

      - `smtp-tls.c libesmtp.h`

        More certificate management.  Added TLS event reporting.

      - `smtp-auth.c`

        Fixed behaviour for zero length responses to server challenges.

      - `base64.c`

        Zero length passwords caused an assertion failure in
        `base64_encode`.  `base64_decode` did not correctly strip blanks
        from strings not terminated by \\0.  Neither did it correctly
        handle zero length strings.

  - 2001-08-21

      - `smtp-tls.c libesmtp.h`

        Added preliminary code for client certificate management.

      - `siobuf.[ch]`

        Changed `sio_set_tlsclient_ctx` to `sio_set_tlsclient_ssl`.  This
        makes things slightly more flexible for supplying different
        client certificates according to the remote host.

  - 2001-08-20

      - `message-source.c`

        Fixed memory leak in `msg_source_destroy`. (Pawel Salek)

  - 2001-08-16

      - `configure.ac Makefile.am`

        Change from using LIBOBJS to LTLIBOBJS.  This prevents the
        wrong objects from being linked by libtool when building
        dynamic libraries.  Added code to configure.ac to correctly
        set LTLIBOBJS and LTALLOCA.

      - `smtp-api.c protocol.c errors.c libesmtp-private.h configure.ac`

        Code now exclusively uses getaddrinfo.  Removed `#ifdef` code for
        gethostbyname.  Added conditionals for using alternative lwres
        library distributed with recent versions of bind.  Delete
        `--enable-gethostbyname` option.  Add `--enable-emulate-getaddrinfo.`

  - 2001-08-14

      - `siobuf.c`

        Remove unnecessary socket include files.

      - `getaddrinfo.[ch]`

        Added emulation of the [RFC 2553]() getaddrinfo resolver interface
        for systems that don't have it.

-----
### 2001-08-13 Version 0.8.4 released

  - 2001-08-13

      - `protocol.c`

        Completely ignore TLS extension if TLS is already in use.

      - `smtp-tls.c`

        Fix wrong comparison when initialising OpenSSL mutexes.  Record
        the fact that TLS is in use.  Change a numeric constant to its
        symbolic equivalent.

      - `crammd5/client-crammd5.c`

        Correct a typo which prevented the hmac computation being
        correctly rendered in hexadecimal.

      - `examples/mail-file.c`

        Added `--tls` and `--require-tls` options and supporting code.

  - 2001-07-31

      - `configure.ac`

        Make plugin directory consistent with RPM.

      - `libesmtp.spec`

        Applied patch from Pawel Salek to run ldconfig after installing.

  - 2001-07-30

      - `protocol.c configure.ac`

        Check for uname and use it in preference to gethostname which
        is not Posix.

  - 2001-07-19

      - `configure.ac`

        Check for the presence of the OpenSSL headers as well as
        the libraries.  Remove `--enable-callbacks` option.

      - `smtp-api.c libesmtp.h`

        Added `smtp_version` API call.

      - `message-callbacks.c`

        Removed callbacks which did \\n -\> CRLF translation.

      - `examples/mail-file.c`

        Use libESMTP provided callback unless the `--crlf` option is
        supplied.

  - 2001-07-07

      - `protocol.c smtp-api.c`

        Only include netinet/in.h if it is actually needed.

-----
### 2001-07-06 Version 0.8.3 released

  - 2001-07-06

      - `examples/mail-file.c`

        Made `--help` more helpful.  Undocumented `--no-crlf` now renamed
        to `--crlf` and documented.  When prompting for authentication
        now reads /dev/tty instead of stdin.

      - `configure.ac`

        Check for `-lsocket.`

      - `protocol.c siobuf.c`

        Zero errno before calling certain functions.  Normally the value
        of errno is only tested if the preceeding system call or function
        wrapping the system call failed.  However, in a few cases, the
        functions are called in a loop and the value of errno might
        be tested after a successful return.  This meant that a test
        on the value of errno might yield an invalid result, sometimes
        causing the connection to the server to be incorrectly dropped.
        Unfortunately this effect depended on the amount of data buffering
        provided by the server\!

  - 2001-06-29

      - `protocol.c et al.`

        Added support for sendmail specific XUSR extension.  This informs
        sendmail the message is a user submission instead of relay,
        so it makes sense to issue the command.  Whether it actually
        does anything ...

      - `siobuf.c`

        Fixed return from poll in `raw_read` and `raw_write` so that EINTR
        is correctly handled.

  - 2001-06-26

      - `auth-client.c`

        Fixed a signed/unsigned comparison that stops compilation
        when using `-Werror.`

-----
### 2001-06-26 Version 0.8.2 released

  - 2001-06-26

      - `protocol.c siobuf.c`

        Resolved a problem related to blocking/non-blocking polling
        for server events.  This could lead to deadlock with certain
        servers.

  - 2001-06-24

      - `configure.ac most C sources`

        Added `--disable-isoc` option.  When using gcc, `-ansi` `-pedantic`
        are now specified by default since the code compiles without
        warnings when using both flags.
        Added `--enable-debug` option to control DEBUG and NDEBUG
        definitions.  Assert macros used to check arguments to
        most internal functions.

  - 2001-06-23

      - `message-source.c`

        `msg_gets` now checks for both \\r and \\n when searching for line
        endings.

      - `errors.c`

        API function now includes ommitted the arguments check.

  - 2001-06-22

      - `protocol.c smtp-api.c configure.ac`

        Now uses [RFC 2553]() / Posix protocol independent getaddrinfo where
        possible instead of gethostbyname family of resolver functions.

      - `gethostbyname.c configure.ac`

        Added ability to use getipnodebyname and corresponding test
        for configure.

  - 2001-06-18

      - `auth-client.c`

        More thorough argument checking on the `auth_xxx` APIs.
        Added some missing malloc return value checking.
        When loading a plugin, make sure it provides a `response()`
        function.

      - `smtp-auth.c`

        Added some missing malloc return value checking.

      - `various sources`

        Changed some 'int' types to '`size_t`'

      - `message-source.c`

        In `msg_gets`, an inconsistent pointer could cause a segfault
        after a realloc which moved the original memory block.
        Increased sizes of malloc/realloc so that [RFC 2821]() maximium
        line length will not cause realloc.
        Added missing malloc/realloc return value checking.

      - `protocol.c`

        If an error occurs while copying the message to the SMTP
        server drop the connection without terminating the message.

-----
### 2001-06-15 Version 0.8.1 released

  - 2001-06-13

      - `configure.in`

        is now configure.ac to suit autoconf 2.5
        Eliminated some redundant stuff concerned with libtool.
        Now uses `AC_HELP_STRING` macro where appropriate.
        Now use standard `AC_FUNC_STRERROR_R` macro.
        Improved checking for time.h and sys/time.h.

      - `libesmtp-config.in`

        If libltdl was installed, the list of libraries was set to the
        wrong thing.

      - `errors.c`

        Only use `strerror_r` if it actually works.

      - `rfc822date.c`

        Try `sys/time` for `struct tm` just in case\!

  - 2001-06-12

      - `configure.in`

        Now checks `-lnsl` when seraching for `gethostbyname_r`.
        Only print a warning if `strerror_r` is not found.
        Chose much more picky compiler warnings when using gcc - this
        has knock on effects through many files.  Compiles should now
        be much cleaner on more platforms.

      - `siobuf.h`

        Gcc will now check `sio_printf()`'s argument types against the
        format string.

      - `headers.c`

        Eliminated a variable which was set but not used.
        The as yet unimplemented `smtp_set_resent_headers` API will
        succeed if `onoff` is zero.

      - `protocol.c`

        Eliminated variables which were set but not used.
        Fixed an uninitialised variable bug which might strike if the
        EHLO command received a 5xx status code.  This is likely with
        older servers and may result in libESMTP dropping the connection
        instead of trying HELO.
        Fixed an uninitialised variable bug which could cause the protocol
        to QUIT inadvertently after processing the response to EHLO.

      - `errors.c`

        Rewrote handling of the thread specific data.

-----
### 2001-06-12 Version 0.8.0 released

  - 2001-06-11

      - `protocol.c smtp-api.c`

        DELIVERBY extension done - still to test.

  - 2001-06-09

      - `message-callbacks.c libesmtp.h`

        Added standard callback functions for reading messages. libesmtp.h
        provides macros to simplify using them.

  - 2001-06-07

      - `message-source.c`

        Changed the declaration of the message callback to make it clearer
        that the first argument in fact points to internal state allocated
        by the callback.  Strings returned to code reading the message
        are now const char \*.

  - 2001-05-31

      - `smtp-api.c protocol.c configure,in`

        Had another go at the `smtp_require_all_recipients()` API hack.
        The original implementation hoped the SMTP server would report
        failure on receiving a zero length message but this isn't
        reliable.  This API must be explicitly enabled by ./configure.

  - 2001-05-28

      - `smtp-auth.c`

        Make sure the client won't attempt to authenticate when already
        authenticated.  This could happen if having authenticated and
        enables a security layer, the server offers AUTH again.

      - `smtp-tls.c`

        Make sure the client won't attempt to negotiate TLS when already
        using TLS.  Also don't use TLS if already authenticated.

      - `siobuf.c`

        Make the code for non-blocking sockets + OpenSSL more robust.

      - `errors.c`

        Added default case for `set_herrno()`.

  - 2001-05-25

      - `smtp-auth.c`

        On authentication failure the same mechanism was selected again
        instead of moving on to the next one.  This caused an infinite
        loop of failing AUTH exchanges.

  - 2001-05-24

      - `libesmtp.spec.in`

        Changed `-a 0` option in %setup macro to `-T -b 0`

      - `configure.in`

        Removed `-Werror` from `--enable-more-warnings=yes` as this can be
        bothersome for punters.  Added `--enable-more-warnings=picky` to
        stop gcc from using internal prototypes for builtin functions;
        also turns on `-Werror.`

      - `protocol.c`

        `free_ghbnctx()` was called twice if `connect()` failed, potentially
        causing a SIGSEGV.  This bug was introduced with support for
        `gethostbyname_r`.

  - 2001-05-23

      - `configure.in`

        Incremented library version and reset the age.  This is important
        because the event callback semantics have changed.
        Detect IPv6 sockaddr structure in `<netinet/in.h>`.

      - `protocol.c`

        Added 8BITMIME support.  New API call `smtp_8bitmime_set_body()`.
        Report extensions after final set is known.  STARTTLS or AUTH
        can change the set of extensions advertised by the server.
        Typo meant the RET=FULL/HDRS parameter was printed as
        SIZE=FULL/HDRS in MAIL FROM: (D'oh\!)

      - `errors.c`

        Changed prototype for `smtp_strerror()` to allow use of `strerror_r`.

  - 2001-05-22

      - `siobuf.c`

        Fixed calls to encode/decode callbacks and added explanation of
        their semantics.  This eliminates potential for a buffer
        overflow bug when decoding expands data read from the socket.

      - `libesmtp.spec.in`

        Fixed inconsistency between package name and tarball.
        Use the bz2 version of the tarball as the source.

      - `Makefile.am`

        Added libesmtp.spec to extra distribution files.

      - `gethostbyname.c`

        Added missing `#include <string.h>` (gcc builtin prototypes
        again - grumble....)

  - 2001-05-21

      - `siobuf.c`

        Restructuring of reading/writing and polling to permit use
        of non-blocking IO.

      - `protocol.c`

        Revised protocol outer loop makes sure the protocol engine reads
        data as soon as it becomes available and defers buffer flushes
        until after pending data from the SMTP server has been read.  In
        conjunction with non-blocking output this avoids a potential
        deadlock described in [RFC 2920]() when PIPELINING is in use.

  - 2001-05-20

      - `smtp-etrn.c`

        Added experimental support for the ETRN extension.

      - `protocol.c smtp-api.c`

        Check for failure to create a message source and added code
        to actually destroy it thus plugging a memory leak.
        More thorough checking of some API function arguments.

      - `siobuf.c`

        Added `sio_mark()`.  When the write buffer is flushed data written
        beyond the mark is retained and the mark is deleted.

      - `protocol.c`

        Added new event types for flagging required extensions not
        available or reporting extensions that provide information to
        the application.
        Command boundaries are marked in the write buffer.  This
        prevents partial commands being sent to the SMTP server.

-----
### 2001-05-18 Version 0.7.1 released

  - 2001-05-18

      - `protocol.c`

        Added `AF_INET6` support.

  - 2001-05-17

      - `protocol.c gethostbyname.[ch] configure.in acinclude.m4`

        `gethostbyname_r()` now in its own file which provides a consistent
        interface.  configure selects which version of the function to
        compile for when building threaded code.

      - `configure.in auth-client.h libesmtp-config.in libesmtp.spec.in`

        Directory for installing authentication plugins is now
        configurable.

  - 2001-05-15

      - `libesmtp.spec.in`

        Added to simplify building RPM packages.

  - 2001-05-14

      - `smtp-api.c`

        Check that all messages have a callback to read the message
        headers and body.

      - `tokens.c`

        Check buffer length in `read_atom()`.
 
        Use of \<ctype.h\> eliminated.

      - `headers.c`

        `init_header_table()` checks for `NULL` pointers to avoid potential
        SIGSEGVs.

  - 2001-05-13

      - `rfc822date.c configure.in`

        Use `localtime_r()` or `gmtime_r()` when building a thread safe
        library.

      - `concatenate.[ch]`

        Fixed incorrect shortfall caclulation in `concatenate()`
        potentially leading to buffer overrun.
        Generally tidied up code.

      - `auth-client.c`

        `auth_response()` fails if `(*context->client->init)()` fails.

      - `base64.c`

        `b64_encode()` now checks the destination buffer length.

  - 2001-05-11

      - `libesmtp-config.in Makefile.am`

        Added config script to simplify compiling and linking.

      - `protocol.c configure.in`

        Use `gethostbyname_r()` when building a thread safe library.

  - 2001-05-09

      - `Makefile.am configure.in`

        libltdl is now part of the tarball and is installed if not
        already present.

      - `protocol.c`

        `do_session()` will now make use of all the addresses returned
        by `gethostbyname()`.  This allows the DNS admin for the domain
        to specify a number of MTAs which handle mail submission.
        Failures trying to connect or when processing the greeting or
        a response to the EHLO/HELO commands will cause a fallback server
        to be tried.  The name server will round robin the responses
        balancing the load among the servers.
        When reading the server greeting accept only 220 otherwise the
        connection may have been made to a non-SMTP service.

-----
### 2001-05-06 Version 0.7.0 released

  - 2001-05-06

      - `protocol.c`

        `session->auth_mechanisms` was incorrectly freed in `do_session()`.
        This should have been done using `destroy_auth_mechanisms()`.
        Moved initialisation of the session variables to the start
        of `do_session()`.  This allows checks for some error conditions
        to be done before attempting to connect to the SMTP server.
        Updated code to select messages and recipients.  This is to
        permit calling `smtp_start_session()` more than once on a given
        session.  Second and subsequent calls should only deliver to
        recipients not successful in a previous SMTP session.  Removed
        a few FIXME comments that no longer apply.

      - `smtp-api.c`

        `smtp_recipient_reset_status()` clears the 'complete' flag so that
        the recipient will be retried on a subsequent `smtp_start_session()`.
        Added a new API `smtp_recipient_check_complete()`.  This is true if
        a subsequent call to `smtp_start_session()` would *not* attempt to
        post the message to this recipient.
        `smtp_destroy_session()` now frees memory allocated for remote
        server hostname.

      - `protocol.c libesmtp-private.h smtp-api.c`

        Renamed 'sent' in `smtp_recipient_t` structure to 'complete'.
        Not all completed recipients might have been sent.

      - `protocol.c headers.c smtp-api.c`

        Only call `gethostname()` once and save the result.  Also added
        new API `smtp_set_hostname()` to allow the application to change
        the default.

  - 2001-05-03

      - `crammd5/Makefile.am crammd5/hmacmd5.[ch]`

        Renamed files to avoid name conflict with Cyrus SASL
        include/ directory.

      - `base64.c message-source.c`

        Added missing `#include <string.h>`, egcs-2.91.66 didn't
        spot the missing prototypes.

      - `examples/mail-file.c`

        Ignore SIGPIPE.  Means the application isn't killed accidentally
        when something times out during the protocol session.

  - 2001-05-02

      - `sasl-tls.c protocol.c`

        Added experimental support for STARTTLS

  - 2001-04-31

      - `cram-md5/Makefile.am`

        hmac-md5.h was missing from the list of sources and hence was
        not in the tarball.

-----
### 2001-04-29 Version 0.6.1 released

  - 2001-04-30

      - `auth-client.c`

        Fixed incorrect SSF comparison for authentication modules
        that were already loaded.

-----
### 2001-04-29 Version 0.6a released

  - 2001-04-28

      - `configure.in smtp-api.c example/mail-file.c`

        Corrected inconsistently named API from `smtp_set_auth_context()`
        to `smtp_auth_set_context()`.

-----
### 2001-04-25 Version 0.6 released

  - 2001-04-25

      - `configure.in Makefile.am */Makefile.am`

        Added detection of MD5 routines in OpenSSL, to enable
        the CRAM-MD5 SASL mechanism.

  - 2001-04-25

      - `protocol.c`

        Corrected parsing bug in `parse_status_triplet()`.

  - 2001-04-16

      - `api.h`

        Added new header file.  This currently contains macros
        to aid argument checking for API functions.

      - `libesmtp.h`

        Changed name of API function argument check macro.

      - `most files`

        Wrapped `#include <config.h>` with `#ifdef HAVE_CONFIG_H`.

  - 2001-04-11

      - `rfc822date.c`

        Make sure the absolute value of minutes is used when
        formatting the date.

      - `protocol.c`

        Fixed potential segfault when DATA fails before transferring
        a message.

      - `smtp-auth.c`

        Support for client authentication plugins now working.

      - `protocol.c`

        Support for SMTP AUTH extension.

  - 2001-04-05

      - `protocol.c smtp-auth.c`

        Removed '`want_enhanced`' argument from `read_smtp_response()`
        since it was unnecessary.
        Added preliminary support for SMTP AUTH extension.

  - 2001-04-04

      - `Many files`

        Changes to accomodate stricter error checking options
        to gcc.

  - 2001-04-03

      - `siobuf.c protocol.c`

        Changed `CONFIG_TLS` to `USE_TLS` (not that it matters yet)
        Changed `HAVE_LIBSASL` to `USE_SASL`
        Mostly for consistency with autoconf convention.

      - `configure.in`

        Checks for pthreads and SASL.

  - 2001-03-21

      - `smtp-api.c`

        Fixed up some missing error reporting.

  - 2001-03-15

      - `protocol.c`

        Now sets timeouts reccommended in [RFC 1123]() when waiting for
        server responses.

      - `errors.c libesmtp.h`

        Changed prefix from ES\_ to SMTP\_ERR\_.  Edits to other files
        to accommodate the change.

  - 2001-03-14

      - `protocol.c libesmtp.h`

        Added first lot of event monitoring callbacks.  Simplified the
        declaration for the callback function.  The callback is called
        with different arguments depending on the actual event.

  - 2001-03-09

      - `tokens.c tokens.h protocol.c`

        Added const to a few things that should have had it.  Fixed a
        corresponding declaration in protocol.c.

      - `smtp-auth.c`

        Basis of the implementation of the SMTP AUTH command.  This is
        not complete or tested yet, pending the decision about how to
        best implement SASL.

      - `siobuf.c siobuf.h`

        Added callback functions which encode or decode data just before
        writing or after reading data between the buffers and the
        socket.  This is for use by SASL security layers.

  - 2001-03-07

      - `concatenate.c concatenate.h`

        Added `minimum_length` parameter to cat\_{init,reset}().

  - 2001-03-07

      - `siobuf.c siobuf.h protocol.c`

        Allow for seperate read and write file descriptors in
        `sio_attach()`.  This is for when support for opening a pipe to
        a program running an SMTP server on its stdin/stdout is added.

      - `siobuf.c siobuf.h`

        Fixed the `#ifdef _buffer_h` lines to `#ifdef _siobuf_h` (the perils
        of cut and paste editing).

        Added typedefs for the sio callback functions.

        Added encoder/decoder callbacks for use with security layer parts
        of SASL.  `CONFIG_SASL` stuff now gone.

  - 2001-03-02

      - `headers.c`

        Implemented the rest of `destroy_header_table()`.

-----
### 2001-02-27 Version 0.5 released

  - 2001-02-26

      - `protocol.c`

        Now issue RSET before MAIL FROM: if a failure response is
        received to the DATA command.

        Fixed possible segfaults when resetting the status in `rsp_rest()`
        and `rsp_quit()`.

      - `headers.c`

        Partially implemented `destroy_header_table()`.

      - `smtp-api.c`

        Now calls `destroy_header_table()`.

      - `htable.c`

        Allow callback in `h_destroy()` to be `NULL`.

-----
### 2001-02-26 Version 0.4 released

  - 2001-02-25

      - `protocol.c`

        Second state for the DATA command now does not transfer the
        message if there were no valid recipients.

  - 2001-02-22

      - `smtp-api.c`

        Added APIs to get/set application data in each of the opaque
        structures.

        Added protocol event callback API but only for a place holder.
        This will be used by applications which want to monitor the
        progress of the session and status changes as they happen.
        This is different from the protocol monitor which dumps the
        actual data transferred on or close to the wire.

      - `headers.c`

        Added code to handle `Sender:`

        Fixed `From:` printing; continuation lines had no leading whitespace.

        In `smtp_set_header_option()` once `Hdr_PROHIBIT` is set, it cannot
        be unset.  Prohibit cannot be set for headers already set.

        Added `smtp_set_resent_headers()` but only for a place holder.

      - `protocol.c`

        Correct parsing of enhanced status codes.  These are only present
        for 2xx, 4xx and 5xx SMTP status codes.

-----
### 2001-02-18 Version 0.3 released

  - 2001-02-19

      - `protocol.c smtp-api.c`

        Port number in session structure stored in host byte order
        instead of network byte order.  This makes the port number easier
        to read in gdb.

      - `protocol.c`

        Removed white space which crept into the MAIL FROM: and RCPT TO:
        commands.  All the servers tested with to date have accepted
        this but it wasn't in [RFC 821]().

      - `headers.c`

        Fixed `From:` and `Disposition-Notification-To:` headers to allow
        multiple mailboxes as per [RFC 822]().

        Corrected syntax for default `Message-Id:` generation.  This
        should have been `addr-spec` per [RFC 822]() but didn't have an @.

      - `examples/mail-file.c`

        Corrected typo that stopped `--reverse-path` from working.

-----
### 2001-02-18 Version 0.2 released

Core libESMTP API now complete.

  - 2001-02-18

      - `examples/mail-file.c`

        Updated to tweak a few more APIs in libESMTP.
        The example now has a very basic Makefile.

  - 2001-02-17

      - `protocol.c`

        Changed use of `strchr()` to `memchr()` since strings read by the
        message callback and header functions are *not* \\0 terminated.

      - `headers.c`

        Changed beyond all recognition. :-)
        Declaration of `smtp_set_header_option()` has changed.

      - New files added to support [RFC 822]() header processing.

  - 2001-02-08

      - `siobuf.c`

        Some additional error checking; extra thoroughness checking the
        return value of `write()`.

      - `siobuf.c`

      - `protocol.c`

        Added the protocol monitor callback mechanism.

      - `libesmtp.h`

      - `libesmtp-private.h`

      - `smtp-api.c`

        Minor changes to the monitor callback declaration and session
        structure for the protocol monitor.

-----
### 2001-02-04 Version 0.1a released

  - 2001-04-04

      - `message-source.c`

        Fixed a bad bug that could cause an infinite loop if a message
        was not properly terminated with a \\n

-----
### 2001-02-04 Version 0.1 released

Initial Release
