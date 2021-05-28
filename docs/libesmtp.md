# Benefits

## Simple

libESMTP relieves developers of the need to handle the SMTP protocol by
providing a high level API. Additionally, libESMTP transparently handles many
SMTP extensions including authentication and TLS security.

## Efficient

libESMTP's primary goal is to be efficient.  Because it buffers all network
traffic and supports the SMTP PIPELINING (RFC 2920) extension when connecting
to a pipelining SMTP server, libESMTP significantly improves network
performance, especially on slow connections or those with high latency.  Even
without a pipelining server libESMTP offers much better performance than would
be expected with a simple client.

## Thread Safety

Providing a few simple restrictions are observed, libESMTP is thread-safe.

## Extensions

libESMTP supports many standard SMTP extensions, useful to mail submission
agents (MSA).

- [RFC 3463, RFC 2034] ENHANCEDSTATUSCODES
- [RFC 2920] PIPELINING
- [RFC 3461] DSN
- [RFC 4954] AUTH (SASL)
- [RFC 3207] STARTTLS
- [RFC 1870] SIZE
- [RFC 3030] CHUNKING
- [RFC 3030] BINARYMIME
- [RFC 6152] 8BITMIME
- [RFC 2852] DELIVERBY
- [RFC 1985] ETRN
- [sendmail] XUSR - *notify sendmail this is an initial submission*
- [exchange] XEXCH50

Many of these are used automatically if advertised by the MTA. Certain
extensions require extra information supplied by associated API calls.

## Target Applications

libESMTP is intended for use with any application that needs to submit mail;
that is where a message is first submitted into the transport and delivery
infrastructure. For example, an application which resubmits messages sent to a
mailing list, an on-line application that mails notifications to users or, of
course, an email program.

A major difference with clients which are part of a mail transport agent is
that libESMTP does not perfrom MX resolution for next-hop determination.  This
is intentional since MX lookup is undesirable behaviour in a program that is
not an MTA, and may even be detrimental if the client is behind a firewall
which blocks access to port 25 on the Internet.

The availability of a reliable, lightweight and thread-safe SMTP client eases
the task of coding for software authors thus improving the quality of the
resulting code.  See the [rationale](rationale.md) for development of this
library.

## QA

While the following steps are not and cannot be an exhaustive QA process,
libESMTP code is required to compile with zero warnings when built using
Meson's `--warnlevel 3 --werror` arguments. Warnlevel 3 is augmented with a
"reasonable" set of extra warnings. The clang/llvm static analyser is used to
verify code at compile time, again all warnings are eliminated. The valgrind
tools are used to analyse behaviour at runtime. Where possible all warnings or
errors are eliminated; occasionally this is not possible if the errors occur
in dependencies.
