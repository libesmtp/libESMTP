# Reporting Bugs

libESMTP will not be changed without good justification.  SMTP and mail related
documents in general have been subject to rigorous peer review in public by the
[IETF](https://www.ietf.org/) before being accepted as standard.  Since mail is
possibly the most mission critical internet application of them all, it is
vital that the infrastructure is not compromised by buggy implementations.


These guidelines are intended to help distinguish features from genuine bugs.
In SMTP, as with many internet protocols, the distinction is subtle at times.
The following paragraphs are examples of what might constitute a bug.  The list
is by no means exhaustive.


## GitHub Issue Tracker

All bug reports should be made using the GitHub issue tracker.  This is only
for discussion of libESMTP sources within, or for pull requests against the git
repository.  Issues opened solely against the libESMTP tarballs will be deleted
without explanation.


## What to Report

If, after reading the following, you are convinced that libESMTP does not do
something it should, or vice-versa, get hold of the relevant RFC or other
standard and read the relevant parts.  Send a bug report which references the
standard and justify why libESMTP must change.  Usually I will not change
behaviour that conforms to normative requirements.  However, mistakes do occur
in standards so sometimes breaking conformance is unavoidable.


### Coding Errors
If you spot a mistake in the code, such as failing to check the return value of
a function for an error condition, please report it.


### Missing Library Calls
If libESMTP uses a C library function that is not provided by a platform then
the distribution should contain a replacement for that function and Meson
should detect that the replacement is needed.  If neither condition is true,
that is a bug.

### Broken Library Calls
If libESMTP uses a C library function that is broken on a given platform then
the distribution should contain a replacement for that function and Meson
should detect that the replacement is needed.  If neither condition is true,
that is a bug.  A library call is considered broken if its behaviour differs
from that described in the appropriate publicly available standard.

Normally, the appropriate standard is Posix or the Single Unix Specification.
Remember that Unix man pages or other manufacturer specific documentation of C
library functions describe an implementation and do not constitute a standard.


### Library Calls in Nonstandard Places
If Meson does not search all possible locations for a particular library call,
that is a bug.


### SMTP Protocol Errors
If libESMTP sends SMTP protocol commands which differ from those described in
the appropriate RFCs, or fails to respond correctly to valid SMTP responses
from a server, that is a bug that should be reported.

SMTP is described in RFC 5321.  The message format is described in RFC 5322.
Various other RFCs describe the SMTP extension mechanism and specific SMTP
extensions.  References to the appropriate documents may be found in comments
within the source code.


### Obsolete Behaviour
If libESMTP implements behaviour described in an obsoleted standard and the
replacement describes different behaviour, that is a bug in libESMTP.

Particular care is needed when referring to RFCs as these may be obsoleted or
updated at any time and the new document will have a new and unrelated number.
For example, do not refer to RFC 821 or RFC 1123 since these are obsoleted by
RFC 5321.

--------------

## What Not to Report
Its amazing how often correct behaviour is reported as a bug.  Familiarise
yourself with the appropriate documentation.  Also read the [libESMTP
API](reference) document.  Many people trip up on the subtleties of SMTP and
often the documents will answer your question saving your time and mine.


### Can't Connect to Server
Do not complain that libESMTP can't connect to port 25.  The default port for
mail submission is 587.  This is a deliberate design choice and will not
change.  Refer to RFC 4409 for more information.

### The headers come out after the message
You got the line endings wrong.  libESMTP strictly adheres to the RFC 5322
message format.  RFC 5322 requires that lines in a message are terminated with
the sequence CR-LF (0x0D 0x0A or \r\n).


### Standards Compliance
Do not report behaviour that conforms to normative requirements in a standard
as a bug, even if you don't like what happens or you find it inconvenient in
some way.

### It's Different to ...
Do not assume that behaviour which differs from your favourite mail client is a
bug.  Just because Mozilla or Outlook or whatever might do something
differently does not necessarily mean they are correct and libESMTP is wrong.

### Obsolete Behaviour
Do not report behaviour that differs from requirements in obsolete standards as
bugs.  For example, RFC 821 does not permit the syntax `MAIL FROM <>` but RFC
5321 does.

### Fallback Behaviour
Do not report libESMTP's default behaviour as a bug if an API call is
provided to change it.  Ever.

### Server Bugs
Do not report bugs in SMTP servers or server misconfiguration as libESMTP bugs.

However, if a server is so broken that libESMTP requires a workaround to
operate with it at all I am interested in hearing about it.  A good example of
this sort of thing is the broken SMTP AUTH extension deployed by Yahoo!.  This
server uses a syntax described in an obsolete and unobtainable internet draft
and omits to implement the syntax described in the standard.  Fortunately, in
this case, the workaround is easy to implement and does not disrupt
interoperation with correctly implemented servers.

### Behaviour Behind a Firewall
Don't report libESMTP's use of hostnames or IP addresses that are not visible
on the internet side of a firewall.  Don't report that NAT makes these
unusable.

When a firewall is in use, especially one that does NAT, it is normal to submit
mail via a local MTA that knows how to translate and/or qualify domain names
and how to enforce site policy.  If the firewall permits access to port 25 on
the internet for hosts without publicly known hostnames and IP addresses, any
mail client will eventually encounter problems with some servers.
