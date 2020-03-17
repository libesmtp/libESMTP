# libESMTP - A Library for Posting Electronic Mail

**libESMTP** is an SMTP client which manages posting (or submission of)
electronic mail via a preconfigured Mail Transport Agent (MTA).  It may be used
as part of a Mail User Agent (MUA) or other program that must be able to post
electronic mail where mail functionality may not be that program's primary
purpose.

The availability of a reliable, lightweight and thread-safe SMTP client eases
the task of coding for software authors thus improving the quality of the
resulting code.  See the [rationale](rationale.md) for development of this
library.

## Features

libESMTP supports many SMTP extensions, notably PIPELINING (RFC 2920), DSN (RFC
1891) and SASL authentication (RFC 2554).

libESMTP is efficient.  Because it buffers all network traffic, when used in
conjunction with a pipelining SMTP server libESMTP significantly increases
network performance, especially on slow connections or those with high latency.
Even without a pipelining server libESMTP offers much better performance than
would be expected with a simple client.

libESMTP is not intended for use as part of a program that implements a Mail
Transport Agent.  This is not because the code is unsuitable for use in an MTA,
in fact the protocol engine is significantly better than those in many MTAs.
Rather, by eliminating the need for MX lookup and next-hop determination, the
design of libESMTP is simplified; thus goals are made achievable.  Besides,
such features are undesirable in a program that is not an MTA, and may even be
detrimental if the client is behind a firewall which blocks access to port 25
on the Internet.

## Licence

libESMTP is licensed under the [GNU Lesser General Public
License](https://www.gnu.org/copyleft/lesser.html) and the example programs are
under the [GNU General Public Licence](https://www.gnu.org/copyleft/gpl.html)

## Author

libESMTP's primary author is Brian Stafford.
