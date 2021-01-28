============
Introduction
============

Overview
--------

For the most part, the libESMTP API is a relatively thin layer over SMTP
protocol operations, that is, most API functions and their arguments are
similar to the corresponding protocol commands.  Further API functions
manage a callback mechanism which is used to read messages from the
application and to report protocol progress to the application.  The
remainder of the API is devoted to reporting on a completed SMTP session.

Although the API closely models the protocol itself, libESMTP relieves the
programmer of all the work needed to properly implement RFC 5321 (formerly
RFC 2821, formerly RFC 821) and avoids many of the pitfalls in typical SMTP
implementations.  It constructs SMTP commands, parses responses, provides
socket buffering and pipelining, where appropriate provides for TLS
connections and provides a failover mechanism based on DNS allowing multiple
redundant MSAs.  Furthermore, support for the SMTP extension mechanism is
incorporated by design rather than as an afterthought.

There is limited support for processing RFC 5322 message headers.  This is
intended to ensure that messages copied to the SMTP server have their
correct complement of headers.  Headers that should not be present are
stripped and reasonable defaults are provided for missing headers.  In
addition, the header API allows the defaults to be tuned and provides a
mechanism to specify message headers when this might be difficult to do
directly in the message data.

libESMTP *does not implement MIME [RFC 2045]* since MIME is, in the words
of RFC 2045, *orthogonal* to RFC 5322.  The developer is expected to use a
separate library to construct MIME documents or the application should
construct them directly.  libESMTP ensures that top level MIME headers are
passed unaltered and the header API functions are guaranteed to fail if any
header in the name space reserved for MIME is specified, thus ensuring that
MIME documents are not accidentally corrupted.

API
---

The libESMTP API is a relatively small and lightweight interface to the SMTP
protocol and its extensions. Internal structures are opaque to the
application accessible only through API calls. The majority of the API is
used to define the messages and recipients to be transferred to the SMTP
server during the protocol session.  Similarly a number of functions are
used to query the status of the transfer after the event.  The entire SMTP
protocol session is performed by a single function call.

Reference
---------

To use the libESMTP API, include the following header files

.. code-block:: c

    #include <auth-client.h>
    #include <libesmtp.h>

Declarations for deprecated symbols must be requested explicitly; define the
macro ``LIBESMTP_ENABLE_DEPRECATED_SYMBOLS`` before including
libesmtp.h. for example

.. code-block:: c

    #define LIBESMTP_ENABLE_DEPRECATED_SYMBOLS
    #include <libesmtp.h>


Internally libESMTP creates and maintains structures to track the state of
an SMTP protocol session.  Opaque pointers to these structures are passed
back to the application by the API and must be supplied in various other API
calls.  The API provides for a callback functions or a simple event
reporting mechanism as appropriate so that the application can provide data
to libESMTP or track the session's progress.  Further API functions allow
the session status to be queried after the event.  The entire SMTP protocol
session is performed by only one function call.

Opaque Pointers
---------------

All structures and pointers maintained by libESMTP are opaque, that is,
the internal detail of libESMTP structures is not made available to the
application.  Object oriented programmers may wish to regard the pointers
as instances of private classes within libESMTP.

Three pointer types are declared as follows

.. code-block:: c

    typedef struct smtp_session *smtp_session_t;
    typedef struct smtp_message *smtp_message_t;
    typedef struct smtp_recipient *smtp_recipient_t;

Thread Safety
-------------

LibESMTP is thread-aware, however the application is responsible for
observing the restrictions below to ensure full thread safety.

Do not access a smtp_session_t, smtp_message_t or smtp_recipient_t
from more than one thread at a time.  A mutex can be used to protect API calls
if the application logic cannot guarantee this.  It is especially important to
observe this restriction during a call to smtp_start_session().

Signal Handling
---------------

It is advisable for your application to catch or ignore SIGPIPE.  libESMTP
sets timeouts as it progresses through the protocol.  In addition the remote
server might close its socket at any time.  Consequently libESMTP may
sometimes try to write to a socket with no reader.  Catching or ignoring
SIGPIPE ensures the application isn't killed accidentally when this happens
during the protocol session.

Code similar to the following may be used to do this

.. code-block:: c

   #include <signal.h>

   void
   ignore_sigpipe (void)
   {
     struct sigaction sa;

     sa.sa_handler = SIG_IGN;
     sigemptyset (&sa.sa_mask);
     sa.sa_flags = 0;
     sigaction (SIGPIPE, &sa, NULL);
   }


