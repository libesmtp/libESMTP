# Installation
**libESMTP** uses **Meson** (https://mesonbuild.com/Getting-meson.html) with
any of the build systems it supports, such as `ninja` (https://ninja-build.org/).
The old `autotools` based build has been removed.

To build with ninja do the following:

```/bin/sh
$ meson [options] builddir
$ ninja -C builddir install
```

Note that the meson/ninja installer does not require `sudo` if `polkit` is
installed, instead it will prompt for a password during install. This avoids
polluting builddir with files owned by root.

As well as the standard meson options, the following are supported:
* `-Dpthreads=[enabled|disabled|auto]`
build with support for Posix threads (default: auto)
* `-Dtls=[enabled|disabled|auto]`
support the STARTTLS extension and NTLM authentication (default: auto)
* `-Dlwres=[enabled|disabled|auto]`
use the ISC lightweight resolver (default: disabled)

## Dependencies

**openssl**

OpenSSL v1.1.0 or later (https://www.openssl.org/) is required to build the
SMTP STARTTLS extension and the NTLM authentication module.  If you have no
need for either of these features, you do not need OpenSSL.

**getaddrinfo**

You will need a modern resolver library providing the getaddrinfo API.
``getaddrinfo()`` is easier to use, protocol independent, thread-safe and RFC
2553 and Posix standard.

Almost all modern systems provide this. If not, the ISC BIND 9 (https://www.isc.org/)
library should be installed. BIND 9 also provides the Lightweight Resolver
(lwres) which may be used in preference to the standard resolver.

As a last resort an emulation is provided for systems that do not have it,
however this is not reccommended. The emulation code will likely be removed in
the near future.

**dlsym**

libESMTP requires that dlsym() is available on your system.  This is
true all modern ELF based systems.  An alternative is to download
and install libltdl which provides a functional equivalent.  Libltdl is
distributed with GNU Libtool, which is available from
https://www.gnu.org/software/libtool/

