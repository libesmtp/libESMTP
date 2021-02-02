API Reference
=============

This document describes the libESMTP programming interface.

This document is for the master branch which is a superset of version 1.0.6,
adding only the `_full()` variants of the application data API.

The libESMTP API is intended for use as an ESMTP client within a Mail User
Agent (MUA) or other program that wishes to submit mail to a preconfigured
Message Submission Agent (MSA).

The documentation is believed to be accurate but may not necessarily reflect
actual behaviour. To quote Douglas Adams:

  Much of it is apocryphal or, at the very least, wildly inaccurate.
  However, where it is inaccurate, it is *definitively* inaccurate.

  -- *The Hitch Hikers' Guide to the Galaxy*

If in doubt, consult the source.

.. toctree::
   :maxdepth: 2

   introduction
   programflow
   _kdoc/libesmtp
   _kdoc/smtp-api
   _kdoc/headers
   _kdoc/smtp-etrn
   _kdoc/smtp-auth
   certificates
   _kdoc/smtp-tls
   _kdoc/message-callbacks
   _kdoc/errors

Auth Client
-----------

.. toctree::
   :maxdepth: 2

   _kdoc/auth-client

