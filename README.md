# ![logo](docs/pillarbox.png) libESMTP, version 1.1

## New Version Released

All libESMTP users are encouraged to upgrade from version 1.0.6.

After more than a decade, libESMTP version 1.0.6 is superceded. Despite proving
robust a little bitrot has occurred, especially regarding OpenSSL support.
Version 1.1 updates libESMTP without breaking API and ABI compatibility and
provides a basis for future development.

## What is libESMTP?

**libESMTP** is an SMTP client which manages submission of electronic mail via
a preconfigured Mail Transport Agent (MTA) such as
[Exim](https://www.exim.org/) or [Postfix](http://www.postfix.org/).

libESMTP relieves the developer of the need to handle the complexity of
negotiating the SMTP protocol by providing a simple API. Additionally
libESMTP transparently handles many SMTP extensions including authentication.

Refer to the [libESMTP Documentation](https://libesmtp.github.io/)
for further information.

## Licence

LibESMTP is licensed under the GNU Lesser General Public License and the
example programs are under the GNU General Public Licence.  Please refer
to LICENSE and COPYING.GPL for full details.
