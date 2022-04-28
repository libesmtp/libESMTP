# Changes since v1.1.0

libESMTP v1.1.1 bugfix release and includes mitigation for a potential plain
text injection attack when upgrading to TLS from a server claiming to support
PIPELINING.

---

[CVE-2022-xxxxx][3] is fixed.
An SMTP server, or man-in-the-middle, might attempt to pipeline plain text
after the response to a STARTTLS request in an attempt to inject malicious data
into libESMTP's receive buffer.

libESMTP 1.1.1 mitigates against this by disabling PIPELINING support if the
server also supports STARTTLS and checking the receive buffer is empty both
before issuing a STARTTLS and after reading the response code.  If these
conditions are met the TLS upgrade takes place, otherwise libESMTP fails the
session.


libESMTP is updated to require [OpenSSL v1.1.1][4] or later. Note that v1.1.1n
should be considered the minimum acceptable release however, at the time of
writing, some distributions still supply v1.1.1l.


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
separate project.  The API reference remains part of libESMTP.


## Summary

* Check receive buffer empty before and after STARTTLS negotiation.
* Require OpenSSL v1.1.1.
* Update smtp\_starttls_set_ctx(), add smtp\_starttls\_get\_ctx() API calls.
* Fix potential memory leak of the SSL_CTX in smtp-tls.c.
* Add separate meson option to enable 'ntlm' support.
* Fix incorrect GMT (UTC) timezone offset when zero or unknown.
* Fix incorrect SONAME version.
* Website content split to separate project, but retaining API reference.

[3]: https://nvd.nist.gov/vuln/detail/CVE-2022-xxxxx
[4]: https://www.openssl.org/
