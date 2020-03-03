# libESMTP 1.0

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

## Licence

LibESMTP is licensed under the GNU Lesser General Public License and the
example programs are under the GNU General Public Licence.  Please refer
to COPYING.GPL and COPYING for full details.



