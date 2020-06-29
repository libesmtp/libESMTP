# FAQ
## Frequently Asked Questions

A selection of questions that have arisen over time.

### What is libESMTP?

libESMTP is an SMTP client designed to manage submission (or posting) of
electronic mail messages.  It is a more flexible alternative to the traditional
technique of piping messages into sendmail.

### Why not pipe messages into sendmail?

Well, one good reason is you are required to install sendmail which
introduces bloat into a system that does not otherwise require an MTA (Mail
Transport Agent).  Administering a mail server isn't trivial.

Another good reason is that sendmail returns a single error code that
encapulates success or failure over multiple recipients.  Consider the case
where you send your multi-megabyte word processor document to 20 recipients and
just one of them fails. How does your application figure out who to resend the
message to? Again, not trivial.

### Why can't it be used in an MTA?

libESMTP is not designed to be used as part of a program that implements a Mail
Transport Agent.  This is not because the code is unsuitable for use in an MTA,
in fact the protocol engine is significantly better than those in many MTAs.
Rather, by eliminating the need for MX lookup and next-hop determination, the
design of libESMTP is simplified; thus goals are made achievable.  Besides,
such features are undesirable in a program that is not an MTA, particularly if
the client is behind a firewall which blocks access to port 25 on the Internet.

### The headers come out after the message, not before!  Surely this is a bug?

No it isn't a bug.  You got the line endings wrong.  libESMTP strictly adheres
to the RFC 5322 message format.  RFC 5322 requires that lines in a message are
terminated with the sequence CR-LF (0x0D 0x0A or \\r\\n).  Since this is
different to the Un\*x convention of terminating lines with \n, which is not
recognised, libESMTP believes it has been fed a message consisting of a single,
improperly terminated, header line.  The RFCs are unclear what to do about
headers embedding lone \n characters so libESMTP just ignores this particular
issue on the grounds that it is a programming error rather than a runtime
error.

### I need to send messages with attachments. How do I do that?

libESMTP accepts only messages in RFC 5322 format. It knows nothing of
attachments or other enhancements to RFC 5322 messages.  Check out MIME which
is described in RFC 2045 et al.  You will need to find a library, such as
Gmime, or write your own code which implements MIME or other features which are
orthogonal to RFC 5322.

### Why connect to port 587?  The SMTP port is 25!

libESMTP is designed for message submission, not message relay.  Since the
semantics of message submission and relay differ even though the protocol looks
the same "on-the-wire", different port numbers have been assigned for these
operations.  Please refer to RFC 4409 for further information on this topic.

### I can't submit mail. My server is MS Exchange.

Some versions of Exchange have a bug in the implementation of the SMTP CHUNKING
extension and reject a chunk size of zero. Because of the design of the
libESMTP API, the final chunk of a message is zero octets long.  libESMTP
attempts to detect when the MTA is Exchange and work round the bug but this
isn't reliable.  If you are bitten by this problem, the only solution at
present is to run Meson with `-Dbdat=false`.

### smtp_start_session() returned success but my message wasn't sent.  What's going on?

smtp_start_session() returns success if the protocol engine operated without
error from start to finish.   This includes the case where the server may have
rejected the message and the session closed down gracefully.  You must check
the status codes returned to your application by the event callback to
determine if the message was accepted by the MTA.  Please be aware that the
server may accept and deliver the message to some but not all of the
recipients.  The event callback provides comprehensive information on these and
other conditions that arise during the SMTP dialogue with the MTA.

### Is libESMTP thread safe?

Yes. Provided the smtp_session_t handle is referenced by only one thread at a
time libESMTP manages other aspects of thread safety internally.  If your
application cannot guarantee that the session, message and recipient variables
are accessed by one thread at a time you must protect them with a mutex or
equivalent.

### Does libESMTP support IPv6?

The answer to this is normally yes, occasionally no.  This depends on your
platform, specifically on the implementation of getaddrinfo() which resolves
host names and services.  getaddrinfo() supplies all parameters to the socket
API which are necessary to connect to the peer thus making libESMTP's code
independent of the underlying network protocol.

