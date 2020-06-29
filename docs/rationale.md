# Rationale

## The case for libESMTP

In the good old days of Un\*x when men were real men, women were real women and
timesharing systems were real timesharing systems and networks were not
commonplace, the *"pipe it into sendmail"* approach was a good one.  Sendmail
can handle many diverse means of mail transport and can parse and translate
addresses and message headers to suit the underlying transport's
characteristics.  It is convenient for programmers, just shell out to
sendmail and pipe the message into sendmail's standard input.  Not much
code is required to do this which is good, less code means fewer bugs.

However, things have changed over the years.  Nowadays, personal workstations
are normal and electronic mail transported using the SMTP protocol is
ubiquitous.  The mail server (known as a Mail Transport Agent or MTA) that is
supposed to handle mail submission is almost invariably remote from the
workstation. There are many other MTAs to choose from than sendmail, and the
alternatives are typically easier to install and maintain.

Unfortunately, the MUAs have not changed much in the way they work.  They still
shell out to sendmail.  This means that MTA authors must provide a sendmail
command to wrap the way their program really works.  Users on workstations need
a sendmail-like MTA on the local host which can be an administrative
nightmare.  There are many command line options which must be accepted but do
nothing; useful SMTP extensions that are not supported via the command line
options (or just not supporetd). Where is sendmail really installed on this
system?  Does the emulation implement every obscure feature an MTA might need?

The solution is simple, an MUA uses SMTP to submit its mail to the MTA.  SMTP
is defined in open standards that are easy to obtain, all MTAs implement it and
it breaks the dependency on an external program's idiosyncrasies and calling
conventions.  Furthermore, it works across a network.  Sendmail talks SMTP so
its ability to transport mail using other mechanisms is not lost.  Nowadays,
most, if not all, users are on workstations and the MTA is on a remote system.
Using SMTP directly surely must be the solution.

Unfortunately, while interacting by the use of standardised protocols is the
solution for the modern networked environment, writing the client side of even
a simple protocol like SMTP is not that trivial.  Quite a lot of code is needed
just to implement the communications between client and server, never mind the
protocol itself.  There are many SMTP extensions which are desirable but not so
easy to implement.  There are restrictions on the format of many of the
protocol elements, for example, the domain of an email address should not
reference a CNAME in the DNS.  MUA authors are caught between the Devil and the
deep blue sea.  Either they opt for the simple solution which requires
installation of a complex piece of MTA software or they choose the protocol
solution which is difficult to implement.

By having a library that implements an electronic mail submission API, it
becomes as easy for programmers to use SMTP for mail submission as it is to use
sendmail.  The library is isolated from the application since it interfaces
only via the API.  On modern systems implementing dynamic linking, the library
can be upgraded without disturbing the application.

Given a simple and intuitive API  and a robust implementation, programmers will
adopt it.  Direct use of SMTP eliminates the necessity for sendmail or any
other MTA to be present on workstations.

**libESMTP** is my attempt at implementing just such a library.

----
Brian Stafford, November 2000.
