# NEWS

```
* libESMTP 1.0.3 stable release. 2004-04-20

- This release contains TLS improvements from
  Pawel Salek <pawsa@theochem.kth.se>
  See ChangeLog for details.

* libESMTP 1.0.2 stable release. 2004-01-06

- See ChangeLog for details.

* libESMTP 1.0.2 stable release. 2004-01-06

- See ChangeLog for details.

* libESMTP 1.0.1 stable release. 2003-09-12

- See ChangeLog for details.

* libESMTP 1.0 stable release. 2002-11-09

  Tarball builds correctly again!

- See ChangeLog for details.

* libESMTP 1.0 stable release. 2002-11-09

  LibESMTP is now considered stable.  Version 1.0 is the best available
  release of libESMTP and all users are urged to upgrade as soon as is
  practicable.

  There have been some minor changes to the configure script such that
  ./configure with no arguments includes all non-experimental features.
  This means that some features formerly not enabled by default are now
  included and, conversely, some features formerly enabled by default must
  now be requested explicitly.  It is intended that, with the exception of
  features such as setting --prefix or --with-gnu-ld, ./configure will
  build the correct configuration for most OS distributions.

- See ChangeLog for details.

  This release fixes a minor compilation issue and a potentially more
  serious memory reference after freeing.

* libESMTP 1.0rc1 stable release candidate 1.	2002-06-24

- See ChangeLog for details.

  o Support for the non-standard AUTH= syntax used by some broken
    servers is now on by default.  This does not appear to interefere
    with correctly implemented SMTP AUTH and having it on by default is
    less confusing for users whose ISPs insist on deploying broken
    servers.

  o Added experimental support for RFC 3030 CHUNKING and BINARYMIME;
    enable with ./configure --enable-chunking.  Feedback on the success
    or otherwise of this code is solicited.

  o New API function to set protocol timeouts.

* libESMTP 0.8.12 development release.	2002-04-24

- See ChangeLog for details.

  o Added missing check for STARTTLS if server does not support ESMTP.

  o Revoked deprecated status from smtp_option_require_all_recipients

* libESMTP 0.8.11 development release.	2002-03-06

Fixed a buffer overflow which could be exploited by a malicious SMTP
server.  By overwriting the stack a carefully crafted response could
cause arbitrary code to be executed.

* libESMTP 0.8.10 development release.	2002-01-29

- Usual autoconf stuff, see ChangeLog for details.

Added an NTLM authentication module.  Currently this requires OpenSSL to
build.  This has not seen much in the way of testing as I don't have
regular access to a server which requires NTLM authentication for SMTP.
However it does generate the correct responses for the test cases I have
tried.  Feedback on the success or otherwise of this module is solicited.

Compilation with --enable-more-warnings=picky seems to be clean again.

* libESMTP 0.8.9 development release.	2002-01-02

- See ChangeLog for details.

Important:
    The use of libltdl is now deprecated in favour of dlopen().  libltdl
    is no longer distributed with libESMTP reducing tarball size.  This
    change simplifies installation for the majority of users, however
    users with platforms which do not supply dlopen or libltdl must now
    obtain and install libltdl separately.

Also Important:
    Building with --enable-more-warnings=yes/picky might prove akward.
    Recent glibc versions seem to have changed their mind about the
    status of strcasecmp and friends to being GNU extensions.
    Naturally, autoconf 2.13 detects the functions in the library but
    not that their declarations are unavailable. For this reason,
    _GNU_SOURCE is defined on gnu type platforms but this might cause
    inconsistent pointer declarations wrt. signedness, YMMV.  If you
    have problems, try ./configure --disable-more-warnings.

A horrible hack:
    Added tentative support/hack for the non-standard AUTH= syntax in
    EHLO responses.  It might work.  Don't complain to me if it doesn't.
    You need to ./configure --enable-nsauth for this support.
      This syntax was only ever described in internet drafts and never
    made it into RFC 2554.  It should *never* have been deployed on the
    internet.  Internet drafts are deleted after 6 months and after
    publication of RFCs.  So there is *no* documentation for this syntax
    and I can't even begin to guess what it is supposed to be or what
    implementation errors there are wrt these unavailable documents.
      My advice is if this hack doesn't work, complain to your ISP and
    recommend that they deploy MTAs that are standards compliant.
    Documentation exists for standards and I am happy to make sure
    libESMTP complies with documents I can actually obtain.

* libESMTP 0.8.8 development release.	2001-11-30

- See ChangeLog for details.

  o Fixes more autoconf issues.

  o Fixed a type mismatch that prevents compilation on some systems.

* libESMTP 0.8.7 development release.	2001-11-7

- See ChangeLog for details.

  o Fixes minor build issues.

  o Improved error handling wrt getaddrinfo

* libESMTP 0.8.6 development release.	2001-10-17

- See ChangeLog for details.

  o Fixes minor build issues.

  o SASL CRAM-MD5 builds without OpenSSL

* libESMTP 0.8.5 development release.	2001-10-04

- See ChangeLog for details.

  o Header code no longer enforces presence of recipient fields.

  o Fixed some build issues related to the automake/libtool interaction.
    Reverted to autoconf 2.13

  o Removed support for gethostbyname resolver interface.  Please
    refer to the 'Dependencies' section in README.

  o Enhancements to STARTTLS support.

  o Calculation of current timezone's offset from GMT (UTC) is now
    portable and thread safe.

* libESMTP 0.8.4 development release.	2001-08-13

- See ChangeLog for details.

* libESMTP 0.8.3 development release.	2001-07-06

- See ChangeLog for details.

  o Support for sendmail's XUSR extension.

  o Fixed a bad bug which caused connections to the server to be dropped
    depending on the amount of buffering provided by the server.

* libESMTP 0.8.2 development release.	2001-06-26

- See ChangeLog for details.

  o Added lots of assertions in the code.

  o Fixed a bad dangling pointer bug that could strike when sending
    messages with lines > 510 characters.

  o Fixed a polling bug that could cause deadlock.

  o Resolver interface now uses Posix standard getaddrinfo.
    Use of gethostbyname is deprecated.

Please note that the current RFC 2822 header API is adequate but
incomplete; for example, interactions between certain headers are not
implemented.  This will not change for a while.  The current priority is
to make the protocol engine robust.

* libESMTP 0.8.1 development release.	2001-06-15

- See ChangeLog for details.

Fixed two uninitialised variable bugs that might cause the protocol
to quit without sending anything to the server.

Enabled many more compiler warnings when compiling with gcc.  Compiles
should now be much cleaner.

* libESMTP 0.8.0 development release.	2001-06-12

- See ChangeLog for details.

The libESMTP feature set and API for version 1.0 is more or less complete.
There have been minor changes to the arguments or semantics of some of
the API functions, particularly wrt. the callback functions.  Applications
using previous libESMTP versions will need to be recompiled or relinked.

From this point on no new features will be added and, as far as possible,
API changes will be resisted.  Having said that, the range of error codes
will likely be expanded.  Effort will now be directed at bug fixes and
improving the documentation and web site, though this is likely to be a
slow process.

Many of the supported SMTP extensions have had only superficial testing
mainly due to lack of access to servers supporting them.  Developers using
libESMTP are encouraged to test extensions against servers to which they
have access and to submit bug reports to <brian@stafford.uklinux.net>.

The libESMTP web site will be updated in the near future to set up
(finally!)  mailing lists and bug tracking.  In addition the web site will
link to projects using libESMTP.  If you would like a mention for your
project, drop a line to <brian@stafford.uklinux.net> with the details.

```
