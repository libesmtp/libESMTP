			libESMTP, version 1.0
			      -- oOo --
	      	Brian Stafford	<brian@stafford.uklinux.net>


What is libESMTP?
-----------------

LibESMTP is a library to manage posting (or submission of) electronic
mail using SMTP to a preconfigured Mail Transport Agent (MTA) such as
Exim or Postfix.  It may be used as part of a Mail User Agent (MUA) or
another program that must be able to post electronic mail but where mail
functionality is not the program's primary purpose.

LibESMTP is not intended to be used as part of a program that implements
a Mail Transport Agent.

It is hoped that the availability of a lightweight library implementing
an SMTP client will both ease the task of coding for software authors
and improve the quality of the resulting code.

Features
--------

Support for many SMTP extensions, notably PIPELINING (RFC 2920),
DSN (RFC 2554) and AUTH (RFC 2554).  Also supported is the
sendmail specific XUSR extension which informs sendmail that the
message is an initial submission.

SASL
----

AUTH is implemented using a SASL (RFC 2222) client library which is
currently integrated into libESMTP.  It was felt that the Cyrus SASL
library was too complex for the needs of a client only SASL
implementation.

If there is sufficient interest in a LGPL SASL library, the SASL client
API will be split off into a separate library in the future.  There may
also be a case for implementing a server side SASL library along the
same lines as the client implementation.

Installation
------------

Please refer to INSTALL for generic installation instructions.  LibESMTP
has a few options when configuring; ./configure --help lists them.

Dependencies
------------

dlsym:

libESMTP requires that dlsym() is available on your system.  This is
true of many modern systems but not all.  An alternative is to download
and install libltdl which provides a functional equivalent.  Libltdl is
distributed with GNU Libtool, which is available from
http://www.gnu.org/software/libtool/

getaddrinfo:

You will need a modern resolver library providing the getaddrinfo API.
getaddrinfo is easier to use, protocol independent, thread-safe and
RFC 2553 and Posix standard.

An emulation of this is provided for systems that do not have it, however
it is reccommended that the version provided in recent versions of GNU
libc or BIND is used.  Most people will already at least one of these
(e.g. virtually every Linux distro).  There is also support for the
lightweight resolver distributed with BIND 9.  BIND may be downloaded
from the ISC (http://www.isc.org/).

openssl:

OpenSSL (http://www.openssl.org/) is required to build the SMTP STARTTLS
extension and the NTLM authentication module.  If you have no need for
either of these features, you do not need OpenSSL.


Licence
-------

LibESMTP is licensed under the GNU Lesser General Public License and the
example programs are under the GNU General Public Licence.  Please refer
to COPYING.GPL and COPYING for full details.

Obtaining libESMTP
------------------

LibESMTP may be obtained from:
	http://www.stafford.uklinux.net/libesmtp/

Documentation
-------------

LibESMTP documentation is available on the web at:
	http://www.stafford.uklinux.net/libesmtp/api.html
This probably (definitely) lags behind the actual source code.

What does the 'E' stand for?
--------------------------

The 'E' in libESMTP is there because support for a number of SMTP
extensions is built in to the library by design.


