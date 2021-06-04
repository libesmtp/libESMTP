# Changes since v1.0.6

libESMTP v1.1.0 is an interim release, pending some roadmap items currently in
development. While libESMTP 1.0.6 has been stable for some time, bit-rot
inevitably occurs as the state-of-the-art and system conventions change over
time and therefore libESMTP must be updated.  It is hoped that these updates
will provide a more modern base for future development and improve the quality
of the associated documentation.

---

[CVE-2019-19977][3] is fixed.

A linker map is now used to ensure only libESMTP's API symbols are made
publicly available.  This should prevent the possibility that a libESMTP
internal symbol collides with an application or other library.  Existing
interfaces and semantics are unchanged; existing applications should link
correctly with the new libESMTP binary, however applications which rely on
internal symbols will no longer link.

libESMTP is updated to use the current [OpenSSL][4] API.  Therefore OpenSSL
v1.1.0 or later is now required.

The GNU autotools build has been completely removed and replaced by [Meson][1].
Meson builds are easy to maintain and Meson is particularly effective when used
in conjunction with [Ninja][2] offering dramatically reduced build times.

[Open Desktop XDG][5] conventions are now used when searching for files.
Currently this affects the name and location of X.509 certificate files.
This change should be transparent to applications, however users may need to
relocate client certificates.  A configuration option allows the legacy layout
to be selected at build time.

API documentation is migrated to [kernel-doc][6] style comments in source files
and Markdown files for non-API sections.  Because of this many of the changes
to source files look more drastic than they actually are, an impression
somewhat amplified by corrections or updates to RFC numbers to match current
standards.

The MTA's canonic hostname is used to check certificate validity, this may
differ from the specified hostname if it references a CNAME record in the DNS.
The new 'smtp\_get\_server\_name()' API may be used to retrieve this.

The 'application data' interface has been improved. The updated API accepts a
'release' callback argument which libESMTP will call when necessary.  This
relieves the application from releasing allocated resources and should
eliminate one class of potential memory leaks.

The 'getaddrinfo()' replacement implementation has been removed since that
interface is ubiquitous in modern C and resolver libraries and since the
replacement was never a complete implementation.

## Summary

* CVE-2019-19977: avoid potential stack overflow in NTLM authenticator.
* Migrate build system to Meson
* Remove GNU libltdl support, assume dlopen() always available.
* Use a linker map to restrict public symbols to API only.
* Add sentinel and 'format printf' attributes to function declarations.
* Remove getaddrinfo() implementation.
* Use strlcpy() for safer string copies, provide implementation for systems that need it.
* Update 'application data' APIs
* Add 'smtp\_get\_server\_name()' API.
* Collect replacement functions into missing.c
* Prohibit Resent-Reply-To: header.
* Use canonic domain name of MTA where known (e.g. due to CNAME record in DNS).
* Implement rfc2822date() with strftime() if available.
* add option for XDG file layout convention instead of ~/.authenticate
* OpenSSL
  - Remove support for OpenSSL versions before v1.1.0
  - Update OpenSSL API calls used for modern versions
  - Require TLS v1 or higher

[1]: https://mesonbuild.com/
[2]: https://ninja-build.org/
[3]: https://nvd.nist.gov/vuln/detail/CVE-2019-19977
[4]: https://www.openssl.org/
[5]: https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
[6]: https://www.kernel.org/doc/html/latest/doc-guide/kernel-doc.html
