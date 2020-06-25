# Download

LibESMTP is considered stable.  Source code is available from
[GitHub](https://github.com/libesmtp/libESMTP).
Clone the repository using:

``` sh
$ git clone https://github.com/libesmtp/libESMTP.git
```

## Dependencies

### OpenSSL

[OpenSSL v1.1.0](https://www.openssl.org/) or later is required to build the
SMTP STARTTLS extension and to build the SASL NTLM authentication module.
libESMTP may be built without OpenSSL, however most contemporary mail
submission agents require TLS connections.

### dlopen()

libESMTP requires dlopen() to dynamically load authentication modules.

### getaddrinfo()

Most modern resolver libraries provide the getaddrinfo() API defined in
RFC 2553.  The Lightweight Resolver (lwres), provided by the
[ISC BIND 9](https://www.isc.org/) library, may be used in preference to the
standard resolver. BIND 9 also provides getaddrinfo() as part of the standard
resolver should it be unavailable on the target platform.

## Installation

LibESMTP uses the
[Meson build system](https://mesonbuild.com/Getting-meson.html).
Refer to the Meson manual for standard configuration options.

Meson supports multiple build system backends.  To build with
[Ninja](https://ninja-build.org/) do the following:

``` sh
$ meson [options] --buildtype=release builddir
$ ninja -C builddir install
```

Note that the meson/ninja installer does not require an explicit `sudo`,
instead it will prompt for a password during install. This avoids polluting
builddir with files owned by root.

As well as the standard meson options, the following are supported:

* -Dpthreads=[enabled|disabled|auto]

  build with support for Posix threads (default: auto)

* -Dtls=[enabled|disabled|auto]

  support the STARTTLS extension and NTLM authentication (default: auto)

* -Dlwres=[enabled|disabled|auto]

  use the ISC lightweight resolver (default: disabled)

* -Dbdat=[true|false]

  enable SMTP BDAT extension (default: true)

* -Detrn=[true|false]

  enable SMTP ETRN extension (default: true)

* -Dxusr=[true|false]

  enable sendmail XUSR extension (default: true)

## Documentation

To build the documentation, you will need [Sphinx](https://www.sphinx-doc.org/)
and the `kernel-doc` script, and therefore [Perl](https://www.perl.org/), from
the [Linux Kernel](https://www.kernel.org/) distribution. It may be found in
the `scripts/` directory.

Change to the `docs/` directory and run `gendoc.sh`.

