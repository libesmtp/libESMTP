# Download

LibESMTP is considered stable.  Source code is available from
[GitHub](https://github.com/libesmtp/libESMTP).  The current version of
libESMTP is 1.1.0. Earlier versions are not recommended.  Clone the repository
as follows:

``` sh
$ git clone https://github.com/libesmtp/libESMTP.git
```

## Dependencies

libESMTP has the following optional dependencies.

### OpenSSL

[OpenSSL v1.1.0](https://www.openssl.org/) or later is required to build the
SMTP STARTTLS extension and to build the SASL NTLM authentication module.
libESMTP may be built without OpenSSL, however most contemporary mail
submission agents require TLS connections.

### ISC Bind 9

libESMTP supports the Lightweight Resolver (lwres), provided by the [ISC BIND
9](https://www.isc.org/), which may be used in preference to the standard
resolver. BIND 9 also provides getaddrinfo() as part of the standard resolver
should it be unavailable on the target platform's C library.

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

Option | Values | Description
-------|--------|------------
xdg | **true**, false | use XDG directory layout instead of ~/.authenticate
pthreads | enabled, disabled, **auto** | build with support for Posix threads
tls | enabled, disabled, **auto** | support the STARTTLS extension and NTLM authentication
lwres | enabled, **disabled**, auto | use the ISC lightweight resolver
bdat | **true**, false | enable SMTP BDAT extension
etrn | **true**, false | enable SMTP ETRN extension
xusr | **true**, false | enable sendmail XUSR extension

Options are specified as `-D<option>=<value>`.

## Static Analyser

It's not obvious how to use `scan-build` with meson and ninja when the binary
does not have the default name.  It also turns out the `SCANBUILD` environment
variable must be set to the *absolute* pathname of the binary.

To build with the clang static analyser, assuming the scan-build binary is
`scan-build-10`, use the following:

``` sh
SCANBUILD=`which scan-build-10` meson builddir [options]
ninja -C builddir scan-build
```

## Documentation

To build the documentation, you will need [Sphinx](https://www.sphinx-doc.org/)
and the `kernel-doc` script, and therefore [Perl](https://www.perl.org/), from
the [Linux Kernel](https://www.kernel.org/) distribution. It may be found in
the `scripts/` directory.

Change to the `docs/` directory and run `gendoc.sh`.

## Repository

The git repository was rebuilt from released tarballs.  Please refer to the
[errata](errata.md) for further information.

