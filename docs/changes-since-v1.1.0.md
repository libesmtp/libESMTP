# Changes since v1.1.0

libESMTP v1.1.1 bugfix release and includes mitigation for a potential plain
text injection attack when upgrading to TLS from a server claiming to support
PIPELINING.

---

## Security related changes

An SMTP server, or man-in-the-middle, might attempt to pipeline plain text
after the response to a STARTTLS request in an attempt to inject malicious data
into libESMTP's receive buffer.

libESMTP 1.1.1 defends against this by disabling PIPELINING support if the
server also supports STARTTLS and checking the receive buffer is empty both
before issuing a STARTTLS and after reading the response code.  If these
conditions are met the TLS upgrade takes place, otherwise libESMTP fails the
session.


Stricter parsing of SMTP response lines. Reject malformed status codes. The
text portion of the response is checked for characters outside the range of
printable ASCII, SP and TAB.


Remove support for invalid AUTH= syntax in EHLO response. This was originally
added 20 years ago to support a broken server deployed by Yahoo! I assume it
is fixed by now.


libESMTP is updated to require [OpenSSL v1.1.1][1] or later. Note that v1.1.1n
should be considered the minimum acceptable release however, at the time of
writing, some distributions still supply v1.1.1l.


## Other changes

Avoid locale dependencies in ctype macros or functions. These functions are
replaces with ASCII-specific versions as required.


Updated semantics of the smtp\_starttls\_set\_ctx() API to avoid a potential
memory leak. This should not affect the majority of applications.  Added
smtp\_starttls\_get\_ctx() API.  This allows the application to access the
SSL\_CTX used by libESMTP, especially useful when it is created automatically.
Please see the API reference for further information on these calls.


A meson option `ntlm` to enable NTLM support is provided.  This is necessary
since deprecated MD4 support may not be provided by the OpenSSL package in some
distributions. NTLM is disabled by default.


When the timezone offset from UTC is zero or unknown, dates may be formatted
with an incorrect time zone.  An offset of zero is now formatted correctly as
`+0000` and an unknown offset as `-0000`.


The libESMTP `.so` version number is now formatted correctly.


Website content is removed from the libESMTP repo and is now maintained as a
separate project.  The API reference remains part of libESMTP and may be
built for Devhelp.

Various documentation updates and additions.


## Summary

* Check receive buffer empty before and after STARTTLS negotiation.
* Stricter response parsing.
* Require OpenSSL v1.1.1.
* Avoid locale dependencies.
* Update smtp\_starttls\_set\_ctx(), add smtp\_starttls\_get\_ctx() API calls.
* Fix potential memory leak of the SSL\_CTX in smtp-tls.c.
* Add separate meson option to enable 'ntlm' support.
* Fix incorrect GMT (UTC) timezone offset when zero or unknown.
* Fix incorrect SONAME version.
* Website content split to separate project, but retaining API reference.
* Improved documentation.

[1]: https://www.openssl.org/
