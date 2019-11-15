# libESMTP 1.0

## Website

The libESMTP website is at http://www.brianstafford.info/libesmtp

## What is libESMTP?

LibESMTP is a library to manage posting (or submission of) electronic
mail using SMTP to a preconfigured Mail Transport Agent (MTA) such as
Exim or Postfix.  It may be used as part of a Mail User Agent (MUA) or
another program that must be able to post electronic mail but where mail
functionality is not the program's primary purpose.

## Features

Support for many SMTP extensions, notably PIPELINING (RFC 2920), DSN (RFC 2554)
and AUTH (RFC 2554).  AUTH is implemented using a SASL (RFC 2222) client
library which is currently integrated into libESMTP. Also supported is the
sendmail specific XUSR extension which informs sendmail that the message is an
initial submission.

## Dependencies

**dlsym**

libESMTP requires that dlsym() is available on your system.  This is
true of many modern systems but not all.  An alternative is to download
and install libltdl which provides a functional equivalent.  Libltdl is
distributed with GNU Libtool, which is available from
https://www.gnu.org/software/libtool/

**getaddrinfo**

You will need a modern resolver library providing the getaddrinfo API.
getaddrinfo is easier to use, protocol independent, thread-safe and
RFC 2553 and Posix standard.

An emulation of this is provided for systems that do not have it, however
it is reccommended that the version provided in recent versions of GNU
libc or BIND is used.  Most people will already at least one of these
(e.g. virtually every Linux distro).  There is also support for the
lightweight resolver distributed with BIND 9.  BIND may be downloaded
from the ISC (https://www.isc.org/).

**openssl**

OpenSSL v1.1.0 or later (https://www.openssl.org/) is required to build the
SMTP STARTTLS extension and the NTLM authentication module.  If you have no
need for either of these features, you do not need OpenSSL.


## Licence

LibESMTP is licensed under the GNU Lesser General Public License and the
example programs are under the GNU General Public Licence.  Please refer
to COPYING.GPL and COPYING for full details.



